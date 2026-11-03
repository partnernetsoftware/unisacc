// omp-superagent 扩展:把 omp 变成"能派活、能收活"的 super-agent 宿主。
// 不新造运行时:无自有 DB/守护进程;dispatch 起子 omp,worker 按 report 契约回一条 JSON。
// 环境:SA_MODEL(子 omp 模型)、SA_OMP_BIN(默认 omp)、SA_RECORD_DIR(默认 out/superagent)。
import type { ExtensionAPI } from "@oh-my-pi/pi-coding-agent";
import { pathsConflict } from "./report.ts";
import { runDispatch, type DispatchOpts, type DispatchResult, type Role } from "./core.ts";
import { SA_VERSION } from "./version.ts";

const MAX_PARALLEL = 8;

interface Live {
  opts: DispatchOpts;
  promise: Promise<DispatchResult>;
  result?: DispatchResult;
}

export default function (pi: ExtensionAPI) {
  const z = pi.zod;
  pi.setLabel("omp-superagent");
  const live = new Map<string, Live>();
  const done: DispatchResult[] = [];
  const tickets = new Set<string>();

  pi.registerTool({
    name: "dispatch",
    label: "dispatch",
    description:
      "派活:起一个子 omp 执行 ticket,task 必须自包含(目标、读哪些路径、验收命令、非目标)。" +
      "role=implementer 必须给 paths(允许写的前缀);同一会话内 ticket 唯一," +
      "写路径与在飞 dispatch 重叠会被拒。返回 {id,state,report,record}。",
    parameters: z.object({
      ticket: z.string().min(1).describe("一句话任务名,会话内唯一"),
      task: z.string().min(1).describe("自包含任务书"),
      role: z.enum(["implementer", "reviewer", "scout"]).optional(),
      model: z.string().optional(),
      tools: z.string().optional().describe("覆盖工具白名单,逗号分隔"),
      paths: z.array(z.string()).optional().describe("implementer 允许写入的前缀"),
      maxTime: z.number().int().positive().optional().describe("秒,缺省 300"),
    }),
    async execute(_callId, params) {
      const opt = params as DispatchOpts;
      const role: Role = opt.role ?? "implementer";
      const fail = (err: string) => ({ content: [{ type: "text" as const, text: err }], isError: true });
      if (tickets.has(opt.ticket)) return fail(`ticket 已派发过: ${opt.ticket}`);
      if (live.size >= MAX_PARALLEL) return fail(`并行上限 ${MAX_PARALLEL}`);
      if (role === "implementer" && (!opt.paths || opt.paths.length === 0))
        return fail("implementer 必须给出 paths");
      if (opt.paths) {
        for (const [id, l] of live) {
          const c = l.opts.paths ? pathsConflict(opt.paths, l.opts.paths) : undefined;
          if (c) return fail(`路径与在飞 dispatch ${id} 冲突(${c}),请串行`);
        }
      }
      tickets.add(opt.ticket);
      const opts: DispatchOpts = { ...opt, role };
      const promise = runDispatch(opts).then((r) => {
        live.delete(r.id);
        done.push(r);
        return r;
      });
      // 先用 ticket 占位,拿到 id 后换键
      const placeholder = `pending:${opt.ticket}`;
      live.set(placeholder, { opts, promise });
      promise.then((r) => live.delete(placeholder));
      const r = await promise;
      return {
        content: [{ type: "text" as const, text: JSON.stringify(r.report ?? { state: r.state, err: r.err }, null, 2) }],
        details: {
        ok: r.state === "done" || r.state === "blocked",
        id: r.id,
        ticket: r.ticket,
        role: r.role,
        state: r.state,
        err: r.err,
        report: r.report,
        verifiedDropped: r.verifiedDropped,
        record: r.record,
        version: r.version,
        ms: r.ms,
        tokens: r.tokens,
        },
      };
    },
  });

  pi.registerTool({
    name: "status",
    label: "status",
    description: "列出本会话 dispatch(时间倒序)。给 id 则打印该条 record 全文。",
    parameters: z.object({ id: z.string().optional() }),
    async execute(_callId, params) {
      const id = (params as { id?: string }).id;
      if (id) {
        const hit = done.find((d) => d.id === id);
        if (!hit)
          return { content: [{ type: "text" as const, text: `没有 ${id}` }], isError: true };
        return { content: [{ type: "text" as const, text: JSON.stringify(hit, null, 2) }] };
      }
      const rows = [...live.entries()].map(([k, l]) => ({
        pending: k,
        ticket: l.opts.ticket,
        role: l.opts.role,
        state: "running",
      }));
      const finished = [...done].reverse().map((d) => ({
        id: d.id,
        ticket: d.ticket,
        role: d.role,
        state: d.state,
        ms: d.ms,
        tokens: d.tokens,
        verified: d.report?.verified.length ?? 0,
        record: d.record,
      }));
      return { content: [{ type: "text" as const, text: JSON.stringify({ version: SA_VERSION, running: rows, finished }, null, 2) }] };
    },
  });

  pi.registerCommand("sa", {
    description: "omp-superagent 状态: /sa [id]",
    handler: async (argv, ctx) => {
      const id = argv.trim();
      const hit = id ? done.find((d) => d.id === id) : undefined;
      if (hit) ctx.ui.notify(JSON.stringify(hit, null, 2));
      else
        ctx.ui.notify(
          JSON.stringify(
            {
              version: SA_VERSION,
              running: [...live.values()].map((l) => l.opts.ticket),
              finished: done.map((d) => `${d.id} ${d.state} ${d.ticket}`),
            },
            null,
            2,
          ),
        );
    },
  });
}

// omp-superagent 扩展:把 omp 变成"能派活、能收活"的 super-agent 宿主。
// 不新造运行时:无自有 DB/守护进程;dispatch 起子 omp,worker 按 report 契约回一条 JSON。
// 子任务的工具权限和写入隔离由核心执行。
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
      "派活:起一个子 omp 执行 ticket,task 必须自包含。" +
      "implementer 必须提供允许写入的 paths；证据绑定成功工具调用。" +
      "done 是 worker 自报且经收据对账，不代表宿主独立验收任务合同。原始 RPC stdout/stderr 写入 record（可能含敏感内容）。同一会话已接纳 ticket 唯一。",
    parameters: z.object({
      ticket: z.string().min(1).describe("一句话任务名,会话内唯一"),
      task: z.object({
        goal: z.string().trim().min(1).describe("明确目标"),
        context: z.array(z.string().trim().min(1)).min(1).describe("上下文及读取路径"),
        accept: z.array(z.string().trim().min(1)).min(1).describe("验收条件；不适用时写 NOT_APPLICABLE"),
        nonGoals: z.array(z.string().trim().min(1)).min(1).describe("非目标"),
      }).describe("结构化任务合同"),
      role: z.enum(["implementer", "reviewer", "scout"]).optional().describe("缺省 scout"),
      model: z.string().optional(),
      tools: z.string().optional().describe("逗号分隔的工具名"),
      paths: z.array(z.string()).optional().describe("implementer 允许写入的路径"),
      maxTime: z.number().int().positive().optional().describe("秒,缺省 300"),
    }),
    async execute(_callId, params, signal) {
      const opt = params as DispatchOpts;
      const role: Role = opt.role ?? "scout";
      const fail = (err: string) => ({ content: [{ type: "text" as const, text: err }], isError: true });
      if (tickets.has(opt.ticket)) return fail(`ticket 已派发过: ${opt.ticket}`);
      if (live.size >= MAX_PARALLEL) return fail(`并行上限 ${MAX_PARALLEL}`);
      if (opt.paths) {
        for (const [id, l] of live) {
          const c = l.opts.paths ? pathsConflict(opt.paths, l.opts.paths) : undefined;
          if (c) return fail(`路径与在飞 dispatch ${id} 冲突(${c}),请串行`);
        }
      }
      tickets.add(opt.ticket);
      const opts: DispatchOpts = { ...opt, role, signal };
      const placeholder = `pending:${opt.ticket}`;
      const promise = runDispatch(opts);
      live.set(placeholder, { opts, promise });
      let r: DispatchResult | undefined;
      try {
        r = await promise;
        done.push(r);
      } finally {
        if (!r?.record || (r.state === "error" && r.err?.startsWith("记录失败:"))) tickets.delete(opt.ticket);
        live.delete(placeholder);
      }
      return {
        content: [{ type: "text" as const, text: JSON.stringify({ report: r.report, state: r.state, err: r.err, acceptance: "未独立验收；done 仅为 worker 自报并经收据对账" }, null, 2) }],
        details: {
        ok: r.state === "done" || r.state === "blocked",
        acceptance: "not_independently_verified",
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

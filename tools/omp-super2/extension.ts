// omp-super2:omp-superagent 的 UI/UX 迭代层。
// 复用 v1 已验证的 core/report/worker,本目录只加呈现与交互,不改 v1。
// v1 由另一车道集成;这里不写 ../omp-superagent 下任何文件。
import type { ExtensionAPI, ExtensionContext } from "@oh-my-pi/pi-coding-agent";
import { pathsConflict } from "../omp-superagent/report.ts";
import { runDispatch, type DispatchOpts, type DispatchResult, type Role } from "../omp-superagent/core.ts";
import { SA2_VERSION } from "./version.ts";
import {
  boardLines,
  callLine,
  detailLines,
  formatDuration,
  notificationText,
  resultLines,
  statusText,
  viewFromResult,
  type DispatchDetails,
  type TicketView,
} from "./ui.ts";

const MAX_PARALLEL = 8;
const MAX_DONE = 20;

interface Live {
  ticket: string;
  opts: DispatchOpts;
  started: number;
  controller: AbortController;
}

class Lines {
  constructor(private readonly rows: string[]) {}
  render(): readonly string[] {
    return this.rows;
  }
}

function callArgs(args: unknown): { ticket?: string; role?: string } {
  if (!args || typeof args !== "object") return {};
  const ticket = "ticket" in args && typeof args.ticket === "string" ? args.ticket : undefined;
  const role = "role" in args && typeof args.role === "string" ? args.role : undefined;
  return { ticket, role };
}

function acceptanceText(acceptance: DispatchResult["acceptance"]): string {
  if (!acceptance) return "not_independently_verified";
  return acceptance.ok ? "passed" : acceptance.err;
}

function statusCounts(details: unknown): { running: number; finished: number } {
  if (!details || typeof details !== "object") return { running: 0, finished: 0 };
  const running = "running" in details && typeof details.running === "number" ? details.running : 0;
  const finished = "finished" in details && typeof details.finished === "number" ? details.finished : 0;
  return { running, finished };
}

export default function (pi: ExtensionAPI) {
  const z = pi.zod;
  pi.setLabel("omp-super2");
  const live = new Map<string, Live>();
  const done: DispatchResult[] = [];
  const tickets = new Set<string>();
  let detail: TicketView | undefined;
  let tick: NodeJS.Timeout | undefined;
  let lastCtx: ExtensionContext | undefined;
  let closed = false;

  const views = (): TicketView[] => [
    ...[...live.values()].map((l) => ({
      ticket: l.ticket,
      role: l.opts.role,
      state: "running" as const,
      started: l.started,
    })),
    ...done.slice(-MAX_DONE).map(viewFromResult),
  ];

  const refresh = (ctx: ExtensionContext) => {
    if (closed) return;
    lastCtx = ctx;
    const now = Date.now();
    const all = views();
    if (detail) ctx.ui.setWidget("sa2", detailLines(detail, now, ctx.ui.theme));
    else if (live.size > 0) ctx.ui.setWidget("sa2", boardLines(all, now, ctx.ui.theme));
    else ctx.ui.setWidget("sa2", undefined);
    ctx.ui.setStatus("sa2", statusText(all, now, ctx.ui.theme));
    if (live.size === 0 && !detail) stopTick();
  };

  const stopTick = () => {
    if (!tick) return;
    clearInterval(tick);
    tick = undefined;
  };

  const startTick = (ctx: ExtensionContext) => {
    lastCtx = ctx;
    if (tick) return;
    tick = setInterval(() => {
      if (lastCtx && (live.size > 0 || detail)) refresh(lastCtx);
      else stopTick();
    }, 1000);
    tick.unref?.();
  };

  const find = (ref: string): TicketView | undefined => {
    return views().find((v) => v.ticket === ref || v.id === ref);
  };

  const show = (ref: string, ctx: ExtensionContext) => {
    const hit = find(ref);
    if (!hit) {
      ctx.ui.notify(`omp-super2: 没有 ${ref}`, "warning");
      return;
    }
    detail = hit;
    refresh(ctx);
    ctx.ui.notify(`详情: ${hit.ticket} · /sa close 关闭`, "info");
  };

  pi.registerTool({
    name: "dispatch",
    label: "dispatch",
    description:
      "派活:起一个子 omp 执行结构化 ticket，implementer 必须提供允许写入的 paths。" +
      "edit/write 检查路径；bash 不隔离，只读及路径边界依赖 worker 协作，不能防恶意命令。" +
      "可选 checks 由宿主检查文件，结果仅覆盖所列条件；bash 不隔离，RPC 轨迹可能含敏感信息。",
    parameters: z.object({
      ticket: z.string().min(1).describe("一句话任务名,会话内唯一"),
      task: z.object({
        goal: z.string().trim().min(1).describe("明确目标"),
        context: z.array(z.string().trim().min(1)).min(1).describe("上下文及读取路径"),
        accept: z.array(z.string().trim().min(1)).min(1).describe("验收条件；不适用时写 NOT_APPLICABLE"),
        nonGoals: z.array(z.string().trim().min(1)).min(1).describe("非目标"),
      }).describe("结构化任务合同"),
      role: z.enum(["implementer", "reviewer", "scout"]).optional().describe("缺省 implementer"),
      model: z.string().optional(),
      tools: z.string().optional().describe("逗号分隔的工具名"),
      paths: z.array(z.string()).optional().describe("implementer 允许写入的路径"),
      checks: z.array(z.union([
        z.object({ kind: z.literal("exists"), path: z.string().min(1) }),
        z.object({ kind: z.literal("text"), path: z.string().min(1), text: z.string() }),
      ])).min(1).optional().describe("宿主独立文件断言，仅覆盖所列条件"),
      maxTime: z.number().int().positive().optional().describe("秒,缺省 300"),
    }),
    renderCall(args, _options, theme) {
      return new Lines([callLine(callArgs(args), theme)]);
    },
    renderResult(result, options, theme) {
      return new Lines([...resultLines(result.details as DispatchDetails | undefined, options.expanded, theme)]);
    },
    async execute(_callId, params, signal, onUpdate, ctx) {
      const opt = params as DispatchOpts;
      const role: Role = opt.role ?? "implementer";
      const fail = (err: string) => {
        ctx.ui.notify(`✗ ${err}`, "error");
        return { content: [{ type: "text" as const, text: err }], isError: true };
      };
      if (tickets.has(opt.ticket)) return fail(`ticket 已派发过: ${opt.ticket}`);
      if (live.size >= MAX_PARALLEL) return fail(`并行上限 ${MAX_PARALLEL}`);
      if (opt.paths) {
        for (const [id, l] of live) {
          const c = l.opts.paths ? pathsConflict(opt.paths, l.opts.paths) : undefined;
          if (c) return fail(`路径与在飞 dispatch ${id} 冲突(${c}),请串行`);
        }
      }

      const controller = new AbortController();
      const abort = () => controller.abort();
      if (signal?.aborted) controller.abort();
      else signal?.addEventListener("abort", abort, { once: true });

      const started = Date.now();
      const entry: Live = { ticket: opt.ticket, opts: { ...opt, role }, started, controller };
      live.set(opt.ticket, entry);
      tickets.add(opt.ticket);
      startTick(ctx);
      refresh(ctx);
      onUpdate?.({
        content: [{ type: "text", text: `▶ ${opt.ticket} 运行中` }],
        details: {
          ticket: opt.ticket,
          role,
          state: "running",
          startedAt: started,
          uiVersion: SA2_VERSION,
        } satisfies DispatchDetails,
      });

      let r: DispatchResult | undefined;
      try {
        r = await runDispatch({ ...entry.opts, signal: controller.signal });
        done.push(r);
        if (done.length > MAX_DONE) done.splice(0, done.length - MAX_DONE);
      } finally {
        signal?.removeEventListener("abort", abort);
        if (!r?.record || (r.state === "error" && r.err?.startsWith("记录失败:"))) tickets.delete(opt.ticket);
        live.delete(opt.ticket);
        refresh(ctx);
      }

      const view = r ? viewFromResult(r) : undefined;
      const msg = view ? notificationText(view) : undefined;
      if (msg) ctx.ui.notify(msg, view?.state === "blocked" ? "warning" : "error");

      return {
        content: [{
          type: "text" as const,
          text: JSON.stringify({
            report: r.report,
            state: r.state,
            err: r.err,
            acceptance: r.acceptance ?? "未独立验收",
          }, null, 2),
        }],
        details: {
          ok: r.state === "done" || r.state === "blocked",
          acceptance: acceptanceText(r.acceptance),
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
          startedAt: started,
          uiVersion: SA2_VERSION,
        } satisfies DispatchDetails,
      };
    },
  });

  pi.registerTool({
    name: "status",
    label: "status",
    description: "列出本会话 dispatch(时间倒序)。给 id 则打印该条 record 全文。",
    parameters: z.object({ id: z.string().optional() }),
    renderResult(result, _options, theme) {
      const counts = statusCounts(result.details);
      return new Lines([theme.fg("muted", `sa2 · ${counts.running} running · ${counts.finished} finished`)]);
    },
    async execute(_callId, params, _signal, _onUpdate, ctx) {
      const id = (params as { id?: string }).id;
      const hit = id ? find(id) : undefined;
      if (id && !hit) return { content: [{ type: "text" as const, text: `没有 ${id}` }], isError: true };
      if (hit) {
        const full = done.find((d) => d.id === hit.id || d.ticket === hit.ticket);
        return { content: [{ type: "text" as const, text: JSON.stringify(full ?? hit, null, 2) }] };
      }
      const running = [...live.values()].map((l) => ({
        ticket: l.ticket,
        role: l.opts.role,
        state: "running",
        ms: Date.now() - l.started,
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
      refresh(ctx);
      return {
        content: [{
          type: "text" as const,
          text: JSON.stringify({ uiVersion: SA2_VERSION, running, finished }, null, 2),
        }],
        details: { running: running.length, finished: finished.length },
      };
    },
  });

  pi.registerCommand("sa", {
    description: "omp-super2 面板: /sa · /sa <id|ticket> · /sa cancel <id|ticket> · /sa clear · /sa close",
    getArgumentCompletions(prefix) {
      const refs = views().flatMap((v) => [v.ticket, v.id].filter((x): x is string => Boolean(x)));
      const words = ["cancel", "clear", "close", ...refs];
      return words
        .filter((w) => w.startsWith(prefix.trim()))
        .map((w) => ({ value: w, label: w }));
    },
    async handler(argv, ctx) {
      const raw = argv.trim();
      const [verb, ...rest] = raw.split(/\s+/);
      if (verb === "close") {
        detail = undefined;
        refresh(ctx);
        return;
      }
      if (verb === "clear") {
        done.splice(0, done.length);
        detail = undefined;
        refresh(ctx);
        ctx.ui.notify("omp-super2: 已清空完成记录", "info");
        return;
      }
      if (verb === "cancel") {
        const ref = rest.join(" ").trim();
        const hit = [...live.values()].find((l) => l.ticket === ref);
        if (!hit) {
          ctx.ui.notify(`omp-super2: 没有在飞 ${ref || "(缺 ticket)"}`, "warning");
          return;
        }
        hit.controller.abort();
        ctx.ui.notify(`⊘ 已请求取消 ${hit.ticket}`, "warning");
        refresh(ctx);
        return;
      }
      if (raw) {
        show(raw, ctx);
        return;
      }
      const all = views();
      if (all.length === 0) {
        ctx.ui.notify("omp-super2: 无 dispatch", "info");
        return;
      }
      const byLabel = new Map<string, TicketView>();
      const options = all.map((v) => {
        const gone = (v.ended ?? Date.now()) - v.started;
        const label = `${v.state === "running" ? "▶" : "·"} ${v.ticket} · ${v.role} · ${formatDuration(gone)} · ${v.state}`;
        byLabel.set(label, v);
        return { label, description: v.summary ?? v.err ?? "" };
      });
      const picked = await ctx.ui.select("omp-super2", options);
      const view = picked ? byLabel.get(picked) : undefined;
      if (view) {
        detail = view;
        refresh(ctx);
        ctx.ui.notify(`详情: ${view.ticket} · /sa close 关闭`, "info");
      }
    },
  });

  pi.on("session_start", async (_event, ctx) => {
    closed = false;
    refresh(ctx);
  });

  pi.on("session_shutdown", async (_event, ctx) => {
    closed = true;
    for (const l of live.values()) l.controller.abort();
    stopTick();
    ctx.ui.setStatus("sa2", undefined);
    ctx.ui.setWidget("sa2", undefined);
  });
}

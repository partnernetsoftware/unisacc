import { describe, expect, test } from "bun:test";
import { mkdtempSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import type { ExtensionAPI, ExtensionContext } from "@oh-my-pi/pi-coding-agent";
import extension from "../extension.ts";

function schema(): unknown {
  return new Proxy((): unknown => schema(), { get: () => (): unknown => schema() });
}

interface Calls {
  status: [string, string | undefined][];
  widgets: [string, unknown][];
  notices: [string, string | undefined][];
}

function fakeCtx(calls: Calls): ExtensionContext {
  return {
    ui: {
      theme: { fg: (_color: string, text: string) => text, bold: (text: string) => text },
      setStatus: (key: string, text: string | undefined) => calls.status.push([key, text]),
      setWidget: (key: string, value: unknown) => calls.widgets.push([key, value]),
      notify: (message: string, type?: string) => calls.notices.push([message, type]),
      select: async (_title: string, options: Array<string | { label: string }>) => {
        const first = options[0];
        return typeof first === "string" ? first : first?.label;
      },
    },
  } as unknown as ExtensionContext;
}

function setup() {
  const calls: Calls = { status: [], widgets: [], notices: [] };
  const tools = new Map<string, {
    execute: (id: string, params: object, signal?: AbortSignal, update?: (u: unknown) => void, ctx?: ExtensionContext) => Promise<{
      isError?: boolean;
      details?: { state?: string };
      content: Array<{ text: string }>;
    }>;
  }>();
  const commands = new Map<string, { handler: (args: string, ctx: ExtensionContext) => Promise<void> }>();
  const handlers = new Map<string, (event: unknown, ctx: ExtensionContext) => Promise<void>>();
  const host = {
    zod: schema(),
    setLabel() {},
    registerTool(tool: { name: string }) { tools.set(tool.name, tool as never); },
    registerCommand(name: string, options: { handler: (args: string, ctx: ExtensionContext) => Promise<void> }) {
      commands.set(name, options);
    },
    on(event: string, handler: (event: unknown, ctx: ExtensionContext) => Promise<void>) {
      handlers.set(event, handler);
    },
  };
  extension(host as unknown as ExtensionAPI);
  const ctx = fakeCtx(calls);
  return { calls, tools, commands, handlers, ctx };
}

const task = {
  goal: "定位示例",
  context: ["example.txt"],
  accept: ["NOT_APPLICABLE"],
  nonGoals: ["不修改文件"],
};

describe("omp-super2 extension", () => {
  test("dispatch streams a running card, finishes quietly, and /sa shows detail", async () => {
    const { calls, tools, commands, ctx } = setup();
    const oldBin = process.env.SA_OMP_BIN;
    const oldMode = process.env.FAKE_MODE;
    process.env.SA_OMP_BIN = join(import.meta.dir, "../../omp-superagent/test/fake-omp.sh");
    process.env.FAKE_MODE = "done";
    const updates: unknown[] = [];
    try {
      const dispatch = tools.get("dispatch")!;
      const result = await dispatch.execute(
        "1",
        { ticket: "T1", task, role: "scout", cwd: mkdtempSync(join(tmpdir(), "sa2-ext-")) },
        undefined,
        (u) => updates.push(u),
        ctx,
      );
      expect(result.details?.state).toBe("done");
      expect(updates).toHaveLength(1);
      expect(JSON.stringify(updates[0])).toContain("running");
      expect(calls.status.some(([, text]) => text?.includes("sa2"))).toBe(true);
      expect(calls.widgets.some(([, value]) => value != null && JSON.stringify(value).includes("T1"))).toBe(true);
      expect(calls.notices).toHaveLength(0);

      await commands.get("sa")!.handler("", ctx);
      expect(calls.widgets.some(([, value]) => value != null && JSON.stringify(value).includes("verified 1"))).toBe(true);
      expect(calls.notices.some(([message]) => message.includes("详情"))).toBe(true);
    } finally {
      if (oldBin === undefined) delete process.env.SA_OMP_BIN; else process.env.SA_OMP_BIN = oldBin;
      if (oldMode === undefined) delete process.env.FAKE_MODE; else process.env.FAKE_MODE = oldMode;
    }
  });

  test("blocked surfaces as one warning", async () => {
    const { calls, tools, ctx } = setup();
    const oldBin = process.env.SA_OMP_BIN;
    const oldMode = process.env.FAKE_MODE;
    process.env.SA_OMP_BIN = join(import.meta.dir, "../../omp-superagent/test/fake-omp.sh");
    process.env.FAKE_MODE = "blocked";
    try {
      const blocked = await tools.get("dispatch")!.execute(
        "2",
        { ticket: "T1", task, role: "scout", cwd: mkdtempSync(join(tmpdir(), "sa2-ext-")) },
        undefined,
        undefined,
        ctx,
      );
      expect(blocked.details?.state).toBe("blocked");
      expect(calls.notices.some(([, type]) => type === "warning")).toBe(true);
    } finally {
      if (oldBin === undefined) delete process.env.SA_OMP_BIN; else process.env.SA_OMP_BIN = oldBin;
      if (oldMode === undefined) delete process.env.FAKE_MODE; else process.env.FAKE_MODE = oldMode;
    }
  });

  test("cancel aborts a running ticket", async () => {
    const { calls, tools, commands, ctx } = setup();
    const oldBin = process.env.SA_OMP_BIN;
    const oldMode = process.env.FAKE_MODE;
    process.env.SA_OMP_BIN = join(import.meta.dir, "../../omp-superagent/test/fake-omp.sh");
    process.env.FAKE_MODE = "hang";
    try {
      const running = tools.get("dispatch")!.execute(
        "3",
        { ticket: "T1", task, role: "scout", cwd: mkdtempSync(join(tmpdir(), "sa2-ext-")) },
        undefined,
        undefined,
        ctx,
      );
      // execute() registers the live ticket before its first await.
      await commands.get("sa")!.handler("cancel T1", ctx);
      const cancelled = await running;
      expect(cancelled.details?.state).toBe("error");
      expect(calls.notices.some(([message]) => message.includes("取消"))).toBe(true);
    } finally {
      if (oldBin === undefined) delete process.env.SA_OMP_BIN; else process.env.SA_OMP_BIN = oldBin;
      if (oldMode === undefined) delete process.env.FAKE_MODE; else process.env.FAKE_MODE = oldMode;
    }
  }, 20_000);

  test("duplicate ticket is rejected while the first is in flight, and shutdown clears the panel", async () => {
    const { calls, tools, handlers, ctx } = setup();
    const oldBin = process.env.SA_OMP_BIN;
    const oldMode = process.env.FAKE_MODE;
    process.env.SA_OMP_BIN = join(import.meta.dir, "../../omp-superagent/test/fake-omp.sh");
    process.env.FAKE_MODE = "hang";
    try {
      const dispatch = tools.get("dispatch")!;
      const params = { ticket: "T1", task, role: "scout", cwd: mkdtempSync(join(tmpdir(), "sa2-ext-")) };
      const running = dispatch.execute("4", params, undefined, undefined, ctx);
      // execute() registers the live ticket before its first await.
      const duplicate = await dispatch.execute("5", params, undefined, undefined, ctx);
      expect(duplicate.isError).toBe(true);
      await handlers.get("session_shutdown")!({}, ctx);
      const after = await running;
      expect(after.details?.state).toBe("error");
      expect(calls.status.at(-1)?.[1]).toBeUndefined();
      expect(calls.widgets.at(-1)?.[1]).toBeUndefined();
    } finally {
      if (oldBin === undefined) delete process.env.SA_OMP_BIN; else process.env.SA_OMP_BIN = oldBin;
      if (oldMode === undefined) delete process.env.FAKE_MODE; else process.env.FAKE_MODE = oldMode;
    }
  }, 20_000);
});

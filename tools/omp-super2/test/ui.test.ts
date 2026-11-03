import { describe, expect, test } from "bun:test";
import {
  boardLines,
  detailLines,
  formatDuration,
  iconFor,
  notificationText,
  resultLines,
  statusText,
  viewFromDetails,
  type Style,
  type TicketView,
} from "../ui.ts";

const theme: Style = { fg: (_color, text) => text, bold: (text) => text };

function view(over: Partial<TicketView> = {}): TicketView {
  return {
    ticket: "window-scout",
    role: "scout",
    state: "running",
    started: 1_000,
    ...over,
  };
}

describe("omp-super2 UI", () => {
  test("state icons and colors are stable", () => {
    expect(iconFor("running")).toBe("▶");
    expect(iconFor("done")).toBe("✓");
    expect(iconFor("blocked")).toBe("⏸");
    expect(iconFor("timeout")).toBe("⌛");
    expect(iconFor("cancelled")).toBe("⊘");
  });

  test("duration crosses minutes and hours", () => {
    expect(formatDuration(0)).toBe("00:00");
    expect(formatDuration(65_000)).toBe("01:05");
    expect(formatDuration(3_725_000)).toBe("1:02:05");
  });

  test("footer names the running ticket and elapsed time", () => {
    const text = statusText([view()], 13_000, theme);
    expect(text).toContain("sa2");
    expect(text).toContain("1 run");
    expect(text).toContain("window-scout");
    expect(text).toContain("00:12");
  });

  test("board puts running first and finished last", () => {
    const lines = boardLines([
      view({ ticket: "old", state: "done", ended: 2_000 }),
      view({ ticket: "live", state: "running" }),
    ], 3_000, theme);
    expect(lines[0]).toContain("1 running");
    expect(lines[1]).toContain("live");
    expect(lines[2]).toContain("old");
  });

  test("detail shows evidence and record", () => {
    const lines = detailLines(view({
      state: "done",
      ended: 4_000,
      id: "id-1",
      summary: "找到了入口",
      verified: 3,
      files: 0,
      record: "out/superagent/id-1.json",
    }), 4_000, theme);
    expect(lines.join("\n")).toContain("done");
    expect(lines.join("\n")).toContain("verified 3 · files 0");
    expect(lines.join("\n")).toContain("找到了入口");
    expect(lines.join("\n")).toContain("out/superagent/id-1.json");
  });

  test("only attention-worthy terminal states notify", () => {
    expect(notificationText(view({ state: "running" }))).toBeUndefined();
    expect(notificationText(view({ state: "done", ended: 2_000 }))).toBeUndefined();
    expect(notificationText(view({ state: "blocked", err: "缺凭证", ended: 2_000 }))).toContain("需要人");
    expect(notificationText(view({ state: "failed", err: "测试失败", ended: 2_000 }))).toContain("测试失败");
  });

  test("result card renders partial and final states", () => {
    const running = resultLines({ ticket: "t", role: "scout", state: "running", startedAt: Date.now() }, false, theme);
    expect(running.join("\n")).toContain("运行中");
    const final = resultLines({
      ticket: "t",
      role: "scout",
      state: "done",
      report: { summary: "ok", verified: [{}, {}], files: [] },
      record: "out/superagent/x.json",
      ms: 1_500,
    }, true, theme);
    expect(final.join("\n")).toContain("verified 2 · files 0");
    expect(final.join("\n")).toContain("out/superagent/x.json");
  });

  test("details without a ticket are rejected", () => {
    expect(viewFromDetails({ state: "done" })).toBeUndefined();
    expect(viewFromDetails({ ticket: "t", role: "reviewer", state: "done", ms: 1000 })).toMatchObject({
      ticket: "t",
      role: "reviewer",
      state: "done",
    });
  });
});

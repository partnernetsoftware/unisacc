/** CDP probe for monopoly demo — boot + synthetic Space roll. */
import { spawn } from "child_process";
import { setTimeout as sleep } from "timers/promises";
import fs from "fs";

const WALL_MS = 45_000;
const CHROME = "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome";
const URL = "http://127.0.0.1:8765/engine/demo/monopoly/?t=" + Date.now();
const PORT = 9361;
const PROFILE = "/tmp/uxe-mono-" + process.pid;
const deadline = Date.now() + WALL_MS;
const left = () => Math.max(0, deadline - Date.now());

fs.rmSync(PROFILE, { recursive: true, force: true });
fs.mkdirSync(PROFILE, { recursive: true });
const chrome = spawn(CHROME, [
  `--remote-debugging-port=${PORT}`, "--headless=new", "--use-angle=swiftshader",
  "--enable-webgl", "--ignore-gpu-blocklist", "--no-first-run",
  `--user-data-dir=${PROFILE}`, URL,
], { stdio: ["ignore", "ignore", "pipe"] });

function cdp(ws, id, method, params = {}) {
  return new Promise((resolve, reject) => {
    const t = setTimeout(() => reject(new Error(method)), Math.min(12000, left() || 1));
    const onMsg = (ev) => {
      let msg; try { msg = JSON.parse(ev.data); } catch { return; }
      if (msg.id !== id) return;
      clearTimeout(t); ws.removeEventListener("message", onMsg);
      if (msg.error) reject(new Error(JSON.stringify(msg.error)));
      else resolve(msg.result);
    };
    ws.addEventListener("message", onMsg);
    ws.send(JSON.stringify({ id, method, params }));
  });
}

try {
  while (Date.now() < deadline) {
    try { if ((await fetch(`http://127.0.0.1:${PORT}/json/version`)).ok) break; } catch {}
    await sleep(Math.min(200, left() || 1));
  }
  const tabs = await (await fetch(`http://127.0.0.1:${PORT}/json/list`)).json();
  const page = tabs.find((t) => t.type === "page") || tabs[0];
  const ws = new WebSocket(page.webSocketDebuggerUrl);
  await new Promise((res, rej) => {
    ws.addEventListener("open", res);
    ws.addEventListener("error", rej);
    setTimeout(() => rej(new Error("ws")), Math.min(8000, left() || 1));
  });
  await cdp(ws, 1, "Runtime.enable");
  await cdp(ws, 2, "Page.enable");
  await cdp(ws, 3, "Page.navigate", { url: URL });

  let ready = null;
  let n = 0;
  while (Date.now() < deadline) {
    n++;
    const r = await cdp(ws, 100 + n, "Runtime.evaluate", {
      expression: `window.__UXE__ || null`,
      returnByValue: true,
    });
    const v = r.result.value;
    if (v?.ready && v.monopoly && v.money0 != null) {
      ready = v;
      break;
    }
    if (v?.ready === false) throw new Error(v.error);
    await sleep(Math.min(350, left() || 1));
  }
  if (!ready) throw new Error("never ready");

  await cdp(ws, 200, "Input.dispatchKeyEvent", {
    type: "keyDown", windowsVirtualKeyCode: 32, code: "Space", key: " ", text: " ",
  });
  await sleep(80);
  await cdp(ws, 201, "Input.dispatchKeyEvent", {
    type: "keyUp", windowsVirtualKeyCode: 32, code: "Space", key: " ",
  });

  let after = null;
  for (let i = 0; i < 40 && Date.now() < deadline; i++) {
    await sleep(200);
    const r = await cdp(ws, 300 + i, "Runtime.evaluate", {
      expression: `window.__UXE__ || null`,
      returnByValue: true,
    });
    const v = r.result.value;
    if (!v?.ready) continue;
    const moved =
      (typeof v.pos0 === "number" && v.pos0 !== 0) ||
      (typeof v.pos1 === "number" && v.pos1 !== 0) ||
      (typeof v.dice === "number" && v.dice > 0) ||
      v.money0 !== 1500 || v.money1 !== 1500 ||
      (typeof v.phase === "number" && v.phase !== 0) ||
      (typeof v.turn === "number" && v.turn !== 0);
    if (moved) { after = v; break; }
  }
  if (!after) throw new Error("Space roll produced no state change");

  console.log("OK_MONOPOLY", {
    backend: after.backend,
    money0: after.money0, money1: after.money1,
    turn: after.turn, phase: after.phase,
    pos0: after.pos0, pos1: after.pos1, dice: after.dice,
  });
  ws.close(); chrome.kill("SIGKILL"); process.exit(0);
} catch (e) {
  try { chrome.kill("SIGKILL"); } catch {}
  console.error("FAIL_MONOPOLY", e.message || e);
  process.exit(1);
}

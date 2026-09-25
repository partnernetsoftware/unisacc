/** CDP probe for monopoly ship-js under /engine/ship/monopoly/ */
import { spawn } from "child_process";
import { setTimeout as sleep } from "timers/promises";
import fs from "fs";

const WALL_MS = 50_000;
const CHROME = "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome";
const URL = "http://127.0.0.1:8765/engine/ship/monopoly/?t=" + Date.now();
const PORT = 9362;
const PROFILE = "/tmp/uxe-mono-ship-" + process.pid;
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

  let n = 0;
  while (Date.now() < deadline) {
    n++;
    const r = await cdp(ws, 100 + n, "Runtime.evaluate", {
      expression: `window.__UXE__ || null`,
      returnByValue: true,
    });
    const v = r.result.value;
    if (v?.ready && v.monopoly && v.ship && v.money0 != null) {
      console.log("OK_MONOPOLY_SHIP", {
        backend: v.backend, money0: v.money0, turn: v.turn, phase: v.phase,
      });
      ws.close(); chrome.kill("SIGKILL"); process.exit(0);
    }
    if (v?.ready === false) throw new Error(v.error);
    await sleep(Math.min(350, left() || 1));
  }
  throw new Error("never ready");
} catch (e) {
  try { chrome.kill("SIGKILL"); } catch {}
  console.error("FAIL_MONOPOLY_SHIP", e.message || e);
  process.exit(1);
}

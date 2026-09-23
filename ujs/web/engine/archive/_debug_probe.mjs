/** CDP: capture console errors from /engine/demo/ */
import { spawn } from "child_process";
import { setTimeout as sleep } from "timers/promises";
import fs from "fs";

const WALL_MS = 25_000;
const CHROME = "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome";
const URL = "http://127.0.0.1:8765/engine/demo/?t=" + Date.now();
const PORT = 9340;
const PROFILE = "/tmp/uxe-debug-" + process.pid;
const deadline = Date.now() + WALL_MS;
const left = () => Math.max(0, deadline - Date.now());

fs.rmSync(PROFILE, { recursive: true, force: true });
fs.mkdirSync(PROFILE, { recursive: true });

const chrome = spawn(CHROME, [
  `--remote-debugging-port=${PORT}`,
  "--headless=new",
  "--use-angle=swiftshader",
  "--enable-webgl",
  "--enable-unsafe-webgpu",
  "--ignore-gpu-blocklist",
  "--no-first-run",
  `--user-data-dir=${PROFILE}`,
  URL,
], { stdio: ["ignore", "ignore", "pipe"] });

const logs = [];
function cdp(ws, id, method, params = {}) {
  return new Promise((resolve, reject) => {
    const t = setTimeout(() => reject(new Error("cdp " + method)), Math.min(10000, left() || 1000));
    const onMsg = (ev) => {
      let msg; try { msg = JSON.parse(ev.data); } catch { return; }
      if (msg.method === "Runtime.consoleAPICalled") {
        logs.push((msg.params.type || "log") + ": " + (msg.params.args || []).map((a) => a.value ?? a.description).join(" "));
      }
      if (msg.method === "Runtime.exceptionThrown") {
        logs.push("EXCEPTION: " + (msg.params.exceptionDetails?.exception?.description
          || msg.params.exceptionDetails?.text || JSON.stringify(msg.params)));
      }
      if (msg.id !== id) return;
      clearTimeout(t);
      ws.removeEventListener("message", onMsg);
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
    await sleep(150);
  }
  const tabs = await (await fetch(`http://127.0.0.1:${PORT}/json/list`)).json();
  const page = tabs.find((t) => t.type === "page") || tabs[0];
  const ws = new WebSocket(page.webSocketDebuggerUrl);
  await new Promise((res, rej) => {
    ws.addEventListener("open", res);
    ws.addEventListener("error", rej);
    setTimeout(() => rej(new Error("ws")), 5000);
  });
  await cdp(ws, 1, "Runtime.enable");
  await cdp(ws, 2, "Page.enable");
  await cdp(ws, 3, "Page.navigate", { url: URL });
  await sleep(3000);
  const r = await cdp(ws, 10, "Runtime.evaluate", {
    expression: `({
      hud: document.getElementById('hud')?.innerText,
      uxe: window.__UXE__,
      err: window.__UXE_LAST_ERR__ || null
    })`,
    returnByValue: true,
  });
  console.log("STATE", JSON.stringify(r.result.value, null, 2));
  console.log("LOGS", JSON.stringify(logs.slice(-20), null, 2));
  ws.close();
  chrome.kill("SIGKILL");
  process.exit(r.result.value?.uxe?.ready ? 0 : 1);
} catch (e) {
  try { chrome.kill("SIGKILL"); } catch {}
  console.error("FAIL", e.message);
  console.error("LOGS", logs.slice(-20));
  process.exit(1);
}

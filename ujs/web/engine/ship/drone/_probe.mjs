/** CDP probe for drone ship-js under /engine/ship/drone/ */
import { spawn } from "child_process";
import { setTimeout as sleep } from "timers/promises";
import fs from "fs";

const WALL_MS = 55_000;
const CHROME = "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome";
const URL = "http://127.0.0.1:8765/engine/ship/drone/?t=" + Date.now();
const PORT = 9367;
const PROFILE = "/tmp/uxe-drone-ship-" + process.pid;
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

async function uxe(ws, id) {
  const r = await cdp(ws, id, "Runtime.evaluate", {
    expression: `window.__UXE__ || null`, returnByValue: true,
  });
  return r.result.value;
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
  for (let n = 0; Date.now() < deadline; n++) {
    const v = await uxe(ws, 100 + n);
    if (v?.ready && v.drone && v.fp && v.ship && v.ammo === 2 && v.controls === "keyboard") {
      ready = v; break;
    }
    if (v?.ready === false) throw new Error(v.error);
    await sleep(Math.min(300, left() || 1));
  }
  if (!ready) throw new Error("never ready ship keyboard ammo=2");

  await cdp(ws, 210, "Input.dispatchKeyEvent", {
    type: "keyDown", windowsVirtualKeyCode: 70, code: "KeyF", key: "f",
  });
  await sleep(100);
  let armed = null;
  for (let i = 0; i < 20; i++) {
    await sleep(100);
    const v = await uxe(ws, 220 + i);
    if (v?.suicideArm) { armed = v; break; }
  }
  await cdp(ws, 230, "Input.dispatchKeyEvent", {
    type: "keyUp", windowsVirtualKeyCode: 70, code: "KeyF", key: "f",
  });
  if (!armed) throw new Error("suicide arm via KeyF failed");

  await cdp(ws, 240, "Runtime.evaluate", {
    expression: `window.__UXE_API__ && window.__UXE_API__.setControls("mouse")`,
  });
  let mouseMode = null;
  for (let i = 0; i < 15; i++) {
    await sleep(100);
    const v = await uxe(ws, 250 + i);
    if (v?.controls === "mouse") { mouseMode = v; break; }
  }
  if (!mouseMode) throw new Error("setControls mouse failed");

  console.log("OK_DRONE_SHIP", {
    backend: mouseMode.backend, fp: true, ship: true,
    controlsDefault: "keyboard", controlsNow: mouseMode.controls,
    ammo: mouseMode.ammo, magazine: mouseMode.magazine,
    suicideArm: true,
  });
  ws.close(); chrome.kill("SIGKILL"); process.exit(0);
} catch (e) {
  try { chrome.kill("SIGKILL"); } catch {}
  console.error("FAIL_DRONE_SHIP", e.message || e);
  process.exit(1);
}

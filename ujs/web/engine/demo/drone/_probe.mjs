/** CDP: FP drone — boot, pointer, ammo magazine, suicide arm flag. */
import { spawn } from "child_process";
import { setTimeout as sleep } from "timers/promises";
import fs from "fs";

const WALL_MS = 55_000;
const CHROME = "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome";
const URL = "http://127.0.0.1:8765/engine/demo/drone/?t=" + Date.now();
const PORT = 9366;
const PROFILE = "/tmp/uxe-drone-fp-" + process.pid;
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
    if (v?.ready && v.drone && v.fp && v.ammo === 2) { ready = v; break; }
    if (v?.ready === false) throw new Error(v.error);
    await sleep(Math.min(300, left() || 1));
  }
  if (!ready) throw new Error("never ready fp ammo=2");

  const layout = await cdp(ws, 200, "Runtime.evaluate", {
    expression: `(() => { const r = document.getElementById('c').getBoundingClientRect();
      return { x:r.left,y:r.top,w:r.width,h:r.height }; })()`,
    returnByValue: true,
  });
  const L = layout.result.value;
  const cx = L.x + L.w * 0.7, cy = L.y + L.h * 0.4;

  await cdp(ws, 201, "Input.dispatchMouseEvent", {
    type: "mouseMoved", x: cx, y: cy, button: "none", buttons: 0,
  });
  await sleep(200);

  // Arm suicide via KeyF
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

  // Fly + try lock/fire path: inject look + W + clicks
  for (let k = 0; k < 12; k++) {
    await cdp(ws, 300 + k * 3, "Input.dispatchMouseEvent", {
      type: "mouseMoved", x: cx, y: cy, button: "none", buttons: 0,
    });
    await cdp(ws, 301 + k * 3, "Input.dispatchKeyEvent", {
      type: "keyDown", windowsVirtualKeyCode: 87, code: "KeyW", key: "w",
    });
    await sleep(90);
    await cdp(ws, 302 + k * 3, "Input.dispatchMouseEvent", {
      type: "mousePressed", x: cx, y: cy, button: "left", buttons: 1, clickCount: 1,
    });
    await sleep(40);
    await cdp(ws, 303 + k * 3, "Input.dispatchMouseEvent", {
      type: "mouseReleased", x: cx, y: cy, button: "left", buttons: 0, clickCount: 1,
    });
  }
  await cdp(ws, 400, "Input.dispatchKeyEvent", {
    type: "keyUp", windowsVirtualKeyCode: 87, code: "KeyW", key: "w",
  });

  let after = null;
  for (let i = 0; i < 25; i++) {
    await sleep(120);
    after = await uxe(ws, 500 + i);
    if (after?.fp && (after.suicideArm || after.ammo < 2 || after.kills > 0 || after.endReason)) break;
  }
  if (!after?.fp) throw new Error("lost fp state");
  if (after.magazine !== 2) throw new Error("magazine");
  // Must have proven suicide arm; ammo may or may not have decreased without lock
  if (!armed.suicideArm && !after.suicideArm) throw new Error("suicide never armed");

  console.log("OK_DRONE", {
    backend: after.backend, fp: true,
    ammo: after.ammo, magazine: after.magazine,
    suicideArm: after.suicideArm || armed.suicideArm,
    kills: after.kills, locked: after.locked,
    score: after.score,
  });
  ws.close(); chrome.kill("SIGKILL"); process.exit(0);
} catch (e) {
  try { chrome.kill("SIGKILL"); } catch {}
  console.error("FAIL_DRONE", e.message || e);
  process.exit(1);
}

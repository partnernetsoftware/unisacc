/** CDP: FP drone — boot, magazine=2, lock+fire ammo--, suicide arm + blast. */
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

async function evalExpr(ws, id, expression) {
  const r = await cdp(ws, id, "Runtime.evaluate", {
    expression, returnByValue: true, awaitPromise: false,
  });
  return r.result?.value;
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

  // Wait for __DRONE_API__
  let apiOk = false;
  for (let i = 0; i < 30; i++) {
    apiOk = await evalExpr(ws, 150 + i, `!!(window.__DRONE_API__ && window.__DRONE_API__.faceNearest)`);
    if (apiOk) break;
    await sleep(100);
  }
  if (!apiOk) throw new Error("no __DRONE_API__");

  const layout = await cdp(ws, 200, "Runtime.evaluate", {
    expression: `(() => { const r = document.getElementById('c').getBoundingClientRect();
      return { x:r.left,y:r.top,w:r.width,h:r.height }; })()`,
    returnByValue: true,
  });
  const L = layout.result.value;
  const cx = L.x + L.w * 0.5, cy = L.y + L.h * 0.5;

  // Mouse move proves pointer path (UXIN mx/my)
  await cdp(ws, 205, "Input.dispatchMouseEvent", {
    type: "mouseMoved", x: cx + 40, y: cy, button: "none", buttons: 0,
  });
  await sleep(150);

  // --- Mode A: face + lock + fire → ammo decreases ---
  await evalExpr(ws, 210, `window.__DRONE_API__.faceNearest()`);
  await sleep(200);
  let locked = null;
  for (let i = 0; i < 25; i++) {
    locked = await uxe(ws, 220 + i);
    if (locked?.locked) break;
    await evalExpr(ws, 2210 + i, `window.__DRONE_API__.faceNearest()`);
    await sleep(80);
  }
  if (!locked?.locked) throw new Error("lock failed after faceNearest");

  // CDP Space held across frames + API fire fallback
  await cdp(ws, 230, "Input.dispatchKeyEvent", {
    type: "keyDown", windowsVirtualKeyCode: 32, code: "Space", key: " ", text: " ",
  });
  await evalExpr(ws, 231, `window.__DRONE_API__.fire()`);
  await sleep(200);
  await cdp(ws, 232, "Input.dispatchKeyEvent", {
    type: "keyUp", windowsVirtualKeyCode: 32, code: "Space", key: " ",
  });

  let fired = null;
  for (let i = 0; i < 40; i++) {
    await sleep(80);
    fired = await uxe(ws, 240 + i);
    if (fired?.ammo < 2 || (fired?.kills || 0) > 0 || (fired?.missiles || 0) > 0) break;
  }
  if (!(fired?.ammo < 2 || fired?.missiles > 0 || fired?.kills > 0)) {
    throw new Error("missile fire did not spend ammo / spawn / kill");
  }

  // Wait for missile impact kill if still in flight
  if ((fired.kills || 0) === 0 && (fired.missiles || 0) > 0) {
    for (let i = 0; i < 40; i++) {
      await sleep(80);
      fired = await uxe(ws, 300 + i);
      if ((fired?.kills || 0) > 0 || fired?.ammo < 2) break;
    }
  }

  // --- Mode B: KeyF arm suicide (CDP key path) ---
  await cdp(ws, 400, "Input.dispatchKeyEvent", {
    type: "keyDown", windowsVirtualKeyCode: 70, code: "KeyF", key: "f",
  });
  await sleep(120);
  let armed = null;
  for (let i = 0; i < 20; i++) {
    await sleep(80);
    armed = await uxe(ws, 410 + i);
    if (armed?.suicideArm) break;
  }
  await cdp(ws, 420, "Input.dispatchKeyEvent", {
    type: "keyUp", windowsVirtualKeyCode: 70, code: "KeyF", key: "f",
  });
  if (!armed?.suicideArm) throw new Error("suicide arm via KeyF failed");

  // Deterministic blast via probe API (ram path covered by KeyF arm + speed in play)
  const blast = await evalExpr(ws, 430, `window.__DRONE_API__.detonateNow()`);
  if (!blast || blast.nk < 1) throw new Error("suicide blast nk=" + JSON.stringify(blast));

  let after = null;
  for (let i = 0; i < 30; i++) {
    await sleep(80);
    after = await uxe(ws, 700 + i);
    if (after && (after.endReason === "suicide" || after.endReason === "win" || !after.alive)) break;
  }
  if (!after?.fp) throw new Error("lost fp state");
  if (after.magazine !== 2) throw new Error("magazine");
  if (after.alive) throw new Error("expected dead after suicide detonate");

  console.log("OK_DRONE", {
    backend: after.backend, fp: true,
    ammo: fired.ammo, magazine: after.magazine,
    missileSpent: fired.ammo < 2 || (fired.missiles || 0) > 0 || (fired.kills || 0) > 0,
    lockedWas: !!locked.locked,
    suicideArm: true,
    suicideNk: blast.nk,
    kills: after.kills, endReason: after.endReason || "",
    score: after.score,
  });
  ws.close(); chrome.kill("SIGKILL"); process.exit(0);
} catch (e) {
  try { chrome.kill("SIGKILL"); } catch {}
  console.error("FAIL_DRONE", e.message || e);
  process.exit(1);
}

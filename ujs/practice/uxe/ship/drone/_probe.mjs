/** CDP probe for drone ship-js under /uxe/ship/drone/ */
import { setTimeout as sleep } from "timers/promises";
import {
  DEMO_ORIGIN, spawnChrome, makeCdp, connectPage, ensureDemoServer,
} from "../../_cdp.mjs";

const WALL_MS = 55_000;
const URL = DEMO_ORIGIN + "/uxe/ship/drone/?t=" + Date.now();
const PORT = 9367;
const PROFILE = "/tmp/uxe-drone-ship-" + process.pid;
const deadline = Date.now() + WALL_MS;
const left = () => Math.max(0, deadline - Date.now());

const server = await ensureDemoServer();
const chrome = spawnChrome({ port: PORT, profile: PROFILE });
const cdp = makeCdp(left);

async function uxe(ws, id) {
  const r = await cdp(ws, id, "Runtime.evaluate", {
    expression: `window.__UXE__ || null`, returnByValue: true,
  });
  return r.result.value;
}

async function evalExpr(ws, id, expression) {
  const r = await cdp(ws, id, "Runtime.evaluate", {
    expression, returnByValue: true,
  });
  return r.result?.value;
}

try {
  const ws = await connectPage(PORT, left);
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

  let apiOk = false;
  for (let i = 0; i < 25; i++) {
    apiOk = await evalExpr(ws, 150 + i, `!!(window.__DRONE_API__ && window.__DRONE_API__.faceNearest)`);
    if (apiOk) break;
    await sleep(80);
  }
  if (!apiOk) throw new Error("no __DRONE_API__");

  await evalExpr(ws, 210, `window.__DRONE_API__.faceNearest()`);
  await sleep(150);
  await evalExpr(ws, 211, `window.__DRONE_API__.fire()`);
  let fired = null;
  for (let i = 0; i < 30; i++) {
    await sleep(80);
    fired = await uxe(ws, 220 + i);
    if (fired?.ammo < 2 || fired?.missiles > 0 || fired?.kills > 0) break;
  }
  if (!(fired?.ammo < 2 || fired?.missiles > 0 || fired?.kills > 0)) {
    throw new Error("ship fire failed");
  }

  await cdp(ws, 230, "Input.dispatchKeyEvent", {
    type: "keyDown", windowsVirtualKeyCode: 70, code: "KeyF", key: "f",
  });
  await sleep(100);
  let armed = null;
  for (let i = 0; i < 15; i++) {
    await sleep(80);
    armed = await uxe(ws, 240 + i);
    if (armed?.suicideArm) break;
  }
  await cdp(ws, 250, "Input.dispatchKeyEvent", {
    type: "keyUp", windowsVirtualKeyCode: 70, code: "KeyF", key: "f",
  });
  if (!armed?.suicideArm) throw new Error("KeyF arm failed");

  await evalExpr(ws, 260, `window.__UXE_API__ && window.__UXE_API__.setControls("mouse")`);
  let mouseMode = null;
  for (let i = 0; i < 15; i++) {
    await sleep(80);
    mouseMode = await uxe(ws, 270 + i);
    if (mouseMode?.controls === "mouse") break;
  }
  if (!mouseMode || mouseMode.controls !== "mouse") throw new Error("setControls mouse failed");

  // Dual-touch: left finger forward thrust (iy=-1), right finger look stick
  const dual = await cdp(ws, 500, "Runtime.evaluate", {
    expression: `(() => {
      const c = document.getElementById('c');
      const r = c.getBoundingClientRect();
      const mk = (id, x, y) => ({
        pointerId: id, pointerType: 'touch', isPrimary: id === 11,
        buttons: 1, button: 0, bubbles: true, cancelable: true,
        clientX: r.left + r.width * x, clientY: r.top + r.height * y,
      });
      c.dispatchEvent(new PointerEvent('pointerdown', mk(11, 0.2, 0.7)));
      c.dispatchEvent(new PointerEvent('pointermove', mk(11, 0.2, 0.35)));
      c.dispatchEvent(new PointerEvent('pointerdown', mk(12, 0.8, 0.55)));
      c.dispatchEvent(new PointerEvent('pointermove', mk(12, 0.95, 0.55)));
      return window.__UXE_HOST__.host_input_read();
    })()`,
    returnByValue: true,
  });
  const d = dual.result.value;
  if (!d || !(d.flags & 4)) throw new Error("dual touch FLAG_TOUCH missing " + JSON.stringify(d));
  if (!(d.flags & 8)) throw new Error("dual touch FLAG_LOOK_STICK missing " + JSON.stringify(d));
  if (d.iy !== -1) throw new Error("dual left thrust iy!=-1 " + JSON.stringify(d));
  if (!(d.mx > 0.2)) throw new Error("dual right look mx " + JSON.stringify(d));

  console.log("OK_DRONE_SHIP", {
    backend: mouseMode.backend, fp: true, ship: true,
    controlsDefault: "keyboard", controlsNow: mouseMode.controls,
    ammo: fired.ammo, magazine: mouseMode.magazine,
    suicideArm: true, missileSpent: true,
    dualTouch: { iy: d.iy, mx: d.mx, flags: d.flags },
  });
  ws.close(); chrome.kill("SIGKILL"); server.stop(); process.exit(0);
} catch (e) {
  try { chrome.kill("SIGKILL"); } catch {}
  try { server.stop(); } catch {}
  console.error("FAIL_DRONE_SHIP", e.message || e);
  process.exit(1);
}

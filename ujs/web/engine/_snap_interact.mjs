/**
 * Interactive CDP probe: asteroid + drone ship pages, assert via __UXE_SNAP__.
 * Drives real pointer/key events; no screenshot asserts.
 */
import { setTimeout as sleep } from "timers/promises";
import {
  DEMO_ORIGIN, spawnChrome, makeCdp, connectPage, ensureDemoServer,
} from "./_cdp.mjs";

const WALL_MS = 90_000;
const PORT = 9377;
const PROFILE = "/tmp/uxe-snap-" + process.pid;
const FLAG_TOUCH = 4;
const FLAG_LOOK_STICK = 8;

const deadline = Date.now() + WALL_MS;
const left = () => {
  const ms = deadline - Date.now();
  if (ms <= 0) throw new Error("wall");
  return ms;
};

const server = await ensureDemoServer();
const chrome = spawnChrome({ port: PORT, profile: PROFILE });
const cdp = makeCdp(left);

async function evalExpr(ws, id, expression) {
  const r = await cdp(ws, id, "Runtime.evaluate", {
    expression, returnByValue: true, awaitPromise: false,
  });
  if (r.exceptionDetails) {
    throw new Error(JSON.stringify(r.exceptionDetails));
  }
  return r.result?.value;
}

async function snap(ws, id) {
  return evalExpr(ws, id, `(() => {
    const h = window.__UXE_HOST__;
    if (h && typeof h.host_debug_snapshot === "function") return h.host_debug_snapshot();
    return window.__UXE_SNAP__ || null;
  })()`);
}

async function waitReady(ws, startId, pred, label) {
  let last = null;
  for (let n = 0; Date.now() < deadline; n++) {
    last = await snap(ws, startId + n);
    if (last && pred(last)) return last;
    if (last?.uxe?.ready === false) throw new Error(label + " error " + (last.uxe.error || ""));
    await sleep(Math.min(300, left()));
  }
  throw new Error(label + " never ready last=" + JSON.stringify(last)?.slice(0, 400));
}

async function navigate(ws, path) {
  const url = DEMO_ORIGIN + path + (path.includes("?") ? "&" : "?") + "t=" + Date.now();
  await cdp(ws, 2, "Page.enable");
  await cdp(ws, 3, "Page.navigate", { url });
}

try {
  const ws = await connectPage(PORT, left);
  await cdp(ws, 1, "Runtime.enable");

  // —— Asteroid ship ——
  await navigate(ws, "/engine/ship/");
  const ast0 = await waitReady(ws, 100, (s) =>
    s.uxe?.ready && s.uxe.ship && s.uxe.wasmGame &&
    (s.uxe.fps > 0 || s.uxe.ujsMs > 0) &&
    s.packet?.eye && Array.isArray(s.packet.eye) &&
    s.backend,
  "asteroid");

  if (!(ast0.packet.eye.length === 3)) throw new Error("asteroid eye len");
  if (!(ast0.packet.clouds?.length >= 1)) throw new Error("asteroid no clouds in packet");
  // Chase cam sits above the playfield; eye.y should be above mean rock yMin.
  const rockY = Math.min(...ast0.packet.clouds.map((c) => c.yMin));
  if (!(ast0.packet.eye[1] > rockY - 5)) {
    throw new Error("asteroid eye not above rocks eye=" + ast0.packet.eye[1] + " rockY=" + rockY);
  }

  const stick = await evalExpr(ws, 200, `(() => {
    const c = document.getElementById('c');
    const r = c.getBoundingClientRect();
    const base = { pointerId: 7, pointerType: 'touch', isPrimary: true,
      buttons: 1, button: 0, bubbles: true, cancelable: true };
    const x0 = r.left + r.width * 0.5, y0 = r.top + r.height * 0.5;
    const x1 = r.left + r.width * 0.95;
    c.dispatchEvent(new PointerEvent('pointerdown', { ...base, clientX: x0, clientY: y0 }));
    c.dispatchEvent(new PointerEvent('pointermove', { ...base, clientX: x1, clientY: y0 }));
    return window.__UXE_HOST__.host_debug_snapshot();
  })()`);
  if (!stick?.input || stick.input.ix !== 1 || !(stick.input.flags & FLAG_TOUCH)) {
    throw new Error("asteroid touch snap " + JSON.stringify(stick?.input));
  }
  if (!(stick.inputRing?.length >= 1)) throw new Error("asteroid inputRing empty");

  // W /↑ → iy=-1 must raise chase-cam eye.y (world +Y = screen up)
  const eyeBefore = stick.packet?.eye?.[1];
  await cdp(ws, 210, "Input.dispatchKeyEvent", {
    type: "keyDown", windowsVirtualKeyCode: 87, code: "KeyW", key: "w",
  });
  let eyeAfter = eyeBefore;
  for (let i = 0; i < 20; i++) {
    await sleep(50);
    const s = await snap(ws, 220 + i);
    eyeAfter = s?.packet?.eye?.[1];
    if (typeof eyeBefore === "number" && typeof eyeAfter === "number" &&
        eyeAfter > eyeBefore + 0.02) break;
  }
  await cdp(ws, 240, "Input.dispatchKeyEvent", {
    type: "keyUp", windowsVirtualKeyCode: 87, code: "KeyW", key: "w",
  });
  if (!(typeof eyeBefore === "number" && eyeAfter > eyeBefore + 0.02)) {
    throw new Error("asteroid W did not raise eye.y before=" + eyeBefore + " after=" + eyeAfter);
  }

  console.log("OK_SNAP_ASTEROID", {
    backend: ast0.backend,
    eye: ast0.packet.eye,
    clouds: ast0.packet.clouds.length,
    touch: { ix: stick.input.ix, flags: stick.input.flags },
    wRaisesEye: { before: eyeBefore, after: eyeAfter },
  });

  // —— Drone ship ——
  await navigate(ws, "/engine/ship/drone/");
  const dr0 = await waitReady(ws, 300, (s) =>
    s.uxe?.ready && s.uxe.drone && s.uxe.ship && s.uxe.ammo === 2 &&
    s.packet?.eye && s.packet.clouds?.length >= 1,
  "drone");

  // Ground plane is near y≈0; camera must sit above it (sky/ground orientation).
  const ground = dr0.packet.clouds.reduce((best, c) =>
    (best == null || c.yMax < best.yMax) ? c : best, null);
  if (!ground) throw new Error("drone no ground cloud");
  if (!(dr0.packet.eye[1] > ground.yMax + 1)) {
    throw new Error(
      "drone eye below/near ground eye=" + dr0.packet.eye[1] +
      " ground.yMax=" + ground.yMax,
    );
  }

  await evalExpr(ws, 400, `!!(window.__DRONE_API__ && window.__DRONE_API__.faceNearest)`);
  await evalExpr(ws, 401, `window.__DRONE_API__.faceNearest()`);
  await sleep(120);
  await evalExpr(ws, 402, `window.__DRONE_API__.fire()`);
  let fired = null;
  for (let i = 0; i < 30; i++) {
    await sleep(80);
    fired = await snap(ws, 410 + i);
    if (fired?.uxe && (fired.uxe.ammo < 2 || fired.uxe.missiles > 0 || fired.uxe.kills > 0)) break;
  }
  if (!(fired?.uxe && (fired.uxe.ammo < 2 || fired.uxe.missiles > 0 || fired.uxe.kills > 0))) {
    throw new Error("drone fire via snap " + JSON.stringify(fired?.uxe));
  }

  const dual = await evalExpr(ws, 500, `(() => {
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
    return window.__UXE_HOST__.host_debug_snapshot();
  })()`);
  const di = dual?.input;
  if (!di || !(di.flags & FLAG_TOUCH)) throw new Error("drone dual FLAG_TOUCH " + JSON.stringify(di));
  if (!(di.flags & FLAG_LOOK_STICK)) throw new Error("drone dual LOOK_STICK " + JSON.stringify(di));
  if (di.iy !== -1) throw new Error("drone dual iy " + JSON.stringify(di));
  if (!(di.mx > 0.2)) throw new Error("drone dual mx " + JSON.stringify(di));

  console.log("OK_SNAP_DRONE", {
    backend: dr0.backend,
    eye: dr0.packet.eye,
    groundYMax: ground.yMax,
    ammo: fired.uxe.ammo,
    dual: { iy: di.iy, mx: di.mx, flags: di.flags },
  });

  console.log("OK_SNAP_INTERACT", { asteroid: true, drone: true });
  ws.close();
  chrome.kill("SIGKILL");
  server.stop();
  process.exit(0);
} catch (e) {
  try { chrome.kill("SIGKILL"); } catch { /* */ }
  try { server.stop(); } catch { /* */ }
  console.error("FAIL_SNAP_INTERACT", e.message || e);
  process.exit(1);
}

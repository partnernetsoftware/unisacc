/** CDP probe for /engine/ship/ — wall clock. */
import { setTimeout as sleep } from "timers/promises";
import {
  DEMO_ORIGIN, spawnChrome, makeCdp, connectPage, ensureDemoServer,
} from "../_cdp.mjs";

const WALL_MS = 40_000;
const URL = DEMO_ORIGIN + "/engine/ship/?t=" + Date.now();
const PORT = 9341;
const PROFILE = "/tmp/uxe-ship-" + process.pid;
const deadline = Date.now() + WALL_MS;
const left = () => {
  const ms = deadline - Date.now();
  if (ms <= 0) throw new Error("wall");
  return ms;
};

const server = await ensureDemoServer();
const chrome = spawnChrome({ port: PORT, profile: PROFILE });
const cdp = makeCdp(left);

try {
  const ws = await connectPage(PORT, left);
  await cdp(ws, 1, "Runtime.enable");
  await cdp(ws, 2, "Page.enable");
  await cdp(ws, 3, "Page.navigate", { url: URL });
  let n = 0;
  while (Date.now() < deadline) {
    n++;
    const r = await cdp(ws, 100 + n, "Runtime.evaluate", {
      expression: `({ uxe: window.__UXE__ || null, scripts: [...document.scripts].map(s => s.src.split('/').pop()) })`,
      returnByValue: true,
    });
    const v = r.result.value || {};
    if (v.uxe?.ready && v.uxe.ship && v.uxe.wasmGame && v.uxe.wasmEngine &&
        (v.uxe.fps > 0 || v.uxe.ujsMs > 0 || v.uxe.score > 0)) {
      const inlineOnly = (v.scripts || []).every((s) => !s || s === "");
      if (!inlineOnly) throw new Error("expected no external script src");

      const layout = await cdp(ws, 200, "Runtime.evaluate", {
        expression: `(() => {
          const c = document.getElementById('c');
          const r = c.getBoundingClientRect();
          const x0 = r.left + r.width * 0.5;
          const y0 = r.top + r.height * 0.5;
          const x1 = r.left + r.width * 0.95;
          const base = { pointerId: 7, pointerType: 'touch', isPrimary: true,
            buttons: 1, button: 0, bubbles: true, cancelable: true };
          c.dispatchEvent(new PointerEvent('pointerdown', { ...base, clientX: x0, clientY: y0 }));
          c.dispatchEvent(new PointerEvent('pointermove', { ...base, clientX: x1, clientY: y0 }));
          return { touchAction: getComputedStyle(c).touchAction, w: r.width };
        })()`,
        returnByValue: true,
      });
      const ta = layout.result.value?.touchAction;
      if (ta !== "none") throw new Error("touch-action " + ta);
      let stick = null;
      let lastSnap = null;
      for (let i = 0; i < 25; i++) {
        await sleep(40);
        const ir = await cdp(ws, 300 + i, "Runtime.evaluate", {
          expression: `(() => {
            const h = window.__UXE_HOST__;
            if (!h) return null;
            return h.host_input_read();
          })()`,
          returnByValue: true,
        });
        const s = ir.result.value;
        lastSnap = s;
        if (s && s.ix === 1 && (s.flags & 4) && s.fire === 1) {
          stick = s; break;
        }
      }
      if (!stick) {
        throw new Error("touch stick did not produce ix=1 FLAG_TOUCH last=" + JSON.stringify(lastSnap));
      }
      console.log("OK_SHIP", {
        backend: v.uxe.backend,
        scripts: v.scripts,
        inlineOnly,
        ujsMs: v.uxe.ujsMs,
        fps: v.uxe.fps,
        n: v.uxe.n,
        wasmGame: v.uxe.wasmGame,
        wasmEngine: v.uxe.wasmEngine,
        touchStick: { ix: stick.ix, flags: stick.flags, mx: stick.mx },
      });
      ws.close(); chrome.kill("SIGKILL"); server.stop(); process.exit(0);
    }
    if (v.uxe?.ready === false) throw new Error(v.uxe.error);
    await sleep(Math.min(350, left()));
  }
  throw new Error("never ready");
} catch (e) {
  try { chrome.kill("SIGKILL"); } catch {}
  try { server.stop(); } catch {}
  console.error("FAIL_SHIP", e.message || e);
  process.exit(1);
}

/** CDP probe for /engine/ship/ — wall clock. */
import { spawn } from "child_process";
import { setTimeout as sleep } from "timers/promises";
import fs from "fs";

const WALL_MS = 40_000;
const CHROME = "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome";
const URL = "http://127.0.0.1:8765/engine/ship/?t=" + Date.now();
const PORT = 9341;
const PROFILE = "/tmp/uxe-ship-" + process.pid;
const deadline = Date.now() + WALL_MS;
const left = () => {
  const ms = deadline - Date.now();
  if (ms <= 0) throw new Error("wall");
  return ms;
};

fs.rmSync(PROFILE, { recursive: true, force: true });
fs.mkdirSync(PROFILE, { recursive: true });
const chromeEnv = {
  ...process.env,
  http_proxy: "", https_proxy: "", HTTP_PROXY: "", HTTPS_PROXY: "",
  ALL_PROXY: "", all_proxy: "", NO_PROXY: "*", no_proxy: "*",
};
const chrome = spawn(CHROME, [
  `--remote-debugging-port=${PORT}`, "--headless=new", "--use-angle=swiftshader",
  "--enable-webgl", "--ignore-gpu-blocklist", "--no-first-run",
  "--no-proxy-server", "--proxy-bypass-list=*",
  `--user-data-dir=${PROFILE}`, "about:blank",
], { stdio: ["ignore", "ignore", "pipe"], env: chromeEnv });

function cdp(ws, id, method, params = {}) {
  return new Promise((resolve, reject) => {
    const t = setTimeout(() => reject(new Error(method)), Math.min(12000, left()));
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
    await sleep(Math.min(200, left()));
  }
  const tabs = await (await fetch(`http://127.0.0.1:${PORT}/json/list`)).json();
  const page = tabs.find((t) => t.type === "page") || tabs[0];
  const ws = new WebSocket(page.webSocketDebuggerUrl);
  await new Promise((res, rej) => {
    ws.addEventListener("open", res);
    ws.addEventListener("error", rej);
    setTimeout(() => rej(new Error("ws")), Math.min(8000, left()));
  });
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

      // Touch relative stick: down near center, drag far right past deadzone → ix=+1
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
      ws.close(); chrome.kill("SIGKILL"); process.exit(0);
    }
    if (v.uxe?.ready === false) throw new Error(v.uxe.error);
    await sleep(Math.min(350, left()));
  }
  throw new Error("never ready");
} catch (e) {
  try { chrome.kill("SIGKILL"); } catch {}
  console.error("FAIL_SHIP", e.message || e);
  process.exit(1);
}

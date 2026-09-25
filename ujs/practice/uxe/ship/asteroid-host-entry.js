/**
 * Asteroid Pages / ship-js entry — M1 path B default (sim.wasm direct).
 * Host/GPU from ./engine.js; game.js only.
 */
import { createBrowserHost } from "./engine.js";
import { runAsteroidCore } from "../app-asteroid.js";
import simMeta from "./sim.meta.json";

/**
 * @param {{
 *   canvas: HTMLCanvasElement,
 *   hud: HTMLElement,
 *   banner?: HTMLElement,
 *   finalEl?: HTMLElement,
 *   prefer?: "auto"|"webgl"|"webgpu",
 *   engineUrl: string,
 *   simUrl?: string,
 * }} cfg
 */
export async function startShip(cfg) {
  const host = await createBrowserHost(cfg.canvas, {
    prefer: cfg.prefer || "auto",
    baseURL: new URL(".", cfg.engineUrl),
    pointerLock: false,
  });
  window.__UXE_HOST__ = host;

  const engineUrl = cfg.engineUrl;
  const simUrl = cfg.simUrl || new URL("sim.wasm", new URL(".", engineUrl)).href;

  function paint(s) {
    window.__UXE__ = {
      ...s, backend: host.backend, ship: true, asteroid: true, pathB: true,
    };
    if (!s.ready) return;
    cfg.hud.innerHTML =
      `<b>Asteroid Rush</b> · ship-js · <b>path B</b><br>` +
      `backend <b>${host.backend}</b> · fps <b>${(s.fps || 0).toFixed(0)}</b><br>` +
      `得分 <b>${(s.score || 0).toFixed(0)}</b>` +
      (s.alive
        ? ` · 活着 · WASD / 触屏移动`
        : ` · <span class="warn">撞毁 — 空格/双击重开</span>`);
    if (!s.alive && cfg.banner) {
      cfg.banner.classList.add("show");
      if (cfg.finalEl) cfg.finalEl.textContent = String(Math.floor(s.score || 0));
    } else if (cfg.banner) {
      cfg.banner.classList.remove("show");
    }
  }

  const simRes = await fetch(simUrl);
  if (!simRes.ok) throw new Error("fetch sim.wasm " + simRes.status);
  const simWasm = new Uint8Array(await simRes.arrayBuffer());
  if (simWasm[0] !== 0 || simWasm[1] !== 0x61 || simWasm[2] !== 0x73 || simWasm[3] !== 0x6d) {
    throw new Error("sim.wasm bad magic");
  }

  // Path B: no ujs_full/engine.wasm — web-build not on ship path
  await runAsteroidCore(host, {
    directSim: {
      wasm: simWasm,
      meta: { globals: simMeta.globals || [], locals: simMeta.locals || [] },
    },
    onHud: paint,
  });

  return { backend: host.backend, pathB: true };
}

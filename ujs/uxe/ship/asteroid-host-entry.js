/**
 * Asteroid Pages / ship-js entry (no asteroid.wasm / no eng_* glue).
 * Host/GPU from ../engine.js (shared); this file is game.js only.
 */
import { createBrowserHost } from "./engine.js";
import { runAsteroidCore } from "../app-asteroid.js";
import embed from "./asteroid.embed.json";

/**
 * @param {{
 *   canvas: HTMLCanvasElement,
 *   hud: HTMLElement,
 *   banner?: HTMLElement,
 *   finalEl?: HTMLElement,
 *   prefer?: "auto"|"webgl"|"webgpu",
 *   engineUrl: string,
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
  const orig = host.host_asset_read.bind(host);
  host.host_asset_read = async (path) => {
    if (path === "ujs_full.wasm" || path === "engine.wasm" || path.endsWith("engine.wasm")) {
      const r = await fetch(engineUrl);
      if (!r.ok) throw new Error("fetch engine " + r.status);
      return new Uint8Array(await r.arrayBuffer());
    }
    return orig(path);
  };

  function paint(s) {
    window.__UXE__ = { ...s, backend: host.backend, ship: true, asteroid: true };
    if (!s.ready) return;
    cfg.hud.innerHTML =
      `<b>Asteroid Rush</b> · ship-js<br>` +
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

  const image = new Uint8Array(embed.image);
  await runAsteroidCore(host, {
    wasmUrl: "engine.wasm",
    precompiled: { image, blob: embed.blob },
    onHud: paint,
  });

  return { backend: host.backend };
}

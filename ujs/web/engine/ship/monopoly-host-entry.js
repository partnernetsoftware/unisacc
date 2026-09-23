/**
 * Monopoly Pages / ship-js entry.
 * Bundled: Host + core + precompiled sim. Fetches gameEngine.wasm only.
 * (C monopoly.wasm 尚未做；此为可外网交付的中间形态。)
 */
import { createBrowserHost } from "../browser-host.js";
import { runMonopolyCore } from "../core-monopoly.js";
import embed from "./monopoly.embed.json";

/**
 * @param {{
 *   canvas: HTMLCanvasElement,
 *   hud: HTMLElement,
 *   prefer?: "auto"|"webgl"|"webgpu",
 *   engineUrl: string,
 * }} cfg
 */
export async function startMonopolyShip(cfg) {
  const host = await createBrowserHost(cfg.canvas, {
    prefer: cfg.prefer || "auto",
    baseURL: new URL(".", cfg.engineUrl),
  });

  // Override asset read so wasmUrl resolves to gameEngine.wasm beside the page.
  const engineUrl = cfg.engineUrl;
  const orig = host.host_asset_read.bind(host);
  host.host_asset_read = async (path) => {
    if (path === "ujs_full.wasm" || path === "gameEngine.wasm" || path.endsWith("gameEngine.wasm")) {
      const r = await fetch(engineUrl);
      if (!r.ok) throw new Error("fetch gameEngine " + r.status);
      return new Uint8Array(await r.arrayBuffer());
    }
    return orig(path);
  };

  function paint(s) {
    window.__UXE__ = { ...s, backend: host.backend, ship: true, monopoly: true };
    if (!s.ready) return;
    const who = s.turn === 0 ? "你" : "AI";
    const win =
      s.phase === 2
        ? (s.winner === 0 ? "你赢了" : s.winner === 1 ? "AI 赢了" : "结束")
        : "";
    cfg.hud.innerHTML =
      `<b>城市大富翁</b> · ship-js + gameEngine<br>` +
      `backend <b>${host.backend}</b> · fps <b>${(s.fps || 0).toFixed(0)}</b><br>` +
      `回合 <b>${who}</b> · ${s.phaseHint || ""}` +
      (win ? ` · <span class="warn">${win}</span>` : "") + `<br>` +
      `你 💰<b>${(s.money0 || 0).toFixed(0)}</b> · 地 ${s.owned0 ?? 0}` +
      ` · AI 💰<b>${(s.money1 || 0).toFixed(0)}</b> · 地 ${s.owned1 ?? 0}<br>` +
      (s.cellName ? `当前格 <b>${s.cellName}</b>` : "") +
      (s.houses ? ` · 房 ${s.houses}` : "") +
      (s.dice ? ` · 骰 <b>${s.dice}</b>` : "") +
      (s.msg ? `<br><span class="warn">${s.msg}</span>` : "");
  }

  const image = new Uint8Array(embed.image);
  await runMonopolyCore(host, {
    wasmUrl: "gameEngine.wasm",
    precompiled: { image, blob: embed.blob },
    onHud: paint,
  });
  return { backend: host.backend };
}

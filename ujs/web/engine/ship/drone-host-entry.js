/**
 * Drone Pages / ship-js entry.
 * Bundled: Host + core + precompiled sim. Fetches engine.wasm only.
 */
import { createBrowserHost } from "../browser-host.js";
import { runDroneCore } from "../core-drone.js";
import embed from "./drone.embed.json";

function helpLine(controls) {
  return controls === "mouse"
    ? "鼠标看 · WASD 飞 · 锁定后点击/空格发射（2发）· F/右键武装自爆"
    : "IJKL 看 · WASD 飞 · 锁定后空格发射（2发）· F 武装自爆";
}

/**
 * @param {{
 *   canvas: HTMLCanvasElement,
 *   hud: HTMLElement,
 *   prefer?: "auto"|"webgl"|"webgpu",
 *   engineUrl: string,
 *   controls?: "keyboard"|"mouse",
 *   onControls?: (c: "keyboard"|"mouse") => void,
 * }} cfg
 */
export async function startDroneShip(cfg) {
  let controls = cfg.controls === "mouse" ? "mouse" : "keyboard";
  const host = await createBrowserHost(cfg.canvas, {
    prefer: cfg.prefer || "auto",
    baseURL: new URL(".", cfg.engineUrl),
    pointerLock: controls === "mouse",
  });

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
    window.__UXE__ = { ...s, backend: host.backend, ship: true, drone: true };
    if (!s.ready) return;
    const ammoBar = "●".repeat(s.ammo || 0) + "○".repeat(Math.max(0, (s.magazine || 2) - (s.ammo || 0)));
    const modeLabel = s.controls === "mouse" ? "键盘+鼠标" : "纯键盘";
    cfg.hud.innerHTML =
      `<b>无人机 · 第一人称驾舱</b> · ship-js<br>` +
      `backend <b>${host.backend}</b> · fps <b>${(s.fps || 0).toFixed(0)}</b> · <b>${modeLabel}</b><br>` +
      `<span class="mode">${s.mode || ""}</span><br>` +
      `弹仓 <b>${ammoBar}</b> (${s.ammo}/${s.magazine}) · 击毁 <b>${s.kills || 0}</b> · 敌 <b>${s.remaining ?? "?"}</b><br>` +
      `得分 <b>${(s.score || 0).toFixed(0)}</b>` +
      (s.locked ? ` · <b class="lock">锁定</b>` : "") +
      (s.suicideArm ? ` · <b class="warn">自爆已武装</b>` : "") + `<br>` +
      (s.alive
        ? helpLine(s.controls)
        : `<span class="warn">${s.endReason === "win" ? "全歼" : "任务结束"} — 空格再出击</span>`);
  }

  const image = new Uint8Array(embed.image);
  const api = await runDroneCore(host, {
    wasmUrl: "engine.wasm",
    precompiled: { image, blob: embed.blob },
    controls,
    onHud: paint,
  });

  return {
    backend: host.backend,
    getControls: () => api.getControls(),
    setControls(next) {
      api.setControls(next);
      controls = api.getControls();
      cfg.onControls?.(controls);
    },
  };
}

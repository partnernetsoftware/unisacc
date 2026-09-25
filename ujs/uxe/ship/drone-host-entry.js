/**
 * Drone Pages / ship-js entry — M1b path B default (sim.wasm direct).
 * Host/GPU from ../engine.js (shared); this file is game.js only.
 */
import { createBrowserHost } from "../engine.js";
import { runDroneCore } from "../app-drone.js";
import simMeta from "./drone/sim.meta.json";

function helpLine(controls, touch) {
  if (touch) {
    return "左指拖飞 · 右指拖看 · 锁定后点发射 · 双击空档再出击";
  }
  return controls === "mouse"
    ? "鼠标看 · WASD 飞 · Shift 加速 · 锁定后点击/空格发射（2发）· F/右键武装自爆"
    : "IJKL 看 · WASD 飞 · Shift 加速 · 锁定后空格发射（2发）· F 武装自爆";
}

/**
 * @param {{
 *   canvas: HTMLCanvasElement,
 *   hud: HTMLElement,
 *   ammoEl?: HTMLElement,
 *   reticle?: HTMLElement,
 *   prefer?: "auto"|"webgl"|"webgpu",
 *   engineUrl: string,
 *   simUrl?: string,
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
  window.__UXE_HOST__ = host;

  const engineUrl = cfg.engineUrl;
  const simUrl = cfg.simUrl || new URL("sim.wasm", new URL(".", engineUrl)).href;

  function paint(s) {
    window.__UXE__ = {
      ...s, backend: host.backend, ship: true, drone: true, pathB: true,
    };
    if (!s.ready) return;
    cfg.hud.classList.toggle("armed", !!s.suicideArm);
    cfg.hud.classList.toggle("locked-on", !!s.locked && !s.suicideArm);
    cfg.reticle?.classList.toggle("lock", !!s.locked && !s.suicideArm);
    cfg.reticle?.classList.toggle("suicide", !!s.suicideArm);
    const ammoBar = "▮".repeat(s.ammo || 0) + "▯".repeat(Math.max(0, (s.magazine || 2) - (s.ammo || 0)));
    if (cfg.ammoEl) cfg.ammoEl.textContent = ammoBar;
    const modeLabel = s.controls === "mouse" ? "键盘+鼠标" : "纯键盘";
    cfg.hud.innerHTML =
      `<b>无人机 · 第一人称驾舱</b> · ship-js · <b>path B</b><br>` +
      `backend <b>${host.backend}</b> · fps <b>${(s.fps || 0).toFixed(0)}</b> · <b>${modeLabel}</b><br>` +
      `<span class="mode">${s.mode || ""}</span><br>` +
      `弹仓 <b>${ammoBar}</b> (${s.ammo}/${s.magazine})` +
      (s.missiles ? ` · 在途 <b>${s.missiles}</b>` : "") +
      ` · 击毁 <b>${s.kills || 0}</b> · 敌 <b>${s.remaining ?? "?"}</b><br>` +
      `得分 <b>${(s.score || 0).toFixed(0)}</b>` +
      (s.locked ? ` · <b class="lock">锁定</b>` : "") +
      (s.suicideArm ? ` · <b class="warn">自爆已武装</b>` : "") + `<br>` +
      (s.alive
        ? helpLine(s.controls, s.touch)
        : `<span class="warn">${s.endReason === "win" ? "全歼" : "任务结束"} — 空格再出击</span>`);
    }

  const simRes = await fetch(simUrl);
  if (!simRes.ok) throw new Error("fetch sim.wasm " + simRes.status);
  const simWasm = new Uint8Array(await simRes.arrayBuffer());
  if (simWasm[0] !== 0 || simWasm[1] !== 0x61 || simWasm[2] !== 0x73 || simWasm[3] !== 0x6d) {
    throw new Error("sim.wasm bad magic");
  }

  // Path B: no ujs_full/engine.wasm — web-build not on ship path
  const api = await runDroneCore(host, {
    directSim: {
      wasm: simWasm,
      meta: { globals: simMeta.globals || [], locals: simMeta.locals || [] },
    },
    controls,
    onHud: paint,
  });
  window.__DRONE_API__ = api.api;
  window.__UXE_API__ = api;

  return {
    backend: host.backend,
    pathB: true,
    getControls: () => api.getControls(),
    setControls(next) {
      api.setControls(next);
      controls = api.getControls();
      cfg.onControls?.(controls);
    },
  };
}

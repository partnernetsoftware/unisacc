import { createBrowserHost } from "../../browser-host.js";
import { runDroneCore } from "../../core-drone.js";

const hud = document.getElementById("hud");
const canvas = document.getElementById("c");
const btnKb = document.getElementById("ctrl-kb");
const btnMs = document.getElementById("ctrl-ms");
hud.textContent = "驾舱启动…";

const params = new URLSearchParams(location.search);
const q = params.get("gpu");
const prefer = q === "webgl" || q === "webgpu" ? q : "auto";
let controls = params.get("controls") === "mouse" ? "mouse" : "keyboard";

function helpLine(c) {
  return c === "mouse"
    ? "鼠标看 · WASD 飞 · 锁定后点击/空格发射（2发）· F/右键武装自爆"
    : "IJKL 看 · WASD 飞 · 锁定后空格发射（2发）· F 武装自爆";
}

function paintCtrl(c) {
  btnKb?.classList.toggle("on", c === "keyboard");
  btnMs?.classList.toggle("on", c === "mouse");
  canvas.style.cursor = c === "mouse" ? "none" : "default";
}
paintCtrl(controls);

const host = await createBrowserHost(canvas, {
  baseURL: new URL("../../../", import.meta.url),
  prefer,
  pointerLock: controls === "mouse",
});

function paint(s) {
  window.__UXE__ = { ...s, backend: host.backend, ship: false };
  if (!s.ready) return;
  const ammoBar = "●".repeat(s.ammo || 0) + "○".repeat(Math.max(0, (s.magazine || 2) - (s.ammo || 0)));
  const modeLabel = s.controls === "mouse" ? "键盘+鼠标" : "纯键盘";
  hud.innerHTML =
    `<b>无人机 · 第一人称驾舱</b><br>` +
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

try {
  const api = await runDroneCore(host, {
    wasmUrl: "ujs_full.wasm",
    simUrl: "game/drone.ujs",
    controls,
    onHud: paint,
  });
  window.__UXE_API__ = api;
  btnKb.onclick = () => {
    api.setControls("keyboard");
    controls = "keyboard";
    paintCtrl(controls);
  };
  btnMs.onclick = () => {
    api.setControls("mouse");
    controls = "mouse";
    paintCtrl(controls);
  };
} catch (e) {
  hud.innerHTML = `<span class="warn">boot failed</span><br>${e.message || e}`;
  window.__UXE__ = { ready: false, error: String(e.message || e), drone: true };
  console.error(e);
}

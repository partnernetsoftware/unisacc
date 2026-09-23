/** Demo glue ONLY: canvas + HUD + createBrowserHost + run core. */
import { createBrowserHost } from "../browser-host.js";
import { runAsteroidCore } from "../core-asteroid.js";

const hud = document.getElementById("hud");
const banner = document.getElementById("banner");
const finalEl = document.getElementById("final");
let canvas = document.getElementById("c");

hud.textContent = "Host ABI boot…";

const prefer = new URLSearchParams(location.search).get("gpu") === "webgpu"
  ? "webgpu"
  : "webgl";

const host = await createBrowserHost(canvas, {
  baseURL: new URL("../../", import.meta.url),
  prefer,
});
canvas = document.getElementById("c") || canvas;

function paintHud(s) {
  window.__UXE__ = { ...s, backend: host.backend };
  if (!s.ready) return;
  if (s.alive === false) {
    banner.classList.add("show");
    finalEl.textContent = (s.score ?? 0).toFixed(0);
  } else {
    banner.classList.remove("show");
  }
  if (s.ujsMs == null) {
    hud.textContent = "UXE Host ABI · warming…";
    return;
  }
  hud.innerHTML =
    `<b>UXE</b> · Host ABI · UXEP+UXIN<br>` +
    `backend <b>${host.backend}</b> · entities <b>${s.n}</b><br>` +
    `ujs <b>${s.ujsMs.toFixed(2)} ms</b> · gpu_submit <b>${s.drawMs.toFixed(2)} ms</b><br>` +
    `fps <b>${s.fps.toFixed(0)}</b> · score <b>${s.score.toFixed(0)}</b>` +
    (host._stats?.().bytes ? `<br>packet <b>${host._stats().bytes}</b> B` : "") +
    (s.alive ? "" : `<br><span class="warn">crashed — Space</span>`);
}

try {
  await runAsteroidCore(host, {
    wasmUrl: "ujs_full.wasm",
    simUrl: "game/sim.ujs",
    onHud: paintHud,
  });
} catch (e) {
  const msg = String(e && e.stack || e.message || e);
  hud.innerHTML = `<span class="warn">boot failed</span><br>${msg}`;
  window.__UXE__ = { ready: false, error: msg };
  window.__UXE_LAST_ERR__ = msg;
  try { host.host_log("error", msg); } catch (_) {}
  console.error(e);
}

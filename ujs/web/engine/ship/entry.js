/**
 * Ship entry — single module graph for esbuild.
 * Dev still uses split sources under ../ ; publish loads one bundle.
 */
import { createBrowserHost } from "../browser-host.js";
import { runAsteroidCore } from "../core-asteroid.js";

const hud = document.getElementById("hud");
const banner = document.getElementById("banner");
const finalEl = document.getElementById("final");
const canvas = document.getElementById("c");

hud.textContent = "ship boot…";

const q = new URLSearchParams(location.search).get("gpu");
const prefer = q === "webgl" || q === "webgpu" ? q : "auto";

const host = await createBrowserHost(canvas, {
  // assets relative to /engine/ship/ → ../../ = web/
  baseURL: new URL("../../", import.meta.url),
  prefer,
});

function paintHud(s) {
  window.__UXE__ = { ...s, backend: host.backend, ship: true };
  if (!s.ready) return;
  if (s.alive === false) {
    banner.classList.add("show");
    finalEl.textContent = (s.score ?? 0).toFixed(0);
  } else {
    banner.classList.remove("show");
  }
  if (s.ujsMs == null) {
    hud.textContent = "UXE ship · warming…";
    return;
  }
  hud.innerHTML =
    `<b>UXE ship</b> · one JS bundle<br>` +
    `backend <b>${host.backend}</b> · entities <b>${s.n}</b><br>` +
    `ujs <b>${s.ujsMs.toFixed(2)} ms</b> · gpu <b>${s.drawMs.toFixed(2)} ms</b><br>` +
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
  window.__UXE__ = { ready: false, error: msg, ship: true };
  console.error(e);
}

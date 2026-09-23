/** Exp host: same sim.ujs + wasm_run; draw via raw WebGL (no Three). */
import { bootRuntime, compile, wasm_run, unwrap } from "../../wasm_run.js";
import { createRawGl } from "./rawgl.js";

const N = 480;
const hud = document.getElementById("hud");
const banner = document.getElementById("banner");
const finalEl = document.getElementById("final");
const canvas = document.getElementById("c");

const keys = new Set();
addEventListener("keydown", (e) => {
  keys.add(e.code);
  if (e.code === "Space") {
    e.preventDefault();
    if (!alive) resetGame();
  }
});
addEventListener("keyup", (e) => keys.delete(e.code));

function inputAxes() {
  let ix = 0, iy = 0;
  if (keys.has("KeyA") || keys.has("ArrowLeft")) ix -= 1;
  if (keys.has("KeyD") || keys.has("ArrowRight")) ix += 1;
  if (keys.has("KeyS") || keys.has("ArrowDown")) iy -= 1;
  if (keys.has("KeyW") || keys.has("ArrowUp")) iy += 1;
  return { ix, iy };
}

function freshState() {
  const xs = [], ys = [], zs = [], vxs = [], vys = [], vzs = [], rs = [];
  for (let i = 0; i < N; i++) {
    let sx = ((i * 13) % 37) - 18;
    let sy = ((i * 7) % 25) - 12;
    if (sx > -3.5 && sx < 3.5 && sy > -3.5 && sy < 3.5) sx += 7;
    xs.push(sx); ys.push(sy); zs.push(-40 - i * 1.55);
    vxs.push(((i % 5) - 2) * 0.55);
    vys.push(((i % 3) - 1) * 0.4);
    vzs.push(8 + (i % 11) * 0.35);
    rs.push(0.55 + (i % 5) * 0.22);
  }
  return { xs, ys, zs, vxs, vys, vzs, rs, px: 0, py: 0, pz: 0, score: 0, alive: 1 };
}

let state = freshState();
let alive = true;
let fnImage = null;
let fnBlob = null;
let gfx = null;

function resetGame() {
  state = freshState();
  alive = true;
  banner.classList.remove("show");
}

async function main() {
  try {
    gfx = createRawGl(canvas, N);
  } catch (e) {
    hud.innerHTML = `<span class="warn">rawgl failed</span><br>${e.message || e}`;
    throw e;
  }
  hud.textContent = "loading wasm + sim.ujs…";
  await bootRuntime(new URL("../../ujs_full.wasm", import.meta.url));
  const src = await fetch(new URL("../sim.ujs", import.meta.url)).then((r) => {
    if (!r.ok) throw new Error("sim.ujs " + r.status);
    return r.text();
  });
  const compiled = compile(src);
  fnImage = compiled.image;
  fnBlob = compiled.blob;

  const warm = await wasm_run({ image: fnImage, blob: fnBlob }, {
    ...state, ix: 0, iy: 0, dt: 0.016,
  }, {});
  if (warm.err) throw new Error(JSON.stringify(warm.err));
  state = { ...state, ...unwrap(warm) };
  alive = !!state.alive;
  window.__UJS_EXP__ = { ready: true, renderer: "rawgl", n: N };

  let last = performance.now();
  let accUjs = 0, accDraw = 0, accFrames = 0, lastFpsT = last;
  let ujsMs = 0, drawMs = 0;

  function schedule(fn) {
    requestAnimationFrame(fn);
  }

  async function frame(now) {
    const dt = Math.min(0.05, (now - last) / 1000);
    last = now;
    const { ix, iy } = inputAxes();

    if (alive) {
      const t0 = performance.now();
      const r = await wasm_run({ image: fnImage, blob: fnBlob }, {
        xs: state.xs, ys: state.ys, zs: state.zs,
        vxs: state.vxs, vys: state.vys, vzs: state.vzs, rs: state.rs,
        px: state.px, py: state.py, pz: state.pz,
        ix, iy, dt, score: state.score, alive: 1,
      }, {});
      ujsMs = performance.now() - t0;
      if (r.err) {
        hud.innerHTML = `<span class="warn">UJS trap</span><br>${r.err.message || JSON.stringify(r.err)}`;
        schedule(frame);
        return;
      }
      const out = unwrap(r);
      state = {
        xs: out.xs, ys: out.ys, zs: out.zs,
        vxs: out.vxs, vys: out.vys, vzs: out.vzs, rs: out.rs,
        px: out.px, py: out.py, pz: out.pz,
        score: out.score, alive: out.alive,
      };
      if (!out.alive || out.hit) {
        alive = false;
        finalEl.textContent = out.score.toFixed(0);
        banner.classList.add("show");
      }
    }

    const d0 = performance.now();
    gfx.draw(state);
    drawMs = performance.now() - d0;

    accUjs += ujsMs;
    accDraw += drawMs;
    accFrames += 1;
    if (now - lastFpsT >= 500) {
      const fps = (accFrames * 1000) / (now - lastFpsT);
      const au = accUjs / accFrames;
      const ad = accDraw / accFrames;
      hud.innerHTML =
        `<b>exp · raw WebGL</b> · same sim.ujs<br>` +
        `entities <b>${N}</b><br>` +
        `ujs <b>${au.toFixed(2)} ms</b> · draw <b>${ad.toFixed(2)} ms</b><br>` +
        `fps <b>${fps.toFixed(0)}</b> · score <b>${state.score.toFixed(0)}</b>` +
        (alive ? "" : `<br><span class="warn">crashed — Space</span>`);
      window.__UJS_EXP__ = {
        ready: true, renderer: "rawgl", n: N, fps, ujsMs: au, drawMs: ad, score: state.score,
      };
      accUjs = 0; accDraw = 0; accFrames = 0; lastFpsT = now;
    }
    schedule(frame);
  }
  setTimeout(() => frame(performance.now()), 0);
}

main().catch((e) => {
  hud.innerHTML = `<span class="warn">boot failed</span><br>${String(e.message || e)}`;
  window.__UJS_EXP__ = { ready: false, error: String(e.message || e) };
  console.error(e);
});

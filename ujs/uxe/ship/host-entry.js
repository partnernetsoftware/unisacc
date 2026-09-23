/**
 * Ship glue: bridge engine.wasm ↔ asteroid.wasm (+ HUD).
 * Host/GPU come from ./engine.js (shared).
 */
import { createBrowserHost, INPUT_BYTES } from "./engine.js";
import meta from "./sim.meta.json";

const N = meta.n || 480;
const GLOBALS = meta.globals;
const gIndex = Object.fromEntries(GLOBALS.map((g, i) => [g, i]));
const TAG = { f64: 3, str: 4, list: 5, dict: 6 };

/**
 * @param {{
 *   canvas: HTMLCanvasElement,
 *   hud: HTMLElement,
 *   banner: HTMLElement,
 *   finalEl: HTMLElement,
 *   prefer?: "auto"|"webgl"|"webgpu",
 *   gameUrl: string,
 *   engineUrl: string,
 * }} cfg
 */
export async function startShip(cfg) {
  const host = await createBrowserHost(cfg.canvas, {
    prefer: cfg.prefer || "auto",
    pointerLock: false,
  });
  window.__UXE_HOST__ = host;

  const [engineBuf, gameBuf] = await Promise.all([
    fetch(cfg.engineUrl).then((r) => {
      if (!r.ok) throw new Error("fetch engine " + r.status);
      return r.arrayBuffer();
    }),
    fetch(cfg.gameUrl).then((r) => {
      if (!r.ok) throw new Error("fetch game " + r.status);
      return r.arrayBuffer();
    }),
  ]);

  const { instance: engine } = await WebAssembly.instantiate(engineBuf);
  const ex = engine.exports;
  for (const n of [
    "memory", "host_reset", "host_prog_addr", "host_load_image", "host_run",
    "host_set_global", "host_mk_f64", "host_mk_list", "host_list_set", "host_list_get",
    "host_len", "host_dict_key", "host_dict_val",
    "tag_of_export", "f64_of_export", "str_len_export", "str_ptr_export", "mem_base",
  ]) {
    if (ex[n] == null) throw new Error("engine missing " + n);
  }

  const engMem = () => new Uint8Array(ex.memory.buffer);

  function engWriteImage(srcBytes) {
    const base = Number(ex.mem_base());
    const addr = Number(ex.host_prog_addr());
    engMem().set(srcBytes, base + addr);
    ex.host_load_image();
  }

  function mkF64List(arr) {
    const h = ex.host_mk_list(arr.length);
    for (let i = 0; i < arr.length; i++) ex.host_list_set(h, i, ex.host_mk_f64(arr[i]));
    return h;
  }

  function readF64List(h, n) {
    const out = new Float64Array(n);
    if (ex.tag_of_export(h) !== TAG.list) return out;
    const m = Math.min(n, Number(ex.host_len(h)));
    for (let i = 0; i < m; i++) out[i] = Number(ex.f64_of_export(ex.host_list_get(h, i)));
    return out;
  }

  function strEq(h, lit) {
    if (ex.tag_of_export(h) !== TAG.str) return false;
    const n = Number(ex.str_len_export(h));
    if (n !== lit.length) return false;
    const p = Number(ex.mem_base()) + Number(ex.str_ptr_export(h));
    const bytes = engMem().subarray(p, p + n);
    for (let i = 0; i < n; i++) if (bytes[i] !== lit.charCodeAt(i)) return false;
    return true;
  }

  function applyReturn(h, state) {
    if (ex.tag_of_export(h) !== TAG.dict) return;
    const n = Number(ex.host_len(h));
    for (let i = 0; i < n; i++) {
      const k = ex.host_dict_key(h, i);
      const v = ex.host_dict_val(h, i);
      if (strEq(k, "xs")) state.xs = readF64List(v, N);
      else if (strEq(k, "ys")) state.ys = readF64List(v, N);
      else if (strEq(k, "zs")) state.zs = readF64List(v, N);
      else if (strEq(k, "vxs")) state.vxs = readF64List(v, N);
      else if (strEq(k, "vys")) state.vys = readF64List(v, N);
      else if (strEq(k, "vzs")) state.vzs = readF64List(v, N);
      else if (strEq(k, "rs")) state.rs = readF64List(v, N);
      else if (strEq(k, "px")) state.px = Number(ex.f64_of_export(v));
      else if (strEq(k, "py")) state.py = Number(ex.f64_of_export(v));
      else if (strEq(k, "pz")) state.pz = Number(ex.f64_of_export(v));
      else if (strEq(k, "score")) state.score = Number(ex.f64_of_export(v));
      else if (strEq(k, "alive")) state.alive = Number(ex.f64_of_export(v));
      else if (strEq(k, "hit") && Number(ex.f64_of_export(v)) !== 0) state.alive = 0;
    }
  }

  function readState(mem, ptr) {
    const dv = new DataView(mem.buffer, ptr, (7 * N + 5) * 8);
    const take = (off, n) => {
      const a = new Float64Array(n);
      for (let i = 0; i < n; i++) a[i] = dv.getFloat64(off + i * 8, true);
      return a;
    };
    let o = 0;
    const xs = take(o, N); o += N * 8;
    const ys = take(o, N); o += N * 8;
    const zs = take(o, N); o += N * 8;
    const vxs = take(o, N); o += N * 8;
    const vys = take(o, N); o += N * 8;
    const vzs = take(o, N); o += N * 8;
    const rs = take(o, N); o += N * 8;
    return {
      xs, ys, zs, vxs, vys, vzs, rs,
      px: dv.getFloat64(o, true),
      py: dv.getFloat64(o + 8, true),
      pz: dv.getFloat64(o + 16, true),
      score: dv.getFloat64(o + 24, true),
      alive: dv.getFloat64(o + 32, true),
    };
  }

  function writeState(mem, ptr, state) {
    const dv = new DataView(mem.buffer, ptr, (7 * N + 5) * 8);
    const put = (off, arr) => {
      for (let i = 0; i < arr.length; i++) dv.setFloat64(off + i * 8, arr[i], true);
    };
    let o = 0;
    put(o, state.xs); o += N * 8;
    put(o, state.ys); o += N * 8;
    put(o, state.zs); o += N * 8;
    put(o, state.vxs); o += N * 8;
    put(o, state.vys); o += N * 8;
    put(o, state.vzs); o += N * 8;
    put(o, state.rs); o += N * 8;
    dv.setFloat64(o, state.px, true);
    dv.setFloat64(o + 8, state.py, true);
    dv.setFloat64(o + 16, state.pz, true);
    dv.setFloat64(o + 24, state.score, true);
    dv.setFloat64(o + 32, state.alive, true);
  }

  let gameMem;
  let gameFrame;
  let hudPtr = 0;
  let hudSize = 0;

  function paintFromHud() {
    if (!gameMem || !hudPtr) return;
    const dv = new DataView(gameMem.buffer, hudPtr, hudSize || 48);
    const snap = {
      ready: dv.getUint32(40, true) !== 0,
      ujsMs: dv.getFloat64(0, true),
      drawMs: dv.getFloat64(8, true),
      fps: dv.getFloat64(16, true),
      score: dv.getFloat64(24, true),
      n: dv.getUint32(32, true),
      alive: dv.getUint32(36, true) !== 0,
      abi: true,
      ship: true,
      wasmGame: true,
      wasmEngine: true,
      backend: host.backend,
    };
    window.__UXE__ = snap;
    if (!snap.ready) return;
    if (!snap.alive) {
      cfg.banner.classList.add("show");
      cfg.finalEl.textContent = (snap.score ?? 0).toFixed(0);
    } else cfg.banner.classList.remove("show");
    if (!(snap.ujsMs > 0) && !(snap.fps > 0)) {
      cfg.hud.textContent = "asteroid + engine · warming…";
      return;
    }
    cfg.hud.innerHTML =
      `<b>Asteroid</b> · ship<br>` +
      `backend <b>${host.backend}</b> · entities <b>${snap.n}</b><br>` +
      `ujs <b>${snap.ujsMs.toFixed(2)} ms</b> · gpu <b>${snap.drawMs.toFixed(2)} ms</b><br>` +
      `fps <b>${snap.fps.toFixed(0)}</b> · score <b>${snap.score.toFixed(0)}</b><br>` +
      `操作 <b>WASD</b> / <b>触屏拖</b> · 撞毁后 <b>双击</b>/空格重开` +
      (host._stats?.().bytes ? ` · packet <b>${host._stats().bytes}</b> B` : "") +
      (snap.alive ? "" : `<br><span class="warn">撞毁 — 双击或空格重开</span>`);
  }

  const imports = {
    env: {
      host_time: () => host.host_time(),
      host_input_read: (ptr, cap) => {
        if (cap < INPUT_BYTES) return 0;
        const buf = new ArrayBuffer(INPUT_BYTES);
        host.host_input_read(buf);
        new Uint8Array(gameMem.buffer, ptr, INPUT_BYTES).set(new Uint8Array(buf));
        return INPUT_BYTES;
      },
      host_frame_begin: () => host.host_frame_begin(),
      host_frame_present: () => {
        host.host_frame_present();
        paintFromHud();
      },
      host_gpu_submit: (ptr, len) => {
        host.host_gpu_submit(gameMem.buffer.slice(ptr, ptr + len));
      },
      host_log: (level, ptr, len) => {
        const levels = ["info", "warn", "error"];
        const msg = new TextDecoder().decode(new Uint8Array(gameMem.buffer, ptr, len));
        host.host_log(levels[level] || "info", msg);
      },
      host_request_frame: () => host.host_request_frame(() => gameFrame()),

      eng_boot: (imagePtr, imageLen) => {
        try {
          ex.host_reset();
          engWriteImage(new Uint8Array(gameMem.buffer, imagePtr, imageLen));
          return 0;
        } catch (e) {
          console.error(e);
          return 1;
        }
      },

      eng_sim_step: (statePtr, ix, iy, dt) => {
        try {
          const state = readState(gameMem, statePtr);
          ex.host_reset();
          ex.host_load_image();
          const set = (name, h) => {
            const i = gIndex[name];
            if (i != null) ex.host_set_global(i, h);
          };
          set("xs", mkF64List(state.xs));
          set("ys", mkF64List(state.ys));
          set("zs", mkF64List(state.zs));
          set("vxs", mkF64List(state.vxs));
          set("vys", mkF64List(state.vys));
          set("vzs", mkF64List(state.vzs));
          set("rs", mkF64List(state.rs));
          set("px", ex.host_mk_f64(state.px));
          set("py", ex.host_mk_f64(state.py));
          set("pz", ex.host_mk_f64(state.pz));
          set("ix", ex.host_mk_f64(ix));
          set("iy", ex.host_mk_f64(iy));
          set("dt", ex.host_mk_f64(dt));
          set("score", ex.host_mk_f64(state.score));
          set("alive", ex.host_mk_f64(state.alive));
          applyReturn(ex.host_run(), state);
          writeState(gameMem, statePtr, state);
          return 0;
        } catch (e) {
          console.error(e);
          return 1;
        }
      },
    },
  };

  const { instance: game } = await WebAssembly.instantiate(gameBuf, imports);
  gameMem = game.exports.memory;
  gameFrame = game.exports.game_frame;
  hudPtr = game.exports.game_hud_ptr();
  hudSize = game.exports.game_hud_size();
  if (!game.exports.game_init()) throw new Error("game_init failed");
  return { backend: host.backend };
}

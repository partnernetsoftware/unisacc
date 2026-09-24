// uxe/ship/asteroid-host-entry.js
import { createBrowserHost } from "../engine.js";

// core/wasm_run.js
var TAG = { null: 0, bool: 1, i64: 2, f64: 3, str: 4, list: 5, dict: 6, fn: 7, tup: 8 };
var rt = null;
async function loadWasmBytes(spec) {
  if (spec instanceof ArrayBuffer) return new Uint8Array(spec);
  if (ArrayBuffer.isView(spec)) return new Uint8Array(spec.buffer, spec.byteOffset, spec.byteLength);
  if (typeof URL !== "undefined" && spec instanceof URL) {
    if (typeof process !== "undefined" && spec.protocol === "file:") {
      const fs = await import("fs");
      return fs.readFileSync(spec);
    }
    const r2 = await fetch(spec);
    if (!r2.ok) throw new Error("fetch wasm failed: " + spec + " (" + r2.status + ")");
    return new Uint8Array(await r2.arrayBuffer());
  }
  if (typeof spec !== "string") throw new Error("bootRuntime: need path, URL, or bytes");
  if (typeof process !== "undefined" && spec.indexOf("://") < 0) {
    const fs = await import("fs");
    const path = await import("path");
    const { fileURLToPath } = await import("url");
    const here = path.dirname(fileURLToPath(import.meta.url));
    const cand = [
      path.isAbsolute(spec) ? spec : null,
      path.join(here, spec),
      path.resolve(spec)
    ].filter(Boolean);
    for (const p of cand) {
      try {
        return fs.readFileSync(p);
      } catch (_) {
      }
    }
    throw new Error(
      "wasm not found: " + spec + " (run: python3 -m ujs web-build / npm run build)"
    );
  }
  const r = await fetch(spec);
  if (!r.ok) throw new Error("fetch wasm failed: " + spec + " (" + r.status + ")");
  return new Uint8Array(await r.arrayBuffer());
}
async function bootRuntime(wasmUrl = "ujs_full.wasm") {
  const buf = await loadWasmBytes(wasmUrl);
  const { instance } = await WebAssembly.instantiate(buf);
  if (typeof instance.exports.host_run !== "function") {
    throw new Error("not ujs_full.wasm (missing host_run); run: npm run build");
  }
  rt = { ex: instance.exports, mem: instance.exports.memory };
  return rt;
}
function ensureRt() {
  if (!rt) throw new Error("call bootRuntime() first");
  return rt;
}
function writeImage(image) {
  const { ex, mem } = ensureRt();
  const base = Number(ex.mem_base());
  const addr = Number(ex.host_prog_addr());
  const u8 = new Uint8Array(mem.buffer);
  u8.set(image, base + addr);
  ex.host_load_image();
}
function pyToHandle(ex, mem, v) {
  if (v === null || v === void 0) return ex.host_mk_null();
  if (typeof v === "boolean") return ex.host_mk_bool(v ? 1 : 0);
  if (typeof v === "number") {
    if (Number.isInteger(v) && Math.abs(v) <= Number.MAX_SAFE_INTEGER)
      return ex.host_mk_i64(BigInt(v));
    return ex.host_mk_f64(v);
  }
  if (typeof v === "bigint") return ex.host_mk_i64(v);
  if (typeof v === "string") {
    const bytes = new TextEncoder().encode(v);
    const scratch = 5e5;
    new Uint8Array(mem.buffer, Number(ex.mem_base()) + scratch, bytes.length).set(bytes);
    return ex.host_mk_str(scratch, bytes.length);
  }
  if (Array.isArray(v)) {
    if (typeof ex.host_mk_list !== "function")
      throw new Error("ujs_full.wasm too old for list bind; npm run build");
    const h = ex.host_mk_list(v.length);
    for (let i = 0; i < v.length; i++)
      ex.host_list_set(h, i, pyToHandle(ex, mem, v[i]));
    return h;
  }
  if (typeof v === "object") {
    if (typeof ex.host_mk_dict !== "function")
      throw new Error("ujs_full.wasm too old for dict bind; npm run build");
    const keys = Object.keys(v);
    const h = ex.host_mk_dict(keys.length);
    for (let i = 0; i < keys.length; i++) {
      const k = pyToHandle(ex, mem, keys[i]);
      const val = pyToHandle(ex, mem, v[keys[i]]);
      ex.host_dict_set(h, i, k, val);
    }
    return h;
  }
  throw new Error("unsupported global/local type: " + typeof v);
}
function readHandle(ex, mem, h) {
  if (!h) return null;
  const t = ex.tag_of_export(h);
  if (t === TAG.null) return null;
  if (t === TAG.bool) return !!new Uint8Array(mem.buffer)[Number(ex.mem_base()) + h + 4];
  if (t === TAG.i64) return Number(ex.i64_of_export(h));
  if (t === TAG.f64) return Number(ex.f64_of_export(h));
  if (t === TAG.str) {
    const p = Number(ex.str_ptr_export(h));
    const n = Number(ex.str_len_export(h));
    const base = Number(ex.mem_base());
    return new TextDecoder().decode(new Uint8Array(mem.buffer, base + p, n));
  }
  if (t === TAG.list && typeof ex.host_list_get === "function") {
    const n = Number(ex.host_len(h));
    const out = [];
    for (let i = 0; i < n; i++) out.push(readHandle(ex, mem, ex.host_list_get(h, i)));
    return out;
  }
  if (t === TAG.dict && typeof ex.host_dict_key === "function") {
    const n = Number(ex.host_len(h));
    const out = {};
    for (let i = 0; i < n; i++) {
      const k = readHandle(ex, mem, ex.host_dict_key(h, i));
      out[k] = readHandle(ex, mem, ex.host_dict_val(h, i));
    }
    return out;
  }
  return { tag: t, handle: h };
}
async function wasm_run(codeOrFn, globalsMap = {}, localsMap = {}) {
  const { ex, mem } = ensureRt();
  let image, blob, askTrace, askHeat;
  try {
    if (typeof codeOrFn === "string") {
      const { compile } = await import("./compiler.js");
      ({ image, blob, askTrace, askHeat } = compile(codeOrFn));
    } else if (codeOrFn && codeOrFn.image) {
      image = codeOrFn.image;
      blob = codeOrFn.blob || {};
    } else {
      return { err: { kind: "type", message: "need code string or fn image" } };
    }
  } catch (e) {
    return { err: { kind: "CompileError", message: String(e.message || e) } };
  }
  try {
    ex.host_reset();
    writeImage(image);
    const gnames = blob.globals || [];
    const lnames = blob.locals || [];
    for (const [k, v] of Object.entries(globalsMap || {})) {
      const ix = gnames.indexOf(k);
      if (ix >= 0) ex.host_set_global(ix, pyToHandle(ex, mem, v));
    }
    for (const [k, v] of Object.entries(localsMap || {})) {
      const ix = lnames.indexOf(k);
      if (ix >= 0) ex.host_set_local(ix, pyToHandle(ex, mem, v));
    }
    const h = ex.host_run();
    const ok = readHandle(ex, mem, h);
    const Lout = {};
    lnames.forEach((n, i) => {
      Lout[n] = readHandle(ex, mem, ex.host_get_local(i));
    });
    const Gout = {};
    gnames.forEach((n, i) => {
      Gout[n] = readHandle(ex, mem, ex.host_get_global(i));
    });
    return {
      ok,
      locals: Lout,
      globals: Gout,
      ic_stub: Number(ex.last_ic_stub_export()),
      askTrace: askTrace || [],
      askHeat: askHeat || {},
      imageBytes: image.length
    };
  } catch (e) {
    return { err: { kind: "Trap", message: String(e.message || e) } };
  }
}
function unwrap(r) {
  if (r && r.err) throw new Error((r.err.kind || "err") + ": " + (r.err.message || ""));
  return r.ok;
}

// uxe/packet.js
var PACKET_MAGIC = 1346721877;
var PACKET_VERSION = 3;
var MESH_OCTA = 0;
var DEFAULT_FOG = { density: 0.018, color: [0.02, 0.024, 0.04] };
var DEFAULT_AMBIENT = { color: [0.25, 0.38, 0.5], intensity: 0.55 };
var DEFAULT_LIGHTS = [
  { dir: [0.35, 0.9, 0.25], color: [1, 0.9, 0.78], intensity: 1.15 },
  { dir: [-0.4, 0.2, -0.5], color: [0.4, 0.53, 1], intensity: 0.45 }
];
function encodeRenderPacket(packet) {
  const clouds = packet.clouds || [];
  const fog = packet.fog || DEFAULT_FOG;
  const ambient = packet.ambient || DEFAULT_AMBIENT;
  const lights = packet.lights || DEFAULT_LIGHTS;
  let nbytes = 8 + 16 + 36 + 16 + 16 + 4 + lights.length * 28 + 4;
  for (const c of clouds) {
    nbytes += 12 + 12 + 4 + 4 + 4 + 4 + c.count * 3 * 4 + c.count * 4;
  }
  const buf = new ArrayBuffer(nbytes);
  const dv = new DataView(buf);
  let o = 0;
  dv.setUint32(o, PACKET_MAGIC, true);
  o += 4;
  dv.setUint32(o, PACKET_VERSION, true);
  o += 4;
  const clear = packet.clear || [0, 0, 0, 1];
  for (let i = 0; i < 4; i++) {
    dv.setFloat32(o, clear[i], true);
    o += 4;
  }
  const cam = packet.camera;
  const camf = [
    cam.fovy,
    cam.near,
    cam.far,
    cam.eye[0],
    cam.eye[1],
    cam.eye[2],
    cam.target[0],
    cam.target[1],
    cam.target[2]
  ];
  for (let i = 0; i < 9; i++) {
    dv.setFloat32(o, camf[i], true);
    o += 4;
  }
  dv.setFloat32(o, fog.density, true);
  o += 4;
  for (let i = 0; i < 3; i++) {
    dv.setFloat32(o, fog.color[i], true);
    o += 4;
  }
  for (let i = 0; i < 3; i++) {
    dv.setFloat32(o, ambient.color[i], true);
    o += 4;
  }
  dv.setFloat32(o, ambient.intensity, true);
  o += 4;
  dv.setUint32(o, lights.length, true);
  o += 4;
  for (const L of lights) {
    for (let i = 0; i < 3; i++) {
      dv.setFloat32(o, L.dir[i], true);
      o += 4;
    }
    for (let i = 0; i < 3; i++) {
      dv.setFloat32(o, L.color[i], true);
      o += 4;
    }
    dv.setFloat32(o, L.intensity, true);
    o += 4;
  }
  dv.setUint32(o, clouds.length, true);
  o += 4;
  for (const c of clouds) {
    for (let i = 0; i < 3; i++) {
      dv.setFloat32(o, c.color[i], true);
      o += 4;
    }
    const em = c.emissive || [0, 0, 0];
    for (let i = 0; i < 3; i++) {
      dv.setFloat32(o, em[i], true);
      o += 4;
    }
    dv.setFloat32(o, c.metalness ?? 0.35, true);
    o += 4;
    dv.setFloat32(o, c.roughness ?? 0.45, true);
    o += 4;
    dv.setUint32(o, c.meshId ?? MESH_OCTA, true);
    o += 4;
    dv.setUint32(o, c.count, true);
    o += 4;
    for (let i = 0; i < c.count * 3; i++) {
      dv.setFloat32(o, c.xyz[i], true);
      o += 4;
    }
    for (let i = 0; i < c.count; i++) {
      dv.setFloat32(o, c.scale[i], true);
      o += 4;
    }
  }
  if (o !== nbytes) throw new Error("encode size mismatch " + o + " vs " + nbytes);
  return buf;
}

// uxe/input.js
var INPUT_MAGIC = 1313429589;
var INPUT_BYTES_V1 = 20;
var INPUT_BYTES = 36;
function decodeInputSnapshot(raw) {
  const buf = ArrayBuffer.isView(raw) ? raw.buffer.slice(raw.byteOffset, raw.byteOffset + raw.byteLength) : raw;
  if (buf.byteLength < INPUT_BYTES_V1) {
    throw new Error("input buffer too short " + buf.byteLength);
  }
  const dv = new DataView(buf);
  const magic = dv.getUint32(0, true);
  if (magic !== INPUT_MAGIC) throw new Error("bad input magic");
  const ver = dv.getUint32(4, true);
  if (ver !== 1 && ver !== 2) throw new Error("bad input version " + ver);
  const out = {
    version: ver,
    ix: dv.getInt32(8, true),
    iy: dv.getInt32(12, true),
    fire: dv.getUint32(16, true),
    mx: 0,
    my: 0,
    buttons: 0,
    flags: 0
  };
  if (ver >= 2 && buf.byteLength >= INPUT_BYTES) {
    out.mx = dv.getFloat32(20, true);
    out.my = dv.getFloat32(24, true);
    out.buttons = dv.getUint32(28, true);
    out.flags = dv.getUint32(32, true);
  }
  return out;
}

// uxe/app-asteroid.js
var N = 480;
function freshState() {
  const xs = [], ys = [], zs = [], vxs = [], vys = [], vzs = [], rs = [];
  for (let i = 0; i < N; i++) {
    let sx = i * 13 % 37 - 18;
    let sy = i * 7 % 25 - 12;
    if (sx > -3.5 && sx < 3.5 && sy > -3.5 && sy < 3.5) sx += 7;
    xs.push(sx);
    ys.push(sy);
    zs.push(-40 - i * 1.55);
    vxs.push((i % 5 - 2) * 0.55);
    vys.push((i % 3 - 1) * 0.4);
    vzs.push(8 + i % 11 * 0.35);
    rs.push(0.55 + i % 5 * 0.22);
  }
  return { xs, ys, zs, vxs, vys, vzs, rs, px: 0, py: 0, pz: 0, score: 0, alive: 1 };
}
async function runAsteroidCore(host, opts) {
  host.host_log("info", "core boot abi");
  const wasmBytes = await host.host_asset_read(opts.wasmUrl);
  await bootRuntime(wasmBytes.buffer.slice(
    wasmBytes.byteOffset,
    wasmBytes.byteOffset + wasmBytes.byteLength
  ));
  let fnImage, fnBlob;
  if (opts.precompiled?.image) {
    const img = opts.precompiled.image;
    fnImage = img instanceof Uint8Array ? img : new Uint8Array(img);
    fnBlob = opts.precompiled.blob || {};
  } else {
    if (true) {
      throw new Error("ship build requires precompiled sim embed");
    }
    const { compile } = await import("../core/compiler.js");
    const simText = new TextDecoder().decode(await host.host_asset_read(opts.simUrl));
    const compiled = compile(simText);
    fnImage = compiled.image;
    fnBlob = compiled.blob;
  }
  let state = freshState();
  let alive = true;
  let lastFire = 0;
  let tapMs = 0;
  const warm = await wasm_run({ image: fnImage, blob: fnBlob }, {
    ...state,
    ix: 0,
    iy: 0,
    dt: 0.016
  }, {});
  if (warm.err) throw new Error(JSON.stringify(warm.err));
  state = { ...state, ...unwrap(warm) };
  alive = !!state.alive;
  const rockXYZ = new Float32Array(N * 3);
  const rockS = new Float32Array(N);
  const shipXYZ = new Float32Array(3);
  const shipS = new Float32Array([0.7]);
  const inputBuf = new ArrayBuffer(INPUT_BYTES);
  let last = host.host_time();
  let accUjs = 0, accGpu = 0, accFrames = 0, lastHud = last;
  function buildPacket() {
    const { xs, ys, zs, rs, px, py, pz } = state;
    for (let i = 0; i < N; i++) {
      rockXYZ[i * 3] = xs[i];
      rockXYZ[i * 3 + 1] = ys[i];
      rockXYZ[i * 3 + 2] = zs[i];
      rockS[i] = rs[i];
    }
    shipXYZ[0] = px;
    shipXYZ[1] = py;
    shipXYZ[2] = pz;
    return encodeRenderPacket({
      clear: [0.02, 0.024, 0.04, 1],
      camera: {
        fovy: Math.PI / 3,
        near: 0.1,
        far: 300,
        eye: [px * 0.15, py * 0.15 + 2.8, pz + 11],
        target: [px * 0.05, py * 0.05, pz - 18]
      },
      clouds: [
        {
          color: [0.55, 0.58, 0.62],
          count: N,
          xyz: rockXYZ,
          scale: rockS,
          meshId: 0,
          metalness: 0.15,
          roughness: 0.75,
          emissive: [0, 0, 0]
        },
        {
          color: [0.35, 0.75, 1],
          count: 1,
          xyz: shipXYZ,
          scale: shipS,
          meshId: 1,
          metalness: 0.45,
          roughness: 0.35,
          emissive: [0.05, 0.12, 0.2]
        }
      ]
    });
  }
  async function tick(now) {
    host.host_frame_begin();
    const dt = Math.min(0.05, (now - last) / 1e3);
    last = now;
    const nIn = host.host_input_read(inputBuf);
    if (nIn !== INPUT_BYTES) throw new Error("host_input_read bytes " + nIn);
    const input = decodeInputSnapshot(inputBuf);
    const fireEdge = !!(input.fire && !lastFire);
    lastFire = input.fire;
    if (!alive && fireEdge) {
      const touch = !!(input.flags & 4);
      if (!touch) {
        state = freshState();
        alive = true;
        tapMs = 0;
      } else {
        const nowTap = host.host_time();
        if (tapMs > 0 && nowTap - tapMs < 450) {
          state = freshState();
          alive = true;
          tapMs = 0;
        } else {
          tapMs = nowTap;
        }
      }
    }
    if (alive) tapMs = 0;
    let ujsMs = 0;
    if (alive) {
      const t0 = host.host_time();
      const r = await wasm_run({ image: fnImage, blob: fnBlob }, {
        xs: state.xs,
        ys: state.ys,
        zs: state.zs,
        vxs: state.vxs,
        vys: state.vys,
        vzs: state.vzs,
        rs: state.rs,
        px: state.px,
        py: state.py,
        pz: state.pz,
        ix: input.ix,
        iy: input.iy,
        dt,
        score: state.score,
        alive: 1
      }, {});
      ujsMs = host.host_time() - t0;
      if (r.err) {
        host.host_log("error", "ujs " + JSON.stringify(r.err));
        host.host_request_frame(tick);
        return;
      }
      const out = unwrap(r);
      state = {
        xs: out.xs,
        ys: out.ys,
        zs: out.zs,
        vxs: out.vxs,
        vys: out.vys,
        vzs: out.vzs,
        rs: out.rs,
        px: out.px,
        py: out.py,
        pz: out.pz,
        score: out.score,
        alive: out.alive
      };
      if (!out.alive || out.hit) alive = false;
    }
    const g0 = host.host_time();
    host.host_gpu_submit(buildPacket());
    host.host_frame_present();
    const gpuMs = host.host_time() - g0;
    accUjs += ujsMs;
    accGpu += gpuMs;
    accFrames += 1;
    if (now - lastHud >= 500) {
      const fps = accFrames * 1e3 / (now - lastHud);
      const snap = {
        ready: true,
        abi: true,
        n: N,
        alive,
        fps,
        ujsMs: accUjs / accFrames,
        drawMs: accGpu / accFrames,
        score: state.score
      };
      opts.onHud?.(snap);
      accUjs = 0;
      accGpu = 0;
      accFrames = 0;
      lastHud = now;
    }
    host.host_request_frame(tick);
  }
  opts.onHud?.({ ready: true, abi: true, n: N, score: state.score, alive });
  host.host_request_frame(tick);
  return { n: N };
}

// uxe/ship/asteroid.embed.json
var asteroid_embed_default = { image: [51, 5, 0, 0, 9, 0, 45, 11, 29, 8, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 8, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 8, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 8, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 8, 4, 9, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 45, 6, 21, 36, 185, 0, 0, 0, 5, 0, 0, 0, 0, 9, 0, 5, 1, 0, 0, 0, 9, 2, 5, 2, 0, 0, 0, 9, 3, 5, 3, 0, 0, 0, 9, 4, 5, 4, 0, 0, 0, 9, 5, 5, 5, 0, 0, 0, 9, 6, 5, 6, 0, 0, 0, 9, 7, 5, 7, 0, 0, 0, 9, 8, 5, 8, 0, 0, 0, 9, 9, 5, 9, 0, 0, 0, 9, 10, 5, 10, 0, 0, 0, 9, 11, 5, 11, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 5, 12, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 5, 13, 0, 0, 0, 7, 0, 33, 14, 39, 9, 8, 9, 12, 3, 20, 0, 0, 0, 0, 0, 0, 0, 45, 2, 14, 9, 13, 45, 2, 14, 45, 0, 12, 10, 8, 9, 8, 11, 9, 9, 9, 14, 3, 20, 0, 0, 0, 0, 0, 0, 0, 45, 2, 14, 9, 13, 45, 2, 14, 45, 1, 13, 10, 9, 9, 9, 11, 9, 10, 3, 42, 0, 0, 0, 0, 0, 0, 0, 9, 13, 45, 2, 14, 45, 1, 13, 10, 10, 9, 10, 11, 9, 8, 3, 18, 0, 0, 0, 0, 0, 0, 0, 19, 36, 42, 1, 0, 0, 3, 18, 0, 0, 0, 0, 0, 0, 0, 10, 8, 9, 8, 11, 9, 8, 3, 18, 0, 0, 0, 0, 0, 0, 0, 3, 255, 255, 255, 255, 255, 255, 255, 255, 14, 45, 5, 17, 36, 95, 1, 0, 0, 3, 18, 0, 0, 0, 0, 0, 0, 0, 3, 255, 255, 255, 255, 255, 255, 255, 255, 14, 10, 8, 9, 8, 11, 9, 9, 3, 12, 0, 0, 0, 0, 0, 0, 0, 19, 36, 126, 1, 0, 0, 3, 12, 0, 0, 0, 0, 0, 0, 0, 10, 9, 9, 9, 11, 9, 9, 3, 12, 0, 0, 0, 0, 0, 0, 0, 3, 255, 255, 255, 255, 255, 255, 255, 255, 14, 45, 5, 17, 36, 179, 1, 0, 0, 3, 12, 0, 0, 0, 0, 0, 0, 0, 3, 255, 255, 255, 255, 255, 255, 255, 255, 14, 10, 9, 9, 9, 11, 3, 0, 0, 0, 0, 0, 0, 0, 0, 8, 1, 7, 1, 11, 7, 1, 7, 0, 45, 5, 17, 36, 149, 4, 0, 0, 9, 0, 7, 1, 9, 0, 7, 1, 45, 7, 26, 9, 4, 7, 1, 45, 7, 26, 9, 13, 45, 2, 14, 45, 0, 12, 45, 8, 27, 11, 9, 2, 7, 1, 9, 2, 7, 1, 45, 7, 26, 9, 5, 7, 1, 45, 7, 26, 9, 13, 45, 2, 14, 45, 0, 12, 45, 8, 27, 11, 9, 3, 7, 1, 9, 3, 7, 1, 45, 7, 26, 9, 6, 7, 1, 45, 7, 26, 9, 13, 45, 2, 14, 45, 0, 12, 45, 8, 27, 11, 9, 3, 7, 1, 45, 7, 26, 9, 10, 3, 8, 0, 0, 0, 0, 0, 0, 0, 45, 0, 12, 19, 36, 6, 4, 0, 0, 9, 3, 7, 1, 9, 10, 3, 70, 0, 0, 0, 0, 0, 0, 0, 45, 1, 13, 7, 1, 3, 23, 0, 0, 0, 0, 0, 0, 0, 45, 4, 16, 4, 51, 51, 51, 51, 51, 51, 251, 63, 45, 2, 14, 45, 1, 13, 45, 8, 27, 11, 7, 1, 3, 13, 0, 0, 0, 0, 0, 0, 0, 45, 2, 14, 3, 37, 0, 0, 0, 0, 0, 0, 0, 45, 4, 16, 3, 18, 0, 0, 0, 0, 0, 0, 0, 45, 1, 13, 8, 3, 7, 3, 11, 7, 1, 3, 7, 0, 0, 0, 0, 0, 0, 0, 45, 2, 14, 3, 25, 0, 0, 0, 0, 0, 0, 0, 45, 4, 16, 3, 12, 0, 0, 0, 0, 0, 0, 0, 45, 1, 13, 8, 4, 7, 4, 11, 7, 3, 4, 0, 0, 0, 0, 0, 0, 12, 64, 3, 255, 255, 255, 255, 255, 255, 255, 255, 14, 19, 36, 58, 3, 0, 0, 7, 3, 4, 0, 0, 0, 0, 0, 0, 12, 64, 45, 5, 17, 36, 58, 3, 0, 0, 7, 4, 4, 0, 0, 0, 0, 0, 0, 12, 64, 3, 255, 255, 255, 255, 255, 255, 255, 255, 14, 19, 36, 58, 3, 0, 0, 7, 4, 4, 0, 0, 0, 0, 0, 0, 12, 64, 45, 5, 17, 36, 58, 3, 0, 0, 7, 3, 3, 7, 0, 0, 0, 0, 0, 0, 0, 45, 0, 12, 8, 3, 7, 3, 11, 9, 0, 7, 1, 7, 3, 45, 8, 27, 11, 9, 2, 7, 1, 7, 4, 45, 8, 27, 11, 9, 4, 7, 1, 7, 1, 3, 5, 0, 0, 0, 0, 0, 0, 0, 45, 4, 16, 3, 2, 0, 0, 0, 0, 0, 0, 0, 45, 1, 13, 4, 154, 153, 153, 153, 153, 153, 225, 63, 45, 2, 14, 45, 8, 27, 11, 9, 5, 7, 1, 7, 1, 3, 3, 0, 0, 0, 0, 0, 0, 0, 45, 4, 16, 3, 1, 0, 0, 0, 0, 0, 0, 0, 45, 1, 13, 4, 154, 153, 153, 153, 153, 153, 217, 63, 45, 2, 14, 45, 8, 27, 11, 9, 6, 7, 1, 3, 8, 0, 0, 0, 0, 0, 0, 0, 7, 1, 3, 11, 0, 0, 0, 0, 0, 0, 0, 45, 4, 16, 4, 102, 102, 102, 102, 102, 102, 214, 63, 45, 2, 14, 45, 0, 12, 45, 8, 27, 11, 9, 7, 7, 1, 4, 154, 153, 153, 153, 153, 153, 225, 63, 7, 1, 3, 5, 0, 0, 0, 0, 0, 0, 0, 45, 4, 16, 4, 41, 92, 143, 194, 245, 40, 204, 63, 45, 2, 14, 45, 0, 12, 45, 8, 27, 11, 9, 0, 7, 1, 45, 7, 26, 9, 8, 45, 1, 13, 8, 5, 9, 2, 7, 1, 45, 7, 26, 9, 9, 45, 1, 13, 8, 6, 9, 3, 7, 1, 45, 7, 26, 9, 10, 45, 1, 13, 8, 7, 9, 7, 7, 1, 45, 7, 26, 4, 102, 102, 102, 102, 102, 102, 230, 63, 45, 0, 12, 8, 8, 7, 5, 7, 5, 45, 2, 14, 7, 6, 7, 6, 45, 2, 14, 45, 0, 12, 7, 7, 7, 7, 45, 2, 14, 45, 0, 12, 7, 8, 7, 8, 45, 2, 14, 45, 5, 17, 36, 125, 4, 0, 0, 3, 1, 0, 0, 0, 0, 0, 0, 0, 8, 2, 7, 2, 11, 7, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 45, 0, 12, 8, 1, 7, 1, 11, 35, 193, 1, 0, 0, 7, 2, 3, 1, 0, 0, 0, 0, 0, 0, 0, 45, 6, 21, 36, 182, 4, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 10, 1, 9, 1, 11, 9, 11, 9, 13, 3, 10, 0, 0, 0, 0, 0, 0, 0, 45, 2, 14, 45, 0, 12, 10, 11, 9, 11, 11, 5, 0, 0, 0, 0, 9, 0, 5, 1, 0, 0, 0, 9, 2, 5, 2, 0, 0, 0, 9, 3, 5, 3, 0, 0, 0, 9, 4, 5, 4, 0, 0, 0, 9, 5, 5, 5, 0, 0, 0, 9, 6, 5, 6, 0, 0, 0, 9, 7, 5, 7, 0, 0, 0, 9, 8, 5, 8, 0, 0, 0, 9, 9, 5, 9, 0, 0, 0, 9, 10, 5, 10, 0, 0, 0, 9, 11, 5, 11, 0, 0, 0, 9, 1, 5, 12, 0, 0, 0, 7, 2, 5, 13, 0, 0, 0, 7, 0, 33, 14, 39, 14, 0, 0, 0, 2, 0, 0, 0, 120, 115, 0, 0, 2, 0, 0, 0, 121, 115, 0, 0, 2, 0, 0, 0, 122, 115, 0, 0, 3, 0, 0, 0, 118, 120, 115, 0, 3, 0, 0, 0, 118, 121, 115, 0, 3, 0, 0, 0, 118, 122, 115, 0, 2, 0, 0, 0, 114, 115, 0, 0, 2, 0, 0, 0, 112, 120, 0, 0, 2, 0, 0, 0, 112, 121, 0, 0, 2, 0, 0, 0, 112, 122, 0, 0, 5, 0, 0, 0, 115, 99, 111, 114, 101, 0, 0, 0, 5, 0, 0, 0, 97, 108, 105, 118, 101, 0, 0, 0, 3, 0, 0, 0, 104, 105, 116, 0, 1, 0, 0, 0, 110, 0, 0, 0, 0, 0, 0, 0], blob: { globals: ["xs", "alive", "ys", "zs", "vxs", "vys", "vzs", "rs", "px", "py", "pz", "score", "ix", "dt", "iy"], locals: ["n", "i", "hit", "sx", "sy", "dx", "dy", "dz", "rr"] } };

// uxe/ship/asteroid-host-entry.js
async function startShip(cfg) {
  const host = await createBrowserHost(cfg.canvas, {
    prefer: cfg.prefer || "auto",
    baseURL: new URL(".", cfg.engineUrl),
    pointerLock: false
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
    cfg.hud.innerHTML = `<b>Asteroid Rush</b> \xB7 ship-js<br>backend <b>${host.backend}</b> \xB7 fps <b>${(s.fps || 0).toFixed(0)}</b><br>\u5F97\u5206 <b>${(s.score || 0).toFixed(0)}</b>` + (s.alive ? ` \xB7 \u6D3B\u7740 \xB7 WASD / \u89E6\u5C4F\u79FB\u52A8` : ` \xB7 <span class="warn">\u649E\u6BC1 \u2014 \u7A7A\u683C/\u53CC\u51FB\u91CD\u5F00</span>`);
    if (!s.alive && cfg.banner) {
      cfg.banner.classList.add("show");
      if (cfg.finalEl) cfg.finalEl.textContent = String(Math.floor(s.score || 0));
    } else if (cfg.banner) {
      cfg.banner.classList.remove("show");
    }
  }
  const image = new Uint8Array(asteroid_embed_default.image);
  await runAsteroidCore(host, {
    wasmUrl: "engine.wasm",
    precompiled: { image, blob: asteroid_embed_default.blob },
    onHud: paint
  });
  return { backend: host.backend };
}
export {
  startShip
};

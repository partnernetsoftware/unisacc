// uxe/ship/drone-host-entry.js
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

// uxe/direct_step.js
var TAG2 = { null: 0, bool: 1, i64: 2, f64: 3, str: 4, list: 5, dict: 6 };
function pyToHandle2(ex, mem, v, scratch) {
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
    const base = Number(ex.mem_base());
    new Uint8Array(mem.buffer, base + scratch, bytes.length).set(bytes);
    return ex.host_mk_str(scratch, bytes.length);
  }
  if (Array.isArray(v)) {
    const h = ex.host_mk_list(v.length);
    for (let i = 0; i < v.length; i++)
      ex.host_list_set(h, i, pyToHandle2(ex, mem, v[i], scratch));
    return h;
  }
  if (typeof v === "object") {
    const keys = Object.keys(v);
    const h = ex.host_mk_dict(keys.length);
    for (let i = 0; i < keys.length; i++) {
      ex.host_dict_set(
        h,
        i,
        pyToHandle2(ex, mem, keys[i], scratch),
        pyToHandle2(ex, mem, v[keys[i]], scratch)
      );
    }
    return h;
  }
  throw new Error("unsupported type " + typeof v);
}
function readHandle2(ex, mem, h) {
  if (!h) return null;
  const t = ex.tag_of_export(h);
  if (t === TAG2.null) return null;
  if (t === TAG2.bool) return !!new Uint8Array(mem.buffer)[Number(ex.mem_base()) + h + 4];
  if (t === TAG2.i64) return Number(ex.i64_of_export(h));
  if (t === TAG2.f64) return Number(ex.f64_of_export(h));
  if (t === TAG2.str) {
    const p = Number(ex.str_ptr_export(h));
    const n = Number(ex.str_len_export(h));
    return new TextDecoder().decode(
      new Uint8Array(mem.buffer, Number(ex.mem_base()) + p, n)
    );
  }
  if (t === TAG2.list) {
    const n = Number(ex.host_len(h));
    const out = [];
    for (let i = 0; i < n; i++) out.push(readHandle2(ex, mem, ex.host_list_get(h, i)));
    return out;
  }
  if (t === TAG2.dict) {
    const n = Number(ex.host_len(h));
    const out = {};
    for (let i = 0; i < n; i++) {
      const k = readHandle2(ex, mem, ex.host_dict_key(h, i));
      out[k] = readHandle2(ex, mem, ex.host_dict_val(h, i));
    }
    return out;
  }
  throw new Error("unsupported tag " + t);
}
async function bootDirectStep(wasmBytes, meta) {
  const buf = wasmBytes instanceof ArrayBuffer ? new Uint8Array(wasmBytes) : wasmBytes;
  const { instance } = await WebAssembly.instantiate(buf);
  const ex = instance.exports;
  if (typeof ex.host_run !== "function" && typeof ex.run_step !== "function")
    throw new Error("direct module missing run_step/host_run");
  const runBody = typeof ex.run_step === "function" ? ex.run_step : ex.host_run;
  const mem = ex.memory;
  const gnames = meta.globals || [];
  const scratch = typeof ex.host_scratch === "function" ? Number(ex.host_scratch()) : 95e4;
  return function step(globalsMap) {
    ex.host_reset();
    for (const [k, v] of Object.entries(globalsMap || {})) {
      const ix = gnames.indexOf(k);
      if (ix >= 0) ex.host_set_global(ix, pyToHandle2(ex, mem, v, scratch));
    }
    return readHandle2(ex, mem, runBody());
  };
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

// uxe/meshes.js
var MESH_OCTA2 = 0;
var MESH_SHIP = 1;
var OCTA_MESH = new Float32Array([
  0,
  1,
  0,
  1,
  0,
  0,
  0,
  0,
  1,
  0,
  1,
  0,
  0,
  0,
  1,
  -1,
  0,
  0,
  0,
  1,
  0,
  -1,
  0,
  0,
  0,
  0,
  -1,
  0,
  1,
  0,
  0,
  0,
  -1,
  1,
  0,
  0,
  0,
  -1,
  0,
  0,
  0,
  1,
  1,
  0,
  0,
  0,
  -1,
  0,
  -1,
  0,
  0,
  0,
  0,
  1,
  0,
  -1,
  0,
  0,
  0,
  -1,
  -1,
  0,
  0,
  0,
  -1,
  0,
  1,
  0,
  0,
  0,
  0,
  -1
]);
var SHIP_MESH = new Float32Array([
  // top fan
  0,
  0.35,
  -1.2,
  0.7,
  0.1,
  0.6,
  -0.7,
  0.1,
  0.6,
  // bottom
  0,
  -0.25,
  -1.2,
  -0.7,
  -0.15,
  0.6,
  0.7,
  -0.15,
  0.6,
  // left
  0,
  0.35,
  -1.2,
  -0.7,
  0.1,
  0.6,
  -0.7,
  -0.15,
  0.6,
  0,
  0.35,
  -1.2,
  -0.7,
  -0.15,
  0.6,
  0,
  -0.25,
  -1.2,
  // right
  0,
  0.35,
  -1.2,
  0.7,
  -0.15,
  0.6,
  0.7,
  0.1,
  0.6,
  0,
  0.35,
  -1.2,
  0,
  -0.25,
  -1.2,
  0.7,
  -0.15,
  0.6,
  // rear
  -0.7,
  0.1,
  0.6,
  0.7,
  0.1,
  0.6,
  0.7,
  -0.15,
  0.6,
  -0.7,
  0.1,
  0.6,
  0.7,
  -0.15,
  0.6,
  -0.7,
  -0.15,
  0.6
]);
var BOX_MESH = new Float32Array([
  // +Y
  -0.5,
  0.5,
  -0.5,
  0.5,
  0.5,
  -0.5,
  0.5,
  0.5,
  0.5,
  -0.5,
  0.5,
  -0.5,
  0.5,
  0.5,
  0.5,
  -0.5,
  0.5,
  0.5,
  // -Y
  -0.5,
  -0.5,
  -0.5,
  0.5,
  -0.5,
  0.5,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  -0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  0.5,
  -0.5,
  0.5,
  // +Z
  -0.5,
  -0.5,
  0.5,
  0.5,
  0.5,
  0.5,
  0.5,
  -0.5,
  0.5,
  -0.5,
  -0.5,
  0.5,
  -0.5,
  0.5,
  0.5,
  0.5,
  0.5,
  0.5,
  // -Z
  -0.5,
  -0.5,
  -0.5,
  0.5,
  -0.5,
  -0.5,
  0.5,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  0.5,
  -0.5,
  -0.5,
  0.5,
  -0.5,
  // +X
  0.5,
  -0.5,
  -0.5,
  0.5,
  -0.5,
  0.5,
  0.5,
  0.5,
  0.5,
  0.5,
  -0.5,
  -0.5,
  0.5,
  0.5,
  0.5,
  0.5,
  0.5,
  -0.5,
  // -X
  -0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  -0.5,
  -0.5,
  0.5,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  0.5,
  -0.5,
  -0.5,
  0.5
]);
var MESH_BOX = 2;
var TABLE = [
  { id: MESH_OCTA2, positions: OCTA_MESH, vCount: OCTA_MESH.length / 3 },
  { id: MESH_SHIP, positions: SHIP_MESH, vCount: SHIP_MESH.length / 3 },
  { id: MESH_BOX, positions: BOX_MESH, vCount: BOX_MESH.length / 3 }
];

// uxe/input.js
var INPUT_MAGIC = 1313429589;
var INPUT_BYTES_V1 = 20;
var INPUT_BYTES = 36;
var BTN_RIGHT = 2;
var FLAG_SUICIDE = 2;
var FLAG_TOUCH = 4;
var FLAG_LOOK_STICK = 8;
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

// uxe/app-drone.js
var NT = 8;
var MAGAZINE = 2;
var LOCK_ALIGN = 0.965;
var LOCK_MAX_DIST = 85;
var LOCK_HOLD = 0.18;
var MSL_SPEED = 55;
var SUICIDE_PROX = 5.5;
var SUICIDE_BLAST = 18;
function basis(yaw, pitch) {
  const cp = Math.cos(pitch), sp = Math.sin(pitch);
  const cy = Math.cos(yaw), sy = Math.sin(yaw);
  const fx = cp * sy, fy = sp, fz = cp * cy;
  const rx = cy, ry = 0, rz = -sy;
  return { fx, fy, fz, rx, ry, rz };
}
function freshTargets() {
  const txs = [], tys = [], tzs = [], thp = [], tang = [];
  for (let i = 0; i < NT; i++) {
    const ang = i / NT * Math.PI * 2 + 0.35;
    const r = 18 + i % 3 * 7;
    tang.push(ang);
    txs.push(Math.sin(ang) * r);
    tys.push(5 + i % 3 * 2.5);
    tzs.push(Math.cos(ang) * r - 10);
    thp.push(1);
  }
  return { txs, tys, tzs, thp, tang };
}
function freshState() {
  return {
    px: 0,
    py: 12,
    pz: 32,
    score: 0,
    alive: 1,
    ammo: MAGAZINE,
    ...freshTargets()
  };
}
function bestLock(state, B) {
  let best = -1, bestAlign = LOCK_ALIGN;
  for (let i = 0; i < NT; i++) {
    if (state.thp[i] <= 0) continue;
    const dx = state.txs[i] - state.px;
    const dy = state.tys[i] - state.py;
    const dz = state.tzs[i] - state.pz;
    const dist = Math.sqrt(dx * dx + dy * dy + dz * dz);
    if (dist < 2.5 || dist > LOCK_MAX_DIST) continue;
    const inv = 1 / dist;
    const align = dx * inv * B.fx + dy * inv * B.fy + dz * inv * B.fz;
    if (align > bestAlign) {
      bestAlign = align;
      best = i;
    }
  }
  return best;
}
function faceTargetIdx(state, i) {
  const dx = state.txs[i] - state.px;
  const dy = state.tys[i] - state.py;
  const dz = state.tzs[i] - state.pz;
  const yaw = Math.atan2(dx, dz);
  const horiz = Math.sqrt(dx * dx + dz * dz) || 1;
  const pitch = Math.atan2(dy, horiz);
  return { yaw, pitch };
}
async function runDroneCore(host, opts) {
  host.host_log("info", "drone FP cockpit boot");
  let controls = opts.controls === "mouse" ? "mouse" : "keyboard";
  host.setPointerLockEnabled?.(controls === "mouse");
  let stepSim;
  if (opts.directSim?.wasm && opts.directSim?.meta) {
    host.host_log("info", "sim path B direct");
    stepSim = await bootDirectStep(opts.directSim.wasm, opts.directSim.meta);
  } else {
    if (true) {
      throw new Error("ship requires directSim (path B); bytecode wasm_run removed from ship");
    }
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
      const { compile } = await import("../core/compiler.js");
      const simText = new TextDecoder().decode(await host.host_asset_read(opts.simUrl));
      const compiled = compile(simText);
      fnImage = compiled.image;
      fnBlob = compiled.blob;
    }
    stepSim = async (globalsMap) => {
      const r = await wasm_run({ image: fnImage, blob: fnBlob }, globalsMap, {});
      if (r.err) throw new Error(JSON.stringify(r.err));
      return unwrap(r);
    };
  }
  let state = freshState();
  let yaw = Math.PI, pitch = -0.06;
  let lastFire = 0;
  let kills = 0;
  let lockIdx = -1;
  let lockTime = 0;
  let locked = false;
  let suicideArm = false;
  let endReason = "";
  let last = host.host_time();
  let accFrames = 0, lastHud = last;
  const inputBuf = new ArrayBuffer(INPUT_BYTES);
  let lastAbsMx = 0, lastAbsMy = 0, haveAbs = false;
  let missiles = [];
  let flash = 0;
  let tOrbit = 0;
  let forceFire = false;
  const gN = 64;
  const gXYZ = new Float32Array(gN * 3);
  const gS = new Float32Array(gN);
  let gi = 0;
  for (let gz = -4; gz <= 3; gz++) {
    for (let gx = -4; gx <= 3; gx++) {
      if (gi >= gN) break;
      gXYZ[gi * 3] = gx * 14;
      gXYZ[gi * 3 + 1] = 0;
      gXYZ[gi * 3 + 2] = gz * 14;
      gS[gi] = 13.2;
      gi++;
    }
  }
  async function stepWith(dt, ix, iy, B, thit, suicide, speedMul) {
    try {
      const out = await stepSim({
        px: state.px,
        py: state.py,
        pz: state.pz,
        score: state.score,
        alive: state.alive,
        ammo: state.ammo,
        ix,
        iy,
        dt,
        suicide,
        speed_mul: speedMul,
        fx: B.fx,
        fy: B.fy,
        fz: B.fz,
        rx: B.rx,
        ry: B.ry,
        rz: B.rz,
        txs: state.txs,
        tys: state.tys,
        tzs: state.tzs,
        thp: state.thp,
        thit
      });
      state = {
        px: out.px,
        py: out.py,
        pz: out.pz,
        score: out.score,
        alive: out.alive,
        ammo: state.ammo,
        txs: out.txs,
        tys: out.tys,
        tzs: out.tzs,
        thp: out.thp,
        tang: state.tang
      };
      kills += out.nk || 0;
    } catch (e) {
      host.host_log("error", "ujs " + (e && e.message ? e.message : String(e)));
    }
  }
  function orbitTargets(dt) {
    tOrbit += dt;
    for (let i = 0; i < NT; i++) {
      if (state.thp[i] <= 0) continue;
      state.tang[i] += dt * (0.22 + i % 3 * 0.04);
      const r = 18 + i % 3 * 7;
      state.txs[i] = Math.sin(state.tang[i]) * r;
      state.tzs[i] = Math.cos(state.tang[i]) * r - 10;
      state.tys[i] = 5 + i % 3 * 2.5 + Math.sin(tOrbit * 1.4 + i) * 0.6;
    }
  }
  function cockpitClouds(B) {
    const pieces = [
      [-0.62, -0.42, 0.88, 0.11],
      [0.62, -0.42, 0.88, 0.11],
      [0, -0.52, 0.82, 0.62],
      [-0.78, 0.05, 0.92, 0.09],
      [0.78, 0.05, 0.92, 0.09],
      [-0.45, 0.38, 0.9, 0.08],
      [0.45, 0.38, 0.9, 0.08],
      [0, 0.42, 0.95, 0.5],
      [-0.9, -0.15, 1.05, 0.07],
      [0.9, -0.15, 1.05, 0.07]
    ];
    const xyz = [], sc = [];
    for (const [lx, ly, lz, s] of pieces) {
      const wx = state.px + B.fx * lz + B.rx * lx;
      const wy = state.py + B.fy * lz + ly;
      const wz = state.pz + B.fz * lz + B.rz * lx;
      xyz.push(wx, wy, wz);
      sc.push(s);
    }
    return {
      color: suicideArm ? [0.18, 0.04, 0.04] : [0.04, 0.06, 0.08],
      count: sc.length,
      xyz: new Float32Array(xyz),
      scale: new Float32Array(sc),
      meshId: MESH_BOX,
      metalness: 0.65,
      roughness: 0.32,
      emissive: suicideArm ? [0.12, 0.01, 0.01] : [0.01, 0.025, 0.035]
    };
  }
  function buildPacket(B) {
    const aXYZ = [], aS = [], lockXYZ = [], lockS = [];
    for (let i = 0; i < NT; i++) {
      if (state.thp[i] <= 0) continue;
      const arr = locked && i === lockIdx ? lockXYZ : aXYZ;
      const sc = locked && i === lockIdx ? lockS : aS;
      arr.push(state.txs[i], state.tys[i], state.tzs[i]);
      sc.push(locked && i === lockIdx ? 2 : 1.4);
    }
    const eye = [state.px, state.py, state.pz];
    const target = [
      state.px + B.fx * 80,
      state.py + B.fy * 80,
      state.pz + B.fz * 80
    ];
    const sky = suicideArm ? [0.42, 0.22, 0.2, 1] : flash > 0 ? [0.55, 0.5, 0.35, 1] : [0.32, 0.46, 0.58, 1];
    const clouds = [
      {
        color: [0.2, 0.24, 0.18],
        count: gN,
        xyz: gXYZ,
        scale: gS,
        meshId: MESH_BOX,
        metalness: 0.08,
        roughness: 0.92,
        emissive: [0, 0, 0]
      },
      cockpitClouds(B)
    ];
    if (aS.length) {
      clouds.push({
        color: [0.78, 0.18, 0.1],
        count: aS.length,
        xyz: new Float32Array(aXYZ),
        scale: new Float32Array(aS),
        meshId: MESH_OCTA,
        metalness: 0.25,
        roughness: 0.48,
        emissive: [0.18, 0.03, 0]
      });
    }
    if (lockS.length) {
      clouds.push({
        color: [0.15, 1, 0.4],
        count: lockS.length,
        xyz: new Float32Array(lockXYZ),
        scale: new Float32Array(lockS),
        meshId: MESH_OCTA,
        metalness: 0.3,
        roughness: 0.3,
        emissive: [0.06, 0.4, 0.1]
      });
    }
    if (missiles.length) {
      const mXYZ = [], mS = [];
      for (const m of missiles) {
        mXYZ.push(m.x, m.y, m.z);
        mS.push(0.35);
      }
      clouds.push({
        color: [1, 0.85, 0.25],
        count: mS.length,
        xyz: new Float32Array(mXYZ),
        scale: new Float32Array(mS),
        meshId: MESH_OCTA,
        metalness: 0.4,
        roughness: 0.25,
        emissive: [0.5, 0.35, 0.05]
      });
    }
    clouds.push({
      color: locked ? [0.15, 1, 0.45] : suicideArm ? [1, 0.35, 0.2] : [1, 0.92, 0.25],
      count: 1,
      xyz: new Float32Array([
        state.px + B.fx * 11,
        state.py + B.fy * 11,
        state.pz + B.fz * 11
      ]),
      scale: new Float32Array([locked ? 0.16 : 0.1]),
      meshId: MESH_BOX,
      metalness: 0.2,
      roughness: 0.4,
      emissive: locked ? [0.05, 0.45, 0.12] : [0.35, 0.28, 0.02]
    });
    return encodeRenderPacket({
      clear: sky,
      camera: { fovy: Math.PI / 2.2, near: 0.06, far: 280, eye, target },
      fog: {
        density: suicideArm ? 0.016 : 0.012,
        color: suicideArm ? [0.45, 0.28, 0.22] : [0.38, 0.5, 0.58]
      },
      ambient: { color: [0.42, 0.48, 0.55], intensity: 0.72 },
      lights: [
        { dir: [0.25, 1, 0.15], color: [1, 0.95, 0.85], intensity: 1.2 },
        { dir: [-0.4, 0.15, -0.35], color: [0.35, 0.45, 0.65], intensity: 0.42 }
      ],
      clouds
    });
  }
  function resetRun() {
    state = freshState();
    yaw = Math.PI;
    pitch = -0.06;
    kills = 0;
    lockIdx = -1;
    lockTime = 0;
    locked = false;
    suicideArm = false;
    endReason = "";
    missiles = [];
    flash = 0;
    tOrbit = 0;
    haveAbs = false;
  }
  function spawnMissile(ti, B) {
    missiles.push({
      x: state.px + B.fx * 1.2,
      y: state.py + B.fy * 1.2,
      z: state.pz + B.fz * 1.2,
      vx: B.fx * MSL_SPEED,
      vy: B.fy * MSL_SPEED,
      vz: B.fz * MSL_SPEED,
      life: 2.2,
      ti
    });
  }
  function updateMissiles(dt, thit) {
    const next = [];
    for (const m of missiles) {
      m.life -= dt;
      if (m.life <= 0) continue;
      if (m.ti >= 0 && state.thp[m.ti] > 0) {
        const dx = state.txs[m.ti] - m.x;
        const dy = state.tys[m.ti] - m.y;
        const dz = state.tzs[m.ti] - m.z;
        const d = Math.sqrt(dx * dx + dy * dy + dz * dz) || 1;
        const pull = 38 * dt;
        m.vx += dx / d * pull;
        m.vy += dy / d * pull;
        m.vz += dz / d * pull;
        const sp = Math.sqrt(m.vx * m.vx + m.vy * m.vy + m.vz * m.vz) || 1;
        const want = MSL_SPEED;
        m.vx = m.vx / sp * want;
        m.vy = m.vy / sp * want;
        m.vz = m.vz / sp * want;
        if (d < 2.2) {
          thit[m.ti] = 1;
          flash = 0.25;
          continue;
        }
      }
      m.x += m.vx * dt;
      m.y += m.vy * dt;
      m.z += m.vz * dt;
      let hit = false;
      for (let i = 0; i < NT; i++) {
        if (state.thp[i] <= 0) continue;
        const dx = state.txs[i] - m.x;
        const dy = state.tys[i] - m.y;
        const dz = state.tzs[i] - m.z;
        if (dx * dx + dy * dy + dz * dz < 4.5) {
          thit[i] = 1;
          flash = 0.25;
          hit = true;
          break;
        }
      }
      if (!hit) next.push(m);
    }
    missiles = next;
  }
  function armSuicideBlast(thit) {
    for (let j = 0; j < NT; j++) {
      if (state.thp[j] <= 0) continue;
      const ex = state.txs[j] - state.px;
      const ey = state.tys[j] - state.py;
      const ez = state.tzs[j] - state.pz;
      if (ex * ex + ey * ey + ez * ez < SUICIDE_BLAST * SUICIDE_BLAST) thit[j] = 1;
    }
  }
  const api = {
    faceNearest() {
      let best = -1, bestD = 1e9;
      for (let i = 0; i < NT; i++) {
        if (state.thp[i] <= 0) continue;
        const dx = state.txs[i] - state.px;
        const dy = state.tys[i] - state.py;
        const dz = state.tzs[i] - state.pz;
        const d = dx * dx + dy * dy + dz * dz;
        if (d < bestD) {
          bestD = d;
          best = i;
        }
      }
      if (best < 0) return false;
      const f = faceTargetIdx(state, best);
      yaw = f.yaw;
      pitch = Math.max(-1.1, Math.min(1.1, f.pitch));
      lockIdx = best;
      lockTime = LOCK_HOLD;
      locked = true;
      return true;
    },
    faceTarget(i) {
      if (i < 0 || i >= NT || state.thp[i] <= 0) return false;
      const f = faceTargetIdx(state, i);
      yaw = f.yaw;
      pitch = Math.max(-1.1, Math.min(1.1, f.pitch));
      lockIdx = i;
      lockTime = LOCK_HOLD;
      locked = true;
      return true;
    },
    /** Probe/helper: fire immediately if locked + ammo, else queue next tick. */
    fire() {
      if (state.alive && locked && state.ammo > 0 && !suicideArm && lockIdx >= 0) {
        spawnMissile(lockIdx, basis(yaw, pitch));
        state.ammo -= 1;
        locked = false;
        lockTime = 0;
        flash = 0.12;
        return { ok: true, ammo: state.ammo, missiles: missiles.length };
      }
      forceFire = true;
      return { ok: false, locked, ammo: state.ammo, suicideArm };
    },
    setSuicideArm(v) {
      suicideArm = !!v;
      return suicideArm;
    },
    /** Instant proximity blast for probe (armed suicide path). */
    detonateNow() {
      if (!state.alive) return false;
      suicideArm = true;
      let best = -1, bestD = 1e9;
      for (let i = 0; i < NT; i++) {
        if (state.thp[i] <= 0) continue;
        const dx = state.txs[i] - state.px;
        const dy = state.tys[i] - state.py;
        const dz = state.tzs[i] - state.pz;
        const d = dx * dx + dy * dy + dz * dz;
        if (d < bestD) {
          bestD = d;
          best = i;
        }
      }
      if (best >= 0) {
        state.px = state.txs[best];
        state.py = state.tys[best];
        state.pz = state.tzs[best];
      }
      const thit = Array(NT).fill(0);
      armSuicideBlast(thit);
      let nk = 0;
      for (let i = 0; i < NT; i++) {
        if (thit[i] && state.thp[i] > 0) {
          state.thp[i] = 0;
          nk++;
          state.score += 500;
        }
      }
      kills += nk;
      state.alive = 0;
      state.score += 150;
      endReason = state.thp.every((h) => h <= 0) ? "win" : "suicide";
      if (endReason === "win") state.score += 1e3;
      flash = 0.5;
      return { nk, endReason, kills, remaining: state.thp.filter((h) => h > 0).length };
    },
    reset: resetRun,
    getSnapshot() {
      return {
        ammo: state.ammo,
        kills,
        locked,
        lockIdx,
        suicideArm,
        remaining: state.thp.filter((h) => h > 0).length,
        alive: state.alive,
        endReason,
        missiles: missiles.length,
        px: state.px,
        py: state.py,
        pz: state.pz,
        yaw,
        pitch
      };
    }
  };
  if (typeof globalThis !== "undefined") globalThis.__DRONE_API__ = api;
  async function tick(now) {
    host.host_frame_begin();
    const dt = Math.min(0.05, (now - last) / 1e3);
    last = now;
    if (flash > 0) flash -= dt;
    let mx = 0, my = 0, buttons = 0, flags = 0, ix = 0, iy = 0, fire = 0;
    let keys = {};
    let pointerLock = false;
    const nIn = host.host_input_read(inputBuf);
    if (nIn === INPUT_BYTES) {
      const input = decodeInputSnapshot(inputBuf);
      ix = input.ix;
      iy = input.iy;
      fire = input.fire;
      mx = input.mx;
      my = input.my;
      buttons = input.buttons;
      flags = input.flags;
    }
    const snapObj = host.host_input_read();
    if (snapObj && typeof snapObj === "object") {
      keys = snapObj.keys || {};
      pointerLock = !!snapObj.pointerLock;
    }
    if (flags & FLAG_TOUCH) {
      if (flags & FLAG_LOOK_STICK) {
        yaw += mx * 2.6 * dt;
        pitch -= my * 2.2 * dt;
      }
      haveAbs = false;
    } else if (controls === "keyboard") {
      let lookX = 0, lookY = 0;
      if (keys.KeyJ || keys.ArrowLeft) lookX -= 1;
      if (keys.KeyL || keys.ArrowRight) lookX += 1;
      if (keys.KeyI || keys.ArrowUp) lookY -= 1;
      if (keys.KeyK || keys.ArrowDown) lookY += 1;
      ix = 0;
      iy = 0;
      if (keys.KeyA) ix -= 1;
      if (keys.KeyD) ix += 1;
      if (keys.KeyS) iy += 1;
      if (keys.KeyW) iy -= 1;
      fire = keys.Space ? 1 : 0;
      yaw += lookX * 2.4 * dt;
      pitch -= lookY * 2 * dt;
      haveAbs = false;
    } else if (pointerLock) {
      yaw += mx * 2.6;
      pitch += my * 2.2;
      haveAbs = false;
    } else if (flags & 1 || haveAbs) {
      if (haveAbs) {
        yaw += (mx - lastAbsMx) * 2.8;
        pitch += (my - lastAbsMy) * 2.4;
      }
      lastAbsMx = mx;
      lastAbsMy = my;
      haveAbs = true;
    }
    if (pitch > 1.2) pitch = 1.2;
    if (pitch < -1.2) pitch = -1.2;
    const B = basis(yaw, pitch);
    const fireEdge = fire && !lastFire || forceFire;
    forceFire = false;
    lastFire = fire;
    const wantSuicide = controls === "mouse" ? !!(flags & FLAG_SUICIDE) || !!(buttons & BTN_RIGHT) || !!keys.KeyF : !!keys.KeyF || !!(flags & FLAG_SUICIDE);
    if (wantSuicide && state.alive) suicideArm = true;
    const cand = bestLock(state, B);
    if (cand >= 0 && cand === lockIdx) lockTime += dt;
    else if (!(locked && lockIdx >= 0 && cand < 0)) {
      if (locked && lockIdx >= 0 && state.thp[lockIdx] > 0) {
        lockTime = Math.max(0, lockTime - dt * 0.5);
        if (lockTime <= 0) {
          locked = false;
          lockIdx = cand;
        }
      } else {
        lockIdx = cand;
        lockTime = 0;
        locked = false;
      }
    } else {
      lockIdx = cand;
      lockTime = 0;
      locked = false;
    }
    if (lockIdx >= 0 && lockTime >= LOCK_HOLD) locked = true;
    const thit = Array(NT).fill(0);
    let suicide = 0;
    const boost = !!(keys.ShiftLeft || keys.ShiftRight);
    let speedMul = (suicideArm ? 2.05 : 1) * (boost ? 1.35 : 1);
    if (state.alive && fireEdge && locked && state.ammo > 0 && !suicideArm) {
      spawnMissile(lockIdx, B);
      state.ammo -= 1;
      locked = false;
      lockTime = 0;
      flash = 0.12;
    }
    if (state.alive) updateMissiles(dt, thit);
    if (state.alive) orbitTargets(dt);
    if (state.alive && suicideArm) {
      for (let i = 0; i < NT; i++) {
        if (state.thp[i] <= 0) continue;
        const dx = state.txs[i] - state.px;
        const dy = state.tys[i] - state.py;
        const dz = state.tzs[i] - state.pz;
        const d2 = dx * dx + dy * dy + dz * dz;
        if (d2 < SUICIDE_PROX * SUICIDE_PROX) {
          armSuicideBlast(thit);
          suicide = 1;
          endReason = "suicide";
          flash = 0.5;
          break;
        }
      }
      if (!suicide && state.py <= 2.55 && iy >= 0) {
        armSuicideBlast(thit);
        suicide = 1;
        endReason = "suicide";
        flash = 0.5;
      }
    }
    if (!state.alive) {
      if (fireEdge) resetRun();
    } else {
      await stepWith(dt, ix, iy, B, thit, suicide, speedMul);
      const rem = state.thp.filter((h) => h > 0).length;
      if (rem === 0 && state.alive) {
        endReason = "win";
        state.alive = 0;
        state.score += 1e3;
      }
      if (!state.alive && !endReason) endReason = suicide ? "suicide" : "crash";
    }
    const remaining = state.thp.filter((h) => h > 0).length;
    host.host_gpu_submit(buildPacket(basis(yaw, pitch)));
    host.host_frame_present();
    accFrames++;
    if (now - lastHud >= 160) {
      const touch = !!(flags & FLAG_TOUCH);
      const mode = !state.alive ? endReason === "win" ? "\u4EFB\u52A1\u5B8C\u6210 \u2014 \u5168\u6B7C" : endReason === "suicide" ? "\u81EA\u7206\u51FA\u51FB" : "\u5931\u8054" : suicideArm ? "\u6A21\u5F0F B\uFF1A\u81EA\u7206\u51B2\u649E \u2014 \u649E\u5411\u76EE\u6807\uFF01" : locked ? "\u6A21\u5F0F A\uFF1A\u5BFC\u5F39\u9501\u5B9A \u2014 \u5C04\u51FB\uFF01" : state.ammo <= 0 ? "\u5F39\u4ED3\u7A7A \u2014 F/\u53F3\u952E\u6B66\u88C5\u81EA\u7206" : touch ? "\u89E6\u5C4F\uFF1A\u5DE6\u6307\u62D6\u98DE \xB7 \u53F3\u6307\u62D6\u770B \xB7 \u53CC\u6307\u5E76\u7528" : "\u6A21\u5F0F A\uFF1A\u641C\u7D22\u9501\u5B9A\u76EE\u6807";
      opts.onHud?.({
        ready: true,
        drone: true,
        fp: true,
        abi: true,
        score: state.score,
        kills,
        remaining,
        alive: state.alive,
        ammo: state.ammo,
        magazine: MAGAZINE,
        locked,
        lockIdx,
        suicideArm,
        endReason,
        mode,
        controls,
        touch,
        mx,
        my,
        buttons,
        flags,
        pointerLock,
        missiles: missiles.length,
        pointer: controls === "mouse",
        fps: accFrames * 1e3 / (now - lastHud)
      });
      accFrames = 0;
      lastHud = now;
    }
    host.host_request_frame(tick);
  }
  opts.onHud?.({
    ready: true,
    drone: true,
    fp: true,
    abi: true,
    score: 0,
    kills: 0,
    remaining: NT,
    alive: 1,
    ammo: MAGAZINE,
    magazine: MAGAZINE,
    locked: false,
    suicideArm: false,
    mode: "\u6A21\u5F0F A\uFF1A\u641C\u7D22\u9501\u5B9A\u76EE\u6807",
    controls,
    mx: 0,
    my: 0,
    buttons: 0,
    flags: 0,
    pointer: controls === "mouse",
    missiles: 0
  });
  host.host_request_frame(tick);
  return {
    nt: NT,
    magazine: MAGAZINE,
    api,
    getControls: () => controls,
    setControls(next) {
      controls = next === "mouse" ? "mouse" : "keyboard";
      host.setPointerLockEnabled?.(controls === "mouse");
      haveAbs = false;
    }
  };
}

// uxe/ship/drone/sim.meta.json
var sim_meta_default = { globals: ["txs", "alive", "px", "py", "pz", "score", "ammo", "tys", "tzs", "thp", "iy", "ix", "fy", "speed_mul", "fx", "rx", "dt", "ry", "fz", "rz", "thit", "suicide"], locals: [] };

// uxe/ship/drone-host-entry.js
function helpLine(controls, touch) {
  if (touch) {
    return "\u5DE6\u6307\u62D6\u98DE \xB7 \u53F3\u6307\u62D6\u770B \xB7 \u9501\u5B9A\u540E\u70B9\u53D1\u5C04 \xB7 \u53CC\u51FB\u7A7A\u6863\u518D\u51FA\u51FB";
  }
  return controls === "mouse" ? "\u9F20\u6807\u770B \xB7 WASD \u98DE \xB7 Shift \u52A0\u901F \xB7 \u9501\u5B9A\u540E\u70B9\u51FB/\u7A7A\u683C\u53D1\u5C04\uFF082\u53D1\uFF09\xB7 F/\u53F3\u952E\u6B66\u88C5\u81EA\u7206" : "IJKL \u770B \xB7 WASD \u98DE \xB7 Shift \u52A0\u901F \xB7 \u9501\u5B9A\u540E\u7A7A\u683C\u53D1\u5C04\uFF082\u53D1\uFF09\xB7 F \u6B66\u88C5\u81EA\u7206";
}
async function startDroneShip(cfg) {
  let controls = cfg.controls === "mouse" ? "mouse" : "keyboard";
  const host = await createBrowserHost(cfg.canvas, {
    prefer: cfg.prefer || "auto",
    baseURL: new URL(".", cfg.engineUrl),
    pointerLock: controls === "mouse"
  });
  window.__UXE_HOST__ = host;
  const engineUrl = cfg.engineUrl;
  const simUrl = cfg.simUrl || new URL("sim.wasm", new URL(".", engineUrl)).href;
  function paint(s) {
    window.__UXE__ = {
      ...s,
      backend: host.backend,
      ship: true,
      drone: true,
      pathB: true
    };
    if (!s.ready) return;
    cfg.hud.classList.toggle("armed", !!s.suicideArm);
    cfg.hud.classList.toggle("locked-on", !!s.locked && !s.suicideArm);
    cfg.reticle?.classList.toggle("lock", !!s.locked && !s.suicideArm);
    cfg.reticle?.classList.toggle("suicide", !!s.suicideArm);
    const ammoBar = "\u25AE".repeat(s.ammo || 0) + "\u25AF".repeat(Math.max(0, (s.magazine || 2) - (s.ammo || 0)));
    if (cfg.ammoEl) cfg.ammoEl.textContent = ammoBar;
    const modeLabel = s.controls === "mouse" ? "\u952E\u76D8+\u9F20\u6807" : "\u7EAF\u952E\u76D8";
    cfg.hud.innerHTML = `<b>\u65E0\u4EBA\u673A \xB7 \u7B2C\u4E00\u4EBA\u79F0\u9A7E\u8231</b> \xB7 ship-js \xB7 <b>path B</b><br>backend <b>${host.backend}</b> \xB7 fps <b>${(s.fps || 0).toFixed(0)}</b> \xB7 <b>${modeLabel}</b><br><span class="mode">${s.mode || ""}</span><br>\u5F39\u4ED3 <b>${ammoBar}</b> (${s.ammo}/${s.magazine})` + (s.missiles ? ` \xB7 \u5728\u9014 <b>${s.missiles}</b>` : "") + ` \xB7 \u51FB\u6BC1 <b>${s.kills || 0}</b> \xB7 \u654C <b>${s.remaining ?? "?"}</b><br>\u5F97\u5206 <b>${(s.score || 0).toFixed(0)}</b>` + (s.locked ? ` \xB7 <b class="lock">\u9501\u5B9A</b>` : "") + (s.suicideArm ? ` \xB7 <b class="warn">\u81EA\u7206\u5DF2\u6B66\u88C5</b>` : "") + `<br>` + (s.alive ? helpLine(s.controls, s.touch) : `<span class="warn">${s.endReason === "win" ? "\u5168\u6B7C" : "\u4EFB\u52A1\u7ED3\u675F"} \u2014 \u7A7A\u683C\u518D\u51FA\u51FB</span>`);
  }
  const simRes = await fetch(simUrl);
  if (!simRes.ok) throw new Error("fetch sim.wasm " + simRes.status);
  const simWasm = new Uint8Array(await simRes.arrayBuffer());
  if (simWasm[0] !== 0 || simWasm[1] !== 97 || simWasm[2] !== 115 || simWasm[3] !== 109) {
    throw new Error("sim.wasm bad magic");
  }
  const api = await runDroneCore(host, {
    directSim: {
      wasm: simWasm,
      meta: { globals: sim_meta_default.globals || [], locals: sim_meta_default.locals || [] }
    },
    controls,
    onHud: paint
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
    }
  };
}
export {
  startDroneShip
};

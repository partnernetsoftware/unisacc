/** Product API: wasm_run(code | fn, globals, locals) in the browser. [A-1][T-5] */
import { compile } from "./compiler.js";

const TAG = { null: 0, bool: 1, i64: 2, f64: 3, str: 4, list: 5, dict: 6, fn: 7, tup: 8 };

const HOST_EXPORTS = [
  "host_reset", "host_prog_addr", "host_load_image", "host_run",
  "host_set_local", "host_set_global", "host_get_local", "host_get_global",
  "host_mk_null", "host_mk_bool", "host_mk_i64", "host_mk_f64", "host_mk_str",
  "host_mk_list", "host_list_set", "host_list_get",
  "host_mk_dict", "host_dict_set", "host_dict_key", "host_dict_val", "host_len",
  "tag_of_export", "i64_of_export", "f64_of_export",
  "str_len_export", "str_ptr_export", "mem_base", "mem_size",
  "last_ic_stub_export", "ujs_ic_ask",
];

let rt = null; // { ex, mem }

async function loadWasmBytes(spec) {
  if (spec instanceof ArrayBuffer) return new Uint8Array(spec);
  if (ArrayBuffer.isView(spec)) return new Uint8Array(spec.buffer, spec.byteOffset, spec.byteLength);
  if (typeof URL !== "undefined" && spec instanceof URL) {
    if (typeof process !== "undefined" && spec.protocol === "file:") {
      const fs = await import("fs");
      return fs.readFileSync(spec);
    }
    const r = await fetch(spec);
    if (!r.ok) throw new Error("fetch wasm failed: " + spec + " (" + r.status + ")");
    return new Uint8Array(await r.arrayBuffer());
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
      path.resolve(spec),
    ].filter(Boolean);
    for (const p of cand) {
      try { return fs.readFileSync(p); } catch (_) {}
    }
    throw new Error(
      "wasm not found: " + spec + " (run: python3 -m ujs web-build / npm run build)");
  }
  const r = await fetch(spec);
  if (!r.ok) throw new Error("fetch wasm failed: " + spec + " (" + r.status + ")");
  return new Uint8Array(await r.arrayBuffer());
}

/** Boot shared full VM. Prefer: bootRuntime(new URL('./ujs_full.wasm', import.meta.url)) */
export async function bootRuntime(wasmUrl = "ujs_full.wasm") {
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
  if (v === null || v === undefined) return ex.host_mk_null();
  if (typeof v === "boolean") return ex.host_mk_bool(v ? 1 : 0);
  if (typeof v === "number") {
    if (Number.isInteger(v) && Math.abs(v) <= Number.MAX_SAFE_INTEGER)
      return ex.host_mk_i64(BigInt(v));
    return ex.host_mk_f64(v);
  }
  if (typeof v === "bigint") return ex.host_mk_i64(v);
  if (typeof v === "string") {
    const bytes = new TextEncoder().encode(v);
    const scratch = 500000; // offset into mem[]
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

/**
 * wasm_run(code | {image,blob}, globals, locals) → { ok } | { err }
 * code: string source (UJS-1 / growing JS surface)
 * fn: precompiled { image: Uint8Array, blob?: {locals,globals} }
 */
export async function wasm_run(codeOrFn, globalsMap = {}, localsMap = {}) {
  const { ex, mem } = ensureRt();
  let image, blob, askTrace, askHeat;
  try {
    if (typeof codeOrFn === "string") {
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
    lnames.forEach((n, i) => { Lout[n] = readHandle(ex, mem, ex.host_get_local(i)); });
    const Gout = {};
    gnames.forEach((n, i) => { Gout[n] = readHandle(ex, mem, ex.host_get_global(i)); });
    return {
      ok, locals: Lout, globals: Gout,
      ic_stub: Number(ex.last_ic_stub_export()),
      askTrace: askTrace || [],
      askHeat: askHeat || {},
      imageBytes: image.length,
    };
  } catch (e) {
    return { err: { kind: "Trap", message: String(e.message || e) } };
  }
}

function unwrap(r) {
  if (r && r.err) throw new Error((r.err.kind || "err") + ": " + (r.err.message || ""));
  return r.ok;
}

export { compile, HOST_EXPORTS, TAG, unwrap };

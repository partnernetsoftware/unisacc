/** Product API: wasm_run(code | fn, globals, locals) in the browser. [A-1][T-5] */
import { compile } from "./compiler.js";

const TAG = { null: 0, bool: 1, i64: 2, f64: 3, str: 4, list: 5, dict: 6, fn: 7, tup: 8 };

const HOST_EXPORTS = [
  "host_reset", "host_prog_addr", "host_load_image", "host_run",
  "host_set_local", "host_set_global", "host_get_local", "host_get_global",
  "host_mk_null", "host_mk_bool", "host_mk_i64", "host_mk_f64", "host_mk_str",
  "tag_of_export", "i64_of_export", "f64_of_export",
  "str_len_export", "str_ptr_export", "mem_base", "mem_size",
  "last_ic_stub_export", "ujs_ic_ask",
];

let rt = null; // { exports, memory }

export async function bootRuntime(wasmUrl = "ujs_full.wasm") {
  let buf;
  if (typeof wasmUrl !== "string") {
    buf = wasmUrl;
  } else if (typeof process !== "undefined" && wasmUrl.indexOf("://") < 0) {
    const fs = await import("fs");
    const path = await import("path");
    const { fileURLToPath } = await import("url");
    const here = path.dirname(fileURLToPath(import.meta.url));
    const cand = [
      path.isAbsolute(wasmUrl) ? wasmUrl : null,
      path.join(here, wasmUrl),
      path.resolve(wasmUrl),
    ].filter(Boolean);
    let buf = null;
    for (const p of cand) {
      try { buf = fs.readFileSync(p); break; } catch (_) {}
    }
    if (!buf) throw new Error("wasm not found: " + wasmUrl);
    // continue with buf below
    const { instance } = await WebAssembly.instantiate(buf);
    rt = { ex: instance.exports, mem: instance.exports.memory };
    return rt;
  } else {
    buf = await fetch(wasmUrl).then((r) => r.arrayBuffer());
  }
  const { instance } = await WebAssembly.instantiate(buf);
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
    const base = Number(ex.mem_base());
    const scratch = base + 500000;
    new Uint8Array(mem.buffer).set(bytes, scratch);
    return ex.host_mk_str(500000, bytes.length); // C offset into mem[]
  }
  throw new Error("unsupported global/local type: " + typeof v);
}

function readHandle(ex, mem, h) {
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

export { compile, HOST_EXPORTS, TAG };

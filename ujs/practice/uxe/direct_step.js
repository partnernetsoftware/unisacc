/**
 * Path-B stepper: ujs2wasm direct module via host_* (no bytecode image).
 * Same export names as ujs_full.wasm binders in wasm_run.js.
 */
const TAG = { null: 0, bool: 1, i64: 2, f64: 3, str: 4, list: 5, dict: 6 };

function pyToHandle(ex, mem, v, scratch) {
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
    new Uint8Array(mem.buffer, base + scratch, bytes.length).set(bytes);
    return ex.host_mk_str(scratch, bytes.length);
  }
  if (Array.isArray(v)) {
    const h = ex.host_mk_list(v.length);
    for (let i = 0; i < v.length; i++)
      ex.host_list_set(h, i, pyToHandle(ex, mem, v[i], scratch));
    return h;
  }
  if (typeof v === "object") {
    const keys = Object.keys(v);
    const h = ex.host_mk_dict(keys.length);
    for (let i = 0; i < keys.length; i++) {
      ex.host_dict_set(h, i,
        pyToHandle(ex, mem, keys[i], scratch),
        pyToHandle(ex, mem, v[keys[i]], scratch));
    }
    return h;
  }
  throw new Error("unsupported type " + typeof v);
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
    return new TextDecoder().decode(
      new Uint8Array(mem.buffer, Number(ex.mem_base()) + p, n));
  }
  if (t === TAG.list) {
    const n = Number(ex.host_len(h));
    const out = [];
    for (let i = 0; i < n; i++) out.push(readHandle(ex, mem, ex.host_list_get(h, i)));
    return out;
  }
  if (t === TAG.dict) {
    const n = Number(ex.host_len(h));
    const out = {};
    for (let i = 0; i < n; i++) {
      const k = readHandle(ex, mem, ex.host_dict_key(h, i));
      out[k] = readHandle(ex, mem, ex.host_dict_val(h, i));
    }
    return out;
  }
  throw new Error("unsupported tag " + t);
}

/**
 * @param {ArrayBuffer|Uint8Array} wasmBytes direct \\0asm from ujs2wasm
 * @param {{ locals?: string[], globals: string[] }} meta emit / encode_fn name tables
 * @returns {Promise<(globals: object) => object>}
 */
export async function bootDirectStep(wasmBytes, meta) {
  const buf = wasmBytes instanceof ArrayBuffer
    ? new Uint8Array(wasmBytes)
    : wasmBytes;
  const { instance } = await WebAssembly.instantiate(buf);
  const ex = instance.exports;
  if (typeof ex.host_run !== "function" && typeof ex.run_step !== "function")
    throw new Error("direct module missing run_step/host_run");
  const runBody = typeof ex.run_step === "function" ? ex.run_step : ex.host_run;
  const mem = ex.memory;
  const gnames = meta.globals || [];
  const scratch = typeof ex.host_scratch === "function"
    ? Number(ex.host_scratch()) : 950000;

  return function step(globalsMap) {
    ex.host_reset();
    for (const [k, v] of Object.entries(globalsMap || {})) {
      const ix = gnames.indexOf(k);
      if (ix >= 0) ex.host_set_global(ix, pyToHandle(ex, mem, v, scratch));
    }
    return readHandle(ex, mem, runBody());
  };
}

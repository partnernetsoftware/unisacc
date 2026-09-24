#!/usr/bin/env node
/**
 * Drive a ujs2wasm *direct* module for one (or more) host-injected steps.
 *
 *   node run_step.mjs module.wasm meta.json globals.json [steps=1]
 *
 * meta.json: { "locals": [...], "globals": [...] }  (encode_fn / emit order)
 * globals.json: object of initial globals (lists/scalars)
 * stdin unused; prints JSON return value of the last step.
 *
 * ABI: host_reset → host_set_global* → run_step|host_run → read dict/list/f64
 * (same host_* names as path-A ujs_full.wasm; no image load).
 */
import fs from "fs";

const TAG = { null: 0, bool: 1, i64: 2, f64: 3, str: 4, list: 5, dict: 6 };

const wasmPath = process.argv[2];
const metaPath = process.argv[3];
const globalsPath = process.argv[4];
const steps = Number(process.argv[5] || "1");
if (!wasmPath || !metaPath || !globalsPath) {
  console.error("usage: run_step.mjs module.wasm meta.json globals.json [steps]");
  process.exit(2);
}

const meta = JSON.parse(fs.readFileSync(metaPath, "utf8"));
const gnames = meta.globals || [];
let state = JSON.parse(fs.readFileSync(globalsPath, "utf8"));

const { instance } = await WebAssembly.instantiate(fs.readFileSync(wasmPath));
const ex = instance.exports;
for (const n of [
  "host_reset", "host_set_global", "host_mk_f64", "host_mk_i64",
  "host_mk_list", "host_list_set", "host_list_get", "host_len",
  "host_dict_key", "host_dict_val", "tag_of_export", "f64_of_export",
]) {
  if (typeof ex[n] !== "function") throw new Error("missing " + n);
}
const runBody = typeof ex.run_step === "function" ? ex.run_step
  : (typeof ex.host_run === "function" ? ex.host_run : null);
if (!runBody) throw new Error("missing run_step/host_run");


function scratch() {
  return typeof ex.host_scratch === "function" ? Number(ex.host_scratch()) : 950000;
}

function pyToHandle(v) {
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
    const ptr = scratch();
    new Uint8Array(ex.memory.buffer, base + ptr, bytes.length).set(bytes);
    return ex.host_mk_str(ptr, bytes.length);
  }
  if (Array.isArray(v)) {
    const h = ex.host_mk_list(v.length);
    for (let i = 0; i < v.length; i++)
      ex.host_list_set(h, i, pyToHandle(v[i]));
    return h;
  }
  if (typeof v === "object") {
    const keys = Object.keys(v);
    const h = ex.host_mk_dict(keys.length);
    for (let i = 0; i < keys.length; i++)
      ex.host_dict_set(h, i, pyToHandle(keys[i]), pyToHandle(v[keys[i]]));
    return h;
  }
  throw new Error("unsupported type " + typeof v);
}

function readHandle(h) {
  if (!h) return null;
  const t = ex.tag_of_export(h);
  if (t === TAG.null) return null;
  if (t === TAG.bool) return !!new Uint8Array(ex.memory.buffer)[Number(ex.mem_base()) + h + 4];
  if (t === TAG.i64) return Number(ex.i64_of_export(h));
  if (t === TAG.f64) return Number(ex.f64_of_export(h));
  if (t === TAG.str) {
    const p = Number(ex.str_ptr_export(h));
    const n = Number(ex.str_len_export(h));
    return new TextDecoder().decode(
      new Uint8Array(ex.memory.buffer, Number(ex.mem_base()) + p, n));
  }
  if (t === TAG.list) {
    const n = Number(ex.host_len(h));
    const out = [];
    for (let i = 0; i < n; i++) out.push(readHandle(ex.host_list_get(h, i)));
    return out;
  }
  if (t === TAG.dict) {
    const n = Number(ex.host_len(h));
    const out = {};
    for (let i = 0; i < n; i++) {
      const k = readHandle(ex.host_dict_key(h, i));
      out[k] = readHandle(ex.host_dict_val(h, i));
    }
    return out;
  }
  throw new Error("unsupported tag " + t);
}

function stepOnce(globalsMap) {
  ex.host_reset();
  for (const [k, v] of Object.entries(globalsMap)) {
    const ix = gnames.indexOf(k);
    if (ix >= 0) ex.host_set_global(ix, pyToHandle(v));
  }
  return readHandle(runBody());
}

let out = null;
for (let s = 0; s < steps; s++) {
  out = stepOnce(state);
  // carry sim-shaped state forward when present
  if (out && typeof out === "object" && !Array.isArray(out)) {
    const next = { ...state };
    for (const k of Object.keys(out)) {
      if (k === "hit" || k === "n") continue;
      next[k] = out[k];
    }
    state = next;
  }
}
process.stdout.write(JSON.stringify(out) + "\n");

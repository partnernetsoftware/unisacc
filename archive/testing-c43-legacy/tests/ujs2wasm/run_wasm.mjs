#!/usr/bin/env node
/** Read a ujs2wasm module, call main_export, print JSON value. */
import fs from "fs";

const path = process.argv[2];
if (!path) {
  console.error("usage: run_wasm.mjs file.wasm");
  process.exit(2);
}
const { instance } = await WebAssembly.instantiate(fs.readFileSync(path));
const ex = instance.exports;
for (const n of ["main_export", "tag_of_export", "i64_of_export"]) {
  if (typeof ex[n] !== "function") throw new Error("missing " + n);
}
const h = ex.main_export();
const t = ex.tag_of_export(h);
let v;
if (t === 2) {
  v = Number(ex.i64_of_export(h));
} else if (t === 3) {
  if (typeof ex.f64_of_export !== "function") throw new Error("missing f64_of_export");
  v = ex.f64_of_export(h);
} else if (t === 4) {
  const p = ex.str_ptr_export(h);
  const n = ex.str_len_export(h);
  const b = typeof ex.mem_base === "function" ? ex.mem_base() : 0;
  v = new TextDecoder().decode(new Uint8Array(ex.memory.buffer, b + p, n));
} else if (t === 1) {
  v = !!new Uint8Array(ex.memory.buffer)[h + 4];
} else if (t === 0) {
  v = null;
} else {
  throw new Error("unsupported tag " + t);
}
process.stdout.write(JSON.stringify(v) + "\n");

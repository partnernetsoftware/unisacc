#!/usr/bin/env node
/**
 * Run UJS compiler core → splice main body into RT template → write \\0asm.
 *
 *   node ujs/scripts/run-compiler-core.mjs core.wasm src.ujs [-o out.wasm] [--template rt.wasm]
 */
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import { rebuildWithMain } from "./rebuild-main.mjs";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const REPO = path.resolve(__dirname, "../..");

function usage() {
  console.error("usage: run-compiler-core.mjs <core.wasm> <src.ujs> [-o out.wasm] [--template rt.wasm]");
  process.exit(2);
}

const args = process.argv.slice(2);
if (args.length < 2) usage();
let out = null;
let templatePath = path.join(REPO, "ujs/core/compiler_rt_stub.wasm");
const pos = [];
for (let i = 0; i < args.length; i++) {
  if (args[i] === "-o" && args[i + 1]) out = args[++i];
  else if (args[i] === "--template" && args[i + 1]) templatePath = args[++i];
  else pos.push(args[i]);
}
if (pos.length !== 2) usage();

const corePath = path.resolve(pos[0]);
const srcPath = path.resolve(pos[1]);
const srcText = fs.readFileSync(srcPath, "utf8");
const chars = Array.from(srcText, (c) => c.charCodeAt(0));

const metaPath = corePath.replace(/\.wasm$/i, "") + ".meta.json";
const meta = JSON.parse(fs.readFileSync(metaPath, "utf8"));
const gnames = meta.globals || [];

const { instance } = await WebAssembly.instantiate(fs.readFileSync(corePath));
const ex = instance.exports;
ex.host_reset();

function mkList(arr) {
  const h = ex.host_mk_list(arr.length);
  for (let i = 0; i < arr.length; i++)
    ex.host_list_set(h, i, ex.host_mk_i64(BigInt(arr[i])));
  return h;
}

const six = gnames.indexOf("SRC");
if (six < 0) throw new Error("core meta missing SRC");
ex.host_set_global(six, mkList(chars));

const ret = ex.run_step();
if (ex.tag_of_export(ret) !== 5) throw new Error("core must return list");
const fullN = Number(ex.host_len(ret));
let outn = fullN;
const oix = gnames.indexOf("OUTN");
if (oix >= 0) {
  const h = ex.host_get_global(oix);
  if (h && ex.tag_of_export(h) === 2) outn = Number(ex.i64_of_export(h));
  else if (h && ex.tag_of_export(h) === 3) outn = Number(ex.f64_of_export(h));
}
if (outn <= 0 || outn > fullN) outn = fullN;

const body = new Uint8Array(outn);
for (let i = 0; i < outn; i++) {
  const h = ex.host_list_get(ret, i);
  const t = ex.tag_of_export(h);
  const v = t === 2 ? Number(ex.i64_of_export(h))
    : t === 3 ? Number(ex.f64_of_export(h)) : 0;
  body[i] = v & 255;
}

const template = fs.readFileSync(templatePath);
const wasm = rebuildWithMain(template, body);
if (wasm[0] !== 0 || wasm[1] !== 0x61 || wasm[2] !== 0x73 || wasm[3] !== 0x6d)
  throw new Error("splice lost \\0asm magic");

const outPath = out || path.join(path.dirname(corePath), "stage-out.wasm");
fs.writeFileSync(outPath, wasm);
process.stdout.write(JSON.stringify({
  wasm: outPath,
  bytes: wasm.length,
  mainBody: outn,
}) + "\n");

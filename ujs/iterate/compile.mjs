#!/usr/bin/env node
/**
 * M2/M3 host entry: UJS source → direct \\0asm (+ meta).  Paper B 出货脊。
 *
 * Default: compiler_core.wasm (compiler.ujs stage1) when present — NOT IntNet.
 * UJS_COMPILER=wasm|stage0 or UJS_REQUIRE_COMPILER_WASM=1 → C stage0.
 * Fallback: python3 -m ujs ujs2wasm --mode direct (dev only; ship gates forbid).
 *
 * Usage:
 *   node ujs/compile.mjs path/to/prog.ujs [-o out.wasm]
 */
import { spawnSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { compileWithCore } from "./run-compiler-core.mjs";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
/** ujs/ root whether this file lives in iterate/ or is reached via ujs/compile.mjs symlink */
function ujsRoot() {
  let d = __dirname;
  for (let i = 0; i < 4; i++) {
    if (fs.existsSync(path.join(d, "prd.md")) && fs.existsSync(path.join(d, "iterate")))
      return d;
    d = path.resolve(d, "..");
  }
  return path.resolve(__dirname, "..");
}
const UJS = ujsRoot();
const REPO = path.resolve(UJS, "..");

/** @typedef {{ locals: string[], globals: string[], slots: string[] }} CompileMeta */
/** @typedef {{ wasm: Uint8Array, meta: CompileMeta }} CompileResult */

function wantCore() {
  const v = (process.env.UJS_COMPILER || "").toLowerCase();
  if (v === "wasm" || v === "stage0" || v === "c" || v === "compiler.wasm")
    return false;
  if (v === "core" || v === "compiler.ujs" || v === "compiler_core")
    return true;
  // Default product path: stage1 core. Force stage0 for body≡ / rebuild core.
  if (process.env.UJS_REQUIRE_COMPILER_WASM === "1") return false;
  return !!findCompilerCore();
}

function coreForced() {
  const v = (process.env.UJS_COMPILER || "").toLowerCase();
  return v === "core" || v === "compiler.ujs" || v === "compiler_core";
}

function findCompilerWasm() {
  for (const rel of [
    "ujs/iterate/compiler.wasm",
    "ujs/core/compiler.wasm",
    "ujs/compiler.wasm",
    "ujs/uxe/ship/compiler.wasm",
  ]) {
    const p = path.join(REPO, rel);
    if (fs.existsSync(p)) return p;
  }
  return null;
}

function findCompilerCore() {
  for (const rel of [
    "ujs/iterate/compiler_core.wasm",
    "ujs/core/compiler_core.wasm",
    "ujs/compiler_core.wasm",
  ]) {
    const p = path.join(REPO, rel);
    if (fs.existsSync(p) && fs.existsSync(p.replace(/\.wasm$/i, "") + ".meta.json"))
      return p;
  }
  return null;
}

let _cwasm = null; // { exports, memory } | null | false

async function loadCompilerWasm() {
  if (_cwasm !== null) return _cwasm || null;
  const p = findCompilerWasm();
  if (!p) {
    _cwasm = false;
    return null;
  }
  const { instance } = await WebAssembly.instantiate(fs.readFileSync(p));
  const ex = instance.exports;
  for (const n of [
    "memory", "alloc", "compile",
    "out_ptr", "out_len", "meta_ptr", "meta_len", "err_ptr", "err_len",
  ]) {
    if (!(n in ex)) throw new Error("compiler.wasm missing export " + n);
  }
  _cwasm = { exports: ex, path: p };
  return _cwasm;
}

async function compileViaWasm(text) {
  const cw = await loadCompilerWasm();
  if (!cw) return null;
  const ex = cw.exports;
  const bytes = new TextEncoder().encode(text);
  const ptr = ex.alloc(bytes.length + 1);
  if (!ptr) throw new Error("compiler.wasm alloc failed");
  const mem = ex.memory;
  new Uint8Array(mem.buffer, ptr, bytes.length).set(bytes);
  const st = ex.compile(ptr, bytes.length);
  if (st !== 0) {
    const ep = ex.err_ptr() >>> 0;
    const el = ex.err_len() >>> 0;
    const msg = el
      ? new TextDecoder().decode(new Uint8Array(mem.buffer, ep, el))
      : "compile failed";
    throw new Error(msg);
  }
  const op = ex.out_ptr() >>> 0;
  const ol = ex.out_len() >>> 0;
  const wasm = new Uint8Array(mem.buffer, op, ol).slice();
  let meta = { locals: [], globals: [], slots: [] };
  const mp = ex.meta_ptr() >>> 0;
  const ml = ex.meta_len() >>> 0;
  if (ml > 0) {
    meta = JSON.parse(new TextDecoder().decode(new Uint8Array(mem.buffer, mp, ml)));
  }
  if (wasm.length < 4 || wasm[0] !== 0 || wasm[1] !== 0x61 ||
      wasm[2] !== 0x73 || wasm[3] !== 0x6d) {
    throw new Error("compiler.wasm did not emit \\0asm");
  }
  return { wasm, meta };
}

async function compileViaCore(text) {
  const corePath = findCompilerCore();
  if (!corePath) return null;
  const { wasm, meta } = await compileWithCore({ corePath, srcText: text });
  return {
    wasm,
    meta: {
      locals: meta.locals || [],
      globals: meta.globals || [],
      slots: [],
    },
  };
}

function compileViaPython(text) {
  const td = fs.mkdtempSync(path.join(process.env.TMPDIR || "/tmp", "ujs-compile-"));
  const inPath = path.join(td, "in.ujs");
  const outPath = path.join(td, "out.wasm");
  fs.writeFileSync(inPath, text);
  const r = spawnSync(
    "python3",
    ["-m", "ujs", "ujs2wasm", inPath, "-o", outPath, "--mode", "direct"],
    { cwd: REPO, encoding: "utf8", env: process.env },
  );
  if (r.status !== 0) {
    try { fs.rmSync(td, { recursive: true, force: true }); } catch (_) {}
    throw new Error((r.stderr || r.stdout || "compile failed").trim());
  }
  const wasm = new Uint8Array(fs.readFileSync(outPath));
  try { fs.rmSync(td, { recursive: true, force: true }); } catch (_) {}
  if (wasm.length < 4 || wasm[0] !== 0 || wasm[1] !== 0x61 ||
      wasm[2] !== 0x73 || wasm[3] !== 0x6d) {
    throw new Error("not a wasm module");
  }
  return { wasm, meta: { locals: [], globals: [], slots: [] } };
}

/**
 * Product contract.
 * @param {Uint8Array|string} src
 * @returns {Promise<CompileResult>}
 */
export async function compile(src) {
  const text = typeof src === "string" ? src : Buffer.from(src).toString("utf8");
  if (wantCore()) {
    const via = await compileViaCore(text);
    if (via) return via;
    if (coreForced())
      throw new Error("UJS_COMPILER=core but compiler_core.wasm missing");
  }
  const via = await compileViaWasm(text);
  if (via) return via;
  return compileViaPython(text);
}

export async function compileBackend() {
  if (wantCore()) {
    return findCompilerCore() ? "compiler_core.wasm" : "missing-core";
  }
  const p = findCompilerWasm();
  return p ? "compiler.wasm" : "python3";
}

function usage() {
  console.error("usage: node ujs/compile.mjs <file.ujs> [-o out.wasm]");
  process.exit(2);
}

async function main(argv) {
  const args = argv.slice(2);
  if (args.length < 1 || args[0] === "-h" || args[0] === "--help") usage();
  let out = null;
  const files = [];
  for (let i = 0; i < args.length; i++) {
    if (args[i] === "-o" && args[i + 1]) {
      out = args[++i];
    } else if (args[i].startsWith("-")) {
      usage();
    } else {
      files.push(args[i]);
    }
  }
  if (files.length !== 1) usage();
  const srcPath = path.resolve(files[0]);
  const backend = await compileBackend();
  if (process.env.UJS_REQUIRE_COMPILER_WASM === "1" && !wantCore()
      && backend !== "compiler.wasm") {
    throw new Error("UJS_REQUIRE_COMPILER_WASM=1 but compiler.wasm missing");
  }
  if (coreForced() && backend !== "compiler_core.wasm") {
    throw new Error("UJS_COMPILER=core but compiler_core.wasm missing");
  }
  const { wasm, meta } = await compile(fs.readFileSync(srcPath));
  const wasmPath = out || srcPath.replace(/\.ujs$/i, "") + ".wasm";
  fs.writeFileSync(wasmPath, wasm);
  const metaPath = wasmPath.replace(/\.wasm$/i, "") + ".meta.json";
  fs.writeFileSync(metaPath, JSON.stringify({
    globals: meta.globals || [],
    locals: meta.locals || [],
  }));
  console.log(JSON.stringify({
    wasm: wasmPath,
    meta: metaPath,
    bytes: wasm.length,
    bridge: backend,
    globals: meta.globals || [],
  }));
}

const isMain = process.argv[1] &&
  fs.realpathSync(path.resolve(process.argv[1])) ===
    fs.realpathSync(fileURLToPath(import.meta.url));
if (isMain) {
  main(process.argv).catch((e) => {
    console.error("compile:", e.message || e);
    process.exit(1);
  });
}

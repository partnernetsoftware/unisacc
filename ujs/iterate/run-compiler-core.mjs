#!/usr/bin/env node
/**
 * Run UJS compiler core → splice main body into RT template → write \\0asm (+ meta).
 *
 *   node ujs/scripts/run-compiler-core.mjs core.wasm src.ujs [-o out.wasm] [--template rt.wasm]
 *
 * v12: reads GF/GO/GL/GN/MG from core after run_step → out.meta.json globals.
 */
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import { rebuildWithMain } from "./rebuild-main.mjs";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const REPO = path.resolve(__dirname, "../..");

function numGlobal(ex, gnames, name) {
  const ix = gnames.indexOf(name);
  if (ix < 0) return null;
  const h = ex.host_get_global(ix);
  if (!h) return null;
  const t = ex.tag_of_export(h);
  if (t === 2) return Number(ex.i64_of_export(h));
  if (t === 3) return Number(ex.f64_of_export(h));
  return null;
}

/** Extract program globals from name table after body in returned OUT list (v12). */
export function readCoreProgramGlobals(ex, gnames, ret, bodyLen) {
  const mg = numGlobal(ex, gnames, "MG");
  if (mg == null || mg < 0) return [];
  const fullN = Number(ex.host_len(ret));
  let p = bodyLen;
  const out = [];
  for (let i = 0; i < mg; i++) {
    if (p >= fullN) break;
    const lenH = ex.host_list_get(ret, p++);
    const len = Number(ex.i64_of_export(lenH));
    let name = "";
    for (let j = 0; j < len; j++) {
      if (p >= fullN) break;
      const c = ex.host_list_get(ret, p++);
      name += String.fromCharCode(Number(ex.i64_of_export(c)));
    }
    out.push(name);
  }
  return out;
}

/**
 * @param {{ corePath: string, srcText: string, templatePath?: string }} opts
 * @returns {Promise<{ wasm: Uint8Array, meta: { globals: string[], locals: string[] }, mainBody: number }>}
 */
export async function compileWithCore(opts) {
  const corePath = opts.corePath;
  const templatePath = opts.templatePath
    || (fs.existsSync(path.join(REPO, "ujs/iterate/compiler_rt_stub.wasm"))
        ? path.join(REPO, "ujs/iterate/compiler_rt_stub.wasm")
        : path.join(REPO, "ujs/core/compiler_rt_stub.wasm"));
  const coreMetaPath = corePath.replace(/\.wasm$/i, "") + ".meta.json";
  const gnames = JSON.parse(fs.readFileSync(coreMetaPath, "utf8")).globals || [];
  const chars = Array.from(opts.srcText, (c) => c.charCodeAt(0));

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
  // error sentinel: OUT[0]==255
  if (outn >= 1) {
    const h0 = ex.host_list_get(ret, 0);
    const t0 = ex.tag_of_export(h0);
    const v0 = t0 === 2 ? Number(ex.i64_of_export(h0))
      : t0 === 3 ? Number(ex.f64_of_export(h0)) : 0;
    if ((v0 & 255) === 255 && outn <= 16)
      throw new Error("compiler.ujs core compile failed");
  }

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

  const globals = readCoreProgramGlobals(ex, gnames, ret, outn);
  return {
    wasm,
    meta: { globals, locals: [] },
    mainBody: outn,
  };
}

function usage() {
  console.error("usage: run-compiler-core.mjs <core.wasm> <src.ujs> [-o out.wasm] [--template rt.wasm]");
  process.exit(2);
}

const isMain = process.argv[1] &&
  fs.realpathSync(process.argv[1]) === fs.realpathSync(fileURLToPath(import.meta.url));
if (isMain) {
  const args = process.argv.slice(2);
  if (args.length < 2) usage();
  let out = null;
  let templatePath = path.join(REPO, "ujs/iterate/compiler_rt_stub.wasm");
  if (!fs.existsSync(templatePath))
    templatePath = path.join(REPO, "ujs/core/compiler_rt_stub.wasm");
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
  const { wasm, meta, mainBody } = await compileWithCore({
    corePath, srcText, templatePath,
  });
  const outPath = out || path.join(path.dirname(corePath), "stage-out.wasm");
  fs.writeFileSync(outPath, wasm);
  const outMeta = outPath.replace(/\.wasm$/i, "") + ".meta.json";
  fs.writeFileSync(outMeta, JSON.stringify({
    globals: meta.globals || [],
    locals: meta.locals || [],
  }));
  process.stdout.write(JSON.stringify({
    wasm: outPath,
    meta: outMeta,
    bytes: wasm.length,
    mainBody,
    globals: meta.globals || [],
  }) + "\n");
}

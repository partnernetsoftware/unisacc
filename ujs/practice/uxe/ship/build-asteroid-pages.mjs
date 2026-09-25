#!/usr/bin/env node
/**
 * Build Asteroid Pages ship-js (no C game wasm / no eng_* bridge).
 * Path B: compile.mjs UJS_COMPILER=core (compiler.ujs) → sim.wasm + meta; no python3.
 */
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import { spawnSync } from "child_process";

const here = path.dirname(fileURLToPath(import.meta.url));
const ujs = path.resolve(here, "../..");
const root = path.resolve(ujs, "..");
const outLocal = here; // ship/ itself is asteroid local face
const outPages = path.join(root, "docs/uxe/asteroid");
const stamp = process.env.UXE_SHIP_STAMP || Date.now().toString(36);
const homeLocal = process.env.UXE_SHIP_HOME || "../";
const homePages = "../../";

const simUjs = path.join(ujs, "web/game/sim.ujs");
const simWasm = path.join(here, "sim.wasm");
const simMeta = path.join(here, "sim.meta.json");

// Path B: compile.mjs → compiler_core.wasm (ignore ambient UJS_REQUIRE_*)
const env = { ...process.env, UJS_COMPILER: "core" };
delete env.UJS_REQUIRE_COMPILER_WASM;
const u2w = spawnSync(
  "node",
  [path.join(ujs, "compile.mjs"), simUjs, "-o", simWasm],
  {
    stdio: ["ignore", "pipe", "inherit"],
    cwd: root,
    env,
    encoding: "utf8",
  },
);
if (u2w.status !== 0) {
  console.error(u2w.stdout || "");
  process.exit(u2w.status || 1);
}
let emitInfo;
try {
  emitInfo = JSON.parse((u2w.stdout || "").trim().split("\n").pop());
} catch (_) {
  console.error("compile.mjs did not print JSON:", u2w.stdout);
  process.exit(1);
}
if (emitInfo.bridge !== "compiler_core.wasm") {
  console.error("ship emit must use compiler_core.wasm, got", emitInfo.bridge);
  process.exit(1);
}
// compile.mjs writes <stem>.meta.json next to -o; normalize name for host-entry import
const emittedMeta = simWasm.replace(/\.wasm$/i, "") + ".meta.json";
if (emittedMeta !== simMeta && fs.existsSync(emittedMeta)) {
  fs.copyFileSync(emittedMeta, simMeta);
}
if (!fs.existsSync(simMeta)) {
  console.error("sim.meta.json missing after compile");
  process.exit(1);
}
const magic = fs.readFileSync(simWasm).subarray(0, 4);
if (magic[0] !== 0 || magic[1] !== 0x61 || magic[2] !== 0x73 || magic[3] !== 0x6d) {
  console.error("sim.wasm missing or not \\0asm");
  process.exit(1);
}
console.log(
  "sim.wasm",
  emitInfo.bytes,
  "B · bridge",
  emitInfo.bridge,
  "· globals",
  (emitInfo.globals || []).join(","),
);

fs.mkdirSync(outPages, { recursive: true });

const engBuild = spawnSync("node", [path.join(here, "build-engine-js.mjs")], {
  stdio: "inherit", cwd: ujs,
});
if (engBuild.status !== 0) process.exit(engBuild.status || 1);

const gameOut = path.join(outLocal, "game.js");
const esbuild = spawnSync("npx", [
  "--yes", "esbuild@0.23.1",
  path.join(here, "asteroid-host-entry.js"),
  "--bundle", "--format=esm", "--platform=browser", "--target=es2022",
  "--loader:.json=json",
  "--define:__UXE_SHIP__=true",
  "--external:./engine.js",
  "--external:../core/compiler.js",
  "--external:../core/compiler.gen.js",
  "--external:./compiler.js",
  "--external:./compiler.gen.js",
  "--external:../compiler.js",
  "--external:fs",
  "--external:path",
  "--external:url",
  `--outfile=${gameOut}`,
], { stdio: "inherit", cwd: ujs });
if (esbuild.status !== 0) process.exit(esbuild.status || 1);

const gameJs = fs.readFileSync(gameOut, "utf8");
if (/compiler\.gen\.js/.test(gameJs)) {
  console.error("asteroid game.js glued compiler.gen — abort");
  process.exit(1);
}
if (/createWebGLRenderer|createWebGPURenderer/.test(gameJs)) {
  console.error("asteroid game.js pulled in Host/GPU — abort");
  process.exit(1);
}
if (/eng_sim_step|eng_boot/.test(gameJs)) {
  console.error("asteroid game.js still has eng_* glue — abort");
  process.exit(1);
}
if (!gameJs.includes("./engine.js") && !gameJs.includes("'./engine.js'")) {
  console.error("asteroid game.js missing ./engine.js import — abort");
  process.exit(1);
}
if (!/bootDirectStep|host_set_global|run_step/.test(gameJs)) {
  console.error("asteroid game.js missing path-B direct_step surface — abort");
  process.exit(1);
}

function bake(homeHref) {
  return `<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <meta http-equiv="Cache-Control" content="no-cache" />
  <title>UXE · Asteroid</title>
  <style>
    :root { color-scheme: dark; }
    html, body { margin: 0; height: 100%; background: #05060a; font-family: ui-sans-serif, system-ui, sans-serif; overflow: hidden; touch-action: none; }
    #c { display: block; width: 100%; height: 100%; touch-action: none; }
    #hud {
      position: fixed; left: 16px; top: 16px; z-index: 2;
      color: #c8d0e0; font-size: 13px; line-height: 1.55;
      text-shadow: 0 1px 2px #000; pointer-events: none;
      background: rgba(8,10,18,.55); padding: 10px 12px; border-radius: 8px;
      border: 1px solid rgba(120,140,180,.25); min-width: 240px;
    }
    #hud b { color: #9fd3ff; }
    #hud .warn { color: #ff8b8b; }
    #banner {
      position: fixed; inset: 0; display: none; align-items: center; justify-content: center;
      z-index: 3; background: rgba(0,0,0,.45); color: #fff; text-align: center;
    }
    #banner.show { display: flex; }
    a.nav {
      position: fixed; z-index: 2; color: #9fb0c8; font-size: 13px; text-decoration: none;
      background: rgba(8,10,18,.55); padding: 8px 10px; border-radius: 8px;
      border: 1px solid rgba(120,140,180,.25);
    }
  </style>
</head>
<body>
  <a class="nav" href="${homeHref}" style="right:16px;top:16px">← 游戏索引</a>
  <div id="hud">ship-js · path B · loading…</div>
  <div id="banner"><div>
    <h1 style="margin:0 0 8px;font-size:28px">撞毁</h1>
    <p style="margin:0;opacity:.85">双击或空格重开 · <span id="final">0</span></p>
  </div></div>
  <canvas id="c"></canvas>
  <script type="module">
import { startShip } from "./game.js?v=${stamp}";
const q = new URLSearchParams(location.search).get("gpu");
const prefer = q === "webgl" || q === "webgpu" ? q : "auto";
const hud = document.getElementById("hud");
try {
  await startShip({
    canvas: document.getElementById("c"),
    hud,
    banner: document.getElementById("banner"),
    finalEl: document.getElementById("final"),
    prefer,
    engineUrl: new URL("./?v=${stamp}", location.href).href,
    simUrl: new URL("./sim.wasm?v=${stamp}", location.href).href,
  });
} catch (e) {
  hud.innerHTML = '<span class="warn">boot failed</span><br>' + String(e.message || e);
  window.__UXE__ = { ready: false, error: String(e.message || e), ship: true, asteroid: true };
  console.error(e);
}
  </script>
</body>
</html>
`;
}

fs.writeFileSync(path.join(outLocal, "index.html"), bake(homeLocal));
// Path B: no ujs_full → engine.wasm (web-build not required)

fs.writeFileSync(path.join(outPages, "index.html"), bake(homePages));
fs.copyFileSync(simWasm, path.join(outPages, "sim.wasm"));
fs.copyFileSync(simMeta, path.join(outPages, "sim.meta.json"));
let pagesGame = fs.readFileSync(gameOut, "utf8")
  .replaceAll('from "./engine.js"', 'from "../engine.js"')
  .replaceAll("from './engine.js'", "from '../engine.js'");
fs.writeFileSync(path.join(outPages, "game.js"), pagesGame);

// drop legacy C / path-A embed / A-core engine.wasm
for (const p of [
  path.join(outLocal, "asteroid.wasm"),
  path.join(outPages, "asteroid.wasm"),
  path.join(outLocal, "asteroid.embed.json"),
  path.join(outLocal, "engine.wasm"),
  path.join(outPages, "engine.wasm"),
]) {
  try { fs.unlinkSync(p); } catch { /* */ }
}

console.log("asteroid ship-js ok (path B directSim):");
console.log("  local ", outLocal);
console.log("  pages ", outPages);

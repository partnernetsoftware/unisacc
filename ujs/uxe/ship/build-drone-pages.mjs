#!/usr/bin/env node
/**
 * Build Drone Pages ship-js.
 * Path B default: compile.mjs (compiler.wasm) → sim.wasm + meta; no python3 emit.
 * Shared Host/GPU: ../engine.js (and docs/uxe/engine.js).
 */
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import { spawnSync } from "child_process";

const here = path.dirname(fileURLToPath(import.meta.url));
const ujs = path.resolve(here, "../..");
const root = path.resolve(ujs, "..");
const outLocal = path.join(here, "drone");
const outPages = path.join(root, "docs/uxe/drone");
const stamp = process.env.UXE_SHIP_STAMP || Date.now().toString(36);

const simUjs = path.join(ujs, "web/game/drone.ujs");
const simWasm = path.join(outLocal, "sim.wasm");
const simMeta = path.join(outLocal, "sim.meta.json");

for (const dir of [outLocal, outPages]) {
  fs.mkdirSync(dir, { recursive: true });
}

// Path B: compiler.wasm → sim.wasm + meta (globals order = host_set_global indices)
const u2w = spawnSync(
  "node",
  [path.join(ujs, "compile.mjs"), simUjs, "-o", simWasm],
  {
    stdio: ["ignore", "pipe", "inherit"],
    cwd: root,
    env: { ...process.env, UJS_REQUIRE_COMPILER_WASM: "1" },
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
if (emitInfo.bridge !== "compiler.wasm") {
  console.error("ship emit must use compiler.wasm, got", emitInfo.bridge);
  process.exit(1);
}
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

// Ensure shared engine.js exists (Host + GPU)
const engBuild = spawnSync("node", [path.join(here, "build-engine-js.mjs")], {
  stdio: "inherit", cwd: ujs,
});
if (engBuild.status !== 0) process.exit(engBuild.status || 1);

const gameOut = path.join(outLocal, "game.js");
const esbuild = spawnSync("npx", [
  "--yes", "esbuild@0.23.1",
  path.join(here, "drone-host-entry.js"),
  "--bundle", "--format=esm", "--platform=browser", "--target=es2022",
  "--loader:.json=json",
  "--define:__UXE_SHIP__=true",
  "--external:../engine.js",
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
  console.error("drone game.js glued compiler.gen — abort");
  process.exit(1);
}
if (/createWebGLRenderer|createWebGPURenderer/.test(gameJs)) {
  console.error("drone game.js pulled in Host/GPU — abort");
  process.exit(1);
}
if (!gameJs.includes("../engine.js")) {
  console.error("drone game.js missing ../engine.js import — abort");
  process.exit(1);
}
if (!/bootDirectStep|host_set_global|run_step/.test(gameJs)) {
  console.error("drone game.js missing path-B direct_step surface — abort");
  process.exit(1);
}

function bake(homeHref) {
  return `<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover" />
  <meta http-equiv="Cache-Control" content="no-cache" />
  <title>UXE · 无人机驾舱</title>
  <style>
    :root { color-scheme: dark; }
    html, body {
      margin: 0; height: 100%; overflow: hidden; touch-action: none;
      background: #121820;
      font-family: "IBM Plex Sans", "Segoe UI", ui-sans-serif, system-ui, sans-serif;
    }
    #c { display: block; width: 100%; height: 100%; touch-action: none; }
    #vignette {
      position: fixed; inset: 0; z-index: 1; pointer-events: none;
      background:
        radial-gradient(ellipse at center, transparent 42%, rgba(0,0,0,.55) 100%),
        linear-gradient(180deg, rgba(0,8,12,.35) 0%, transparent 18%, transparent 82%, rgba(0,0,0,.45) 100%);
    }
    #hud {
      position: fixed; left: 16px; top: 16px; z-index: 2;
      color: #d5e8f2; font-size: 13px; line-height: 1.55;
      background: rgba(4,10,16,.78); padding: 12px 14px; border-radius: 4px;
      border: 1px solid rgba(100,200,140,.4); min-width: min(300px, calc(100vw - 32px));
      pointer-events: none; font-variant-numeric: tabular-nums;
      box-shadow: 0 0 24px rgba(40,120,80,.15);
    }
    #hud b { color: #9fe7b0; }
    #hud .warn { color: #ff8a7a; }
    #hud .lock { color: #5dff8a; }
    #hud .mode { color: #f0d878; }
    #hud.armed { border-color: rgba(255,90,70,.55); }
    #hud.locked-on { border-color: rgba(80,255,140,.65); }
    #reticle {
      position: fixed; left: 50%; top: 50%; width: 28px; height: 28px;
      margin: -14px 0 0 -14px; pointer-events: none; z-index: 2;
      border: 2px solid rgba(180,255,160,.7); border-radius: 50%;
    }
    #reticle.lock { border-color: #5dff8a; transform: scale(1.15); }
    #reticle.suicide { border-color: #ff6a55; }
    #ammo {
      position: fixed; left: 50%; bottom: 72px; transform: translateX(-50%);
      z-index: 2; pointer-events: none; letter-spacing: 6px; font-size: 18px;
      color: #f0d878;
    }
    a.nav {
      position: fixed; z-index: 2; right: 16px; top: 16px;
      color: #c8d8e0; font-size: 13px; text-decoration: none;
      background: rgba(6,12,18,.72); padding: 8px 10px; border-radius: 4px;
      border: 1px solid rgba(120,180,140,.35);
    }
    #controls {
      position: fixed; z-index: 3; right: 16px; bottom: 16px;
      display: flex; gap: 6px; background: rgba(6,12,18,.78);
      padding: 8px; border-radius: 8px;
      border: 1px solid rgba(120,180,140,.35);
    }
    #controls button {
      appearance: none; border: 1px solid rgba(140,180,160,.4);
      background: rgba(20,28,34,.9); color: #c8d8e0;
      font-size: 12px; padding: 7px 10px; border-radius: 6px; cursor: pointer;
    }
    #controls button.on {
      border-color: rgba(120,220,150,.75); color: #9fe7b0;
      background: rgba(30,50,40,.95);
    }
  </style>
</head>
<body>
  <a class="nav" href="${homeHref}">← 游戏索引</a>
  <div id="vignette"></div>
  <div id="reticle"></div>
  <div id="ammo"></div>
  <div id="hud">ship-js · path B · 驾舱启动…</div>
  <div id="controls">
    <button type="button" id="ctrl-kb" class="on">纯键盘</button>
    <button type="button" id="ctrl-ms">键盘+鼠标</button>
  </div>
  <canvas id="c"></canvas>
  <script type="module">
import { startDroneShip } from "./game.js?v=${stamp}";
const params = new URLSearchParams(location.search);
const q = params.get("gpu");
const prefer = q === "webgl" || q === "webgpu" ? q : "auto";
const initial = params.get("controls") === "mouse" ? "mouse" : "keyboard";
const hud = document.getElementById("hud");
const btnKb = document.getElementById("ctrl-kb");
const btnMs = document.getElementById("ctrl-ms");
function paintCtrl(c) {
  btnKb.classList.toggle("on", c === "keyboard");
  btnMs.classList.toggle("on", c === "mouse");
  document.getElementById("c").style.cursor = c === "mouse" ? "none" : "default";
}
paintCtrl(initial);
try {
  const api = await startDroneShip({
    canvas: document.getElementById("c"),
    hud,
    ammoEl: document.getElementById("ammo"),
    reticle: document.getElementById("reticle"),
    prefer,
    controls: initial,
    engineUrl: new URL("./engine.wasm?v=${stamp}", location.href).href,
    simUrl: new URL("./sim.wasm?v=${stamp}", location.href).href,
    onControls: paintCtrl,
  });
  window.__UXE_API__ = api;
  btnKb.onclick = () => { api.setControls("keyboard"); paintCtrl("keyboard"); };
  btnMs.onclick = () => { api.setControls("mouse"); paintCtrl("mouse"); };
} catch (e) {
  hud.innerHTML = '<span class="warn">boot failed</span><br>' + String(e.message || e);
  window.__UXE__ = { ready: false, error: String(e.message || e), drone: true, ship: true };
  console.error(e);
}
  </script>
</body>
</html>
`;
}

fs.writeFileSync(path.join(outLocal, "index.html"), bake("../../"));
fs.copyFileSync(path.join(ujs, "core/ujs_full.wasm"), path.join(outLocal, "engine.wasm"));

fs.writeFileSync(path.join(outPages, "index.html"), bake("../../"));
fs.copyFileSync(path.join(outLocal, "engine.wasm"), path.join(outPages, "engine.wasm"));
fs.copyFileSync(simWasm, path.join(outPages, "sim.wasm"));
fs.copyFileSync(simMeta, path.join(outPages, "sim.meta.json"));
fs.copyFileSync(gameOut, path.join(outPages, "game.js"));

// drop legacy path-A embed / fat host
for (const p of [
  path.join(here, "drone.embed.json"),
  path.join(outLocal, "uxe-host.js"),
  path.join(outPages, "uxe-host.js"),
]) {
  try { fs.unlinkSync(p); } catch { /* */ }
}

console.log("drone ship-js ok (path B directSim):");
console.log("  local ", outLocal);
console.log("  pages ", outPages);

#!/usr/bin/env node
/**
 * Build Drone Pages ship (JS core + precompiled sim + engine.wasm).
 * Output: docs/uxe/drone/  and  web/engine/ship/drone/
 */
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import { spawnSync } from "child_process";
import { compile } from "../../compiler.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const web = path.resolve(here, "../..");
const root = path.resolve(web, "../..");
const outLocal = path.join(here, "drone");
const outPages = path.join(root, "docs/uxe/drone");

const simSrc = fs.readFileSync(path.join(web, "game/drone.ujs"), "utf8");
const { image, blob } = compile(simSrc);
const embedPath = path.join(here, "drone.embed.json");
fs.writeFileSync(embedPath, JSON.stringify({
  image: [...image],
  blob: { globals: blob.globals || [], locals: blob.locals || [] },
}));
console.log("embed", image.length, "B · globals", (blob.globals || []).join(","));

for (const dir of [outLocal, outPages]) {
  fs.mkdirSync(dir, { recursive: true });
}

const esbuild = spawnSync("npx", [
  "--yes", "esbuild@0.23.1",
  path.join(here, "drone-host-entry.js"),
  "--bundle", "--format=esm", "--platform=browser", "--target=es2022",
  "--loader:.json=json",
  "--define:__UXE_SHIP__=true",
  "--external:fs",
  "--external:path",
  "--external:url",
  "--external:./compiler.js",
  "--external:./compiler.gen.js",
  "--external:../compiler.js",
  `--outfile=${path.join(outLocal, "uxe-host.js")}`,
], { stdio: "inherit", cwd: path.join(web, "..") });
if (esbuild.status !== 0) process.exit(esbuild.status || 1);

const hostRaw = fs.readFileSync(path.join(outLocal, "uxe-host.js"), "utf8");
if (/compiler\.gen\.js/.test(hostRaw)) {
  console.error("drone ship glued compiler.gen — abort");
  process.exit(1);
}
const hostJs = hostRaw.replace(/<\/script/gi, "<\\/script");

function bake(homeHref) {
  return `<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>UXE · 无人机驾舱</title>
  <style>
    :root { color-scheme: dark; }
    html, body { margin: 0; height: 100%; background: #1a2228; font-family: ui-sans-serif, system-ui, sans-serif; overflow: hidden; }
    #c { display: block; width: 100%; height: 100%; }
    #hud {
      position: fixed; left: 16px; top: 16px; z-index: 2;
      color: #d8e6f0; font-size: 13px; line-height: 1.55;
      text-shadow: 0 1px 2px #000; pointer-events: none;
      background: rgba(6,12,18,.72); padding: 10px 12px; border-radius: 8px;
      border: 1px solid rgba(120,180,140,.35); min-width: 280px;
      font-variant-numeric: tabular-nums;
    }
    #hud b { color: #9fe7b0; }
    #hud .warn { color: #ff8a7a; }
    #hud .lock { color: #5dff8a; }
    #hud .mode { color: #f0d878; }
    #reticle {
      position: fixed; left: 50%; top: 50%; width: 22px; height: 22px;
      margin: -11px 0 0 -11px; pointer-events: none; z-index: 2;
      border: 2px solid rgba(180,255,160,.75); border-radius: 50%;
      box-shadow: 0 0 0 1px rgba(0,0,0,.5), inset 0 0 8px rgba(80,255,120,.15);
    }
    #reticle::before, #reticle::after {
      content: ""; position: absolute; background: rgba(180,255,160,.75);
    }
    #reticle::before { left: 50%; top: -6px; width: 2px; height: 6px; margin-left: -1px; }
    #reticle::after { left: 50%; bottom: -6px; width: 2px; height: 6px; margin-left: -1px; }
    a.nav {
      position: fixed; z-index: 2; right: 16px; top: 16px;
      color: #c8d8e0; font-size: 13px; text-decoration: none;
      background: rgba(6,12,18,.72); padding: 8px 10px; border-radius: 8px;
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
  <div id="reticle"></div>
  <div id="hud">驾舱启动…</div>
  <div id="controls">
    <button type="button" id="ctrl-kb" class="on">纯键盘</button>
    <button type="button" id="ctrl-ms">键盘+鼠标</button>
  </div>
  <canvas id="c"></canvas>
  <script type="module">
${hostJs}

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
    prefer,
    controls: initial,
    engineUrl: new URL("./engine.wasm", location.href).href,
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
fs.copyFileSync(path.join(web, "ujs_full.wasm"), path.join(outLocal, "engine.wasm"));

fs.writeFileSync(path.join(outPages, "index.html"), bake("../../"));
fs.copyFileSync(path.join(outLocal, "engine.wasm"), path.join(outPages, "engine.wasm"));

console.log("drone ship-js ok:");
console.log("  local ", outLocal);
console.log("  pages ", outPages);

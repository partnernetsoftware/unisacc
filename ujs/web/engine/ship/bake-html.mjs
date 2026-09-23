#!/usr/bin/env node
/** Bake ship/index.html: tiny glue + asteroid.wasm + engine.wasm */
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const ship = path.dirname(fileURLToPath(import.meta.url));
const host = fs.readFileSync(path.join(ship, "uxe-host.js"), "utf8")
  .replace(/<\/script/gi, "<\\/script");

const home = process.env.UXE_SHIP_HOME || "../";
const stamp = process.env.UXE_SHIP_STAMP || Date.now().toString(36);
const html = `<!DOCTYPE html>
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
  <a class="nav" href="${home}" style="right:16px;top:16px">← 游戏索引</a>
  <div id="hud">ship · loading wasms…</div>
  <div id="banner"><div>
    <h1 style="margin:0 0 8px;font-size:28px">撞毁</h1>
    <p style="margin:0;opacity:.85">双击或空格重开 · <span id="final">0</span></p>
  </div></div>
  <canvas id="c"></canvas>
  <script type="module">
/* tiny Host glue — wires asteroid.wasm ↔ engine.wasm + GPU */
${host}

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
    gameUrl: new URL("./asteroid.wasm?v=${stamp}", location.href).href,
    engineUrl: new URL("./engine.wasm?v=${stamp}", location.href).href,
  });
} catch (e) {
  hud.innerHTML = '<span class="warn">boot failed</span><br>' + String(e.message || e);
  window.__UXE__ = { ready: false, error: String(e.message || e), ship: true };
  console.error(e);
}
  </script>
</body>
</html>
`;

fs.writeFileSync(path.join(ship, "index.html"), html);
console.log("baked", path.join(ship, "index.html"), "home=", home);

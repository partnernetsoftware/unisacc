#!/usr/bin/env node
/**
 * ARCHIVED — builds monopoly ship locally; not part of ship:pages.
 * See web/uxe/archive/README.md. Output may write docs/uxe/monopoly/
 * which ship-engine.sh deletes on the next asteroid ship.
 */
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import { spawnSync } from "child_process";
import { compile } from "../../compiler.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const web = path.resolve(here, "../..");
const root = path.resolve(web, "../..");
const outLocal = path.join(here, "monopoly");
const outPages = path.join(root, "docs/uxe/monopoly");

const simSrc = fs.readFileSync(path.join(web, "game/monopoly.ujs"), "utf8");
const { image, blob } = compile(simSrc);
const embedPath = path.join(here, "monopoly.embed.json");
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
  path.join(here, "monopoly-host-entry.js"),
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
  console.error("monopoly ship glued compiler.gen — abort");
  process.exit(1);
}
const hostJs = hostRaw.replace(/<\/script/gi, "<\\/script");

function bake(homeHref) {
  return `<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>UXE · 城市大富翁</title>
  <style>
    :root { color-scheme: dark; }
    html, body { margin: 0; height: 100%; background: #05060a; font-family: ui-sans-serif, system-ui, sans-serif; overflow: hidden; }
    #c { display: block; width: 100%; height: 100%; }
    #hud {
      position: fixed; left: 16px; top: 16px; z-index: 2;
      color: #c8d0e0; font-size: 13px; line-height: 1.55;
      text-shadow: 0 1px 2px #000; pointer-events: none;
      background: rgba(8,10,18,.55); padding: 10px 12px; border-radius: 8px;
      border: 1px solid rgba(120,140,180,.25); min-width: 260px;
    }
    #hud b { color: #9fd3ff; }
    #hud .warn { color: #ff8b8b; }
    a.nav {
      position: fixed; z-index: 2; right: 16px; top: 16px;
      color: #9fb0c8; font-size: 13px; text-decoration: none;
      background: rgba(8,10,18,.55); padding: 8px 10px; border-radius: 8px;
      border: 1px solid rgba(120,140,180,.25);
    }
  </style>
</head>
<body>
  <a class="nav" href="${homeHref}">← 游戏索引</a>
  <div id="hud">monopoly · loading…</div>
  <canvas id="c"></canvas>
  <script type="module">
${hostJs}

const q = new URLSearchParams(location.search).get("gpu");
const prefer = q === "webgl" || q === "webgpu" ? q : "auto";
const hud = document.getElementById("hud");
try {
  await startMonopolyShip({
    canvas: document.getElementById("c"),
    hud,
    prefer,
    engineUrl: new URL("./engine.wasm", location.href).href,
  });
} catch (e) {
  hud.innerHTML = '<span class="warn">boot failed</span><br>' + String(e.message || e);
  window.__UXE__ = { ready: false, error: String(e.message || e), monopoly: true, ship: true };
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

console.log("monopoly ship-js ok:");
console.log("  local ", outLocal);
console.log("  pages ", outPages);

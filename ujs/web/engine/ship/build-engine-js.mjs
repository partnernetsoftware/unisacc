#!/usr/bin/env node
/**
 * Build shared ship/engine.js (Host + GPU) for all UXE games.
 * Output: web/engine/ship/engine.js  and  docs/uxe/engine.js
 */
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import { spawnSync } from "child_process";

const ship = path.dirname(fileURLToPath(import.meta.url));
const web = path.resolve(ship, "../..");
const root = path.resolve(web, "../..");
const outShip = path.join(ship, "engine.js");
const outPages = path.join(root, "docs/uxe/engine.js");

const r = spawnSync("npx", [
  "--yes", "esbuild@0.23.1",
  path.join(ship, "engine-entry.js"),
  "--bundle", "--format=esm", "--platform=browser", "--target=es2022",
  `--outfile=${outShip}`,
], { stdio: "inherit", cwd: path.join(web, "..") });
if (r.status !== 0) process.exit(r.status || 1);

const js = fs.readFileSync(outShip, "utf8");
if (/compiler\.gen|bootRuntime|wasm_run/.test(js)) {
  console.error("engine.js pulled in compiler/runtime — abort");
  process.exit(1);
}

fs.mkdirSync(path.dirname(outPages), { recursive: true });
fs.copyFileSync(outShip, outPages);
console.log("engine.js", js.length, "B →", outPages);

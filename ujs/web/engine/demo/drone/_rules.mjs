/** Node rules for FP drone.ujs */
import { bootRuntime, wasm_run, unwrap } from "../../../wasm_run.js";
import { compile } from "../../../compiler.js";
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const here = path.dirname(fileURLToPath(import.meta.url));
const web = path.resolve(here, "../../..");
const src = fs.readFileSync(path.join(web, "game/drone.ujs"), "utf8");
const { image, blob } = compile(src);
await bootRuntime(path.join(web, "ujs_full.wasm"));

function base() {
  return {
    px: 0, py: 14, pz: 20, score: 0, alive: 1, ammo: 2,
    ix: 0, iy: -1, dt: 0.05, suicide: 0, speed_mul: 1,
    fx: 0, fy: 0, fz: -1, rx: 1, ry: 0, rz: 0,
    txs: [0, 10], tys: [8, 8], tzs: [-15, -15], thp: [1, 1], thit: [0, 0],
  };
}

async function run(g) {
  const r = await wasm_run({ image, blob }, g, {});
  if (r.err) throw new Error(JSON.stringify(r.err));
  return unwrap(r);
}

let o = await run(base());
if (o.pz >= 20) throw new Error("thrust forward expected pz=" + o.pz);

o = await run({ ...base(), iy: 0, thit: [1, 0] });
if (o.thp[0] !== 0 || o.nk !== 1) throw new Error("missile kill");
if (o.score < 500) throw new Error("score");

o = await run({ ...base(), iy: 0, suicide: 1, thit: [1, 1] });
if (o.alive !== 0) throw new Error("suicide ends alive");
if (o.thp[0] !== 0 || o.thp[1] !== 0) throw new Error("suicide wipe");

// nose-up climb bleed while thrusting
o = await run({ ...base(), fy: 0.5, iy: -1 });
if (!(o.py > 14)) throw new Error("look-climb expected py=" + o.py);

console.log("OK_DRONE_RULES", {
  pz_move: true, missile: true, suicide: true, look_climb: true,
});

/** Node self-test for Asteroid Rush sim (no browser). Invoked with alarm wrapper. */
import { bootRuntime, compile, wasm_run, unwrap } from "../wasm_run.js";
import fs from "fs";

const N = 480;
const src = fs.readFileSync(new URL("./sim.ujs", import.meta.url), "utf8");
await bootRuntime(new URL("../ujs_full.wasm", import.meta.url));
const c = compile(src);

function fresh() {
  const xs = [], ys = [], zs = [], vxs = [], vys = [], vzs = [], rs = [];
  for (let i = 0; i < N; i++) {
    let sx = ((i * 13) % 37) - 18;
    let sy = ((i * 7) % 25) - 12;
    if (sx > -3.5 && sx < 3.5 && sy > -3.5 && sy < 3.5) sx += 7;
    xs.push(sx);
    ys.push(sy);
    zs.push(-40 - i * 1.55);
    vxs.push(((i % 5) - 2) * 0.55);
    vys.push(((i % 3) - 1) * 0.4);
    vzs.push(8 + (i % 11) * 0.35);
    rs.push(0.55 + (i % 5) * 0.22);
  }
  return { xs, ys, zs, vxs, vys, vzs, rs, px: 0, py: 0, pz: 0, score: 0, alive: 1 };
}

async function step(st, ix, iy, dt = 0.016) {
  const r = await wasm_run(
    { image: c.image, blob: c.blob },
    { ...st, ix, iy, dt, alive: st.alive ? 1 : 0 },
    {},
  );
  if (r.err) throw new Error(JSON.stringify(r.err));
  return unwrap(r);
}

let st = fresh();
const t0 = performance.now();
for (let k = 0; k < 120; k++) st = await step(st, 0, 0);
const avgFly = (performance.now() - t0) / 120;
console.log("fly120", {
  avg: +avgFly.toFixed(3),
  score: +st.score.toFixed(2),
  alive: st.alive,
  pz: +st.pz.toFixed(2),
});
if (!st.alive) throw new Error("unexpected crash while flying straight");
if (!(st.score > 1)) throw new Error("score not advancing");

st.xs[0] = st.px;
st.ys[0] = st.py;
st.zs[0] = st.pz;
st = await step(st, 0, 0);
console.log("forceHit", { alive: st.alive, hit: st.hit, score: st.score });
if (st.alive !== 0 && st.hit !== 1) throw new Error("collision missed");

const frozen = st.score;
st = await step({ ...st, alive: 0 }, 1, 1);
console.log("deadFreeze", { alive: st.alive, score: st.score });
if (st.alive !== 0) throw new Error("dead not sticky");
if (st.score !== frozen) throw new Error("dead should not change score");

st = fresh();
const t1 = performance.now();
let crashes = 0;
for (let k = 0; k < 600; k++) {
  st = await step(st, k % 5 === 0 ? 1 : 0, k % 7 === 0 ? -1 : 0);
  if (!st.alive) {
    crashes++;
    st = fresh();
  }
}
console.log("soak600", {
  avg: +((performance.now() - t1) / 600).toFixed(3),
  crashes,
  finalScore: +st.score.toFixed(1),
});
console.log("OK host-parity sim");

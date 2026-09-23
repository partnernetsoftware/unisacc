/** Node rules selftest for monopoly.ujs (no browser). */
import { bootRuntime, wasm_run, unwrap } from "../../../wasm_run.js";
import { compile } from "../../../compiler.js";
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const here = path.dirname(fileURLToPath(import.meta.url));
const web = path.resolve(here, "../../..");
const src = fs.readFileSync(path.join(web, "game/monopoly.ujs"), "utf8");
const { image, blob } = compile(src);

await bootRuntime(path.join(web, "ujs_full.wasm"));

function base() {
  const owned = Array(16).fill(-1);
  const houses = Array(16).fill(0);
  return {
    turn: 0, phase: 0, pos0: 0, pos1: 0,
    money0: 1500, money1: 1500, owned, houses,
    msg: 0, dice: 0, winner: -1,
  };
}

let st = base();
async function step(action, dice) {
  const r = await wasm_run({ image, blob }, { ...st, action, dice: dice || 0 }, {});
  if (r.err) throw new Error(JSON.stringify(r.err));
  st = { ...st, ...unwrap(r) };
  return st;
}

await step(4, 0);
if (st.money0 !== 1500 || st.pos0 !== 0) throw new Error("restart failed");

await step(1, 3); // land cell 3 → offer buy
if (st.pos0 !== 3) throw new Error("pos0 want 3 got " + st.pos0);
if (st.phase !== 1) throw new Error("want offer_buy phase=1 got " + st.phase);

const moneyBefore = st.money0;
await step(2, 0);
if (st.owned[3] !== 0) throw new Error("own cell 3");
if (st.money0 >= moneyBefore) throw new Error("money should drop after buy");
if (st.turn !== 1) throw new Error("turn should flip to AI");

// AI pays rent on owned cell 3
st = { ...st, turn: 1, phase: 0, pos1: 0 };
await step(1, 3);
if (st.money1 >= 1500) throw new Error("AI should pay rent, money1=" + st.money1);
if (st.turn !== 0) throw new Error("after rent turn back to human, got " + st.turn);

// skip buy
st = { ...base(), turn: 0, phase: 0 };
await step(4, 0);
await step(1, 2);
if (st.phase !== 1) throw new Error("offer expected");
await step(3, 0);
if (st.owned[2] !== -1) throw new Error("skip should not own");
if (st.turn !== 1) throw new Error("skip flips turn");

// own-land → build offer
st = { ...base(), turn: 0, phase: 0, pos0: 0, owned: Array(16).fill(-1) };
st.owned[5] = 0;
await step(4, 0);
st = { ...st, owned: st.owned.map((_, i) => (i === 5 ? 0 : -1)), turn: 0, phase: 0, pos0: 0, money0: 1500 };
await step(1, 5); // → cell 5 owned by self
if (st.phase !== 3) throw new Error("want build offer phase=3 got " + st.phase);
await step(5, 0);
if (st.houses[5] !== 1) throw new Error("house not built");
if (st.turn !== 1) throw new Error("build flips turn");

// GO bonus
st = { ...base(), turn: 0, phase: 0, pos0: 14, money0: 1000 };
await step(1, 5);
if (st.pos0 !== 3) throw new Error("wrap pos");
if (st.money0 < 1200) throw new Error("GO +200 expected, money=" + st.money0);

console.log("OK_MONOPOLY_RULES", {
  pos0: st.pos0, money0: st.money0, phase: st.phase, turn: st.turn, houses5: 1,
});

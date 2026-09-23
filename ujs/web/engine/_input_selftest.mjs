import {
  encodeInputSnapshot,
  decodeInputSnapshot,
  INPUT_MAGIC,
  INPUT_BYTES,
} from "./input.js";

const snap = { ix: -1, iy: 1, fire: 1 };
const buf = encodeInputSnapshot(snap);
const dv = new DataView(buf);
if (buf.byteLength !== INPUT_BYTES) throw new Error("size");
if (dv.getUint32(0, true) !== INPUT_MAGIC) throw new Error("magic");
if (dv.getInt32(8, true) !== -1) throw new Error("ix");
if (dv.getInt32(12, true) !== 1) throw new Error("iy");
if (dv.getUint32(16, true) !== 1) throw new Error("fire");

const out = decodeInputSnapshot(buf);
if (out.ix !== -1 || out.iy !== 1 || out.fire !== 1) throw new Error("roundtrip");

const bigger = new ArrayBuffer(INPUT_BYTES + 8);
encodeInputSnapshot({ ix: 0, iy: 0, fire: 0 }, bigger);
const out2 = decodeInputSnapshot(bigger);
if (out2.ix !== 0 || out2.fire !== 0) throw new Error("reserved");

console.log("OK_INPUT", { bytes: buf.byteLength, ix: out.ix, iy: out.iy, fire: out.fire });

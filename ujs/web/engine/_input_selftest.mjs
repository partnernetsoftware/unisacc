import {
  encodeInputSnapshot,
  decodeInputSnapshot,
  axesFromStick,
  INPUT_MAGIC,
  INPUT_BYTES,
  INPUT_BYTES_V1,
  INPUT_VERSION,
  FLAG_TOUCH,
} from "./input.js";

const snap = { ix: -1, iy: 1, fire: 1, mx: 0.5, my: -0.25, buttons: 1, flags: 1 | FLAG_TOUCH };
const buf = encodeInputSnapshot(snap);
const dv = new DataView(buf);
if (buf.byteLength !== INPUT_BYTES) throw new Error("size " + buf.byteLength);
if (dv.getUint32(0, true) !== INPUT_MAGIC) throw new Error("magic");
if (dv.getUint32(4, true) !== INPUT_VERSION) throw new Error("ver");
if (dv.getInt32(8, true) !== -1) throw new Error("ix");
if (dv.getInt32(12, true) !== 1) throw new Error("iy");
if (dv.getUint32(16, true) !== 1) throw new Error("fire");
if (Math.abs(dv.getFloat32(20, true) - 0.5) > 1e-6) throw new Error("mx");
if (Math.abs(dv.getFloat32(24, true) + 0.25) > 1e-6) throw new Error("my");
if (dv.getUint32(28, true) !== 1) throw new Error("buttons");
if (dv.getUint32(32, true) !== (1 | FLAG_TOUCH)) throw new Error("flags");

const out = decodeInputSnapshot(buf);
if (out.ix !== -1 || out.iy !== 1 || out.fire !== 1) throw new Error("roundtrip keys");
if (Math.abs(out.mx - 0.5) > 1e-6 || Math.abs(out.my + 0.25) > 1e-6) throw new Error("roundtrip ptr");
if (out.buttons !== 1 || out.flags !== (1 | FLAG_TOUCH) || out.version !== 2) throw new Error("roundtrip meta");

// v1 buffer still encodable / decodable
const v1buf = new ArrayBuffer(INPUT_BYTES_V1);
encodeInputSnapshot({ ix: 1, iy: 0, fire: 0 }, v1buf);
const v1 = decodeInputSnapshot(v1buf);
if (v1.version !== 1 || v1.ix !== 1 || v1.mx !== 0) throw new Error("v1 compat");

const right = axesFromStick(0.8, 0);
if (right.ix !== 1 || right.iy !== 0) throw new Error("stick right");
const up = axesFromStick(0, 0.8);
if (up.ix !== 0 || up.iy !== -1) throw new Error("stick up");
const dead = axesFromStick(0.05, -0.05);
if (dead.ix !== 0 || dead.iy !== 0) throw new Error("stick deadzone");
const soft = axesFromStick(0.25, 0);
if (soft.ix !== 0) throw new Error("stick soft dead 0.25");

console.log("OK_INPUT", {
  bytes: buf.byteLength, version: out.version,
  ix: out.ix, mx: out.mx, buttons: out.buttons, flags: out.flags,
  stick: { right, up },
});

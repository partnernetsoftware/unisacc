/**
 * UXE input snapshot binary layout (Host ABI host_input_read).
 *
 * Little-endian UXIN:
 *   v1 (20 B): magic | version=1 | ix | iy | fire
 *   v2 (36 B): v1 fields + mx | my | buttons | flags   ← current
 *
 * mx, my: canvas NDC ∈ [-1, 1], origin center, +x right, +y up
 * buttons: bit0 left · bit1 right · bit2 middle
 * flags: bit0 pointer inside · bit1 suicide · bit2 touch (vs mouse/pen)
 *
 * Browser Host: when primary button is held and keyboard axes are idle,
 * mx/my drive a virtual stick into ix/iy (touch + mouse drag). Pointer lock
 * is opt-in; touch never locks.
 */
export const INPUT_MAGIC = 0x4e495855; // 'UXIN' LE
export const INPUT_VERSION = 2;
/** v1 size (still decodable). */
export const INPUT_BYTES_V1 = 20;
/** Current encode size (v2). */
export const INPUT_BYTES = 36;

export const BTN_LEFT = 1;
export const BTN_RIGHT = 2;
export const BTN_MIDDLE = 4;
export const FLAG_POINTER_IN = 1;
/** Host may set: operator armed suicide / kamikaze (KeyF). */
export const FLAG_SUICIDE = 2;
/** Primary contact is a touch (finger), not mouse/pen. */
export const FLAG_TOUCH = 4;
/** Dead-zone radius for Host virtual stick (NDC). */
export const STICK_DEADZONE = 0.2;

/**
 * Map canvas NDC to discrete stick axes (same convention as WASD).
 * my +up → iy −1；my −down → iy +1.
 * @param {number} mx
 * @param {number} my
 * @param {number} [dead]
 * @returns {{ ix: number, iy: number }}
 */
export function axesFromStick(mx, my, dead = STICK_DEADZONE) {
  let ix = 0, iy = 0;
  if (mx <= -dead) ix = -1;
  else if (mx >= dead) ix = 1;
  if (my <= -dead) iy = 1;
  else if (my >= dead) iy = -1;
  return { ix, iy };
}


/**
 * @param {{
 *   ix?: number, iy?: number, fire?: number,
 *   mx?: number, my?: number, buttons?: number, flags?: number
 * }} snap
 * @param {ArrayBuffer} [out]
 * @returns {ArrayBuffer}
 */
export function encodeInputSnapshot(snap, out) {
  const want = INPUT_BYTES;
  const buf = out || new ArrayBuffer(want);
  if (buf.byteLength < INPUT_BYTES_V1) {
    throw new Error("input buffer too small " + buf.byteLength);
  }
  const dv = new DataView(buf);
  const ver = buf.byteLength >= INPUT_BYTES ? INPUT_VERSION : 1;
  dv.setUint32(0, INPUT_MAGIC, true);
  dv.setUint32(4, ver, true);
  dv.setInt32(8, snap.ix | 0, true);
  dv.setInt32(12, snap.iy | 0, true);
  dv.setUint32(16, (snap.fire >>> 0) || 0, true);
  if (ver >= 2) {
    dv.setFloat32(20, Number(snap.mx) || 0, true);
    dv.setFloat32(24, Number(snap.my) || 0, true);
    dv.setUint32(28, (snap.buttons >>> 0) || 0, true);
    dv.setUint32(32, (snap.flags >>> 0) || 0, true);
  }
  return buf;
}

/**
 * @param {ArrayBuffer|ArrayBufferView} raw
 * @returns {{
 *   ix: number, iy: number, fire: number,
 *   mx: number, my: number, buttons: number, flags: number,
 *   version: number
 * }}
 */
export function decodeInputSnapshot(raw) {
  const buf = ArrayBuffer.isView(raw)
    ? raw.buffer.slice(raw.byteOffset, raw.byteOffset + raw.byteLength)
    : raw;
  if (buf.byteLength < INPUT_BYTES_V1) {
    throw new Error("input buffer too short " + buf.byteLength);
  }
  const dv = new DataView(buf);
  const magic = dv.getUint32(0, true);
  if (magic !== INPUT_MAGIC) throw new Error("bad input magic");
  const ver = dv.getUint32(4, true);
  if (ver !== 1 && ver !== 2) throw new Error("bad input version " + ver);
  const out = {
    version: ver,
    ix: dv.getInt32(8, true),
    iy: dv.getInt32(12, true),
    fire: dv.getUint32(16, true),
    mx: 0, my: 0, buttons: 0, flags: 0,
  };
  if (ver >= 2 && buf.byteLength >= INPUT_BYTES) {
    out.mx = dv.getFloat32(20, true);
    out.my = dv.getFloat32(24, true);
    out.buttons = dv.getUint32(28, true);
    out.flags = dv.getUint32(32, true);
  }
  return out;
}

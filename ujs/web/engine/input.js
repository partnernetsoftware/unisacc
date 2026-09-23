/**
 * UXE input snapshot binary layout (Host ABI host_input_read).
 *
 * Little-endian:
 *   magic "UXIN" (u32) | version (u32=1)
 *   ix (i32) | iy (i32) | fire (u32)
 *   optional reserved (ignored on decode)
 */
export const INPUT_MAGIC = 0x4e495855; // 'UXIN' LE
export const INPUT_VERSION = 1;
/** Fixed header size in bytes (no reserved). */
export const INPUT_BYTES = 20;

/**
 * @param {{ ix: number, iy: number, fire: number }} snap
 * @param {ArrayBuffer} [out]  if provided, write into it (must be ≥ INPUT_BYTES)
 * @returns {ArrayBuffer}
 */
export function encodeInputSnapshot(snap, out) {
  const buf = out || new ArrayBuffer(INPUT_BYTES);
  if (buf.byteLength < INPUT_BYTES) {
    throw new Error("input buffer too small " + buf.byteLength);
  }
  const dv = new DataView(buf);
  dv.setUint32(0, INPUT_MAGIC, true);
  dv.setUint32(4, INPUT_VERSION, true);
  dv.setInt32(8, snap.ix | 0, true);
  dv.setInt32(12, snap.iy | 0, true);
  dv.setUint32(16, snap.fire >>> 0, true);
  return buf;
}

/**
 * @param {ArrayBuffer|ArrayBufferView} raw
 * @returns {{ ix: number, iy: number, fire: number }}
 */
export function decodeInputSnapshot(raw) {
  const buf = ArrayBuffer.isView(raw)
    ? raw.buffer.slice(raw.byteOffset, raw.byteOffset + raw.byteLength)
    : raw;
  if (buf.byteLength < INPUT_BYTES) {
    throw new Error("input buffer too short " + buf.byteLength);
  }
  const dv = new DataView(buf);
  const magic = dv.getUint32(0, true);
  if (magic !== INPUT_MAGIC) throw new Error("bad input magic");
  const ver = dv.getUint32(4, true);
  if (ver !== INPUT_VERSION) throw new Error("bad input version " + ver);
  return {
    ix: dv.getInt32(8, true),
    iy: dv.getInt32(12, true),
    fire: dv.getUint32(16, true),
  };
}

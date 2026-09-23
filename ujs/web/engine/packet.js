/**
 * UXE render packet binary layout (Host ABI gpu_submit).
 *
 * Little-endian:
 *   magic "UXEP" (u32) | version (u32=1)
 *   clear rgba (4×f32)
 *   camera: fovy, near, far, eye×3, target×3 (9×f32)
 *   nClouds (u32)
 *   per cloud: color×3 (f32) | count (u32) | xyz[count*3] (f32) | scale[count] (f32)
 */
export const PACKET_MAGIC = 0x50455855; // 'UXEP' LE
export const PACKET_VERSION = 1;

/**
 * @param {{
 *   clear: number[],
 *   camera: { fovy: number, near: number, far: number, eye: number[], target: number[] },
 *   clouds: Array<{ color: number[], count: number, xyz: Float32Array, scale: Float32Array }>
 * }} packet
 * @returns {ArrayBuffer}
 */
export function encodeRenderPacket(packet) {
  const clouds = packet.clouds || [];
  let nbytes = 8 + 16 + 36 + 4; // hdr + clear + camera + nClouds
  for (const c of clouds) {
    nbytes += 12 + 4 + c.count * 3 * 4 + c.count * 4;
  }
  const buf = new ArrayBuffer(nbytes);
  const dv = new DataView(buf);
  let o = 0;
  dv.setUint32(o, PACKET_MAGIC, true); o += 4;
  dv.setUint32(o, PACKET_VERSION, true); o += 4;
  const clear = packet.clear || [0, 0, 0, 1];
  for (let i = 0; i < 4; i++) { dv.setFloat32(o, clear[i], true); o += 4; }
  const cam = packet.camera;
  const camf = [
    cam.fovy, cam.near, cam.far,
    cam.eye[0], cam.eye[1], cam.eye[2],
    cam.target[0], cam.target[1], cam.target[2],
  ];
  for (let i = 0; i < 9; i++) { dv.setFloat32(o, camf[i], true); o += 4; }
  dv.setUint32(o, clouds.length, true); o += 4;
  for (const c of clouds) {
    for (let i = 0; i < 3; i++) { dv.setFloat32(o, c.color[i], true); o += 4; }
    dv.setUint32(o, c.count, true); o += 4;
    const xyz = c.xyz;
    for (let i = 0; i < c.count * 3; i++) { dv.setFloat32(o, xyz[i], true); o += 4; }
    const sc = c.scale;
    for (let i = 0; i < c.count; i++) { dv.setFloat32(o, sc[i], true); o += 4; }
  }
  if (o !== nbytes) throw new Error("encode size mismatch " + o + " vs " + nbytes);
  return buf;
}

/**
 * @param {ArrayBuffer|ArrayBufferView} raw
 */
export function decodeRenderPacket(raw) {
  const buf = ArrayBuffer.isView(raw)
    ? raw.buffer.slice(raw.byteOffset, raw.byteOffset + raw.byteLength)
    : raw;
  const dv = new DataView(buf);
  let o = 0;
  const magic = dv.getUint32(o, true); o += 4;
  if (magic !== PACKET_MAGIC) throw new Error("bad packet magic");
  const ver = dv.getUint32(o, true); o += 4;
  if (ver !== PACKET_VERSION) throw new Error("bad packet version " + ver);
  const clear = [
    dv.getFloat32(o, true), dv.getFloat32(o + 4, true),
    dv.getFloat32(o + 8, true), dv.getFloat32(o + 12, true),
  ];
  o += 16;
  const fovy = dv.getFloat32(o, true); o += 4;
  const near = dv.getFloat32(o, true); o += 4;
  const far = dv.getFloat32(o, true); o += 4;
  const eye = [dv.getFloat32(o, true), dv.getFloat32(o + 4, true), dv.getFloat32(o + 8, true)];
  o += 12;
  const target = [dv.getFloat32(o, true), dv.getFloat32(o + 4, true), dv.getFloat32(o + 8, true)];
  o += 12;
  const nClouds = dv.getUint32(o, true); o += 4;
  const clouds = [];
  for (let c = 0; c < nClouds; c++) {
    const color = [dv.getFloat32(o, true), dv.getFloat32(o + 4, true), dv.getFloat32(o + 8, true)];
    o += 12;
    const count = dv.getUint32(o, true); o += 4;
    const xyz = new Float32Array(count * 3);
    for (let i = 0; i < count * 3; i++) { xyz[i] = dv.getFloat32(o, true); o += 4; }
    const scale = new Float32Array(count);
    for (let i = 0; i < count; i++) { scale[i] = dv.getFloat32(o, true); o += 4; }
    clouds.push({ color, count, xyz, scale });
  }
  return {
    clear,
    camera: { fovy, near, far, eye, target },
    clouds,
  };
}

/**
 * UXE render packet binary layout (Host ABI gpu_submit).
 *
 * Little-endian:
 *   magic "UXEP" (u32) | version (u32)
 *   clear rgba (4×f32)
 *   camera: fovy, near, far, eye×3, target×3 (9×f32)
 *   —— v2+ ——
 *   fog: density, color×3 (4×f32)
 *   ambient: color×3, intensity (4×f32)
 *   nLights (u32)
 *   per light: dir×3, color×3, intensity (7×f32)
 *   —— ——
 *   nClouds (u32)
 *   per cloud:
 *     v1/v2: color×3 | count | xyz… | scale…
 *     v3+:   color×3 | emissive×3 | metal | rough | mesh_id | count | xyz… | scale…
 *
 * mesh_id: 0=octahedron(rock), 1=ship wedge, 2=box（见 meshes.js）
 */
export const PACKET_MAGIC = 0x50455855; // 'UXEP' LE
/** Current encode version without textures (fog/lights + mesh/material). */
export const PACKET_VERSION = 3;
/** v4 when textures or mapId present. See prd_uxe_sped.md 2.1. */
export const PACKET_VERSION_TEX = 4;

export const MESH_OCTA = 0;
export const MESH_SHIP = 1;
export const MESH_BOX = 2;
export const MESH_PLANE = 3;

const DEFAULT_FOG = { density: 0.018, color: [0.02, 0.024, 0.04] };
const DEFAULT_AMBIENT = { color: [0.25, 0.38, 0.5], intensity: 0.55 };
const DEFAULT_LIGHTS = [
  { dir: [0.35, 0.9, 0.25], color: [1.0, 0.9, 0.78], intensity: 1.15 },
  { dir: [-0.4, 0.2, -0.5], color: [0.4, 0.53, 1.0], intensity: 0.45 },
];

/**
 * @param {{
 *   clear: number[],
 *   camera: object,
 *   fog?: object,
 *   ambient?: object,
 *   lights?: object[],
 *   clouds: Array<{
 *     color: number[], count: number,
 *     xyz: Float32Array, scale: Float32Array,
 *     emissive?: number[], metalness?: number, roughness?: number, meshId?: number
 *   }>
 * }} packet
 */
export function encodeRenderPacket(packet) {
  const clouds = packet.clouds || [];
  const fog = packet.fog || DEFAULT_FOG;
  const ambient = packet.ambient || DEFAULT_AMBIENT;
  const lights = packet.lights || DEFAULT_LIGHTS;
  const texs = packet.textures || [];
  let useV4 = texs.length > 0;
  for (const c of clouds) {
    if ((c.mapId | 0) !== 0 || (c.alphaMode | 0) || (c.instColor && c.instColor.length)) useV4 = 1;
  }
  const ver = useV4 ? PACKET_VERSION_TEX : PACKET_VERSION;
  let nbytes = 8 + 16 + 36 + 16 + 16 + 4 + lights.length * 28 + 4;
  for (const c of clouds) {
    nbytes += 12 + 12 + 4 + 4 + 4 + 4 + c.count * 3 * 4 + c.count * 4;
    if (useV4) nbytes += 4;
    if (c.instColor && c.instColor.length) nbytes += c.count * 4;
  }

  if (useV4) {
    nbytes += 4;
    for (const t of texs) {
      const n = t.w * t.h * 4;
      if (n > 1048576) throw new Error("tex too large");
      nbytes += 10 + n;
    }
  }
  const buf = new ArrayBuffer(nbytes);
  const dv = new DataView(buf);
  let o = 0;
  dv.setUint32(o, PACKET_MAGIC, true); o += 4;
  dv.setUint32(o, ver, true); o += 4;
  const clear = packet.clear || [0, 0, 0, 1];
  for (let i = 0; i < 4; i++) { dv.setFloat32(o, clear[i], true); o += 4; }
  const cam = packet.camera;
  const camf = [
    cam.fovy, cam.near, cam.far,
    cam.eye[0], cam.eye[1], cam.eye[2],
    cam.target[0], cam.target[1], cam.target[2],
  ];
  for (let i = 0; i < 9; i++) { dv.setFloat32(o, camf[i], true); o += 4; }

  dv.setFloat32(o, fog.density, true); o += 4;
  for (let i = 0; i < 3; i++) { dv.setFloat32(o, fog.color[i], true); o += 4; }
  for (let i = 0; i < 3; i++) { dv.setFloat32(o, ambient.color[i], true); o += 4; }
  dv.setFloat32(o, ambient.intensity, true); o += 4;
  dv.setUint32(o, lights.length, true); o += 4;
  for (const L of lights) {
    for (let i = 0; i < 3; i++) { dv.setFloat32(o, L.dir[i], true); o += 4; }
    for (let i = 0; i < 3; i++) { dv.setFloat32(o, L.color[i], true); o += 4; }
    dv.setFloat32(o, L.intensity, true); o += 4;
  }

  dv.setUint32(o, clouds.length, true); o += 4;
  for (const c of clouds) {
    for (let i = 0; i < 3; i++) { dv.setFloat32(o, c.color[i], true); o += 4; }
    const em = c.emissive || [0, 0, 0];
    for (let i = 0; i < 3; i++) { dv.setFloat32(o, em[i], true); o += 4; }
    dv.setFloat32(o, c.metalness ?? 0.35, true); o += 4;
    dv.setFloat32(o, c.roughness ?? 0.45, true); o += 4;
    dv.setUint32(o, c.meshId ?? MESH_OCTA, true); o += 4;
    dv.setUint32(o, c.count, true); o += 4;
    for (let i = 0; i < c.count * 3; i++) { dv.setFloat32(o, c.xyz[i], true); o += 4; }
    for (let i = 0; i < c.count; i++) { dv.setFloat32(o, c.scale[i], true); o += 4; }
    if (useV4) {
      dv.setUint16(o, c.mapId ?? 0, true); o += 2;
      const hasIC = c.instColor && c.instColor.length >= c.count * 4;
      const flags = ((c.uvOrigin | 0) & 1) | (((c.alphaMode | 0) & 1) << 1) | (hasIC ? 4 : 0);
      dv.setUint16(o, flags, true); o += 2;
      if (hasIC) {
        for (let i = 0; i < c.count * 4; i++) dv.setUint8(o + i, c.instColor[i] & 255);
        o += c.count * 4;
      }
    }


  }
  if (useV4) {
    dv.setUint32(o, texs.length, true); o += 4;
    for (const t of texs) {
      dv.setUint16(o, t.id, true); o += 2;
      dv.setUint16(o, t.w, true); o += 2;
      dv.setUint16(o, t.h, true); o += 2;
      dv.setUint8(o, t.wrap ?? 0); o += 1;
      dv.setUint8(o, t.filter ?? 0); o += 1;
      dv.setUint16(o, 0, true); o += 2;
      const rgba = t.rgba;
      const n = t.w * t.h * 4;
      for (let i = 0; i < n; i++) dv.setUint8(o + i, rgba[i] & 255);
      o += n;
    }
  }
  if (o !== nbytes) throw new Error("encode size mismatch " + o + " vs " + nbytes);
  return buf;
}

function readClouds(dv, o, nClouds, ver) {
  const clouds = [];
  for (let c = 0; c < nClouds; c++) {
    const color = [dv.getFloat32(o, true), dv.getFloat32(o + 4, true), dv.getFloat32(o + 8, true)];
    o += 12;
    let emissive = [0, 0, 0], metalness = 0.2, roughness = 0.5, meshId = MESH_OCTA;
    if (ver >= 3) {
      emissive = [dv.getFloat32(o, true), dv.getFloat32(o + 4, true), dv.getFloat32(o + 8, true)];
      o += 12;
      metalness = dv.getFloat32(o, true); o += 4;
      roughness = dv.getFloat32(o, true); o += 4;
      meshId = dv.getUint32(o, true); o += 4;
    }
    const count = dv.getUint32(o, true); o += 4;
    const xyz = new Float32Array(count * 3);
    for (let i = 0; i < count * 3; i++) { xyz[i] = dv.getFloat32(o, true); o += 4; }
    const scale = new Float32Array(count);
    for (let i = 0; i < count; i++) { scale[i] = dv.getFloat32(o, true); o += 4; }
    let mapId = 0, uvOrigin = 0, alphaMode = 0, instColor = null;
    if (ver >= 4) {
      mapId = dv.getUint16(o, true); o += 2;
      const flags = dv.getUint16(o, true); o += 2;
      uvOrigin = flags & 1;
      alphaMode = (flags >> 1) & 1;
      if (flags & 4) {
        instColor = new Uint8Array(count * 4);
        for (let i = 0; i < count * 4; i++) instColor[i] = dv.getUint8(o + i);
        o += count * 4;
      }
    }
    clouds.push({ color, emissive, metalness, roughness, meshId, count, xyz, scale, mapId, uvOrigin, alphaMode, instColor });


  }
  return { clouds, o };
}

export function decodeRenderPacket(raw, cache) {
  const buf = ArrayBuffer.isView(raw)
    ? raw.buffer.slice(raw.byteOffset, raw.byteOffset + raw.byteLength)
    : raw;
  const dv = new DataView(buf);
  let o = 0;
  const magic = dv.getUint32(o, true); o += 4;
  if (magic !== PACKET_MAGIC) throw new Error("bad packet magic");
  const ver = dv.getUint32(o, true); o += 4;
  if (ver < 1 || ver > 4) throw new Error("bad packet version " + ver);
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

  let fog = { ...DEFAULT_FOG, color: [...DEFAULT_FOG.color] };
  let ambient = { color: [...DEFAULT_AMBIENT.color], intensity: DEFAULT_AMBIENT.intensity };
  let lights = DEFAULT_LIGHTS.map((L) => ({
    dir: [...L.dir], color: [...L.color], intensity: L.intensity,
  }));

  if (ver >= 2) {
    fog = {
      density: dv.getFloat32(o, true),
      color: [dv.getFloat32(o + 4, true), dv.getFloat32(o + 8, true), dv.getFloat32(o + 12, true)],
    };
    o += 16;
    ambient = {
      color: [dv.getFloat32(o, true), dv.getFloat32(o + 4, true), dv.getFloat32(o + 8, true)],
      intensity: dv.getFloat32(o + 12, true),
    };
    o += 16;
    const nLights = dv.getUint32(o, true); o += 4;
    lights = [];
    for (let i = 0; i < nLights; i++) {
      lights.push({
        dir: [dv.getFloat32(o, true), dv.getFloat32(o + 4, true), dv.getFloat32(o + 8, true)],
        color: [dv.getFloat32(o + 12, true), dv.getFloat32(o + 16, true), dv.getFloat32(o + 20, true)],
        intensity: dv.getFloat32(o + 24, true),
      });
      o += 28;
    }
  }

  const nClouds = dv.getUint32(o, true); o += 4;
  const rc = readClouds(dv, o, nClouds, ver);
  const clouds = rc.clouds;
  o = rc.o;
  const textures = [];
  if (ver >= 4 && o + 4 <= dv.byteLength) {
    const nTex = dv.getUint32(o, true); o += 4;
    let sum = 0;
    for (let i = 0; i < nTex; i++) {
      if (o + 8 > dv.byteLength) throw new Error("truncated tex header");
      const id = dv.getUint16(o, true); o += 2;
      const w = dv.getUint16(o, true); o += 2;
      const h = dv.getUint16(o, true); o += 2;
      const wrap = dv.getUint8(o); o += 1;
      const filter = dv.getUint8(o); o += 1;
      o += 2;
      if (!w || !h || w > 4096 || h > 4096) throw new Error("tex bound");
      const n = w * h * 4;
      if (n > 1048576) throw new Error("tex too large");
      sum += n;
      if (sum > 1048576) throw new Error("tex too large");
      if (o + n > dv.byteLength) throw new Error("truncated tex");
      const rgba = new Uint8Array(n);
      for (let k = 0; k < n; k++) rgba[k] = dv.getUint8(o + k);
      o += n;
      textures.push({ id, w, h, wrap, filter, rgba });
      if (cache) cache[id] = 1;
    }
  }

  for (let i = 0; i < clouds.length; i++) {
    const id = clouds[i].mapId | 0;
    if (!id) continue;
    let hit = 0;
    for (let j = 0; j < textures.length; j++) if (textures[j].id === id) hit = 1;
    if (!hit && cache && cache[id]) hit = 1;
    if (!hit) throw new Error("dangling mapId " + id);
  }
  return {
    version: ver,
    clear,
    camera: { fovy, near, far, eye, target },
    fog,
    ambient,
    lights,
    clouds,
    textures,
  };
}

/**
 * Stable JSON summary of a decoded UXEP for agent / CDP asserts.
 * @param {ReturnType<typeof decodeRenderPacket>|object|null|undefined} packet
 */
export function summarizeRenderPacket(packet) {
  if (!packet) return null;
  const cam = packet.camera || {};
  const clouds = (packet.clouds || []).map((c) => {
    let yMin = Infinity;
    let yMax = -Infinity;
    const xyz = c.xyz;
    const n = c.count | 0;
    for (let i = 0; i < n; i++) {
      const y = xyz[i * 3 + 1];
      if (y < yMin) yMin = y;
      if (y > yMax) yMax = y;
    }
    if (n === 0) {
      yMin = 0;
      yMax = 0;
    }
    return {
      meshId: c.meshId ?? 0,
      count: n,
      yMin: yMin === Infinity ? 0 : yMin,
      yMax: yMax === -Infinity ? 0 : yMax,
    };
  });
  return {
    clear: packet.clear ? Array.from(packet.clear) : null,
    eye: cam.eye ? Array.from(cam.eye) : null,
    target: cam.target ? Array.from(cam.target) : null,
    fogDensity: packet.fog?.density ?? 0,
    clouds,
  };
}

export { DEFAULT_FOG, DEFAULT_AMBIENT, DEFAULT_LIGHTS };

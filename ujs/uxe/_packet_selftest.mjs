import { encodeRenderPacket, decodeRenderPacket, summarizeRenderPacket, PACKET_MAGIC } from "./packet.js";

const N = 480;
const xyz = new Float32Array(N * 3);
const sc = new Float32Array(N);
for (let i = 0; i < N; i++) {
  xyz[i * 3] = i; xyz[i * 3 + 1] = -i; xyz[i * 3 + 2] = i * 0.5;
  sc[i] = 0.5 + (i % 5) * 0.1;
}
const ship = new Float32Array([1, 2, 3]);
const shipS = new Float32Array([0.7]);

const buf = encodeRenderPacket({
  clear: [0.1, 0.2, 0.3, 1],
  camera: {
    fovy: 1.1, near: 0.1, far: 300,
    eye: [0, 3, 12], target: [0, 0, -10],
  },
  clouds: [
    { color: [0.5, 0.5, 0.5], count: N, xyz, scale: sc, meshId: 0 },
    { color: [0.2, 0.8, 1], count: 1, xyz: ship, scale: shipS, meshId: 1, emissive: [0.05, 0.1, 0.2] },
  ],
});

const dv = new DataView(buf);
if (dv.getUint32(0, true) !== PACKET_MAGIC) throw new Error("magic");
const out = decodeRenderPacket(buf);
if (out.version !== 3) throw new Error("version " + out.version);
if (out.clouds.length !== 2) throw new Error("n clouds");
if (out.clouds[0].count !== N) throw new Error("count");
if (Math.abs(out.clouds[0].xyz[3] - xyz[3]) > 1e-5) throw new Error("xyz");
if (Math.abs(out.camera.eye[1] - 3) > 1e-5) throw new Error("eye");
if (Math.abs(out.fog.density - 0.018) > 1e-6) throw new Error("fog");
if (out.lights.length < 1) throw new Error("lights");
if (out.clouds[0].meshId !== 0) throw new Error("mesh rock");
if (out.clouds[1].meshId !== 1) throw new Error("mesh ship");
const sum = summarizeRenderPacket(out);
if (!sum || sum.eye[1] !== 3) throw new Error("summarize eye");
if (sum.clouds[0].yMin !== -(N - 1) || sum.clouds[0].count !== N) throw new Error("summarize y");
console.log("OK_PACKET", {
  bytes: buf.byteLength, ver: out.version,
  clouds: out.clouds.length, n0: out.clouds[0].count,
  fog: out.fog.density, lights: out.lights.length,
  mesh: [out.clouds[0].meshId, out.clouds[1].meshId],
  snapY: [sum.clouds[0].yMin, sum.clouds[0].yMax],
});

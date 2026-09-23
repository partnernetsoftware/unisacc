/** UXE WebGPU — multi-mesh instancing + fog/lights/material (UXEP v3). */
import { mat4, perspective, lookAt, mul } from "./math.js";
import { allMeshes } from "./meshes.js";

const WGSL = /* wgsl */ `
struct Uniforms {
  vp: mat4x4f,
  color: vec3f,
  fog_d: f32,
  eye: vec3f,
  amb_i: f32,
  amb: vec3f,
  metal: f32,
  l0_dir: vec3f,
  l0_i: f32,
  l0_col: vec3f,
  rough: f32,
  l1_dir: vec3f,
  l1_i: f32,
  l1_col: vec3f,
  _p2: f32,
  fog_c: vec3f,
  _p3: f32,
  emissive: vec3f,
  _p4: f32,
}

@group(0) @binding(0) var<uniform> u: Uniforms;

struct VsIn {
  @location(0) pos: vec3f,
  @location(1) inst: vec3f,
  @location(2) scale: f32,
}

struct VsOut {
  @builtin(position) clip: vec4f,
  @location(0) n: vec3f,
  @location(1) fog_depth: f32,
}

@vertex
fn vs_main(v: VsIn) -> VsOut {
  var o: VsOut;
  let p = v.pos * v.scale + v.inst;
  o.clip = u.vp * vec4f(p, 1.0);
  o.n = normalize(v.pos);
  o.fog_depth = length(p - u.eye);
  return o;
}

@fragment
fn fs_main(v: VsOut) -> @location(0) vec4f {
  let n = normalize(v.n);
  let amb = u.amb * u.amb_i;
  let d0 = max(dot(n, normalize(u.l0_dir)), 0.0);
  let d1 = max(dot(n, normalize(u.l1_dir)), 0.0);
  let soft = mix(1.0, 0.55, clamp(u.rough, 0.0, 1.0));
  let diff = u.l0_col * u.l0_i * d0 * soft + u.l1_col * u.l1_i * d1 * soft;
  let base = mix(u.color, u.color * 0.85 + vec3f(0.15), clamp(u.metal, 0.0, 1.0));
  var col = base * (amb + diff) + u.emissive;
  let fog_f = 1.0 - exp(-u.fog_d * u.fog_d * v.fog_depth * v.fog_depth);
  col = mix(col, u.fog_c, clamp(fog_f, 0.0, 1.0));
  return vec4f(col, 1.0);
}
`;

const UNIFORM_STRIDE = 256;
const UNIFORM_FLOATS = UNIFORM_STRIDE / 4;
const UNIFORM_PAYLOAD = 208;

function norm3(v) {
  const L = Math.hypot(v[0], v[1], v[2]) || 1;
  return [v[0] / L, v[1] / L, v[2] / L];
}

function adaptProjectionForWebGPU(P) {
  P[5] = -P[5];
  const z0 = P[2], z1 = P[6], z2 = P[10], z3 = P[14];
  const w0 = P[3], w1 = P[7], w2 = P[11], w3 = P[15];
  P[2] = z0 * 0.5 + w0 * 0.5;
  P[6] = z1 * 0.5 + w1 * 0.5;
  P[10] = z2 * 0.5 + w2 * 0.5;
  P[14] = z3 * 0.5 + w3 * 0.5;
}

/**
 * @param {HTMLCanvasElement} canvas
 */
export async function createWebGPURenderer(canvas) {
  if (typeof navigator === "undefined" || !navigator.gpu) return null;

  let adapter;
  try { adapter = await navigator.gpu.requestAdapter(); } catch { return null; }
  if (!adapter) return null;

  let device;
  try { device = await adapter.requestDevice(); } catch { return null; }

  const format = navigator.gpu.getPreferredCanvasFormat();
  let pipeline, bgl, meshBufs;

  try {
    const shader = device.createShaderModule({ code: WGSL });
    bgl = device.createBindGroupLayout({
      entries: [{
        binding: 0,
        visibility: GPUShaderStage.VERTEX | GPUShaderStage.FRAGMENT,
        buffer: { type: "uniform", hasDynamicOffset: true, minBindingSize: UNIFORM_PAYLOAD },
      }],
    });
    const pll = device.createPipelineLayout({ bindGroupLayouts: [bgl] });
    pipeline = device.createRenderPipeline({
      layout: pll,
      vertex: {
        module: shader,
        entryPoint: "vs_main",
        buffers: [
          { arrayStride: 12, stepMode: "vertex", attributes: [{ shaderLocation: 0, offset: 0, format: "float32x3" }] },
          { arrayStride: 12, stepMode: "instance", attributes: [{ shaderLocation: 1, offset: 0, format: "float32x3" }] },
          { arrayStride: 4, stepMode: "instance", attributes: [{ shaderLocation: 2, offset: 0, format: "float32" }] },
        ],
      },
      fragment: {
        module: shader,
        entryPoint: "fs_main",
        targets: [{ format }],
      },
      primitive: { topology: "triangle-list" },
      depthStencil: { format: "depth24plus", depthWriteEnabled: true, depthCompare: "less" },
    });
    meshBufs = [];
    for (const m of allMeshes()) {
      const buf = device.createBuffer({
        size: m.positions.byteLength,
        usage: GPUBufferUsage.VERTEX | GPUBufferUsage.COPY_DST,
        label: "uxe-mesh-" + m.id,
      });
      device.queue.writeBuffer(buf, 0, m.positions);
      meshBufs.push({ buf, vCount: m.vCount });
    }
  } catch {
    try { device.destroy(); } catch { /* ignore */ }
    return null;
  }

  const ctx = canvas.getContext("webgpu");
  if (!ctx) {
    try { device.destroy(); } catch { /* ignore */ }
    return null;
  }

  let uniformBuf = null, uniformCpu = null, uniformSlots = 0, bindGroup = null;
  let instXyzBuf = null, instScaleBuf = null, instCap = 0;
  let depthTex = null, depthW = 0, depthH = 0;
  const P = mat4(), V = mat4(), VP = mat4();

  function ensureUniformSlots(n) {
    if (n <= uniformSlots && uniformBuf) return;
    const slots = Math.max(n, 4);
    if (uniformBuf) uniformBuf.destroy();
    uniformBuf = device.createBuffer({
      size: slots * UNIFORM_STRIDE,
      usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
    });
    uniformCpu = new Float32Array(slots * UNIFORM_FLOATS);
    bindGroup = device.createBindGroup({
      layout: bgl,
      entries: [{ binding: 0, resource: { buffer: uniformBuf, offset: 0, size: UNIFORM_STRIDE } }],
    });
    uniformSlots = slots;
  }

  function ensureInstanceCapacity(n) {
    if (n <= instCap && instXyzBuf && instScaleBuf) return;
    const cap = Math.max(n, 64);
    if (instXyzBuf) instXyzBuf.destroy();
    if (instScaleBuf) instScaleBuf.destroy();
    instXyzBuf = device.createBuffer({ size: cap * 12, usage: GPUBufferUsage.VERTEX | GPUBufferUsage.COPY_DST });
    instScaleBuf = device.createBuffer({ size: cap * 4, usage: GPUBufferUsage.VERTEX | GPUBufferUsage.COPY_DST });
    instCap = cap;
  }

  function ensureDepth(w, h) {
    if (depthTex && depthW === w && depthH === h) return;
    if (depthTex) depthTex.destroy();
    depthTex = device.createTexture({
      size: [w, h], format: "depth24plus", usage: GPUTextureUsage.RENDER_ATTACHMENT,
    });
    depthW = w; depthH = h;
  }

  function resize() {
    const w = innerWidth || 800, h = innerHeight || 600;
    const dpr = Math.min(devicePixelRatio || 1, 2);
    canvas.width = (w * dpr) | 0;
    canvas.height = (h * dpr) | 0;
    canvas.style.width = w + "px";
    canvas.style.height = h + "px";
    ctx.configure({ device, format, alphaMode: "opaque", usage: GPUTextureUsage.RENDER_ATTACHMENT });
    ensureDepth(canvas.width, canvas.height);
  }

  function render(scene, camera) {
    const w = canvas.width, h = canvas.height;
    if (w < 1 || h < 1) return;
    ensureDepth(w, h);

    perspective(camera.fovy, w / Math.max(1, h), camera.near, camera.far, P);
    adaptProjectionForWebGPU(P);
    const [ex, ey, ez] = camera.eye;
    const [tx, ty, tz] = camera.target;
    lookAt(ex, ey, ez, tx, ty, tz, V);
    mul(P, V, VP);

    const clouds = [];
    for (const cloud of scene.clouds.values()) {
      if (cloud.count > 0) clouds.push(cloud);
    }

    const [cr, cg, cb, ca] = scene.clearColor;
    const beginPass = (enc) => enc.beginRenderPass({
      colorAttachments: [{
        view: ctx.getCurrentTexture().createView(),
        clearValue: { r: cr, g: cg, b: cb, a: ca },
        loadOp: "clear", storeOp: "store",
      }],
      depthStencilAttachment: {
        view: depthTex.createView(),
        depthClearValue: 1, depthLoadOp: "clear", depthStoreOp: "store",
      },
    });

    if (clouds.length === 0) {
      const enc = device.createCommandEncoder();
      const pass = beginPass(enc);
      pass.end();
      device.queue.submit([enc.finish()]);
      return;
    }

    let xyzFloats = 0, scaleFloats = 0;
    for (const c of clouds) { xyzFloats += c.count * 3; scaleFloats += c.count; }
    ensureInstanceCapacity(Math.max(xyzFloats / 3, 1));
    ensureUniformSlots(clouds.length);

    const fog = scene.fog || { density: 0, color: [cr, cg, cb] };
    const amb = scene.ambient || { color: [1, 1, 1], intensity: 0.4 };
    const lights = scene.lights || [];
    const L0 = lights[0] || { dir: [0, 1, 0], color: [1, 1, 1], intensity: 0 };
    const L1 = lights[1] || { dir: [0, 1, 0], color: [1, 1, 1], intensity: 0 };
    const d0 = norm3(L0.dir), d1 = norm3(L1.dir);

    for (let i = 0; i < clouds.length; i++) {
      const cloud = clouds[i];
      const em = cloud.emissive || [0, 0, 0];
      const base = i * UNIFORM_FLOATS;
      uniformCpu.set(VP, base);
      uniformCpu[base + 16] = cloud.color[0];
      uniformCpu[base + 17] = cloud.color[1];
      uniformCpu[base + 18] = cloud.color[2];
      uniformCpu[base + 19] = fog.density;
      uniformCpu[base + 20] = ex;
      uniformCpu[base + 21] = ey;
      uniformCpu[base + 22] = ez;
      uniformCpu[base + 23] = amb.intensity;
      uniformCpu[base + 24] = amb.color[0];
      uniformCpu[base + 25] = amb.color[1];
      uniformCpu[base + 26] = amb.color[2];
      uniformCpu[base + 27] = cloud.metalness ?? 0.2;
      uniformCpu[base + 28] = d0[0];
      uniformCpu[base + 29] = d0[1];
      uniformCpu[base + 30] = d0[2];
      uniformCpu[base + 31] = L0.intensity;
      uniformCpu[base + 32] = L0.color[0];
      uniformCpu[base + 33] = L0.color[1];
      uniformCpu[base + 34] = L0.color[2];
      uniformCpu[base + 35] = cloud.roughness ?? 0.5;
      uniformCpu[base + 36] = d1[0];
      uniformCpu[base + 37] = d1[1];
      uniformCpu[base + 38] = d1[2];
      uniformCpu[base + 39] = L1.intensity;
      uniformCpu[base + 40] = L1.color[0];
      uniformCpu[base + 41] = L1.color[1];
      uniformCpu[base + 42] = L1.color[2];
      uniformCpu[base + 43] = 0;
      uniformCpu[base + 44] = fog.color[0];
      uniformCpu[base + 45] = fog.color[1];
      uniformCpu[base + 46] = fog.color[2];
      uniformCpu[base + 47] = 0;
      uniformCpu[base + 48] = em[0];
      uniformCpu[base + 49] = em[1];
      uniformCpu[base + 50] = em[2];
      uniformCpu[base + 51] = 0;
    }
    device.queue.writeBuffer(uniformBuf, 0, uniformCpu.subarray(0, clouds.length * UNIFORM_FLOATS));

    const xyzPack = new Float32Array(xyzFloats);
    const scalePack = new Float32Array(scaleFloats);
    const ranges = [];
    let xo = 0, so = 0;
    for (const cloud of clouds) {
      xyzPack.set(cloud.xyz.subarray(0, cloud.count * 3), xo);
      scalePack.set(cloud.scale.subarray(0, cloud.count), so);
      const mid = cloud.meshId ?? 0;
      ranges.push({
        xyzOff: xo * 4, scaleOff: so * 4, count: cloud.count,
        mesh: meshBufs[mid] || meshBufs[0],
      });
      xo += cloud.count * 3;
      so += cloud.count;
      cloud.markClean?.();
    }
    device.queue.writeBuffer(instXyzBuf, 0, xyzPack);
    device.queue.writeBuffer(instScaleBuf, 0, scalePack);

    const enc = device.createCommandEncoder();
    const pass = beginPass(enc);
    pass.setPipeline(pipeline);
    for (let i = 0; i < clouds.length; i++) {
      const r = ranges[i];
      pass.setBindGroup(0, bindGroup, [i * UNIFORM_STRIDE]);
      pass.setVertexBuffer(0, r.mesh.buf);
      pass.setVertexBuffer(1, instXyzBuf, r.xyzOff);
      pass.setVertexBuffer(2, instScaleBuf, r.scaleOff);
      pass.draw(r.mesh.vCount, r.count, 0, 0);
    }
    pass.end();
    device.queue.submit([enc.finish()]);
  }

  return { backend: "webgpu", device, resize, render };
}

export const tryCreateWebGPURenderer = createWebGPURenderer;

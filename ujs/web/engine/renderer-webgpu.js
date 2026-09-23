/** UXE WebGPU backend — octahedron instancing (same MESH / packet as WebGL). */
import { mat4, perspective, lookAt, mul } from "./math.js";

const MESH = new Float32Array([
  0, 1, 0,  1, 0, 0,  0, 0, 1,
  0, 1, 0,  0, 0, 1, -1, 0, 0,
  0, 1, 0, -1, 0, 0,  0, 0,-1,
  0, 1, 0,  0, 0,-1,  1, 0, 0,
  0,-1, 0,  0, 0, 1,  1, 0, 0,
  0,-1, 0, -1, 0, 0,  0, 0, 1,
  0,-1, 0,  0, 0,-1, -1, 0, 0,
  0,-1, 0,  1, 0, 0,  0, 0,-1,
]);

const WGSL = /* wgsl */ `
struct Uniforms {
  vp: mat4x4f,
  color: vec3f,
  _pad: f32,
}

@group(0) @binding(0) var<uniform> u: Uniforms;

struct VsIn {
  @location(0) pos: vec3f,
  @location(1) inst: vec3f,
  @location(2) scale: f32,
}

struct VsOut {
  @builtin(position) clip: vec4f,
  @location(0) col: vec3f,
}

@vertex
fn vs_main(v: VsIn) -> VsOut {
  var o: VsOut;
  let p = v.pos * v.scale + v.inst;
  o.clip = u.vp * vec4f(p, 1.0);
  let sh = 0.45 + 0.55 * v.pos.y;
  o.col = u.color * sh;
  return o;
}

@fragment
fn fs_main(v: VsOut) -> @location(0) vec4f {
  return vec4f(v.col, 1.0);
}
`;

/** Dynamic-offset stride (WebGPU min 256). Payload is mat4 + color. */
const UNIFORM_STRIDE = 256;
const UNIFORM_FLOATS = UNIFORM_STRIDE / 4;
const vCount = MESH.length / 3;

/** OpenGL-style P → WebGPU clip (Y flip for top-left FB + Z [-1,1]→[0,1]). */
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
 * @returns {Promise<null | { backend: "webgpu", resize: () => void, render: (scene, camera) => void }>}
 */
export async function createWebGPURenderer(canvas) {
  if (typeof navigator === "undefined" || !navigator.gpu) return null;

  let adapter;
  try {
    adapter = await navigator.gpu.requestAdapter();
  } catch {
    return null;
  }
  if (!adapter) return null;

  let device;
  try {
    device = await adapter.requestDevice();
  } catch {
    return null;
  }

  const ctx = canvas.getContext("webgpu");
  if (!ctx) return null;

  const format = navigator.gpu.getPreferredCanvasFormat();
  const shader = device.createShaderModule({ code: WGSL });

  const bgl = device.createBindGroupLayout({
    entries: [{
      binding: 0,
      visibility: GPUShaderStage.VERTEX | GPUShaderStage.FRAGMENT,
      buffer: { type: "uniform", hasDynamicOffset: true, minBindingSize: 80 },
    }],
  });
  const pll = device.createPipelineLayout({ bindGroupLayouts: [bgl] });

  const pipeline = device.createRenderPipeline({
    layout: pll,
    vertex: {
      module: shader,
      entryPoint: "vs_main",
      buffers: [
        {
          arrayStride: 12,
          stepMode: "vertex",
          attributes: [{ shaderLocation: 0, offset: 0, format: "float32x3" }],
        },
        {
          arrayStride: 12,
          stepMode: "instance",
          attributes: [{ shaderLocation: 1, offset: 0, format: "float32x3" }],
        },
        {
          arrayStride: 4,
          stepMode: "instance",
          attributes: [{ shaderLocation: 2, offset: 0, format: "float32" }],
        },
      ],
    },
    fragment: {
      module: shader,
      entryPoint: "fs_main",
      targets: [{ format }],
    },
    primitive: { topology: "triangle-list" },
    depthStencil: {
      format: "depth24plus",
      depthWriteEnabled: true,
      depthCompare: "less",
    },
  });

  const meshBuf = device.createBuffer({
    size: MESH.byteLength,
    usage: GPUBufferUsage.VERTEX | GPUBufferUsage.COPY_DST,
    label: "uxe-mesh",
  });
  device.queue.writeBuffer(meshBuf, 0, MESH);

  let uniformBuf = null;
  let uniformCpu = null;
  let uniformSlots = 0;
  let bindGroup = null;

  let instXyzBuf = null;
  let instScaleBuf = null;
  let instCap = 0;
  let depthTex = null;
  let depthW = 0, depthH = 0;

  const P = mat4(), V = mat4(), VP = mat4();

  function ensureUniformSlots(n) {
    if (n <= uniformSlots && uniformBuf) return;
    const slots = Math.max(n, 4);
    if (uniformBuf) uniformBuf.destroy();
    uniformBuf = device.createBuffer({
      size: slots * UNIFORM_STRIDE,
      usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
      label: "uxe-uniforms",
    });
    uniformCpu = new Float32Array(slots * UNIFORM_FLOATS);
    bindGroup = device.createBindGroup({
      layout: bgl,
      entries: [{
        binding: 0,
        resource: { buffer: uniformBuf, offset: 0, size: UNIFORM_STRIDE },
      }],
    });
    uniformSlots = slots;
  }

  function ensureInstanceCapacity(n) {
    if (n <= instCap && instXyzBuf && instScaleBuf) return;
    const cap = Math.max(n, 64);
    if (instXyzBuf) instXyzBuf.destroy();
    if (instScaleBuf) instScaleBuf.destroy();
    instXyzBuf = device.createBuffer({
      size: cap * 3 * 4,
      usage: GPUBufferUsage.VERTEX | GPUBufferUsage.COPY_DST,
      label: "uxe-inst-xyz",
    });
    instScaleBuf = device.createBuffer({
      size: cap * 4,
      usage: GPUBufferUsage.VERTEX | GPUBufferUsage.COPY_DST,
      label: "uxe-inst-scale",
    });
    instCap = cap;
  }

  function ensureDepth(w, h) {
    if (depthTex && depthW === w && depthH === h) return;
    if (depthTex) depthTex.destroy();
    depthTex = device.createTexture({
      size: [w, h],
      format: "depth24plus",
      usage: GPUTextureUsage.RENDER_ATTACHMENT,
      label: "uxe-depth",
    });
    depthW = w;
    depthH = h;
  }

  function resize() {
    const w = innerWidth || 800, h = innerHeight || 600;
    const dpr = Math.min(devicePixelRatio || 1, 2);
    canvas.width = (w * dpr) | 0;
    canvas.height = (h * dpr) | 0;
    canvas.style.width = w + "px";
    canvas.style.height = h + "px";
    ctx.configure({
      device,
      format,
      alphaMode: "opaque",
      usage: GPUTextureUsage.RENDER_ATTACHMENT,
    });
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
    if (clouds.length === 0) {
      const [cr, cg, cb, ca] = scene.clearColor;
      const enc = device.createCommandEncoder();
      const pass = enc.beginRenderPass({
        colorAttachments: [{
          view: ctx.getCurrentTexture().createView(),
          clearValue: { r: cr, g: cg, b: cb, a: ca },
          loadOp: "clear",
          storeOp: "store",
        }],
        depthStencilAttachment: {
          view: depthTex.createView(),
          depthClearValue: 1,
          depthLoadOp: "clear",
          depthStoreOp: "store",
        },
      });
      pass.end();
      device.queue.submit([enc.finish()]);
      return;
    }

    let maxInst = 0;
    for (const c of clouds) maxInst = Math.max(maxInst, c.count);
    ensureInstanceCapacity(maxInst);
    ensureUniformSlots(clouds.length);

    // Upload all instance + uniform data before the pass (one buffer write per cloud uniforms).
    for (let i = 0; i < clouds.length; i++) {
      const cloud = clouds[i];
      const base = i * UNIFORM_FLOATS;
      uniformCpu.set(VP, base);
      uniformCpu[base + 16] = cloud.color[0];
      uniformCpu[base + 17] = cloud.color[1];
      uniformCpu[base + 18] = cloud.color[2];
      uniformCpu[base + 19] = 0;
    }
    device.queue.writeBuffer(uniformBuf, 0, uniformCpu.subarray(0, clouds.length * UNIFORM_FLOATS));

    // Instance buffers: one shared pair; rewrite between draws is unsafe mid-pass,
    // so pack sequentially into the growable buffers with per-cloud offsets via separate buffers.
    // Practical path: allocate per-cloud vertex ranges in one big buffer.
    let xyzFloats = 0;
    let scaleFloats = 0;
    for (const c of clouds) {
      xyzFloats += c.count * 3;
      scaleFloats += c.count;
    }
    ensureInstanceCapacity(Math.max(xyzFloats / 3, scaleFloats));

    const xyzPack = new Float32Array(xyzFloats);
    const scalePack = new Float32Array(scaleFloats);
    const ranges = [];
    let xo = 0, so = 0;
    for (const cloud of clouds) {
      xyzPack.set(cloud.xyz.subarray(0, cloud.count * 3), xo);
      scalePack.set(cloud.scale.subarray(0, cloud.count), so);
      ranges.push({ xyzOff: xo * 4, scaleOff: so * 4, count: cloud.count });
      xo += cloud.count * 3;
      so += cloud.count;
      cloud.markClean?.();
    }
    device.queue.writeBuffer(instXyzBuf, 0, xyzPack);
    device.queue.writeBuffer(instScaleBuf, 0, scalePack);

    const [cr, cg, cb, ca] = scene.clearColor;
    const enc = device.createCommandEncoder();
    const pass = enc.beginRenderPass({
      colorAttachments: [{
        view: ctx.getCurrentTexture().createView(),
        clearValue: { r: cr, g: cg, b: cb, a: ca },
        loadOp: "clear",
        storeOp: "store",
      }],
      depthStencilAttachment: {
        view: depthTex.createView(),
        depthClearValue: 1,
        depthLoadOp: "clear",
        depthStoreOp: "store",
      },
    });
    pass.setPipeline(pipeline);
    pass.setVertexBuffer(0, meshBuf);

    for (let i = 0; i < clouds.length; i++) {
      const r = ranges[i];
      pass.setBindGroup(0, bindGroup, [i * UNIFORM_STRIDE]);
      pass.setVertexBuffer(1, instXyzBuf, r.xyzOff);
      pass.setVertexBuffer(2, instScaleBuf, r.scaleOff);
      pass.draw(vCount, r.count, 0, 0);
    }
    pass.end();
    device.queue.submit([enc.finish()]);
  }

  return {
    backend: "webgpu",
    device,
    resize,
    render,
  };
}

/** @deprecated alias — prefer createWebGPURenderer */
export const tryCreateWebGPURenderer = createWebGPURenderer;

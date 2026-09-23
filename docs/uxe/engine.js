// web/engine/math.js
function mat4() {
  return new Float32Array(16);
}
function perspective(fovy, aspect, near, far, out) {
  const f = 1 / Math.tan(fovy / 2);
  const nf = 1 / (near - far);
  out[0] = f / aspect;
  out[1] = 0;
  out[2] = 0;
  out[3] = 0;
  out[4] = 0;
  out[5] = f;
  out[6] = 0;
  out[7] = 0;
  out[8] = 0;
  out[9] = 0;
  out[10] = (far + near) * nf;
  out[11] = -1;
  out[12] = 0;
  out[13] = 0;
  out[14] = 2 * far * near * nf;
  out[15] = 0;
  return out;
}
function lookAt(ex, ey, ez, cx, cy, cz, out) {
  let zx = ex - cx, zy = ey - cy, zz = ez - cz;
  let len = Math.hypot(zx, zy, zz) || 1;
  zx /= len;
  zy /= len;
  zz /= len;
  let xx = zz, xy = 0, xz = -zx;
  len = Math.hypot(xx, xy, xz) || 1;
  xx /= len;
  xy /= len;
  xz /= len;
  const yx = zy * xz - zz * xy;
  const yy = zz * xx - zx * xz;
  const yz = zx * xy - zy * xx;
  out[0] = xx;
  out[1] = yx;
  out[2] = zx;
  out[3] = 0;
  out[4] = xy;
  out[5] = yy;
  out[6] = zy;
  out[7] = 0;
  out[8] = xz;
  out[9] = yz;
  out[10] = zz;
  out[11] = 0;
  out[12] = -(xx * ex + xy * ey + xz * ez);
  out[13] = -(yx * ex + yy * ey + yz * ez);
  out[14] = -(zx * ex + zy * ey + zz * ez);
  out[15] = 1;
  return out;
}
function mul(a, b, out) {
  for (let c = 0; c < 4; c++) {
    const b0 = b[c * 4], b1 = b[c * 4 + 1], b2 = b[c * 4 + 2], b3 = b[c * 4 + 3];
    out[c * 4] = a[0] * b0 + a[4] * b1 + a[8] * b2 + a[12] * b3;
    out[c * 4 + 1] = a[1] * b0 + a[5] * b1 + a[9] * b2 + a[13] * b3;
    out[c * 4 + 2] = a[2] * b0 + a[6] * b1 + a[10] * b2 + a[14] * b3;
    out[c * 4 + 3] = a[3] * b0 + a[7] * b1 + a[11] * b2 + a[15] * b3;
  }
  return out;
}

// web/engine/meshes.js
var MESH_OCTA = 0;
var MESH_SHIP = 1;
var OCTA_MESH = new Float32Array([
  0,
  1,
  0,
  1,
  0,
  0,
  0,
  0,
  1,
  0,
  1,
  0,
  0,
  0,
  1,
  -1,
  0,
  0,
  0,
  1,
  0,
  -1,
  0,
  0,
  0,
  0,
  -1,
  0,
  1,
  0,
  0,
  0,
  -1,
  1,
  0,
  0,
  0,
  -1,
  0,
  0,
  0,
  1,
  1,
  0,
  0,
  0,
  -1,
  0,
  -1,
  0,
  0,
  0,
  0,
  1,
  0,
  -1,
  0,
  0,
  0,
  -1,
  -1,
  0,
  0,
  0,
  -1,
  0,
  1,
  0,
  0,
  0,
  0,
  -1
]);
var SHIP_MESH = new Float32Array([
  // top fan
  0,
  0.35,
  -1.2,
  0.7,
  0.1,
  0.6,
  -0.7,
  0.1,
  0.6,
  // bottom
  0,
  -0.25,
  -1.2,
  -0.7,
  -0.15,
  0.6,
  0.7,
  -0.15,
  0.6,
  // left
  0,
  0.35,
  -1.2,
  -0.7,
  0.1,
  0.6,
  -0.7,
  -0.15,
  0.6,
  0,
  0.35,
  -1.2,
  -0.7,
  -0.15,
  0.6,
  0,
  -0.25,
  -1.2,
  // right
  0,
  0.35,
  -1.2,
  0.7,
  -0.15,
  0.6,
  0.7,
  0.1,
  0.6,
  0,
  0.35,
  -1.2,
  0,
  -0.25,
  -1.2,
  0.7,
  -0.15,
  0.6,
  // rear
  -0.7,
  0.1,
  0.6,
  0.7,
  0.1,
  0.6,
  0.7,
  -0.15,
  0.6,
  -0.7,
  0.1,
  0.6,
  0.7,
  -0.15,
  0.6,
  -0.7,
  -0.15,
  0.6
]);
var BOX_MESH = new Float32Array([
  // +Y
  -0.5,
  0.5,
  -0.5,
  0.5,
  0.5,
  -0.5,
  0.5,
  0.5,
  0.5,
  -0.5,
  0.5,
  -0.5,
  0.5,
  0.5,
  0.5,
  -0.5,
  0.5,
  0.5,
  // -Y
  -0.5,
  -0.5,
  -0.5,
  0.5,
  -0.5,
  0.5,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  -0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  0.5,
  -0.5,
  0.5,
  // +Z
  -0.5,
  -0.5,
  0.5,
  0.5,
  0.5,
  0.5,
  0.5,
  -0.5,
  0.5,
  -0.5,
  -0.5,
  0.5,
  -0.5,
  0.5,
  0.5,
  0.5,
  0.5,
  0.5,
  // -Z
  -0.5,
  -0.5,
  -0.5,
  0.5,
  -0.5,
  -0.5,
  0.5,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  0.5,
  -0.5,
  -0.5,
  0.5,
  -0.5,
  // +X
  0.5,
  -0.5,
  -0.5,
  0.5,
  -0.5,
  0.5,
  0.5,
  0.5,
  0.5,
  0.5,
  -0.5,
  -0.5,
  0.5,
  0.5,
  0.5,
  0.5,
  0.5,
  -0.5,
  // -X
  -0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  -0.5,
  -0.5,
  0.5,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  0.5,
  -0.5,
  -0.5,
  0.5
]);
var MESH_BOX = 2;
var TABLE = [
  { id: MESH_OCTA, positions: OCTA_MESH, vCount: OCTA_MESH.length / 3 },
  { id: MESH_SHIP, positions: SHIP_MESH, vCount: SHIP_MESH.length / 3 },
  { id: MESH_BOX, positions: BOX_MESH, vCount: BOX_MESH.length / 3 }
];
function allMeshes() {
  return TABLE;
}

// web/engine/renderer-webgl.js
var VS = `#version 100
attribute vec3 aPos;
attribute vec3 aInst;
attribute float aScale;
uniform mat4 uVP;
uniform vec3 uEye;
varying vec3 vN;
varying float vFogDepth;
void main() {
  vec3 p = aPos * aScale + aInst;
  vN = normalize(aPos);
  vFogDepth = length(p - uEye);
  gl_Position = uVP * vec4(p, 1.0);
}`;
var FS = `#version 100
precision mediump float;
uniform vec3 uColor;
uniform vec3 uEmissive;
uniform float uMetal;
uniform float uRough;
uniform vec3 uAmbient;
uniform float uAmbI;
uniform vec3 uL0Dir;
uniform vec3 uL0Col;
uniform float uL0I;
uniform vec3 uL1Dir;
uniform vec3 uL1Col;
uniform float uL1I;
uniform float uFogD;
uniform vec3 uFogC;
varying vec3 vN;
varying float vFogDepth;
void main() {
  vec3 n = normalize(vN);
  vec3 amb = uAmbient * uAmbI;
  float d0 = max(dot(n, normalize(uL0Dir)), 0.0);
  float d1 = max(dot(n, normalize(uL1Dir)), 0.0);
  float soft = mix(1.0, 0.55, clamp(uRough, 0.0, 1.0));
  vec3 diff = uL0Col * uL0I * d0 * soft + uL1Col * uL1I * d1 * soft;
  vec3 base = mix(uColor, uColor * 0.85 + vec3(0.15), clamp(uMetal, 0.0, 1.0));
  vec3 col = base * (amb + diff) + uEmissive;
  float fogF = 1.0 - exp(-uFogD * uFogD * vFogDepth * vFogDepth);
  col = mix(col, uFogC, clamp(fogF, 0.0, 1.0));
  gl_FragColor = vec4(col, 1.0);
}`;
function compile(gl, type, src) {
  const s = gl.createShader(type);
  gl.shaderSource(s, src);
  gl.compileShader(s);
  if (!gl.getShaderParameter(s, gl.COMPILE_STATUS))
    throw new Error(gl.getShaderInfoLog(s) || "shader");
  return s;
}
function norm3(v) {
  const L = Math.hypot(v[0], v[1], v[2]) || 1;
  return [v[0] / L, v[1] / L, v[2] / L];
}
function createWebGLRenderer(canvas) {
  const gl = canvas.getContext("webgl", { antialias: true, powerPreference: "high-performance" });
  if (!gl) throw new Error("WebGL unavailable");
  const ext = gl.getExtension("ANGLE_instanced_arrays");
  if (!ext) throw new Error("ANGLE_instanced_arrays required");
  const prog = gl.createProgram();
  gl.attachShader(prog, compile(gl, gl.VERTEX_SHADER, VS));
  gl.attachShader(prog, compile(gl, gl.FRAGMENT_SHADER, FS));
  gl.linkProgram(prog);
  if (!gl.getProgramParameter(prog, gl.LINK_STATUS))
    throw new Error(gl.getProgramInfoLog(prog) || "link");
  const meshBufs = allMeshes().map((m) => {
    const buf = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, buf);
    gl.bufferData(gl.ARRAY_BUFFER, m.positions, gl.STATIC_DRAW);
    return { buf, vCount: m.vCount };
  });
  const ibo = gl.createBuffer();
  const sbo = gl.createBuffer();
  const aPos = gl.getAttribLocation(prog, "aPos");
  const aInst = gl.getAttribLocation(prog, "aInst");
  const aScale = gl.getAttribLocation(prog, "aScale");
  const u = (n) => gl.getUniformLocation(prog, n);
  const uVP = u("uVP"), uEye = u("uEye");
  const uColor = u("uColor"), uEmissive = u("uEmissive");
  const uMetal = u("uMetal"), uRough = u("uRough");
  const uAmbient = u("uAmbient"), uAmbI = u("uAmbI");
  const uL0Dir = u("uL0Dir"), uL0Col = u("uL0Col"), uL0I = u("uL0I");
  const uL1Dir = u("uL1Dir"), uL1Col = u("uL1Col"), uL1I = u("uL1I");
  const uFogD = u("uFogD"), uFogC = u("uFogC");
  const P = mat4(), V = mat4(), VP = mat4();
  function resize() {
    const w = innerWidth || 800, h = innerHeight || 600;
    const dpr = Math.min(devicePixelRatio || 1, 2);
    canvas.width = w * dpr | 0;
    canvas.height = h * dpr | 0;
    canvas.style.width = w + "px";
    canvas.style.height = h + "px";
    gl.viewport(0, 0, canvas.width, canvas.height);
  }
  function drawCloud(cloud) {
    const mesh = meshBufs[cloud.meshId] || meshBufs[0];
    gl.uniform3fv(uColor, cloud.color);
    gl.uniform3fv(uEmissive, cloud.emissive || [0, 0, 0]);
    gl.uniform1f(uMetal, cloud.metalness ?? 0.2);
    gl.uniform1f(uRough, cloud.roughness ?? 0.5);
    gl.bindBuffer(gl.ARRAY_BUFFER, mesh.buf);
    gl.enableVertexAttribArray(aPos);
    gl.vertexAttribPointer(aPos, 3, gl.FLOAT, false, 0, 0);
    ext.vertexAttribDivisorANGLE(aPos, 0);
    gl.bindBuffer(gl.ARRAY_BUFFER, ibo);
    gl.bufferData(gl.ARRAY_BUFFER, cloud.xyz.subarray(0, cloud.count * 3), gl.DYNAMIC_DRAW);
    gl.enableVertexAttribArray(aInst);
    gl.vertexAttribPointer(aInst, 3, gl.FLOAT, false, 0, 0);
    ext.vertexAttribDivisorANGLE(aInst, 1);
    gl.bindBuffer(gl.ARRAY_BUFFER, sbo);
    gl.bufferData(gl.ARRAY_BUFFER, cloud.scale.subarray(0, cloud.count), gl.DYNAMIC_DRAW);
    gl.enableVertexAttribArray(aScale);
    gl.vertexAttribPointer(aScale, 1, gl.FLOAT, false, 0, 0);
    ext.vertexAttribDivisorANGLE(aScale, 1);
    ext.drawArraysInstancedANGLE(gl.TRIANGLES, 0, mesh.vCount, cloud.count);
    cloud.markClean();
  }
  return {
    backend: "webgl",
    resize,
    render(scene, camera) {
      const w = canvas.width, h = canvas.height;
      const [cr, cg, cb, ca] = scene.clearColor;
      gl.enable(gl.DEPTH_TEST);
      gl.clearColor(cr, cg, cb, ca);
      gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
      perspective(camera.fovy, w / Math.max(1, h), camera.near, camera.far, P);
      const [ex, ey, ez] = camera.eye;
      const [tx, ty, tz] = camera.target;
      lookAt(ex, ey, ez, tx, ty, tz, V);
      mul(P, V, VP);
      const fog = scene.fog || { density: 0, color: [cr, cg, cb] };
      const amb = scene.ambient || { color: [1, 1, 1], intensity: 0.4 };
      const lights = scene.lights || [];
      const L0 = lights[0] || { dir: [0, 1, 0], color: [1, 1, 1], intensity: 0 };
      const L1 = lights[1] || { dir: [0, 1, 0], color: [1, 1, 1], intensity: 0 };
      gl.useProgram(prog);
      gl.uniformMatrix4fv(uVP, false, VP);
      gl.uniform3f(uEye, ex, ey, ez);
      gl.uniform3fv(uAmbient, amb.color);
      gl.uniform1f(uAmbI, amb.intensity);
      gl.uniform3fv(uL0Dir, norm3(L0.dir));
      gl.uniform3fv(uL0Col, L0.color);
      gl.uniform1f(uL0I, L0.intensity);
      gl.uniform3fv(uL1Dir, norm3(L1.dir));
      gl.uniform3fv(uL1Col, L1.color);
      gl.uniform1f(uL1I, L1.intensity);
      gl.uniform1f(uFogD, fog.density);
      gl.uniform3fv(uFogC, fog.color);
      for (const cloud of scene.clouds.values()) {
        if (cloud.count > 0) drawCloud(cloud);
      }
    }
  };
}

// web/engine/renderer-webgpu.js
var WGSL = (
  /* wgsl */
  `
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
`
);
var UNIFORM_STRIDE = 256;
var UNIFORM_FLOATS = UNIFORM_STRIDE / 4;
var UNIFORM_PAYLOAD = 208;
function norm32(v) {
  const L = Math.hypot(v[0], v[1], v[2]) || 1;
  return [v[0] / L, v[1] / L, v[2] / L];
}
function adaptProjectionForWebGPU(P) {
  const z0 = P[2], z1 = P[6], z2 = P[10], z3 = P[14];
  const w0 = P[3], w1 = P[7], w2 = P[11], w3 = P[15];
  P[2] = z0 * 0.5 + w0 * 0.5;
  P[6] = z1 * 0.5 + w1 * 0.5;
  P[10] = z2 * 0.5 + w2 * 0.5;
  P[14] = z3 * 0.5 + w3 * 0.5;
}
async function createWebGPURenderer(canvas) {
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
  const format = navigator.gpu.getPreferredCanvasFormat();
  let pipeline, bgl, meshBufs;
  try {
    const shader = device.createShaderModule({ code: WGSL });
    bgl = device.createBindGroupLayout({
      entries: [{
        binding: 0,
        visibility: GPUShaderStage.VERTEX | GPUShaderStage.FRAGMENT,
        buffer: { type: "uniform", hasDynamicOffset: true, minBindingSize: UNIFORM_PAYLOAD }
      }]
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
          { arrayStride: 4, stepMode: "instance", attributes: [{ shaderLocation: 2, offset: 0, format: "float32" }] }
        ]
      },
      fragment: {
        module: shader,
        entryPoint: "fs_main",
        targets: [{ format }]
      },
      primitive: { topology: "triangle-list" },
      depthStencil: { format: "depth24plus", depthWriteEnabled: true, depthCompare: "less" }
    });
    meshBufs = [];
    for (const m of allMeshes()) {
      const buf = device.createBuffer({
        size: m.positions.byteLength,
        usage: GPUBufferUsage.VERTEX | GPUBufferUsage.COPY_DST,
        label: "uxe-mesh-" + m.id
      });
      device.queue.writeBuffer(buf, 0, m.positions);
      meshBufs.push({ buf, vCount: m.vCount });
    }
  } catch {
    try {
      device.destroy();
    } catch {
    }
    return null;
  }
  const ctx = canvas.getContext("webgpu");
  if (!ctx) {
    try {
      device.destroy();
    } catch {
    }
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
      usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST
    });
    uniformCpu = new Float32Array(slots * UNIFORM_FLOATS);
    bindGroup = device.createBindGroup({
      layout: bgl,
      entries: [{ binding: 0, resource: { buffer: uniformBuf, offset: 0, size: UNIFORM_STRIDE } }]
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
      size: [w, h],
      format: "depth24plus",
      usage: GPUTextureUsage.RENDER_ATTACHMENT
    });
    depthW = w;
    depthH = h;
  }
  function resize() {
    const w = innerWidth || 800, h = innerHeight || 600;
    const dpr = Math.min(devicePixelRatio || 1, 2);
    canvas.width = w * dpr | 0;
    canvas.height = h * dpr | 0;
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
    const beginPass = (enc2) => enc2.beginRenderPass({
      colorAttachments: [{
        view: ctx.getCurrentTexture().createView(),
        clearValue: { r: cr, g: cg, b: cb, a: ca },
        loadOp: "clear",
        storeOp: "store"
      }],
      depthStencilAttachment: {
        view: depthTex.createView(),
        depthClearValue: 1,
        depthLoadOp: "clear",
        depthStoreOp: "store"
      }
    });
    if (clouds.length === 0) {
      const enc2 = device.createCommandEncoder();
      const pass2 = beginPass(enc2);
      pass2.end();
      device.queue.submit([enc2.finish()]);
      return;
    }
    let xyzFloats = 0, scaleFloats = 0;
    for (const c of clouds) {
      xyzFloats += c.count * 3;
      scaleFloats += c.count;
    }
    ensureInstanceCapacity(Math.max(xyzFloats / 3, 1));
    ensureUniformSlots(clouds.length);
    const fog = scene.fog || { density: 0, color: [cr, cg, cb] };
    const amb = scene.ambient || { color: [1, 1, 1], intensity: 0.4 };
    const lights = scene.lights || [];
    const L0 = lights[0] || { dir: [0, 1, 0], color: [1, 1, 1], intensity: 0 };
    const L1 = lights[1] || { dir: [0, 1, 0], color: [1, 1, 1], intensity: 0 };
    const d0 = norm32(L0.dir), d1 = norm32(L1.dir);
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
        xyzOff: xo * 4,
        scaleOff: so * 4,
        count: cloud.count,
        mesh: meshBufs[mid] || meshBufs[0]
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

// web/engine/scene.js
var PerspectiveCamera = class {
  constructor(fovy = Math.PI / 3, near = 0.1, far = 300) {
    this.fovy = fovy;
    this.near = near;
    this.far = far;
    this.eye = [0, 3, 12];
    this.target = [0, 0, -18];
  }
  setEye(x, y, z) {
    this.eye[0] = x;
    this.eye[1] = y;
    this.eye[2] = z;
  }
  setTarget(x, y, z) {
    this.target[0] = x;
    this.target[1] = y;
    this.target[2] = z;
  }
};
var InstanceCloud = class {
  constructor(capacity, color = [0.55, 0.58, 0.62]) {
    this.capacity = capacity;
    this.count = capacity;
    this.xyz = new Float32Array(capacity * 3);
    this.scale = new Float32Array(capacity);
    this.color = color;
    this.emissive = [0, 0, 0];
    this.metalness = 0.2;
    this.roughness = 0.5;
    this.meshId = 0;
    this.dirty = true;
  }
  set(i, x, y, z, s) {
    const o = i * 3;
    this.xyz[o] = x;
    this.xyz[o + 1] = y;
    this.xyz[o + 2] = z;
    this.scale[i] = s;
    this.dirty = true;
  }
  markClean() {
    this.dirty = false;
  }
};
var Scene = class {
  constructor() {
    this.clouds = /* @__PURE__ */ new Map();
    this.clearColor = [0.02, 0.024, 0.04, 1];
    this.fog = { density: 0.018, color: [0.02, 0.024, 0.04] };
    this.ambient = { color: [0.25, 0.38, 0.5], intensity: 0.55 };
    this.lights = [
      { dir: [0.35, 0.9, 0.25], color: [1, 0.9, 0.78], intensity: 1.15 },
      { dir: [-0.4, 0.2, -0.5], color: [0.4, 0.53, 1], intensity: 0.45 }
    ];
  }
  instances(name, capacity, color) {
    const c = new InstanceCloud(capacity, color);
    this.clouds.set(name, c);
    return c;
  }
  get(name) {
    return this.clouds.get(name);
  }
};

// web/engine/host-abi.js
var HOST_ABI_VERSION = 0;

// web/engine/packet.js
var PACKET_MAGIC = 1346721877;
var PACKET_VERSION = 3;
var MESH_OCTA2 = 0;
var MESH_SHIP2 = 1;
var DEFAULT_FOG = { density: 0.018, color: [0.02, 0.024, 0.04] };
var DEFAULT_AMBIENT = { color: [0.25, 0.38, 0.5], intensity: 0.55 };
var DEFAULT_LIGHTS = [
  { dir: [0.35, 0.9, 0.25], color: [1, 0.9, 0.78], intensity: 1.15 },
  { dir: [-0.4, 0.2, -0.5], color: [0.4, 0.53, 1], intensity: 0.45 }
];
function encodeRenderPacket(packet) {
  const clouds = packet.clouds || [];
  const fog = packet.fog || DEFAULT_FOG;
  const ambient = packet.ambient || DEFAULT_AMBIENT;
  const lights = packet.lights || DEFAULT_LIGHTS;
  let nbytes = 8 + 16 + 36 + 16 + 16 + 4 + lights.length * 28 + 4;
  for (const c of clouds) {
    nbytes += 12 + 12 + 4 + 4 + 4 + 4 + c.count * 3 * 4 + c.count * 4;
  }
  const buf = new ArrayBuffer(nbytes);
  const dv = new DataView(buf);
  let o = 0;
  dv.setUint32(o, PACKET_MAGIC, true);
  o += 4;
  dv.setUint32(o, PACKET_VERSION, true);
  o += 4;
  const clear = packet.clear || [0, 0, 0, 1];
  for (let i = 0; i < 4; i++) {
    dv.setFloat32(o, clear[i], true);
    o += 4;
  }
  const cam = packet.camera;
  const camf = [
    cam.fovy,
    cam.near,
    cam.far,
    cam.eye[0],
    cam.eye[1],
    cam.eye[2],
    cam.target[0],
    cam.target[1],
    cam.target[2]
  ];
  for (let i = 0; i < 9; i++) {
    dv.setFloat32(o, camf[i], true);
    o += 4;
  }
  dv.setFloat32(o, fog.density, true);
  o += 4;
  for (let i = 0; i < 3; i++) {
    dv.setFloat32(o, fog.color[i], true);
    o += 4;
  }
  for (let i = 0; i < 3; i++) {
    dv.setFloat32(o, ambient.color[i], true);
    o += 4;
  }
  dv.setFloat32(o, ambient.intensity, true);
  o += 4;
  dv.setUint32(o, lights.length, true);
  o += 4;
  for (const L of lights) {
    for (let i = 0; i < 3; i++) {
      dv.setFloat32(o, L.dir[i], true);
      o += 4;
    }
    for (let i = 0; i < 3; i++) {
      dv.setFloat32(o, L.color[i], true);
      o += 4;
    }
    dv.setFloat32(o, L.intensity, true);
    o += 4;
  }
  dv.setUint32(o, clouds.length, true);
  o += 4;
  for (const c of clouds) {
    for (let i = 0; i < 3; i++) {
      dv.setFloat32(o, c.color[i], true);
      o += 4;
    }
    const em = c.emissive || [0, 0, 0];
    for (let i = 0; i < 3; i++) {
      dv.setFloat32(o, em[i], true);
      o += 4;
    }
    dv.setFloat32(o, c.metalness ?? 0.35, true);
    o += 4;
    dv.setFloat32(o, c.roughness ?? 0.45, true);
    o += 4;
    dv.setUint32(o, c.meshId ?? MESH_OCTA2, true);
    o += 4;
    dv.setUint32(o, c.count, true);
    o += 4;
    for (let i = 0; i < c.count * 3; i++) {
      dv.setFloat32(o, c.xyz[i], true);
      o += 4;
    }
    for (let i = 0; i < c.count; i++) {
      dv.setFloat32(o, c.scale[i], true);
      o += 4;
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
    let emissive = [0, 0, 0], metalness = 0.2, roughness = 0.5, meshId = MESH_OCTA2;
    if (ver >= 3) {
      emissive = [dv.getFloat32(o, true), dv.getFloat32(o + 4, true), dv.getFloat32(o + 8, true)];
      o += 12;
      metalness = dv.getFloat32(o, true);
      o += 4;
      roughness = dv.getFloat32(o, true);
      o += 4;
      meshId = dv.getUint32(o, true);
      o += 4;
    }
    const count = dv.getUint32(o, true);
    o += 4;
    const xyz = new Float32Array(count * 3);
    for (let i = 0; i < count * 3; i++) {
      xyz[i] = dv.getFloat32(o, true);
      o += 4;
    }
    const scale = new Float32Array(count);
    for (let i = 0; i < count; i++) {
      scale[i] = dv.getFloat32(o, true);
      o += 4;
    }
    clouds.push({ color, emissive, metalness, roughness, meshId, count, xyz, scale });
  }
  return { clouds, o };
}
function decodeRenderPacket(raw) {
  const buf = ArrayBuffer.isView(raw) ? raw.buffer.slice(raw.byteOffset, raw.byteOffset + raw.byteLength) : raw;
  const dv = new DataView(buf);
  let o = 0;
  const magic = dv.getUint32(o, true);
  o += 4;
  if (magic !== PACKET_MAGIC) throw new Error("bad packet magic");
  const ver = dv.getUint32(o, true);
  o += 4;
  if (ver < 1 || ver > 3) throw new Error("bad packet version " + ver);
  const clear = [
    dv.getFloat32(o, true),
    dv.getFloat32(o + 4, true),
    dv.getFloat32(o + 8, true),
    dv.getFloat32(o + 12, true)
  ];
  o += 16;
  const fovy = dv.getFloat32(o, true);
  o += 4;
  const near = dv.getFloat32(o, true);
  o += 4;
  const far = dv.getFloat32(o, true);
  o += 4;
  const eye = [dv.getFloat32(o, true), dv.getFloat32(o + 4, true), dv.getFloat32(o + 8, true)];
  o += 12;
  const target = [dv.getFloat32(o, true), dv.getFloat32(o + 4, true), dv.getFloat32(o + 8, true)];
  o += 12;
  let fog = { ...DEFAULT_FOG, color: [...DEFAULT_FOG.color] };
  let ambient = { color: [...DEFAULT_AMBIENT.color], intensity: DEFAULT_AMBIENT.intensity };
  let lights = DEFAULT_LIGHTS.map((L) => ({
    dir: [...L.dir],
    color: [...L.color],
    intensity: L.intensity
  }));
  if (ver >= 2) {
    fog = {
      density: dv.getFloat32(o, true),
      color: [dv.getFloat32(o + 4, true), dv.getFloat32(o + 8, true), dv.getFloat32(o + 12, true)]
    };
    o += 16;
    ambient = {
      color: [dv.getFloat32(o, true), dv.getFloat32(o + 4, true), dv.getFloat32(o + 8, true)],
      intensity: dv.getFloat32(o + 12, true)
    };
    o += 16;
    const nLights = dv.getUint32(o, true);
    o += 4;
    lights = [];
    for (let i = 0; i < nLights; i++) {
      lights.push({
        dir: [dv.getFloat32(o, true), dv.getFloat32(o + 4, true), dv.getFloat32(o + 8, true)],
        color: [dv.getFloat32(o + 12, true), dv.getFloat32(o + 16, true), dv.getFloat32(o + 20, true)],
        intensity: dv.getFloat32(o + 24, true)
      });
      o += 28;
    }
  }
  const nClouds = dv.getUint32(o, true);
  o += 4;
  const { clouds } = readClouds(dv, o, nClouds, ver);
  return {
    version: ver,
    clear,
    camera: { fovy, near, far, eye, target },
    fog,
    ambient,
    lights,
    clouds
  };
}
function summarizeRenderPacket(packet) {
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
      yMax: yMax === -Infinity ? 0 : yMax
    };
  });
  return {
    clear: packet.clear ? Array.from(packet.clear) : null,
    eye: cam.eye ? Array.from(cam.eye) : null,
    target: cam.target ? Array.from(cam.target) : null,
    fogDensity: packet.fog?.density ?? 0,
    clouds
  };
}

// web/engine/input.js
var INPUT_MAGIC = 1313429589;
var INPUT_VERSION = 2;
var INPUT_BYTES_V1 = 20;
var INPUT_BYTES = 36;
var BTN_LEFT = 1;
var BTN_RIGHT = 2;
var BTN_MIDDLE = 4;
var FLAG_POINTER_IN = 1;
var FLAG_SUICIDE = 2;
var FLAG_TOUCH = 4;
var FLAG_LOOK_STICK = 8;
var STICK_DEADZONE = 0.28;
function axesFromDrag(dx, dy, dead = STICK_DEADZONE) {
  let ix = 0, iy = 0;
  if (dx <= -dead) ix = -1;
  else if (dx >= dead) ix = 1;
  if (dy <= -dead) iy = 1;
  else if (dy >= dead) iy = -1;
  return { ix, iy };
}
function analogFromDrag(dx, dy, dead = STICK_DEADZONE) {
  const reach = Math.max(dead + 0.05, 0.55);
  let x = 0, y = 0;
  if (Math.abs(dx) >= dead) x = Math.max(-1, Math.min(1, dx / reach));
  if (Math.abs(dy) >= dead) y = Math.max(-1, Math.min(1, dy / reach));
  return { x, y };
}
function encodeInputSnapshot(snap, out) {
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
  dv.setUint32(16, snap.fire >>> 0 || 0, true);
  if (ver >= 2) {
    dv.setFloat32(20, Number(snap.mx) || 0, true);
    dv.setFloat32(24, Number(snap.my) || 0, true);
    dv.setUint32(28, snap.buttons >>> 0 || 0, true);
    dv.setUint32(32, snap.flags >>> 0 || 0, true);
  }
  return buf;
}
function decodeInputSnapshot(raw) {
  const buf = ArrayBuffer.isView(raw) ? raw.buffer.slice(raw.byteOffset, raw.byteOffset + raw.byteLength) : raw;
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
    mx: 0,
    my: 0,
    buttons: 0,
    flags: 0
  };
  if (ver >= 2 && buf.byteLength >= INPUT_BYTES) {
    out.mx = dv.getFloat32(20, true);
    out.my = dv.getFloat32(24, true);
    out.buttons = dv.getUint32(28, true);
    out.flags = dv.getUint32(32, true);
  }
  return out;
}

// web/engine/browser-host.js
var INPUT_RING_CAP = 32;
async function createBrowserHost(canvas, opts = {}) {
  const baseURL = opts.baseURL || new URL(".", import.meta.url);
  const prefer = opts.prefer || "auto";
  let renderer = null;
  const tryGpu = prefer === "auto" || prefer === "webgpu";
  if (tryGpu) {
    try {
      renderer = await createWebGPURenderer(canvas);
    } catch (e) {
      console.warn("[uxe-host] webgpu init failed", e);
      renderer = null;
    }
    if (!renderer && prefer === "webgpu") {
      console.warn("[uxe-host] prefer=webgpu unavailable; falling back to webgl");
    }
  }
  if (!renderer) {
    renderer = createWebGLRenderer(canvas);
  }
  const scene = new Scene();
  const camera = new PerspectiveCamera();
  renderer.resize();
  addEventListener("resize", () => renderer.resize());
  canvas.style.touchAction = "none";
  canvas.style.userSelect = "none";
  canvas.style.webkitUserSelect = "none";
  const keys = /* @__PURE__ */ Object.create(null);
  const onKey = (down) => (e) => {
    keys[e.code] = down;
    if (e.code === "Space") e.preventDefault();
  };
  addEventListener("keydown", onKey(true));
  addEventListener("keyup", onKey(false));
  let mx = 0, my = 0, buttons = 0, flags = 0;
  let movAccX = 0, movAccY = 0;
  let pointerLockEnabled = opts.pointerLock === true;
  const ptrs = /* @__PURE__ */ new Map();
  function ndcFromEvent(e) {
    const r = canvas.getBoundingClientRect();
    if (r.width <= 0 || r.height <= 0) return null;
    const nx = (e.clientX - r.left) / r.width * 2 - 1;
    const ny = -((e.clientY - r.top) / r.height * 2 - 1);
    return {
      x: Math.max(-1, Math.min(1, nx)),
      y: Math.max(-1, Math.min(1, ny)),
      inside: e.clientX >= r.left && e.clientX <= r.right && e.clientY >= r.top && e.clientY <= r.bottom
    };
  }
  function syncPointerFromEvent(e) {
    const p = ndcFromEvent(e);
    if (!p) return;
    mx = p.x;
    my = p.y;
    flags = p.inside ? FLAG_POINTER_IN : 0;
  }
  const ptrOpts = { passive: false };
  canvas.addEventListener("pointermove", (e) => {
    if (document.pointerLockElement === canvas) {
      movAccX += e.movementX || 0;
      movAccY += e.movementY || 0;
      flags = FLAG_POINTER_IN;
      return;
    }
    const p = ndcFromEvent(e);
    if (!p) return;
    const rec = ptrs.get(e.pointerId);
    if (rec) {
      rec.x = p.x;
      rec.y = p.y;
    }
    if (!rec || !rec.touch) {
      mx = p.x;
      my = p.y;
      flags = p.inside ? FLAG_POINTER_IN : 0;
    } else {
      flags = FLAG_POINTER_IN;
    }
    if (e.cancelable) e.preventDefault();
  }, ptrOpts);
  canvas.addEventListener("pointerdown", (e) => {
    const isTouch = e.pointerType === "touch";
    if (pointerLockEnabled && !isTouch && document.pointerLockElement !== canvas) {
      canvas.requestPointerLock?.();
    }
    const p = ndcFromEvent(e);
    if (p) {
      ptrs.set(e.pointerId, {
        x: p.x,
        y: p.y,
        ox: p.x,
        oy: p.y,
        touch: isTouch
      });
      mx = p.x;
      my = p.y;
      flags = p.inside ? FLAG_POINTER_IN : 0;
    }
    canvas.setPointerCapture?.(e.pointerId);
    if (e.isPrimary !== false && (isTouch || e.button === 0 || e.button === -1 || e.buttons & 1)) {
      buttons |= BTN_LEFT;
    }
    if (e.button === 1) buttons |= BTN_MIDDLE;
    if (e.button === 2) buttons |= BTN_RIGHT;
    if (e.cancelable) e.preventDefault();
  }, ptrOpts);
  canvas.addEventListener("pointerup", (e) => {
    ptrs.delete(e.pointerId);
    const isTouch = e.pointerType === "touch";
    if (isTouch || e.button === 0 || e.button === -1) buttons &= ~BTN_LEFT;
    if (e.button === 1) buttons &= ~BTN_MIDDLE;
    if (e.button === 2) buttons &= ~BTN_RIGHT;
    if (ptrs.size === 0) buttons &= ~BTN_LEFT;
    else flags = FLAG_POINTER_IN;
    if (e.cancelable) e.preventDefault();
  }, ptrOpts);
  canvas.addEventListener("pointercancel", (e) => {
    ptrs.delete(e.pointerId);
    if (ptrs.size === 0) buttons &= ~BTN_LEFT;
  }, ptrOpts);
  canvas.addEventListener("pointerleave", () => {
    if (document.pointerLockElement !== canvas && ptrs.size === 0) flags = 0;
  });
  canvas.addEventListener("contextmenu", (e) => e.preventDefault());
  document.addEventListener("pointerlockchange", () => {
    if (document.pointerLockElement === canvas) flags = FLAG_POINTER_IN;
  });
  function touchSticks() {
    const list = [];
    for (const rec of ptrs.values()) {
      if (rec.touch) list.push(rec);
    }
    if (list.length === 0) return { move: null, look: null };
    if (list.length === 1) return { move: list[0], look: null };
    list.sort((a, b) => a.x - b.x);
    return { move: list[0], look: list[list.length - 1] };
  }
  let lastPacketClouds = 0;
  let lastPacketBytes = 0;
  let lastPacketSummary = null;
  const inputRing = [];
  let lastDebugSnap = null;
  function publishDebugSnap(extraInput) {
    const input = extraInput || lastSnap;
    const snap = {
      t: performance.now(),
      backend: renderer.backend,
      input: input ? {
        ix: input.ix,
        iy: input.iy,
        fire: input.fire,
        mx: input.mx,
        my: input.my,
        buttons: input.buttons,
        flags: input.flags
      } : null,
      inputRing: inputRing.slice(),
      uxe: typeof window !== "undefined" && window.__UXE__ || null,
      packet: lastPacketSummary
    };
    lastDebugSnap = snap;
    if (typeof window !== "undefined") window.__UXE_SNAP__ = snap;
    return snap;
  }
  function pushInputRing(snap) {
    inputRing.push({
      ix: snap.ix,
      iy: snap.iy,
      fire: snap.fire,
      mx: snap.mx,
      my: snap.my,
      buttons: snap.buttons,
      flags: snap.flags
    });
    while (inputRing.length > INPUT_RING_CAP) inputRing.shift();
  }
  function applyObjectPacket(packet) {
    const [r, g, b, a] = packet.clear || [0.02, 0.024, 0.04, 1];
    scene.clearColor = [r, g, b, a];
    const cam = packet.camera;
    camera.fovy = cam.fovy;
    camera.near = cam.near;
    camera.far = cam.far;
    camera.setEye(cam.eye[0], cam.eye[1], cam.eye[2]);
    camera.setTarget(cam.target[0], cam.target[1], cam.target[2]);
    if (packet.fog) scene.fog = packet.fog;
    if (packet.ambient) scene.ambient = packet.ambient;
    if (packet.lights) scene.lights = packet.lights;
    scene.clouds.clear();
    let i = 0;
    for (const c of packet.clouds || []) {
      const name = "c" + i++;
      const cloud = scene.instances(name, c.count, c.color);
      cloud.count = c.count;
      cloud.xyz = c.xyz;
      cloud.scale = c.scale;
      cloud.emissive = c.emissive || [0, 0, 0];
      cloud.metalness = c.metalness ?? 0.2;
      cloud.roughness = c.roughness ?? 0.5;
      cloud.meshId = c.meshId ?? 0;
      cloud.dirty = true;
    }
    lastPacketClouds = packet.clouds?.length || 0;
    lastPacketSummary = summarizeRenderPacket(packet);
    renderer.render(scene, camera);
    publishDebugSnap();
  }
  function stickAxes(dx, dy) {
    return axesFromDrag(dx, dy);
  }
  let lastSnap = null;
  return {
    version: HOST_ABI_VERSION,
    backend: renderer.backend,
    host_time() {
      return performance.now();
    },
    host_input_read(buf) {
      let ix = 0, iy = 0;
      if (keys.KeyA || keys.ArrowLeft) ix -= 1;
      if (keys.KeyD || keys.ArrowRight) ix += 1;
      if (keys.KeyS || keys.ArrowDown) iy += 1;
      if (keys.KeyW || keys.ArrowUp) iy -= 1;
      const fireKey = keys.Space ? 1 : 0;
      const { move, look } = touchSticks();
      const anyPtr = ptrs.size > 0;
      const contact = anyPtr || (buttons & BTN_LEFT) !== 0;
      const fireBtn = contact ? 1 : 0;
      let outMx = mx, outMy = my;
      let outFlags = flags;
      if (keys.KeyF) outFlags |= FLAG_SUICIDE;
      if (document.pointerLockElement === canvas) {
        outMx = Math.max(-1, Math.min(1, movAccX / 48));
        outMy = Math.max(-1, Math.min(1, -movAccY / 48));
        movAccX = 0;
        movAccY = 0;
      } else if (move || look) {
        outFlags |= FLAG_TOUCH;
        if (move && ix === 0 && iy === 0) {
          const a = stickAxes(move.x - move.ox, move.y - move.oy);
          ix = a.ix;
          iy = a.iy;
        }
        if (look) {
          const a = analogFromDrag(look.x - look.ox, look.y - look.oy);
          outMx = a.x;
          outMy = a.y;
          outFlags |= FLAG_LOOK_STICK;
        } else {
          outMx = 0;
          outMy = 0;
        }
      } else if (contact && ix === 0 && iy === 0) {
        const mouse = [...ptrs.values()].find((r) => !r.touch);
        if (mouse) {
          const a = stickAxes(mouse.x - mouse.ox, mouse.y - mouse.oy);
          ix = a.ix;
          iy = a.iy;
        }
      }
      const outButtons = contact ? buttons | BTN_LEFT : buttons;
      const snap = {
        ix,
        iy,
        fire: fireKey || fireBtn,
        mx: outMx,
        my: outMy,
        buttons: outButtons,
        flags: outFlags,
        keys: { ...keys },
        pointerLock: document.pointerLockElement === canvas
      };
      lastSnap = snap;
      pushInputRing(snap);
      publishDebugSnap(snap);
      if (buf != null) {
        const ab = buf instanceof ArrayBuffer ? buf : buf.buffer.slice(buf.byteOffset, buf.byteOffset + buf.byteLength);
        encodeInputSnapshot(snap, ab);
        return ab.byteLength >= INPUT_BYTES ? INPUT_BYTES : ab.byteLength >= 20 ? 20 : 0;
      }
      return snap;
    },
    host_frame_begin() {
    },
    host_gpu_submit(packetOrBuf) {
      let packet = packetOrBuf;
      if (packetOrBuf instanceof ArrayBuffer || ArrayBuffer.isView(packetOrBuf)) {
        lastPacketBytes = ArrayBuffer.isView(packetOrBuf) ? packetOrBuf.byteLength : packetOrBuf.byteLength;
        packet = decodeRenderPacket(packetOrBuf);
      } else {
        lastPacketBytes = 0;
      }
      applyObjectPacket(packet);
    },
    host_frame_present() {
      publishDebugSnap();
    },
    /** One-frame JSON for agents / CDP (also mirrored on window.__UXE_SNAP__). */
    host_debug_snapshot() {
      const input = this.host_input_read();
      return publishDebugSnap(input);
    },
    async host_asset_read(path) {
      const url = /^(https?:|data:)/i.test(path) || path.startsWith("/") ? path : new URL(path, baseURL).href;
      const r = await fetch(url);
      if (!r.ok) throw new Error("host_asset_read " + path + " " + r.status);
      return new Uint8Array(await r.arrayBuffer());
    },
    host_log(level, msg) {
      const line = `[uxe-host ${level}] ${msg}`;
      if (level === "error") console.error(line);
      else if (level === "warn") console.warn(line);
      else console.log(line);
    },
    host_request_frame(cb) {
      let fired = false;
      const once = (t) => {
        if (fired) return;
        fired = true;
        cb(typeof t === "number" ? t : performance.now());
      };
      requestAnimationFrame(once);
      setTimeout(once, 50);
    },
    setPointerLockEnabled(on) {
      pointerLockEnabled = !!on;
      if (!pointerLockEnabled && document.pointerLockElement === canvas) {
        document.exitPointerLock?.();
      }
    },
    _lastInput() {
      return lastSnap;
    },
    _stats() {
      return { clouds: lastPacketClouds, bytes: lastPacketBytes, backend: renderer.backend };
    }
  };
}
export {
  BTN_LEFT,
  BTN_MIDDLE,
  BTN_RIGHT,
  FLAG_LOOK_STICK,
  FLAG_POINTER_IN,
  FLAG_SUICIDE,
  FLAG_TOUCH,
  HOST_ABI_VERSION,
  INPUT_BYTES,
  MESH_BOX,
  MESH_OCTA2 as MESH_OCTA,
  MESH_SHIP2 as MESH_SHIP,
  createBrowserHost,
  decodeInputSnapshot,
  decodeRenderPacket,
  encodeInputSnapshot,
  encodeRenderPacket,
  summarizeRenderPacket
};

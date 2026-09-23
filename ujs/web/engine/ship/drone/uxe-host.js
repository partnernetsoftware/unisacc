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
  P[5] = -P[5];
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
var STICK_DEADZONE = 0.2;
function axesFromStick(mx, my, dead = STICK_DEADZONE) {
  let ix = 0, iy = 0;
  if (mx <= -dead) ix = -1;
  else if (mx >= dead) ix = 1;
  if (my <= -dead) iy = 1;
  else if (my >= dead) iy = -1;
  return { ix, iy };
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
  let primaryType = "mouse";
  let primaryId = -1;
  let pointerLockEnabled = opts.pointerLock === true;
  function syncPointerFromEvent(e) {
    const r = canvas.getBoundingClientRect();
    if (r.width <= 0 || r.height <= 0) return;
    const nx = (e.clientX - r.left) / r.width * 2 - 1;
    const ny = -((e.clientY - r.top) / r.height * 2 - 1);
    mx = Math.max(-1, Math.min(1, nx));
    my = Math.max(-1, Math.min(1, ny));
    const inside = e.clientX >= r.left && e.clientX <= r.right && e.clientY >= r.top && e.clientY <= r.bottom;
    flags = inside ? FLAG_POINTER_IN : 0;
  }
  const ptrOpts = { passive: false };
  canvas.addEventListener("pointermove", (e) => {
    if (document.pointerLockElement === canvas) {
      movAccX += e.movementX || 0;
      movAccY += e.movementY || 0;
      flags = FLAG_POINTER_IN;
      return;
    }
    if (primaryId >= 0 && e.pointerId !== primaryId) return;
    syncPointerFromEvent(e);
    if (e.cancelable) e.preventDefault();
  }, ptrOpts);
  canvas.addEventListener("pointerdown", (e) => {
    const isTouch = e.pointerType === "touch";
    if (pointerLockEnabled && !isTouch && document.pointerLockElement !== canvas) {
      canvas.requestPointerLock?.();
    }
    if (primaryId < 0 || e.pointerId === primaryId) {
      primaryId = e.pointerId;
      primaryType = e.pointerType || "mouse";
    }
    canvas.setPointerCapture?.(e.pointerId);
    if (document.pointerLockElement !== canvas) syncPointerFromEvent(e);
    if (e.isPrimary !== false && (isTouch || e.button === 0 || e.button === -1 || e.buttons & 1)) {
      buttons |= BTN_LEFT;
    }
    if (e.button === 1) buttons |= BTN_MIDDLE;
    if (e.button === 2) buttons |= BTN_RIGHT;
    if (e.cancelable) e.preventDefault();
  }, ptrOpts);
  canvas.addEventListener("pointerup", (e) => {
    if (document.pointerLockElement !== canvas) syncPointerFromEvent(e);
    const isTouch = e.pointerType === "touch";
    if (isTouch || e.button === 0 || e.button === -1) buttons &= ~BTN_LEFT;
    if (e.button === 1) buttons &= ~BTN_MIDDLE;
    if (e.button === 2) buttons &= ~BTN_RIGHT;
    if (e.pointerId === primaryId) {
      primaryId = -1;
      buttons &= ~BTN_LEFT;
    }
    if (e.cancelable) e.preventDefault();
  }, ptrOpts);
  canvas.addEventListener("pointercancel", (e) => {
    if (e.pointerId === primaryId) {
      primaryId = -1;
      buttons &= ~BTN_LEFT;
    }
  }, ptrOpts);
  canvas.addEventListener("pointerleave", () => {
    if (document.pointerLockElement !== canvas && primaryId < 0) flags = 0;
  });
  canvas.addEventListener("contextmenu", (e) => e.preventDefault());
  document.addEventListener("pointerlockchange", () => {
    if (document.pointerLockElement === canvas) flags = FLAG_POINTER_IN;
  });
  let lastPacketClouds = 0;
  let lastPacketBytes = 0;
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
    renderer.render(scene, camera);
  }
  function stickAxes(outMx, outMy) {
    return axesFromStick(outMx, outMy);
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
      const contact = primaryId >= 0 || (buttons & BTN_LEFT) !== 0;
      const fireBtn = contact ? 1 : 0;
      let outMx = mx, outMy = my;
      let outFlags = flags;
      if (keys.KeyF) outFlags |= FLAG_SUICIDE;
      if (document.pointerLockElement === canvas) {
        outMx = Math.max(-1, Math.min(1, movAccX / 48));
        outMy = Math.max(-1, Math.min(1, -movAccY / 48));
        movAccX = 0;
        movAccY = 0;
      }
      if (contact && ix === 0 && iy === 0 && document.pointerLockElement !== canvas) {
        const a = stickAxes(outMx, outMy);
        ix = a.ix;
        iy = a.iy;
      }
      if (primaryId >= 0 && primaryType === "touch") outFlags |= FLAG_TOUCH;
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
      requestAnimationFrame(cb);
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

// web/wasm_run.js
var TAG = { null: 0, bool: 1, i64: 2, f64: 3, str: 4, list: 5, dict: 6, fn: 7, tup: 8 };
var rt = null;
async function loadWasmBytes(spec) {
  if (spec instanceof ArrayBuffer) return new Uint8Array(spec);
  if (ArrayBuffer.isView(spec)) return new Uint8Array(spec.buffer, spec.byteOffset, spec.byteLength);
  if (typeof URL !== "undefined" && spec instanceof URL) {
    if (typeof process !== "undefined" && spec.protocol === "file:") {
      const fs = await import("fs");
      return fs.readFileSync(spec);
    }
    const r2 = await fetch(spec);
    if (!r2.ok) throw new Error("fetch wasm failed: " + spec + " (" + r2.status + ")");
    return new Uint8Array(await r2.arrayBuffer());
  }
  if (typeof spec !== "string") throw new Error("bootRuntime: need path, URL, or bytes");
  if (typeof process !== "undefined" && spec.indexOf("://") < 0) {
    const fs = await import("fs");
    const path = await import("path");
    const { fileURLToPath } = await import("url");
    const here = path.dirname(fileURLToPath(import.meta.url));
    const cand = [
      path.isAbsolute(spec) ? spec : null,
      path.join(here, spec),
      path.resolve(spec)
    ].filter(Boolean);
    for (const p of cand) {
      try {
        return fs.readFileSync(p);
      } catch (_) {
      }
    }
    throw new Error(
      "wasm not found: " + spec + " (run: python3 -m ujs web-build / npm run build)"
    );
  }
  const r = await fetch(spec);
  if (!r.ok) throw new Error("fetch wasm failed: " + spec + " (" + r.status + ")");
  return new Uint8Array(await r.arrayBuffer());
}
async function bootRuntime(wasmUrl = "ujs_full.wasm") {
  const buf = await loadWasmBytes(wasmUrl);
  const { instance } = await WebAssembly.instantiate(buf);
  if (typeof instance.exports.host_run !== "function") {
    throw new Error("not ujs_full.wasm (missing host_run); run: npm run build");
  }
  rt = { ex: instance.exports, mem: instance.exports.memory };
  return rt;
}
function ensureRt() {
  if (!rt) throw new Error("call bootRuntime() first");
  return rt;
}
function writeImage(image) {
  const { ex, mem } = ensureRt();
  const base = Number(ex.mem_base());
  const addr = Number(ex.host_prog_addr());
  const u8 = new Uint8Array(mem.buffer);
  u8.set(image, base + addr);
  ex.host_load_image();
}
function pyToHandle(ex, mem, v) {
  if (v === null || v === void 0) return ex.host_mk_null();
  if (typeof v === "boolean") return ex.host_mk_bool(v ? 1 : 0);
  if (typeof v === "number") {
    if (Number.isInteger(v) && Math.abs(v) <= Number.MAX_SAFE_INTEGER)
      return ex.host_mk_i64(BigInt(v));
    return ex.host_mk_f64(v);
  }
  if (typeof v === "bigint") return ex.host_mk_i64(v);
  if (typeof v === "string") {
    const bytes = new TextEncoder().encode(v);
    const scratch = 5e5;
    new Uint8Array(mem.buffer, Number(ex.mem_base()) + scratch, bytes.length).set(bytes);
    return ex.host_mk_str(scratch, bytes.length);
  }
  if (Array.isArray(v)) {
    if (typeof ex.host_mk_list !== "function")
      throw new Error("ujs_full.wasm too old for list bind; npm run build");
    const h = ex.host_mk_list(v.length);
    for (let i = 0; i < v.length; i++)
      ex.host_list_set(h, i, pyToHandle(ex, mem, v[i]));
    return h;
  }
  if (typeof v === "object") {
    if (typeof ex.host_mk_dict !== "function")
      throw new Error("ujs_full.wasm too old for dict bind; npm run build");
    const keys = Object.keys(v);
    const h = ex.host_mk_dict(keys.length);
    for (let i = 0; i < keys.length; i++) {
      const k = pyToHandle(ex, mem, keys[i]);
      const val = pyToHandle(ex, mem, v[keys[i]]);
      ex.host_dict_set(h, i, k, val);
    }
    return h;
  }
  throw new Error("unsupported global/local type: " + typeof v);
}
function readHandle(ex, mem, h) {
  if (!h) return null;
  const t = ex.tag_of_export(h);
  if (t === TAG.null) return null;
  if (t === TAG.bool) return !!new Uint8Array(mem.buffer)[Number(ex.mem_base()) + h + 4];
  if (t === TAG.i64) return Number(ex.i64_of_export(h));
  if (t === TAG.f64) return Number(ex.f64_of_export(h));
  if (t === TAG.str) {
    const p = Number(ex.str_ptr_export(h));
    const n = Number(ex.str_len_export(h));
    const base = Number(ex.mem_base());
    return new TextDecoder().decode(new Uint8Array(mem.buffer, base + p, n));
  }
  if (t === TAG.list && typeof ex.host_list_get === "function") {
    const n = Number(ex.host_len(h));
    const out = [];
    for (let i = 0; i < n; i++) out.push(readHandle(ex, mem, ex.host_list_get(h, i)));
    return out;
  }
  if (t === TAG.dict && typeof ex.host_dict_key === "function") {
    const n = Number(ex.host_len(h));
    const out = {};
    for (let i = 0; i < n; i++) {
      const k = readHandle(ex, mem, ex.host_dict_key(h, i));
      out[k] = readHandle(ex, mem, ex.host_dict_val(h, i));
    }
    return out;
  }
  return { tag: t, handle: h };
}
async function wasm_run(codeOrFn, globalsMap = {}, localsMap = {}) {
  const { ex, mem } = ensureRt();
  let image, blob, askTrace, askHeat;
  try {
    if (typeof codeOrFn === "string") {
      const { compile: compile2 } = await import("./compiler.js");
      ({ image, blob, askTrace, askHeat } = compile2(codeOrFn));
    } else if (codeOrFn && codeOrFn.image) {
      image = codeOrFn.image;
      blob = codeOrFn.blob || {};
    } else {
      return { err: { kind: "type", message: "need code string or fn image" } };
    }
  } catch (e) {
    return { err: { kind: "CompileError", message: String(e.message || e) } };
  }
  try {
    ex.host_reset();
    writeImage(image);
    const gnames = blob.globals || [];
    const lnames = blob.locals || [];
    for (const [k, v] of Object.entries(globalsMap || {})) {
      const ix = gnames.indexOf(k);
      if (ix >= 0) ex.host_set_global(ix, pyToHandle(ex, mem, v));
    }
    for (const [k, v] of Object.entries(localsMap || {})) {
      const ix = lnames.indexOf(k);
      if (ix >= 0) ex.host_set_local(ix, pyToHandle(ex, mem, v));
    }
    const h = ex.host_run();
    const ok = readHandle(ex, mem, h);
    const Lout = {};
    lnames.forEach((n, i) => {
      Lout[n] = readHandle(ex, mem, ex.host_get_local(i));
    });
    const Gout = {};
    gnames.forEach((n, i) => {
      Gout[n] = readHandle(ex, mem, ex.host_get_global(i));
    });
    return {
      ok,
      locals: Lout,
      globals: Gout,
      ic_stub: Number(ex.last_ic_stub_export()),
      askTrace: askTrace || [],
      askHeat: askHeat || {},
      imageBytes: image.length
    };
  } catch (e) {
    return { err: { kind: "Trap", message: String(e.message || e) } };
  }
}
function unwrap(r) {
  if (r && r.err) throw new Error((r.err.kind || "err") + ": " + (r.err.message || ""));
  return r.ok;
}

// web/engine/core-drone.js
var NT = 8;
var MAGAZINE = 2;
var LOCK_ALIGN = 0.965;
var LOCK_MAX_DIST = 85;
var LOCK_HOLD = 0.18;
var MSL_SPEED = 55;
var SUICIDE_PROX = 5.5;
var SUICIDE_BLAST = 18;
function basis(yaw, pitch) {
  const cp = Math.cos(pitch), sp = Math.sin(pitch);
  const cy = Math.cos(yaw), sy = Math.sin(yaw);
  const fx = cp * sy, fy = sp, fz = cp * cy;
  const rx = cy, ry = 0, rz = -sy;
  return { fx, fy, fz, rx, ry, rz };
}
function freshTargets() {
  const txs = [], tys = [], tzs = [], thp = [], tang = [];
  for (let i = 0; i < NT; i++) {
    const ang = i / NT * Math.PI * 2 + 0.35;
    const r = 18 + i % 3 * 7;
    tang.push(ang);
    txs.push(Math.sin(ang) * r);
    tys.push(5 + i % 3 * 2.5);
    tzs.push(Math.cos(ang) * r - 10);
    thp.push(1);
  }
  return { txs, tys, tzs, thp, tang };
}
function freshState() {
  return {
    px: 0,
    py: 12,
    pz: 32,
    score: 0,
    alive: 1,
    ammo: MAGAZINE,
    ...freshTargets()
  };
}
function bestLock(state, B) {
  let best = -1, bestAlign = LOCK_ALIGN;
  for (let i = 0; i < NT; i++) {
    if (state.thp[i] <= 0) continue;
    const dx = state.txs[i] - state.px;
    const dy = state.tys[i] - state.py;
    const dz = state.tzs[i] - state.pz;
    const dist = Math.sqrt(dx * dx + dy * dy + dz * dz);
    if (dist < 2.5 || dist > LOCK_MAX_DIST) continue;
    const inv = 1 / dist;
    const align = dx * inv * B.fx + dy * inv * B.fy + dz * inv * B.fz;
    if (align > bestAlign) {
      bestAlign = align;
      best = i;
    }
  }
  return best;
}
function faceTargetIdx(state, i) {
  const dx = state.txs[i] - state.px;
  const dy = state.tys[i] - state.py;
  const dz = state.tzs[i] - state.pz;
  const yaw = Math.atan2(dx, dz);
  const horiz = Math.sqrt(dx * dx + dz * dz) || 1;
  const pitch = Math.atan2(dy, horiz);
  return { yaw, pitch };
}
async function runDroneCore(host, opts) {
  host.host_log("info", "drone FP cockpit boot");
  let controls = opts.controls === "mouse" ? "mouse" : "keyboard";
  host.setPointerLockEnabled?.(controls === "mouse");
  const wasmBytes = await host.host_asset_read(opts.wasmUrl);
  await bootRuntime(wasmBytes.buffer.slice(
    wasmBytes.byteOffset,
    wasmBytes.byteOffset + wasmBytes.byteLength
  ));
  let fnImage, fnBlob;
  if (opts.precompiled?.image) {
    const img = opts.precompiled.image;
    fnImage = img instanceof Uint8Array ? img : new Uint8Array(img);
    fnBlob = opts.precompiled.blob || {};
  } else {
    if (true) {
      throw new Error("drone ship requires precompiled sim");
    }
    const { compile: compile2 } = await import("../compiler.js");
    const simText = new TextDecoder().decode(await host.host_asset_read(opts.simUrl));
    const compiled = compile2(simText);
    fnImage = compiled.image;
    fnBlob = compiled.blob;
  }
  let state = freshState();
  let yaw = Math.PI, pitch = -0.06;
  let lastFire = 0;
  let kills = 0;
  let lockIdx = -1;
  let lockTime = 0;
  let locked = false;
  let suicideArm = false;
  let endReason = "";
  let last = host.host_time();
  let accFrames = 0, lastHud = last;
  const inputBuf = new ArrayBuffer(INPUT_BYTES);
  let lastAbsMx = 0, lastAbsMy = 0, haveAbs = false;
  let missiles = [];
  let flash = 0;
  let tOrbit = 0;
  let forceFire = false;
  const gN = 64;
  const gXYZ = new Float32Array(gN * 3);
  const gS = new Float32Array(gN);
  let gi = 0;
  for (let gz = -4; gz <= 3; gz++) {
    for (let gx = -4; gx <= 3; gx++) {
      if (gi >= gN) break;
      gXYZ[gi * 3] = gx * 14;
      gXYZ[gi * 3 + 1] = 0;
      gXYZ[gi * 3 + 2] = gz * 14;
      gS[gi] = 13.2;
      gi++;
    }
  }
  async function stepWith(dt, ix, iy, B, thit, suicide, speedMul) {
    const r = await wasm_run({ image: fnImage, blob: fnBlob }, {
      px: state.px,
      py: state.py,
      pz: state.pz,
      score: state.score,
      alive: state.alive,
      ammo: state.ammo,
      ix,
      iy,
      dt,
      suicide,
      speed_mul: speedMul,
      fx: B.fx,
      fy: B.fy,
      fz: B.fz,
      rx: B.rx,
      ry: B.ry,
      rz: B.rz,
      txs: state.txs,
      tys: state.tys,
      tzs: state.tzs,
      thp: state.thp,
      thit
    }, {});
    if (r.err) {
      host.host_log("error", "ujs " + JSON.stringify(r.err));
      return;
    }
    const out = unwrap(r);
    state = {
      px: out.px,
      py: out.py,
      pz: out.pz,
      score: out.score,
      alive: out.alive,
      ammo: state.ammo,
      txs: out.txs,
      tys: out.tys,
      tzs: out.tzs,
      thp: out.thp,
      tang: state.tang
    };
    kills += out.nk || 0;
  }
  function orbitTargets(dt) {
    tOrbit += dt;
    for (let i = 0; i < NT; i++) {
      if (state.thp[i] <= 0) continue;
      state.tang[i] += dt * (0.22 + i % 3 * 0.04);
      const r = 18 + i % 3 * 7;
      state.txs[i] = Math.sin(state.tang[i]) * r;
      state.tzs[i] = Math.cos(state.tang[i]) * r - 10;
      state.tys[i] = 5 + i % 3 * 2.5 + Math.sin(tOrbit * 1.4 + i) * 0.6;
    }
  }
  function cockpitClouds(B) {
    const pieces = [
      [-0.62, -0.42, 0.88, 0.11],
      [0.62, -0.42, 0.88, 0.11],
      [0, -0.52, 0.82, 0.62],
      [-0.78, 0.05, 0.92, 0.09],
      [0.78, 0.05, 0.92, 0.09],
      [-0.45, 0.38, 0.9, 0.08],
      [0.45, 0.38, 0.9, 0.08],
      [0, 0.42, 0.95, 0.5],
      [-0.9, -0.15, 1.05, 0.07],
      [0.9, -0.15, 1.05, 0.07]
    ];
    const xyz = [], sc = [];
    for (const [lx, ly, lz, s] of pieces) {
      const wx = state.px + B.fx * lz + B.rx * lx;
      const wy = state.py + B.fy * lz + ly;
      const wz = state.pz + B.fz * lz + B.rz * lx;
      xyz.push(wx, wy, wz);
      sc.push(s);
    }
    return {
      color: suicideArm ? [0.18, 0.04, 0.04] : [0.04, 0.06, 0.08],
      count: sc.length,
      xyz: new Float32Array(xyz),
      scale: new Float32Array(sc),
      meshId: MESH_BOX,
      metalness: 0.65,
      roughness: 0.32,
      emissive: suicideArm ? [0.12, 0.01, 0.01] : [0.01, 0.025, 0.035]
    };
  }
  function buildPacket(B) {
    const aXYZ = [], aS = [], lockXYZ = [], lockS = [];
    for (let i = 0; i < NT; i++) {
      if (state.thp[i] <= 0) continue;
      const arr = locked && i === lockIdx ? lockXYZ : aXYZ;
      const sc = locked && i === lockIdx ? lockS : aS;
      arr.push(state.txs[i], state.tys[i], state.tzs[i]);
      sc.push(locked && i === lockIdx ? 2 : 1.4);
    }
    const eye = [state.px, state.py, state.pz];
    const target = [
      state.px + B.fx * 80,
      state.py + B.fy * 80,
      state.pz + B.fz * 80
    ];
    const sky = suicideArm ? [0.42, 0.22, 0.2, 1] : flash > 0 ? [0.55, 0.5, 0.35, 1] : [0.32, 0.46, 0.58, 1];
    const clouds = [
      {
        color: [0.2, 0.24, 0.18],
        count: gN,
        xyz: gXYZ,
        scale: gS,
        meshId: MESH_BOX,
        metalness: 0.08,
        roughness: 0.92,
        emissive: [0, 0, 0]
      },
      cockpitClouds(B)
    ];
    if (aS.length) {
      clouds.push({
        color: [0.78, 0.18, 0.1],
        count: aS.length,
        xyz: new Float32Array(aXYZ),
        scale: new Float32Array(aS),
        meshId: MESH_OCTA2,
        metalness: 0.25,
        roughness: 0.48,
        emissive: [0.18, 0.03, 0]
      });
    }
    if (lockS.length) {
      clouds.push({
        color: [0.15, 1, 0.4],
        count: lockS.length,
        xyz: new Float32Array(lockXYZ),
        scale: new Float32Array(lockS),
        meshId: MESH_OCTA2,
        metalness: 0.3,
        roughness: 0.3,
        emissive: [0.06, 0.4, 0.1]
      });
    }
    if (missiles.length) {
      const mXYZ = [], mS = [];
      for (const m of missiles) {
        mXYZ.push(m.x, m.y, m.z);
        mS.push(0.35);
      }
      clouds.push({
        color: [1, 0.85, 0.25],
        count: mS.length,
        xyz: new Float32Array(mXYZ),
        scale: new Float32Array(mS),
        meshId: MESH_OCTA2,
        metalness: 0.4,
        roughness: 0.25,
        emissive: [0.5, 0.35, 0.05]
      });
    }
    clouds.push({
      color: locked ? [0.15, 1, 0.45] : suicideArm ? [1, 0.35, 0.2] : [1, 0.92, 0.25],
      count: 1,
      xyz: new Float32Array([
        state.px + B.fx * 11,
        state.py + B.fy * 11,
        state.pz + B.fz * 11
      ]),
      scale: new Float32Array([locked ? 0.16 : 0.1]),
      meshId: MESH_BOX,
      metalness: 0.2,
      roughness: 0.4,
      emissive: locked ? [0.05, 0.45, 0.12] : [0.35, 0.28, 0.02]
    });
    return encodeRenderPacket({
      clear: sky,
      camera: { fovy: Math.PI / 2.2, near: 0.06, far: 280, eye, target },
      fog: {
        density: suicideArm ? 0.016 : 0.012,
        color: suicideArm ? [0.45, 0.28, 0.22] : [0.38, 0.5, 0.58]
      },
      ambient: { color: [0.42, 0.48, 0.55], intensity: 0.72 },
      lights: [
        { dir: [0.25, 1, 0.15], color: [1, 0.95, 0.85], intensity: 1.2 },
        { dir: [-0.4, 0.15, -0.35], color: [0.35, 0.45, 0.65], intensity: 0.42 }
      ],
      clouds
    });
  }
  function resetRun() {
    state = freshState();
    yaw = Math.PI;
    pitch = -0.06;
    kills = 0;
    lockIdx = -1;
    lockTime = 0;
    locked = false;
    suicideArm = false;
    endReason = "";
    missiles = [];
    flash = 0;
    tOrbit = 0;
    haveAbs = false;
  }
  function spawnMissile(ti, B) {
    missiles.push({
      x: state.px + B.fx * 1.2,
      y: state.py + B.fy * 1.2,
      z: state.pz + B.fz * 1.2,
      vx: B.fx * MSL_SPEED,
      vy: B.fy * MSL_SPEED,
      vz: B.fz * MSL_SPEED,
      life: 2.2,
      ti
    });
  }
  function updateMissiles(dt, thit) {
    const next = [];
    for (const m of missiles) {
      m.life -= dt;
      if (m.life <= 0) continue;
      if (m.ti >= 0 && state.thp[m.ti] > 0) {
        const dx = state.txs[m.ti] - m.x;
        const dy = state.tys[m.ti] - m.y;
        const dz = state.tzs[m.ti] - m.z;
        const d = Math.sqrt(dx * dx + dy * dy + dz * dz) || 1;
        const pull = 38 * dt;
        m.vx += dx / d * pull;
        m.vy += dy / d * pull;
        m.vz += dz / d * pull;
        const sp = Math.sqrt(m.vx * m.vx + m.vy * m.vy + m.vz * m.vz) || 1;
        const want = MSL_SPEED;
        m.vx = m.vx / sp * want;
        m.vy = m.vy / sp * want;
        m.vz = m.vz / sp * want;
        if (d < 2.2) {
          thit[m.ti] = 1;
          flash = 0.25;
          continue;
        }
      }
      m.x += m.vx * dt;
      m.y += m.vy * dt;
      m.z += m.vz * dt;
      let hit = false;
      for (let i = 0; i < NT; i++) {
        if (state.thp[i] <= 0) continue;
        const dx = state.txs[i] - m.x;
        const dy = state.tys[i] - m.y;
        const dz = state.tzs[i] - m.z;
        if (dx * dx + dy * dy + dz * dz < 4.5) {
          thit[i] = 1;
          flash = 0.25;
          hit = true;
          break;
        }
      }
      if (!hit) next.push(m);
    }
    missiles = next;
  }
  function armSuicideBlast(thit) {
    for (let j = 0; j < NT; j++) {
      if (state.thp[j] <= 0) continue;
      const ex = state.txs[j] - state.px;
      const ey = state.tys[j] - state.py;
      const ez = state.tzs[j] - state.pz;
      if (ex * ex + ey * ey + ez * ez < SUICIDE_BLAST * SUICIDE_BLAST) thit[j] = 1;
    }
  }
  const api = {
    faceNearest() {
      let best = -1, bestD = 1e9;
      for (let i = 0; i < NT; i++) {
        if (state.thp[i] <= 0) continue;
        const dx = state.txs[i] - state.px;
        const dy = state.tys[i] - state.py;
        const dz = state.tzs[i] - state.pz;
        const d = dx * dx + dy * dy + dz * dz;
        if (d < bestD) {
          bestD = d;
          best = i;
        }
      }
      if (best < 0) return false;
      const f = faceTargetIdx(state, best);
      yaw = f.yaw;
      pitch = Math.max(-1.1, Math.min(1.1, f.pitch));
      lockIdx = best;
      lockTime = LOCK_HOLD;
      locked = true;
      return true;
    },
    faceTarget(i) {
      if (i < 0 || i >= NT || state.thp[i] <= 0) return false;
      const f = faceTargetIdx(state, i);
      yaw = f.yaw;
      pitch = Math.max(-1.1, Math.min(1.1, f.pitch));
      lockIdx = i;
      lockTime = LOCK_HOLD;
      locked = true;
      return true;
    },
    /** Probe/helper: fire immediately if locked + ammo, else queue next tick. */
    fire() {
      if (state.alive && locked && state.ammo > 0 && !suicideArm && lockIdx >= 0) {
        spawnMissile(lockIdx, basis(yaw, pitch));
        state.ammo -= 1;
        locked = false;
        lockTime = 0;
        flash = 0.12;
        return { ok: true, ammo: state.ammo, missiles: missiles.length };
      }
      forceFire = true;
      return { ok: false, locked, ammo: state.ammo, suicideArm };
    },
    setSuicideArm(v) {
      suicideArm = !!v;
      return suicideArm;
    },
    /** Instant proximity blast for probe (armed suicide path). */
    detonateNow() {
      if (!state.alive) return false;
      suicideArm = true;
      let best = -1, bestD = 1e9;
      for (let i = 0; i < NT; i++) {
        if (state.thp[i] <= 0) continue;
        const dx = state.txs[i] - state.px;
        const dy = state.tys[i] - state.py;
        const dz = state.tzs[i] - state.pz;
        const d = dx * dx + dy * dy + dz * dz;
        if (d < bestD) {
          bestD = d;
          best = i;
        }
      }
      if (best >= 0) {
        state.px = state.txs[best];
        state.py = state.tys[best];
        state.pz = state.tzs[best];
      }
      const thit = Array(NT).fill(0);
      armSuicideBlast(thit);
      let nk = 0;
      for (let i = 0; i < NT; i++) {
        if (thit[i] && state.thp[i] > 0) {
          state.thp[i] = 0;
          nk++;
          state.score += 500;
        }
      }
      kills += nk;
      state.alive = 0;
      state.score += 150;
      endReason = state.thp.every((h) => h <= 0) ? "win" : "suicide";
      if (endReason === "win") state.score += 1e3;
      flash = 0.5;
      return { nk, endReason, kills, remaining: state.thp.filter((h) => h > 0).length };
    },
    reset: resetRun,
    getSnapshot() {
      return {
        ammo: state.ammo,
        kills,
        locked,
        lockIdx,
        suicideArm,
        remaining: state.thp.filter((h) => h > 0).length,
        alive: state.alive,
        endReason,
        missiles: missiles.length,
        px: state.px,
        py: state.py,
        pz: state.pz,
        yaw,
        pitch
      };
    }
  };
  if (typeof globalThis !== "undefined") globalThis.__DRONE_API__ = api;
  async function tick(now) {
    host.host_frame_begin();
    const dt = Math.min(0.05, (now - last) / 1e3);
    last = now;
    if (flash > 0) flash -= dt;
    let mx = 0, my = 0, buttons = 0, flags = 0, ix = 0, iy = 0, fire = 0;
    let keys = {};
    let pointerLock = false;
    const nIn = host.host_input_read(inputBuf);
    if (nIn === INPUT_BYTES) {
      const input = decodeInputSnapshot(inputBuf);
      ix = input.ix;
      iy = input.iy;
      fire = input.fire;
      mx = input.mx;
      my = input.my;
      buttons = input.buttons;
      flags = input.flags;
    }
    const snapObj = host.host_input_read();
    if (snapObj && typeof snapObj === "object") {
      keys = snapObj.keys || {};
      pointerLock = !!snapObj.pointerLock;
    }
    if (controls === "keyboard") {
      let lookX = 0, lookY = 0;
      if (keys.KeyJ || keys.ArrowLeft) lookX -= 1;
      if (keys.KeyL || keys.ArrowRight) lookX += 1;
      if (keys.KeyI || keys.ArrowUp) lookY -= 1;
      if (keys.KeyK || keys.ArrowDown) lookY += 1;
      ix = 0;
      iy = 0;
      if (keys.KeyA) ix -= 1;
      if (keys.KeyD) ix += 1;
      if (keys.KeyS) iy += 1;
      if (keys.KeyW) iy -= 1;
      fire = keys.Space ? 1 : 0;
      yaw += lookX * 2.4 * dt;
      pitch -= lookY * 2 * dt;
      haveAbs = false;
    } else if (pointerLock) {
      yaw += mx * 2.6;
      pitch += my * 2.2;
      haveAbs = false;
    } else if (flags & 1 || haveAbs) {
      if (haveAbs) {
        yaw += (mx - lastAbsMx) * 2.8;
        pitch += (my - lastAbsMy) * 2.4;
      }
      lastAbsMx = mx;
      lastAbsMy = my;
      haveAbs = true;
    }
    if (pitch > 1.2) pitch = 1.2;
    if (pitch < -1.2) pitch = -1.2;
    const B = basis(yaw, pitch);
    const fireEdge = fire && !lastFire || forceFire;
    forceFire = false;
    lastFire = fire;
    const wantSuicide = controls === "mouse" ? !!(flags & FLAG_SUICIDE) || !!(buttons & BTN_RIGHT) || !!keys.KeyF : !!keys.KeyF || !!(flags & FLAG_SUICIDE);
    if (wantSuicide && state.alive) suicideArm = true;
    const cand = bestLock(state, B);
    if (cand >= 0 && cand === lockIdx) lockTime += dt;
    else if (!(locked && lockIdx >= 0 && cand < 0)) {
      if (locked && lockIdx >= 0 && state.thp[lockIdx] > 0) {
        lockTime = Math.max(0, lockTime - dt * 0.5);
        if (lockTime <= 0) {
          locked = false;
          lockIdx = cand;
        }
      } else {
        lockIdx = cand;
        lockTime = 0;
        locked = false;
      }
    } else {
      lockIdx = cand;
      lockTime = 0;
      locked = false;
    }
    if (lockIdx >= 0 && lockTime >= LOCK_HOLD) locked = true;
    const thit = Array(NT).fill(0);
    let suicide = 0;
    const boost = !!(keys.ShiftLeft || keys.ShiftRight);
    let speedMul = (suicideArm ? 2.05 : 1) * (boost ? 1.35 : 1);
    if (state.alive && fireEdge && locked && state.ammo > 0 && !suicideArm) {
      spawnMissile(lockIdx, B);
      state.ammo -= 1;
      locked = false;
      lockTime = 0;
      flash = 0.12;
    }
    if (state.alive) updateMissiles(dt, thit);
    if (state.alive) orbitTargets(dt);
    if (state.alive && suicideArm) {
      for (let i = 0; i < NT; i++) {
        if (state.thp[i] <= 0) continue;
        const dx = state.txs[i] - state.px;
        const dy = state.tys[i] - state.py;
        const dz = state.tzs[i] - state.pz;
        const d2 = dx * dx + dy * dy + dz * dz;
        if (d2 < SUICIDE_PROX * SUICIDE_PROX) {
          armSuicideBlast(thit);
          suicide = 1;
          endReason = "suicide";
          flash = 0.5;
          break;
        }
      }
      if (!suicide && state.py <= 2.55 && iy >= 0) {
        armSuicideBlast(thit);
        suicide = 1;
        endReason = "suicide";
        flash = 0.5;
      }
    }
    if (!state.alive) {
      if (fireEdge) resetRun();
    } else {
      await stepWith(dt, ix, iy, B, thit, suicide, speedMul);
      const rem = state.thp.filter((h) => h > 0).length;
      if (rem === 0 && state.alive) {
        endReason = "win";
        state.alive = 0;
        state.score += 1e3;
      }
      if (!state.alive && !endReason) endReason = suicide ? "suicide" : "crash";
    }
    const remaining = state.thp.filter((h) => h > 0).length;
    host.host_gpu_submit(buildPacket(basis(yaw, pitch)));
    host.host_frame_present();
    accFrames++;
    if (now - lastHud >= 160) {
      const mode = !state.alive ? endReason === "win" ? "\u4EFB\u52A1\u5B8C\u6210 \u2014 \u5168\u6B7C" : endReason === "suicide" ? "\u81EA\u7206\u51FA\u51FB" : "\u5931\u8054" : suicideArm ? "\u6A21\u5F0F B\uFF1A\u81EA\u7206\u51B2\u649E \u2014 \u649E\u5411\u76EE\u6807\uFF01" : locked ? "\u6A21\u5F0F A\uFF1A\u5BFC\u5F39\u9501\u5B9A \u2014 \u5C04\u51FB\uFF01" : state.ammo <= 0 ? "\u5F39\u4ED3\u7A7A \u2014 F/\u53F3\u952E\u6B66\u88C5\u81EA\u7206" : "\u6A21\u5F0F A\uFF1A\u641C\u7D22\u9501\u5B9A\u76EE\u6807";
      opts.onHud?.({
        ready: true,
        drone: true,
        fp: true,
        abi: true,
        score: state.score,
        kills,
        remaining,
        alive: state.alive,
        ammo: state.ammo,
        magazine: MAGAZINE,
        locked,
        lockIdx,
        suicideArm,
        endReason,
        mode,
        controls,
        mx,
        my,
        buttons,
        flags,
        pointerLock,
        missiles: missiles.length,
        pointer: controls === "mouse",
        fps: accFrames * 1e3 / (now - lastHud)
      });
      accFrames = 0;
      lastHud = now;
    }
    host.host_request_frame(tick);
  }
  opts.onHud?.({
    ready: true,
    drone: true,
    fp: true,
    abi: true,
    score: 0,
    kills: 0,
    remaining: NT,
    alive: 1,
    ammo: MAGAZINE,
    magazine: MAGAZINE,
    locked: false,
    suicideArm: false,
    mode: "\u6A21\u5F0F A\uFF1A\u641C\u7D22\u9501\u5B9A\u76EE\u6807",
    controls,
    mx: 0,
    my: 0,
    buttons: 0,
    flags: 0,
    pointer: controls === "mouse",
    missiles: 0
  });
  host.host_request_frame(tick);
  return {
    nt: NT,
    magazine: MAGAZINE,
    api,
    getControls: () => controls,
    setControls(next) {
      controls = next === "mouse" ? "mouse" : "keyboard";
      host.setPointerLockEnabled?.(controls === "mouse");
      haveAbs = false;
    }
  };
}

// web/engine/ship/drone.embed.json
var drone_embed_default = { image: [232, 2, 0, 0, 9, 0, 45, 11, 29, 8, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 8, 1, 3, 26, 0, 0, 0, 0, 0, 0, 0, 8, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 8, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 8, 4, 3, 0, 0, 0, 0, 0, 0, 0, 0, 8, 5, 3, 0, 0, 0, 0, 0, 0, 0, 0, 8, 6, 9, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 45, 6, 21, 36, 193, 0, 0, 0, 5, 0, 0, 0, 0, 9, 2, 5, 1, 0, 0, 0, 9, 3, 5, 2, 0, 0, 0, 9, 4, 5, 3, 0, 0, 0, 9, 5, 5, 4, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 0, 0, 0, 9, 6, 5, 6, 0, 0, 0, 9, 0, 5, 7, 0, 0, 0, 9, 7, 5, 8, 0, 0, 0, 9, 8, 5, 9, 0, 0, 0, 9, 9, 5, 10, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 5, 11, 0, 0, 0, 7, 0, 33, 12, 39, 3, 0, 0, 0, 0, 0, 0, 0, 0, 9, 10, 45, 1, 13, 8, 3, 7, 3, 11, 9, 11, 8, 4, 7, 4, 11, 9, 12, 7, 3, 45, 2, 14, 4, 102, 102, 102, 102, 102, 102, 214, 63, 45, 2, 14, 8, 5, 7, 5, 11, 3, 26, 0, 0, 0, 0, 0, 0, 0, 9, 13, 45, 2, 14, 8, 2, 7, 2, 11, 9, 2, 9, 14, 7, 3, 45, 2, 14, 9, 15, 7, 4, 45, 2, 14, 45, 0, 12, 7, 2, 45, 2, 14, 9, 16, 45, 2, 14, 45, 0, 12, 10, 2, 9, 2, 11, 9, 3, 9, 12, 7, 3, 45, 2, 14, 4, 154, 153, 153, 153, 153, 153, 225, 63, 45, 2, 14, 7, 5, 45, 0, 12, 9, 17, 7, 4, 45, 2, 14, 45, 0, 12, 7, 2, 45, 2, 14, 9, 16, 45, 2, 14, 45, 0, 12, 10, 3, 9, 3, 11, 9, 4, 9, 18, 7, 3, 45, 2, 14, 9, 19, 7, 4, 45, 2, 14, 45, 0, 12, 7, 2, 45, 2, 14, 9, 16, 45, 2, 14, 45, 0, 12, 10, 4, 9, 4, 11, 9, 3, 4, 0, 0, 0, 0, 0, 0, 4, 64, 45, 5, 17, 36, 167, 1, 0, 0, 4, 0, 0, 0, 0, 0, 0, 4, 64, 10, 3, 9, 3, 11, 9, 3, 3, 55, 0, 0, 0, 0, 0, 0, 0, 19, 36, 198, 1, 0, 0, 3, 55, 0, 0, 0, 0, 0, 0, 0, 10, 3, 9, 3, 11, 3, 0, 0, 0, 0, 0, 0, 0, 0, 8, 1, 7, 1, 11, 7, 1, 7, 0, 45, 5, 17, 36, 93, 2, 0, 0, 9, 20, 7, 1, 45, 7, 26, 3, 1, 0, 0, 0, 0, 0, 0, 0, 45, 6, 21, 36, 69, 2, 0, 0, 9, 9, 7, 1, 45, 7, 26, 3, 0, 0, 0, 0, 0, 0, 0, 0, 19, 36, 69, 2, 0, 0, 9, 9, 7, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 45, 8, 27, 11, 7, 6, 3, 1, 0, 0, 0, 0, 0, 0, 0, 45, 0, 12, 8, 6, 7, 6, 11, 9, 5, 3, 244, 1, 0, 0, 0, 0, 0, 0, 45, 0, 12, 10, 5, 9, 5, 11, 7, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 45, 0, 12, 8, 1, 7, 1, 11, 35, 212, 1, 0, 0, 9, 21, 3, 1, 0, 0, 0, 0, 0, 0, 0, 45, 6, 21, 36, 145, 2, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 10, 1, 9, 1, 11, 9, 5, 3, 150, 0, 0, 0, 0, 0, 0, 0, 45, 0, 12, 10, 5, 9, 5, 11, 5, 0, 0, 0, 0, 9, 2, 5, 1, 0, 0, 0, 9, 3, 5, 2, 0, 0, 0, 9, 4, 5, 3, 0, 0, 0, 9, 5, 5, 4, 0, 0, 0, 9, 1, 5, 5, 0, 0, 0, 9, 6, 5, 6, 0, 0, 0, 9, 0, 5, 7, 0, 0, 0, 9, 7, 5, 8, 0, 0, 0, 9, 8, 5, 9, 0, 0, 0, 9, 9, 5, 10, 0, 0, 0, 7, 6, 5, 11, 0, 0, 0, 7, 0, 33, 12, 39, 12, 0, 0, 0, 2, 0, 0, 0, 112, 120, 0, 0, 2, 0, 0, 0, 112, 121, 0, 0, 2, 0, 0, 0, 112, 122, 0, 0, 5, 0, 0, 0, 115, 99, 111, 114, 101, 0, 0, 0, 5, 0, 0, 0, 97, 108, 105, 118, 101, 0, 0, 0, 4, 0, 0, 0, 97, 109, 109, 111, 3, 0, 0, 0, 116, 120, 115, 0, 3, 0, 0, 0, 116, 121, 115, 0, 3, 0, 0, 0, 116, 122, 115, 0, 3, 0, 0, 0, 116, 104, 112, 0, 2, 0, 0, 0, 110, 107, 0, 0, 1, 0, 0, 0, 110, 0, 0, 0, 0, 0, 0, 0], blob: { globals: ["txs", "alive", "px", "py", "pz", "score", "ammo", "tys", "tzs", "thp", "iy", "ix", "fy", "speed_mul", "fx", "rx", "dt", "ry", "fz", "rz", "thit", "suicide"], locals: ["n", "i", "speed", "thrust", "strafe", "climb", "nk"] } };

// web/engine/ship/drone-host-entry.js
function helpLine(controls) {
  return controls === "mouse" ? "\u9F20\u6807\u770B \xB7 WASD \u98DE \xB7 Shift \u52A0\u901F \xB7 \u9501\u5B9A\u540E\u70B9\u51FB/\u7A7A\u683C\u53D1\u5C04\uFF082\u53D1\uFF09\xB7 F/\u53F3\u952E\u6B66\u88C5\u81EA\u7206" : "IJKL \u770B \xB7 WASD \u98DE \xB7 Shift \u52A0\u901F \xB7 \u9501\u5B9A\u540E\u7A7A\u683C\u53D1\u5C04\uFF082\u53D1\uFF09\xB7 F \u6B66\u88C5\u81EA\u7206";
}
async function startDroneShip(cfg) {
  let controls = cfg.controls === "mouse" ? "mouse" : "keyboard";
  const host = await createBrowserHost(cfg.canvas, {
    prefer: cfg.prefer || "auto",
    baseURL: new URL(".", cfg.engineUrl),
    pointerLock: controls === "mouse"
  });
  window.__UXE_HOST__ = host;
  const engineUrl = cfg.engineUrl;
  const orig = host.host_asset_read.bind(host);
  host.host_asset_read = async (path) => {
    if (path === "ujs_full.wasm" || path === "engine.wasm" || path.endsWith("engine.wasm")) {
      const r = await fetch(engineUrl);
      if (!r.ok) throw new Error("fetch engine " + r.status);
      return new Uint8Array(await r.arrayBuffer());
    }
    return orig(path);
  };
  function paint(s) {
    window.__UXE__ = { ...s, backend: host.backend, ship: true, drone: true };
    if (!s.ready) return;
    cfg.hud.classList.toggle("armed", !!s.suicideArm);
    cfg.hud.classList.toggle("locked-on", !!s.locked && !s.suicideArm);
    cfg.reticle?.classList.toggle("lock", !!s.locked && !s.suicideArm);
    cfg.reticle?.classList.toggle("suicide", !!s.suicideArm);
    const ammoBar = "\u25AE".repeat(s.ammo || 0) + "\u25AF".repeat(Math.max(0, (s.magazine || 2) - (s.ammo || 0)));
    if (cfg.ammoEl) cfg.ammoEl.textContent = ammoBar;
    const modeLabel = s.controls === "mouse" ? "\u952E\u76D8+\u9F20\u6807" : "\u7EAF\u952E\u76D8";
    cfg.hud.innerHTML = `<b>\u65E0\u4EBA\u673A \xB7 \u7B2C\u4E00\u4EBA\u79F0\u9A7E\u8231</b> \xB7 ship-js<br>backend <b>${host.backend}</b> \xB7 fps <b>${(s.fps || 0).toFixed(0)}</b> \xB7 <b>${modeLabel}</b><br><span class="mode">${s.mode || ""}</span><br>\u5F39\u4ED3 <b>${ammoBar}</b> (${s.ammo}/${s.magazine})` + (s.missiles ? ` \xB7 \u5728\u9014 <b>${s.missiles}</b>` : "") + ` \xB7 \u51FB\u6BC1 <b>${s.kills || 0}</b> \xB7 \u654C <b>${s.remaining ?? "?"}</b><br>\u5F97\u5206 <b>${(s.score || 0).toFixed(0)}</b>` + (s.locked ? ` \xB7 <b class="lock">\u9501\u5B9A</b>` : "") + (s.suicideArm ? ` \xB7 <b class="warn">\u81EA\u7206\u5DF2\u6B66\u88C5</b>` : "") + `<br>` + (s.alive ? helpLine(s.controls) : `<span class="warn">${s.endReason === "win" ? "\u5168\u6B7C" : "\u4EFB\u52A1\u7ED3\u675F"} \u2014 \u7A7A\u683C\u518D\u51FA\u51FB</span>`);
  }
  const image = new Uint8Array(drone_embed_default.image);
  const api = await runDroneCore(host, {
    wasmUrl: "engine.wasm",
    precompiled: { image, blob: drone_embed_default.blob },
    controls,
    onHud: paint
  });
  window.__DRONE_API__ = api.api;
  window.__UXE_API__ = api;
  return {
    backend: host.backend,
    getControls: () => api.getControls(),
    setControls(next) {
      api.setControls(next);
      controls = api.getControls();
      cfg.onControls?.(controls);
    }
  };
}
export {
  startDroneShip
};

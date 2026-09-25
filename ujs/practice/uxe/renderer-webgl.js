/** UXE WebGL1 — multi-mesh instancing + fog/lights/material (UXEP v3). */
import { mat4, perspective, lookAt, mul } from "./math.js";
import { allMeshes } from "./meshes.js";

const VS = `#version 100
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

const FS = `#version 100
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

export function createWebGLRenderer(canvas) {
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
    canvas.width = (w * dpr) | 0;
    canvas.height = (h * dpr) | 0;
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
    },
  };
}

void allMeshes; // used

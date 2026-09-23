/** UXE WebGL1 backend — octahedron instancing. */
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

const VS = `#version 100
attribute vec3 aPos;
attribute vec3 aInst;
attribute float aScale;
uniform mat4 uVP;
uniform vec3 uColor;
varying vec3 vCol;
void main() {
  vec3 p = aPos * aScale + aInst;
  gl_Position = uVP * vec4(p, 1.0);
  float sh = 0.45 + 0.55 * aPos.y;
  vCol = uColor * sh;
}`;
const FS = `#version 100
precision mediump float;
varying vec3 vCol;
void main() { gl_FragColor = vec4(vCol, 1.0); }`;

function compile(gl, type, src) {
  const s = gl.createShader(type);
  gl.shaderSource(s, src);
  gl.compileShader(s);
  if (!gl.getShaderParameter(s, gl.COMPILE_STATUS))
    throw new Error(gl.getShaderInfoLog(s) || "shader");
  return s;
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

  const vbo = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, vbo);
  gl.bufferData(gl.ARRAY_BUFFER, MESH, gl.STATIC_DRAW);
  const vCount = MESH.length / 3;

  const ibo = gl.createBuffer();
  const sbo = gl.createBuffer();
  const aPos = gl.getAttribLocation(prog, "aPos");
  const aInst = gl.getAttribLocation(prog, "aInst");
  const aScale = gl.getAttribLocation(prog, "aScale");
  const uVP = gl.getUniformLocation(prog, "uVP");
  const uColor = gl.getUniformLocation(prog, "uColor");

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
    gl.uniform3fv(uColor, cloud.color);
    gl.bindBuffer(gl.ARRAY_BUFFER, vbo);
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

    ext.drawArraysInstancedANGLE(gl.TRIANGLES, 0, vCount, cloud.count);
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

      gl.useProgram(prog);
      gl.uniformMatrix4fv(uVP, false, VP);
      for (const cloud of scene.clouds.values()) {
        if (cloud.count > 0) drawCloud(cloud);
      }
    },
  };
}

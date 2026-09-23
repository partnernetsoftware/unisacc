/** Minimal instanced WebGL draw — no Three. Same sim contract as ../host.js */
export function createRawGl(canvas, N) {
  const gl = canvas.getContext("webgl", { antialias: true, powerPreference: "high-performance" });
  if (!gl) throw new Error("WebGL unavailable");

  const vs = `#version 100
attribute vec3 aPos;
attribute vec3 aInst; // x,y,z
attribute float aScale;
uniform mat4 uVP;
varying float vShade;
void main() {
  vec3 p = aPos * aScale + aInst;
  gl_Position = uVP * vec4(p, 1.0);
  vShade = 0.45 + 0.55 * aPos.y;
}`;
  const fs = `#version 100
precision mediump float;
varying float vShade;
void main() {
  gl_FragColor = vec4(0.55 * vShade, 0.58 * vShade, 0.62 * vShade, 1.0);
}`;

  function compile(type, src) {
    const s = gl.createShader(type);
    gl.shaderSource(s, src);
    gl.compileShader(s);
    if (!gl.getShaderParameter(s, gl.COMPILE_STATUS))
      throw new Error(gl.getShaderInfoLog(s) || "shader");
    return s;
  }
  const prog = gl.createProgram();
  gl.attachShader(prog, compile(gl.VERTEX_SHADER, vs));
  gl.attachShader(prog, compile(gl.FRAGMENT_SHADER, fs));
  gl.linkProgram(prog);
  if (!gl.getProgramParameter(prog, gl.LINK_STATUS))
    throw new Error(gl.getProgramInfoLog(prog) || "link");

  // unit icosa-ish (octahedron) mesh
  const mesh = new Float32Array([
    0, 1, 0,  1, 0, 0,  0, 0, 1,
    0, 1, 0,  0, 0, 1, -1, 0, 0,
    0, 1, 0, -1, 0, 0,  0, 0,-1,
    0, 1, 0,  0, 0,-1,  1, 0, 0,
    0,-1, 0,  0, 0, 1,  1, 0, 0,
    0,-1, 0, -1, 0, 0,  0, 0, 1,
    0,-1, 0,  0, 0,-1, -1, 0, 0,
    0,-1, 0,  1, 0, 0,  0, 0,-1,
  ]);
  const vCount = mesh.length / 3;
  const vbo = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, vbo);
  gl.bufferData(gl.ARRAY_BUFFER, mesh, gl.STATIC_DRAW);

  const inst = new Float32Array(N * 3);
  const scales = new Float32Array(N);
  const ibo = gl.createBuffer();
  const sbo = gl.createBuffer();

  const aPos = gl.getAttribLocation(prog, "aPos");
  const aInst = gl.getAttribLocation(prog, "aInst");
  const aScale = gl.getAttribLocation(prog, "aScale");
  const uVP = gl.getUniformLocation(prog, "uVP");

  // ANGLE_instanced_arrays
  const ext = gl.getExtension("ANGLE_instanced_arrays");
  if (!ext) throw new Error("need ANGLE_instanced_arrays");

  function resize() {
    const w = innerWidth || 800, h = innerHeight || 600;
    canvas.width = w * Math.min(devicePixelRatio, 2);
    canvas.height = h * Math.min(devicePixelRatio, 2);
    canvas.style.width = w + "px";
    canvas.style.height = h + "px";
    gl.viewport(0, 0, canvas.width, canvas.height);
  }
  resize();
  addEventListener("resize", resize);

  // column-major lookAt * perspective (hand-rolled, tiny)
  function perspective(fovy, aspect, near, far, out) {
    const f = 1 / Math.tan(fovy / 2);
    const nf = 1 / (near - far);
    out[0] = f / aspect; out[1] = 0; out[2] = 0; out[3] = 0;
    out[4] = 0; out[5] = f; out[6] = 0; out[7] = 0;
    out[8] = 0; out[9] = 0; out[10] = (far + near) * nf; out[11] = -1;
    out[12] = 0; out[13] = 0; out[14] = 2 * far * near * nf; out[15] = 0;
  }
  function lookAt(ex, ey, ez, cx, cy, cz, out) {
    let zx = ex - cx, zy = ey - cy, zz = ez - cz;
    let len = Math.hypot(zx, zy, zz) || 1;
    zx /= len; zy /= len; zz /= len;
    let xx = zy * 0 - zz * 1, xy = zz * 0 - zx * 0, xz = zx * 1 - zy * 0;
    len = Math.hypot(xx, xy, xz) || 1;
    xx /= len; xy /= len; xz /= len;
    const yx = zy * xz - zz * xy, yy = zz * xx - zx * xz, yz = zx * xy - zy * xx;
    out[0] = xx; out[1] = yx; out[2] = zx; out[3] = 0;
    out[4] = xy; out[5] = yy; out[6] = zy; out[7] = 0;
    out[8] = xz; out[9] = yz; out[10] = zz; out[11] = 0;
    out[12] = -(xx * ex + xy * ey + xz * ez);
    out[13] = -(yx * ex + yy * ey + yz * ez);
    out[14] = -(zx * ex + zy * ey + zz * ez);
    out[15] = 1;
  }
  function mul(a, b, out) {
    for (let c = 0; c < 4; c++) {
      const b0 = b[c * 4], b1 = b[c * 4 + 1], b2 = b[c * 4 + 2], b3 = b[c * 4 + 3];
      out[c * 4] = a[0] * b0 + a[4] * b1 + a[8] * b2 + a[12] * b3;
      out[c * 4 + 1] = a[1] * b0 + a[5] * b1 + a[9] * b2 + a[13] * b3;
      out[c * 4 + 2] = a[2] * b0 + a[6] * b1 + a[10] * b2 + a[14] * b3;
      out[c * 4 + 3] = a[3] * b0 + a[7] * b1 + a[11] * b2 + a[15] * b3;
    }
  }
  const P = new Float32Array(16), V = new Float32Array(16), VP = new Float32Array(16);

  // player as single draw (reuse mesh, 1 instance)
  const pInst = new Float32Array(3);
  const pScale = new Float32Array([0.7]);

  return {
    ok: true,
    draw(state) {
      const { xs, ys, zs, rs, px, py, pz } = state;
      const w = canvas.width, h = canvas.height;
      perspective(Math.PI / 3, w / h, 0.1, 300, P);
      const cx = px * 0.15, cy = py * 0.15 + 2.8, cz = pz + 11;
      lookAt(cx, cy, cz, px * 0.05, py * 0.05, pz - 18, V);
      mul(P, V, VP);

      for (let i = 0; i < N; i++) {
        inst[i * 3] = xs[i];
        inst[i * 3 + 1] = ys[i];
        inst[i * 3 + 2] = zs[i];
        scales[i] = rs[i];
      }
      pInst[0] = px; pInst[1] = py; pInst[2] = pz;

      gl.enable(gl.DEPTH_TEST);
      gl.clearColor(0.02, 0.024, 0.04, 1);
      gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
      gl.useProgram(prog);
      gl.uniformMatrix4fv(uVP, false, VP);

      gl.bindBuffer(gl.ARRAY_BUFFER, vbo);
      gl.enableVertexAttribArray(aPos);
      gl.vertexAttribPointer(aPos, 3, gl.FLOAT, false, 0, 0);
      ext.vertexAttribDivisorANGLE(aPos, 0);

      gl.bindBuffer(gl.ARRAY_BUFFER, ibo);
      gl.bufferData(gl.ARRAY_BUFFER, inst, gl.DYNAMIC_DRAW);
      gl.enableVertexAttribArray(aInst);
      gl.vertexAttribPointer(aInst, 3, gl.FLOAT, false, 0, 0);
      ext.vertexAttribDivisorANGLE(aInst, 1);

      gl.bindBuffer(gl.ARRAY_BUFFER, sbo);
      gl.bufferData(gl.ARRAY_BUFFER, scales, gl.DYNAMIC_DRAW);
      gl.enableVertexAttribArray(aScale);
      gl.vertexAttribPointer(aScale, 1, gl.FLOAT, false, 0, 0);
      ext.vertexAttribDivisorANGLE(aScale, 1);

      ext.drawArraysInstancedANGLE(gl.TRIANGLES, 0, vCount, N);

      // player (cyan-ish via scale hack: draw again with one instance)
      gl.bindBuffer(gl.ARRAY_BUFFER, ibo);
      gl.bufferData(gl.ARRAY_BUFFER, pInst, gl.DYNAMIC_DRAW);
      gl.bindBuffer(gl.ARRAY_BUFFER, sbo);
      gl.bufferData(gl.ARRAY_BUFFER, pScale, gl.DYNAMIC_DRAW);
      ext.drawArraysInstancedANGLE(gl.TRIANGLES, 0, vCount, 1);
    },
  };
}

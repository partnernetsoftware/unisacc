const vertexSource = `#version 300 es
void main() {
  vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
  gl_Position = vec4(2.0 * p - 1.0, 0.0, 1.0);
}`;

const fragmentSource = `#version 300 es
precision highp float;
uniform highp sampler2D image;
uniform ivec2 pixel;
out vec4 color;
void main() {
  color = vec4(texelFetch(image, pixel, 0).r, 0.0, 0.0, 1.0);
}`;

function compileProgram(gl) {
  const program = gl.createProgram();
  if (!program) return null;
  for (const [type, source] of [[gl.VERTEX_SHADER, vertexSource], [gl.FRAGMENT_SHADER, fragmentSource]]) {
    const shader = gl.createShader(type);
    if (!shader) { gl.deleteProgram(program); return null; }
    gl.shaderSource(shader, source);
    gl.compileShader(shader);
    const compiled = gl.getShaderParameter(shader, gl.COMPILE_STATUS);
    if (compiled) gl.attachShader(program, shader);
    gl.deleteShader(shader);
    if (!compiled) { gl.deleteProgram(program); return null; }
  }
  gl.linkProgram(program);
  if (!gl.getProgramParameter(program, gl.LINK_STATUS)) { gl.deleteProgram(program); return null; }
  return program;
}

export function createDepthTextureHost(gl, currentGeneration) {
  if (typeof gl.drawBuffers !== "function") throw new TypeError("DepthTexture requires WebGL2");
  const live = new Map();
  const maxSize = Math.min(2048, gl.getParameter(gl.MAX_TEXTURE_SIZE));
  const maxBytes = 32 * 1024 * 1024;
  let generation = 0;
  let reader = null;
  const pixel = new Uint8Array(4);
  const toggles = [gl.DEPTH_TEST, gl.STENCIL_TEST, gl.CULL_FACE, gl.BLEND, gl.SCISSOR_TEST,
    gl.DITHER, gl.RASTERIZER_DISCARD, gl.SAMPLE_ALPHA_TO_COVERAGE, gl.SAMPLE_COVERAGE];

  function bindings() {
    return [gl.getParameter(gl.DRAW_FRAMEBUFFER_BINDING), gl.getParameter(gl.READ_FRAMEBUFFER_BINDING)];
  }

  function restore(draw, read) {
    gl.bindFramebuffer(gl.DRAW_FRAMEBUFFER, draw);
    gl.bindFramebuffer(gl.READ_FRAMEBUFFER, read);
  }

  function layout(format, type) {
    if (format === 1027 && type === 1020) {
      return [gl.DEPTH24_STENCIL8, gl.DEPTH_STENCIL, gl.UNSIGNED_INT_24_8, gl.DEPTH_STENCIL_ATTACHMENT, 4];
    }
    if (format !== 1026) return null;
    if (type === 1012) return [gl.DEPTH_COMPONENT16, gl.DEPTH_COMPONENT, gl.UNSIGNED_SHORT, gl.DEPTH_ATTACHMENT, 2];
    if (type === 1014) return [gl.DEPTH_COMPONENT24, gl.DEPTH_COMPONENT, gl.UNSIGNED_INT, gl.DEPTH_ATTACHMENT, 4];
    if (type === 1015) return [gl.DEPTH_COMPONENT32F, gl.DEPTH_COMPONENT, gl.FLOAT, gl.DEPTH_ATTACHMENT, 4];
    return null;
  }

  function destroy(attachment) {
    gl.deleteTexture(attachment.texture);
    gl.deleteFramebuffer(attachment.framebuffer);
  }

  function allocate(width, height, format, type, storage) {
    const frame = bindings();
    const previousTexture = gl.getParameter(gl.TEXTURE_BINDING_2D);
    const unpack = gl.getParameter(gl.PIXEL_UNPACK_BUFFER_BINDING);
    const target = { width, height, format, type, size:width * height * storage[4],
      framebuffer:gl.createFramebuffer(), texture:gl.createTexture() };
    let complete = false;
    try {
      if (!target.framebuffer || !target.texture) return null;
      gl.bindBuffer(gl.PIXEL_UNPACK_BUFFER, null);
      gl.bindTexture(gl.TEXTURE_2D, target.texture);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
      gl.texImage2D(gl.TEXTURE_2D, 0, storage[0], width, height, 0, storage[1], storage[2], null);
      gl.bindFramebuffer(gl.FRAMEBUFFER, target.framebuffer);
      gl.framebufferTexture2D(gl.FRAMEBUFFER, storage[3], gl.TEXTURE_2D, target.texture, 0);
      gl.drawBuffers([gl.NONE]);
      gl.readBuffer(gl.NONE);
      complete = gl.checkFramebufferStatus(gl.FRAMEBUFFER) === gl.FRAMEBUFFER_COMPLETE &&
        gl.getError() === gl.NO_ERROR;
      return complete ? target : null;
    } finally {
      restore(...frame);
      gl.bindTexture(gl.TEXTURE_2D, previousTexture);
      gl.bindBuffer(gl.PIXEL_UNPACK_BUFFER, unpack);
      if (!complete) destroy(target);
    }
  }

  function deleteReader(state) {
    gl.deleteProgram(state.program);
    gl.deleteFramebuffer(state.frame);
    gl.deleteRenderbuffer(state.color);
    gl.deleteVertexArray(state.vao);
  }

  function makeReader() {
    const state = { program:compileProgram(gl), frame:gl.createFramebuffer(),
      color:gl.createRenderbuffer(), vao:gl.createVertexArray() };
    const saved = bindings();
    const renderbuffer = gl.getParameter(gl.RENDERBUFFER_BINDING);
    let ready = false;
    try {
      if (!state.program || !state.frame || !state.color || !state.vao) return null;
      gl.bindRenderbuffer(gl.RENDERBUFFER, state.color);
      gl.renderbufferStorage(gl.RENDERBUFFER, gl.RGBA8, 1, 1);
      gl.bindFramebuffer(gl.FRAMEBUFFER, state.frame);
      gl.framebufferRenderbuffer(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.RENDERBUFFER, state.color);
      gl.drawBuffers([gl.COLOR_ATTACHMENT0]);
      gl.readBuffer(gl.COLOR_ATTACHMENT0);
      ready = gl.checkFramebufferStatus(gl.FRAMEBUFFER) === gl.FRAMEBUFFER_COMPLETE &&
        gl.getError() === gl.NO_ERROR;
      if (!ready) return null;
      state.image = gl.getUniformLocation(state.program, "image");
      state.pixel = gl.getUniformLocation(state.program, "pixel");
      return state;
    } finally {
      gl.bindRenderbuffer(gl.RENDERBUFFER, renderbuffer);
      restore(...saved);
      if (!ready) deleteReader(state);
    }
  }

  function read(target, x, y) {
    if (!reader) reader = makeReader();
    if (!reader) return -14n;
    const framebuffer = bindings();
    const viewport = gl.getParameter(gl.VIEWPORT);
    const program = gl.getParameter(gl.CURRENT_PROGRAM);
    const vao = gl.getParameter(gl.VERTEX_ARRAY_BINDING);
    const pack = gl.getParameter(gl.PIXEL_PACK_BUFFER_BINDING);
    const packing = [gl.PACK_ROW_LENGTH, gl.PACK_SKIP_ROWS, gl.PACK_SKIP_PIXELS, gl.PACK_ALIGNMENT];
    const packState = packing.map(flag => gl.getParameter(flag));
    const mask = gl.getParameter(gl.COLOR_WRITEMASK);
    const active = gl.getParameter(gl.ACTIVE_TEXTURE);
    const enabled = toggles.map(flag => gl.isEnabled(flag));
    gl.activeTexture(gl.TEXTURE0);
    const texture = gl.getParameter(gl.TEXTURE_BINDING_2D);
    const sampler = gl.getParameter(gl.SAMPLER_BINDING);
    try {
      gl.bindFramebuffer(gl.FRAMEBUFFER, reader.frame);
      gl.viewport(0, 0, 1, 1);
      for (const flag of toggles) gl.disable(flag);
      gl.colorMask(true, true, true, true);
      gl.useProgram(reader.program);
      gl.bindVertexArray(reader.vao);
      gl.bindTexture(gl.TEXTURE_2D, target.texture);
      gl.bindSampler(0, null);
      gl.uniform1i(reader.image, 0);
      gl.uniform2i(reader.pixel, x, y);
      gl.drawArrays(gl.TRIANGLES, 0, 3);
      gl.bindBuffer(gl.PIXEL_PACK_BUFFER, null);
      packing.forEach((flag, index) => gl.pixelStorei(flag, index === 3 ? 1 : 0));
      gl.readPixels(0, 0, 1, 1, gl.RGBA, gl.UNSIGNED_BYTE, pixel);
      return gl.getError() === gl.NO_ERROR ? BigInt(pixel[0]) : -14n;
    } finally {
      gl.bindTexture(gl.TEXTURE_2D, texture);
      gl.bindSampler(0, sampler);
      gl.activeTexture(active);
      gl.bindBuffer(gl.PIXEL_PACK_BUFFER, pack);
      packing.forEach((flag, index) => gl.pixelStorei(flag, packState[index]));
      gl.bindVertexArray(vao);
      gl.useProgram(program);
      restore(...framebuffer);
      gl.viewport(...viewport);
      gl.colorMask(...mask);
      toggles.forEach((flag, index) => enabled[index] ? gl.enable(flag) : gl.disable(flag));
    }
  }

  function releaseAll() {
    for (const attachment of live.values()) destroy(attachment);
    live.clear();
    if (reader) deleteReader(reader);
    reader = null;
  }

  function submit(operation, identity, stamp, ...args) {
    if (gl.isContextLost()) return -6n;
    if (stamp < 1n || stamp > 4294967295n || stamp !== BigInt(currentGeneration())) return -3n;
    if (identity < 1n || identity > 1024n || operation < 1n || operation > 5n ||
        args.some(value => value < 0n || value > 4294967295n)) return -1n;
    const epoch = Number(stamp);
    if (generation !== epoch) {
      releaseAll();
      generation = epoch;
    }
    const op = Number(operation);
    const id = Number(identity);
    const [a, b, c, d] = args.map(Number);
    const old = live.get(id);
    if (op === 1 || op === 2) {
      if (op === 1 ? old || live.size >= 32 : !old || old.format !== c || old.type !== d) return -12n;
      const storage = layout(c, d);
      if (!storage || a < 1 || b < 1 || a > maxSize || b > maxSize) return -1n;
      if (old?.width === a && old.height === b) return 0n;
      let bytes = a * b * storage[4];
      for (const target of live.values()) bytes += target.size;
      if (bytes > maxBytes) return -12n;
      const next = allocate(a, b, c, d, storage);
      if (!next) return -14n;
      live.set(id, next);
      if (old) {
        const [draw, read] = bindings();
        if (draw === old.framebuffer || read === old.framebuffer) {
          restore(draw === old.framebuffer ? next.framebuffer : draw, read === old.framebuffer ? next.framebuffer : read);
          if (draw === old.framebuffer) gl.viewport(0, 0, a, b);
        }
        destroy(old);
      }
      return 0n;
    }
    if (!old) return -12n;
    if (op === 3) {
      gl.bindFramebuffer(gl.FRAMEBUFFER, old.framebuffer);
      gl.viewport(0, 0, old.width, old.height);
      return 0n;
    }
    if (op === 4) return a < old.width && b < old.height ? read(old, a, b) : -1n;
    const [draw, readFramebuffer] = bindings();
    restore(draw === old.framebuffer ? null : draw, readFramebuffer === old.framebuffer ? null : readFramebuffer);
    if (draw === old.framebuffer) gl.viewport(0, 0, gl.drawingBufferWidth, gl.drawingBufferHeight);
    destroy(old);
    live.delete(id);
    return 0n;
  }

  return { imports:{ host_depth_texture:submit }, dispose:releaseAll };
}

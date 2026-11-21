import { createGpuTargets } from "./gpu-target-abi.js";

const bufferBase = 2097152;
const bufferSize = 6291456;
const shaderRecordSize = 16;
const utf8 = new TextDecoder("utf-8", { fatal: true });

function readShaders(memory, offset) {
  if (offset < 0n || offset > BigInt(bufferSize - shaderRecordSize) || offset % 4n !== 0n) return null;
  const buffer = memory().buffer;
  const start = bufferBase + Number(offset);
  if (start + shaderRecordSize > buffer.byteLength) return null;
  const record = new DataView(buffer, start, shaderRecordSize);
  const shaders = [];
  for (const field of [0, 8]) {
    const address = record.getUint32(field, true);
    const bytes = record.getUint32(field + 4, true);
    if (bytes < 1 || bytes > 4096 || address + bytes > bufferSize ||
        bufferBase + address + bytes > buffer.byteLength) return null;
    try {
      const source = utf8.decode(new Uint8Array(buffer, bufferBase + address, bytes));
      if (source.includes("\0")) return null;
      shaders.push(source);
    } catch {
      return null;
    }
  }
  return shaders;
}

function compile(gl, type, source) {
  const shader = gl.createShader(type);
  if (!shader) return null;
  gl.shaderSource(shader, source);
  gl.compileShader(shader);
  if (gl.getShaderParameter(shader, gl.COMPILE_STATUS)) return shader;
  gl.deleteShader(shader);
  return null;
}

function copyProgram(gl, vertex, fragment) {
  const vs = compile(gl, gl.VERTEX_SHADER, vertex);
  const fs = compile(gl, gl.FRAGMENT_SHADER, fragment);
  let program = vs && fs && gl.createProgram();
  if (program) {
    gl.attachShader(program, vs);
    gl.attachShader(program, fs);
    gl.linkProgram(program);
    if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
      gl.deleteProgram(program);
      program = null;
    }
  }
  if (vs) gl.deleteShader(vs);
  if (fs) gl.deleteShader(fs);
  return program;
}

// The target interface is passed to createGpuSubmit. The same adapter exposes
// a separate Wasm import for sampling an existing 2D color attachment.
export function createGpuNativeSample(gl, currentGeneration, memory) {
  if (typeof gl.bindSampler !== "function") throw new TypeError("WebGL2 required");
  const base = createGpuTargets(gl);
  const descriptors = new Map();
  let generation = 0;
  let bound = [0, 0];
  const targets = {
    plan(packet, stamp) {
      const plan = base.plan(packet, stamp);
      return {
        ...plan,
        prepare() {
          plan.prepare();
          if (generation !== stamp) {
            descriptors.clear();
            bound = [0, 0];
            generation = stamp;
          }
        },
      };
    },
    execute(op, command, packet) {
      const status = base.execute(op, command, packet);
      if (status !== 0n) return status;
      if (op === 9) descriptors.set(command.id, command);
      if (op === 10) bound = command;
      if (op === 12) descriptors.delete(command[0]);
      return status;
    },
  };

  function copy(stamp, read, write, shader) {
    if (gl.isContextLost()) return -6n;
    if (stamp !== BigInt(currentGeneration())) return -3n;
    if (read < 1n || read > 1024n || write < 0n || write > 1024n || read === write) return -1n;
    if (stamp !== BigInt(generation)) return -12n;
    const source = descriptors.get(Number(read));
    const output = write === 0n ?
      { kind: 0, width: gl.drawingBufferWidth, height: gl.drawingBufferHeight } :
      descriptors.get(Number(write));
    if (source?.kind !== 0 || output?.kind !== 0 ||
        source.width !== output.width || source.height !== output.height) return -12n;
    const sources = readShaders(memory, shader);
    if (!sources) return -1n;
    const program = copyProgram(gl, ...sources);
    if (!program) return gl.isContextLost() ? -6n : -14n;
    const sampler = gl.getUniformLocation(program, "source");
    if (sampler === null) {
      gl.deleteProgram(program);
      return -14n;
    }
    const previousProgram = gl.getParameter(gl.CURRENT_PROGRAM);
    const viewport = gl.getParameter(gl.VIEWPORT);
    const colorMask = gl.getParameter(gl.COLOR_WRITEMASK);
    const activeUnit = gl.getParameter(gl.ACTIVE_TEXTURE);
    const toggles = [gl.BLEND, gl.DEPTH_TEST, gl.STENCIL_TEST, gl.CULL_FACE, gl.SCISSOR_TEST,
      gl.RASTERIZER_DISCARD, gl.SAMPLE_ALPHA_TO_COVERAGE, gl.SAMPLE_COVERAGE, gl.DITHER];
    const enabled = toggles.map(state => gl.isEnabled(state));
    gl.activeTexture(gl.TEXTURE0);
    const previousTexture = gl.getParameter(gl.TEXTURE_BINDING_2D);
    const previousSampler = gl.getParameter(gl.SAMPLER_BINDING);
    try {
      if (base.execute(10, [Number(read), 0]) !== 0n) return -14n;
      const texture = gl.getFramebufferAttachmentParameter(gl.FRAMEBUFFER,
        gl.COLOR_ATTACHMENT0, gl.FRAMEBUFFER_ATTACHMENT_OBJECT_NAME);
      if (!texture || base.execute(10, [Number(write), 0]) !== 0n) return -14n;
      for (const state of toggles) gl.disable(state);
      gl.colorMask(true, true, true, true);
      gl.viewport(0, 0, output.width, output.height);
      gl.useProgram(program);
      gl.bindTexture(gl.TEXTURE_2D, texture);
      gl.bindSampler(0, null);
      gl.uniform1i(sampler, 0);
      gl.drawArrays(gl.TRIANGLES, 0, 3);
      return gl.isContextLost() ? -6n : gl.getError() === gl.NO_ERROR ? 0n : -14n;
    } finally {
      gl.bindTexture(gl.TEXTURE_2D, previousTexture);
      gl.bindSampler(0, previousSampler);
      gl.activeTexture(activeUnit);
      gl.useProgram(previousProgram);
      gl.viewport(...viewport);
      gl.colorMask(...colorMask);
      for (const [i, state] of toggles.entries()) {
        if (enabled[i]) gl.enable(state);
        else gl.disable(state);
      }
      base.execute(10, bound);
      gl.deleteProgram(program);
    }
  }
  return { targets, copy };
}

import { applyRenderState, readRenderState } from "./gpu-state-abi.js";
const magic = 0x50455855;
const bufferBase = 2097152;
const bufferSize = 6291456;
const headerSize = 24;
const utf8 = new TextDecoder("utf-8", { fatal: true });

function readDraw(packet, at, span) {
  if (span < 24) return null;
  const word = field => packet.getUint32(at + field, true);
  const count = word(8);
  const vertexBytes = word(12);
  const fragmentBytes = word(16);
  const vertexSpan = Math.ceil(vertexBytes / 4) * 4;
  const fragmentSpan = Math.ceil(fragmentBytes / 4) * 4;
  const data = 24 + vertexSpan + fragmentSpan;
  if (count === 0 || count % 3 !== 0 || word(20) !== 0 ||
      vertexBytes < 1 || vertexBytes > 4096 || fragmentBytes < 1 || fragmentBytes > 4096 ||
      data + count * 28 !== span) return null;
  const text = (start, bytes, padded) => {
    const encoded = new Uint8Array(packet.buffer, packet.byteOffset + at + start, padded);
    if (encoded.subarray(bytes).some(value => value !== 0)) throw new Error("shader padding");
    return utf8.decode(encoded.subarray(0, bytes));
  };
  let vertex, fragment;
  try {
    vertex = text(24, vertexBytes, vertexSpan);
    fragment = text(24 + vertexSpan, fragmentBytes, fragmentSpan);
  } catch {
    return null;
  }
  const vertices = new Float32Array(count * 7);
  for (let i = 0; i < vertices.length; i++) {
    const value = packet.getFloat32(at + data + i * 4, true);
    if (!Number.isFinite(value)) return null;
    vertices[i] = value;
  }
  return { count, vertex, fragment, vertices };
}

function makeDraw(gl, draw) {
  const program = gl.createProgram();
  if (!program) return null;
  for (const [stage, source] of [[gl.VERTEX_SHADER, draw.vertex], [gl.FRAGMENT_SHADER, draw.fragment]]) {
    const shader = gl.createShader(stage);
    if (!shader) {
      gl.deleteProgram(program);
      return null;
    }
    gl.shaderSource(shader, source);
    gl.compileShader(shader);
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
      gl.deleteShader(shader);
      gl.deleteProgram(program);
      return null;
    }
    gl.attachShader(program, shader);
    gl.deleteShader(shader);
  }
  gl.bindAttribLocation(program, 0, "a_position");
  gl.bindAttribLocation(program, 1, "a_color");
  gl.linkProgram(program);
  if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
    gl.deleteProgram(program);
    return null;
  }
  const buffer = gl.createBuffer();
  if (!buffer) {
    gl.deleteProgram(program);
    return null;
  }
  return { program, buffer };
}

function runCommands(gl, commands, targets, packet) {
  const draws = new Map();
  try {
    for (const command of commands) {
      if (command[0] !== 4) continue;
      const setup = makeDraw(gl, command[1]);
      if (!setup) return gl.isContextLost() ? -6n : -7n;
      draws.set(command, setup);
    }
    for (const command of commands) {
      const [opcode, a, b, c, d] = command;
      if (opcode === 1) gl.viewport(a, b, c, d);
      if (opcode === 2) {
        gl.clearColor(a / 255, b / 255, c / 255, d / 255);
        gl.clear(gl.COLOR_BUFFER_BIT);
      }
      if (opcode === 3) {
        if (a === 1) gl.enable(gl.DEPTH_TEST);
        else gl.disable(gl.DEPTH_TEST);
        gl.depthFunc(gl.LESS);
        gl.depthMask(b === 1);
        if (c === 1) gl.enable(gl.BLEND);
        else gl.disable(gl.BLEND);
        gl.blendEquation(gl.FUNC_ADD);
        gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
      }
      if (opcode === 4) {
        const { program, buffer } = draws.get(command);
        gl.useProgram(program);
        gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
        gl.bufferData(gl.ARRAY_BUFFER, a.vertices, gl.STREAM_DRAW);
        gl.enableVertexAttribArray(0);
        gl.enableVertexAttribArray(1);
        gl.vertexAttribPointer(0, 3, gl.FLOAT, false, 28, 0);
        gl.vertexAttribPointer(1, 4, gl.FLOAT, false, 28, 12);
        gl.drawArrays(gl.TRIANGLES, 0, a.count);
        gl.disableVertexAttribArray(0);
        gl.disableVertexAttribArray(1);
        gl.bindBuffer(gl.ARRAY_BUFFER, null);
        gl.useProgram(null);
      }
      if (opcode === 5) {
        gl.clearDepth(a);
        gl.clear(gl.DEPTH_BUFFER_BIT);
      }
      if (opcode === 6) applyRenderState(gl, a);
      if (opcode === 7) {
        if (a) gl.enable(gl.SCISSOR_TEST);
        else gl.disable(gl.SCISSOR_TEST);
        gl.scissor(b, c, d, command[5]);
      }
      if (opcode === 8) {
        let bits = 0;
        if (a & 1) {
          gl.clearColor((b & 255) / 255, (b >>> 8 & 255) / 255, (b >>> 16 & 255) / 255, (b >>> 24) / 255);
          bits |= gl.COLOR_BUFFER_BIT;
        }
        if (a & 2) { gl.clearDepth(c); bits |= gl.DEPTH_BUFFER_BIT; }
        if (a & 4) { gl.clearStencil(d); bits |= gl.STENCIL_BUFFER_BIT; }
        gl.clear(bits);
      }
      if (opcode >= 9) {
        const result = targets.execute(opcode, a, packet);
        if (result !== 0n) return result;
      }
    }
    return gl.isContextLost() ? -6n : 0n;
  } finally {
    for (const { program, buffer } of draws.values()) {
      gl.deleteBuffer(buffer);
      gl.deleteProgram(program);
    }
  }
}

export function createGpuSubmit(gl, memory, generation, targets = null) {
  return (offset, length) => {
    if (offset < 0n || length < 24n || length > 65536n ||
        offset + length > BigInt(bufferSize) || offset % 4n !== 0n) return -1n;
    const start = bufferBase + Number(offset);
    const size = Number(length);
    const buffer = memory().buffer;
    if (start + size > buffer.byteLength) return -1n;
    const packet = new DataView(buffer, start, size);
    const word = at => packet.getUint32(at, true);
    if (word(0) !== magic || word(8) !== size || word(20) !== 0) return -1n;
    if (word(4) !== 5) return -2n;
    const stamp = word(12);
    if (stamp === 0 || stamp !== generation()) return -3n;
    const count = word(16);
    if (count > 256) return -4n;
    if (gl.isContextLost()) return -6n;
    const targetPlan = targets?.plan(packet, stamp);
    const commands = [];
    let at = headerSize;
    for (let i = 0; i < count; i++) {
      if (size - at < 8) return -4n;
      const opcode = word(at);
      const span = word(at + 4);
      if (span < 8 || span % 4 !== 0 || span > size - at) return -4n;
      const [drawWidth, drawHeight] = targetPlan ? targetPlan.size() : [gl.drawingBufferWidth, gl.drawingBufferHeight];
      if (opcode === 1) {
        if (span !== 24) return -4n;
        const x = word(at + 8);
        const y = word(at + 12);
        const width = word(at + 16);
        const height = word(at + 20);
        if (width === 0 || height === 0 || x + width > Math.min(8192, drawWidth) ||
            y + height > Math.min(8192, drawHeight)) return -4n;
        commands.push([opcode, x, y, width, height]);
      } else if (opcode === 2) {
        if (span !== 12) return -4n;
        const rgba = word(at + 8);
        commands.push([opcode, rgba & 255, rgba >>> 8 & 255, rgba >>> 16 & 255, rgba >>> 24]);
      } else if (opcode === 3) {
        if (span !== 20) return -4n;
        const state = [word(at + 8), word(at + 12), word(at + 16)];
        if (state.some(value => value > 1)) return -4n;
        commands.push([opcode, ...state]);
      } else if (opcode === 4) {
        const draw = readDraw(packet, at, span);
        if (!draw) return -4n;
        commands.push([opcode, draw]);
      } else if (opcode === 5) {
        if (span !== 12) return -4n;
        const depth = packet.getFloat32(at + 8, true);
        if (!Number.isFinite(depth) || depth < 0 || depth > 1) return -4n;
        commands.push([opcode, depth]);
      } else if (opcode === 6) {
        const state = readRenderState(gl, packet, at, span);
        if (typeof state === "bigint") return state;
        commands.push([opcode, state]);
      } else if (opcode === 7) {
        if (span !== 28 || word(at + 8) > 1) return -4n;
        const [x, y, width, height] = [12, 16, 20, 24].map(offset => word(at + offset));
        if (x + width > Math.min(8192, drawWidth) || y + height > Math.min(8192, drawHeight)) return -4n;
        commands.push([opcode, word(at + 8), x, y, width, height]);
      } else if (opcode === 8) {
        if (span !== 24) return -4n;
        const mask = word(at + 8), depth = packet.getFloat32(at + 16, true), stencil = word(at + 20);
        if (mask < 1 || mask > 7 || !Number.isFinite(depth) || depth < 0 || depth > 1 || stencil > 255) return -4n;
        commands.push([opcode, mask, word(at + 12), depth, stencil]);
      } else if (opcode >= 9 && opcode <= 12) {
        if (!targetPlan) return -13n;
        const target = targetPlan.read(opcode, at, span);
        if (typeof target === "bigint") return target;
        commands.push([opcode, target]);
      } else {
        return -5n;
      }
      at += span;
    }
    if (at !== size) return -4n;
    targetPlan?.prepare();
    return runCommands(gl, commands, targets, packet);
  };
}

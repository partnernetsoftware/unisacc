const commandLengths = [36, 16, 40, 12];
const lastId = 1024;
const byteBudget = 32 * 1024 * 1024;

function targetCost(target) {
  const { width, height, layers, colors, depth } = target;
  return width * height * (4 * layers * colors + 2 * depth);
}

function deleteTarget(gl, target) {
  for (const image of target.images) gl.deleteTexture(image);
  if (target.depthBuffer) gl.deleteRenderbuffer(target.depthBuffer);
  gl.deleteFramebuffer(target.frame);
}

function makeTarget(gl, descriptor) {
  const { kind, width, height, layers, colors, depth } = descriptor;
  const type = [gl.TEXTURE_2D, gl.TEXTURE_CUBE_MAP, gl.TEXTURE_2D_ARRAY, gl.TEXTURE_3D, gl.TEXTURE_2D][kind];
  const target = { ...descriptor, type, images: [], frame: gl.createFramebuffer(), depthBuffer: null };
  if (!target.frame) return null;
  for (let i = 0; i < colors; i++) {
    const image = gl.createTexture();
    if (!image) { deleteTarget(gl, target); return null; }
    target.images.push(image);
    gl.bindTexture(type, image);
    for (const parameter of [gl.TEXTURE_MIN_FILTER, gl.TEXTURE_MAG_FILTER]) gl.texParameteri(type, parameter, gl.NEAREST);
    for (const parameter of [gl.TEXTURE_WRAP_S, gl.TEXTURE_WRAP_T]) gl.texParameteri(type, parameter, gl.CLAMP_TO_EDGE);
    if (kind === 2 || kind === 3) {
      gl.texParameteri(type, gl.TEXTURE_WRAP_R, gl.CLAMP_TO_EDGE);
      gl.texImage3D(type, 0, gl.RGBA8, width, height, layers, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
    } else {
      for (let face = 0; face < (kind === 1 ? 6 : 1); face++) {
        const faceTarget = kind === 1 ? gl.TEXTURE_CUBE_MAP_POSITIVE_X + face : type;
        gl.texImage2D(faceTarget, 0, gl.RGBA, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
      }
    }
    gl.bindTexture(type, null);
  }
  if (depth) {
    target.depthBuffer = gl.createRenderbuffer();
    if (!target.depthBuffer) { deleteTarget(gl, target); return null; }
    gl.bindRenderbuffer(gl.RENDERBUFFER, target.depthBuffer);
    gl.renderbufferStorage(gl.RENDERBUFFER, gl.DEPTH_COMPONENT16, width, height);
    gl.bindRenderbuffer(gl.RENDERBUFFER, null);
  }
  if (gl.getError() !== gl.NO_ERROR) { deleteTarget(gl, target); return null; }
  return target;
}

export function createGpuTargets(gl) {
  const live = new Map();
  let bound = [0, 0];
  let generation = 0;
  const layered = typeof gl.framebufferTextureLayer === "function";
  const maxSize = Math.min(2048, gl.getParameter(gl.MAX_TEXTURE_SIZE), gl.getParameter(gl.MAX_RENDERBUFFER_SIZE));
  const maxCube = gl.getParameter(gl.MAX_CUBE_MAP_TEXTURE_SIZE);
  const maxColors = layered ? Math.min(gl.getParameter(gl.MAX_COLOR_ATTACHMENTS), gl.getParameter(gl.MAX_DRAW_BUFFERS)) : 1;
  const maxLayers = layered ? gl.getParameter(gl.MAX_ARRAY_TEXTURE_LAYERS) : 0;
  const max3d = layered ? gl.getParameter(gl.MAX_3D_TEXTURE_SIZE) : 0;

  function attach(id, layer) {
    const target = live.get(id);
    gl.bindFramebuffer(gl.FRAMEBUFFER, target?.frame ?? null);
    if (!target) return true;
    const { kind, type, images, depthBuffer } = target;
    const attachments = images.map((image, i) => {
      const slot = gl.COLOR_ATTACHMENT0 + i;
      if (kind === 2 || kind === 3) gl.framebufferTextureLayer(gl.FRAMEBUFFER, slot, image, 0, layer);
      else gl.framebufferTexture2D(gl.FRAMEBUFFER, slot, kind === 1 ? gl.TEXTURE_CUBE_MAP_POSITIVE_X + layer : type, image, 0);
      return slot;
    });
    if (layered) gl.drawBuffers(attachments);
    if (depthBuffer) gl.framebufferRenderbuffer(gl.FRAMEBUFFER, gl.DEPTH_ATTACHMENT, gl.RENDERBUFFER, depthBuffer);
    return gl.checkFramebufferStatus(gl.FRAMEBUFFER) === gl.FRAMEBUFFER_COMPLETE;
  }

  function execute(opcode, command, packet) {
    if (opcode === 9) {
      const target = makeTarget(gl, command);
      if (!target) return -14n;
      live.set(command.id, target);
      if (!attach(command.id, 0)) {
        deleteTarget(gl, target);
        live.delete(command.id);
        attach(...bound);
        return -14n;
      }
      attach(...bound);
    } else if (opcode === 10) {
      if (!attach(...command)) { attach(...bound); return -14n; }
      bound = command;
    } else if (opcode === 11) {
      const [id, layer, color, x, y, width, height, output] = command;
      if (!attach(id, layer)) { attach(...bound); return -14n; }
      if (layered) gl.readBuffer(gl.COLOR_ATTACHMENT0 + color);
      gl.readPixels(x, y, width, height, gl.RGBA, gl.UNSIGNED_BYTE,
        new Uint8Array(packet.buffer, 2097152 + output, width * height * 4));
      attach(...bound);
      if (gl.getError() !== gl.NO_ERROR) return -14n;
    } else {
      deleteTarget(gl, live.get(command[0]));
      live.delete(command[0]);
    }
    return 0n;
  }

  return {
    execute,
    plan(packet, stamp) {
      const reset = stamp !== generation;
      const planned = reset ? new Map() : new Map(live);
      let active = reset ? 0 : bound[0];
      const word = at => packet.getUint32(at, true);
      return {
        prepare() {
          if (!reset) return;
          attach(0, 0);
          for (const target of live.values()) deleteTarget(gl, target);
          live.clear();
          bound = [0, 0];
          generation = stamp;
        },
        size() {
          const target = planned.get(active);
          return target ? [target.width, target.height] : [gl.drawingBufferWidth, gl.drawingBufferHeight];
        },
        read(opcode, at, span) {
          if (span !== commandLengths[opcode - 9]) return -4n;
          const values = Array.from({ length: (span - 8) / 4 }, (_, i) => word(at + 8 + 4 * i));
          if (opcode === 9) {
            const [kind, id, width, height, layers, colors, depth] = values;
            if (kind > 4 || id < 1 || id > lastId || depth > 1 || planned.has(id) || planned.size >= 32) return -12n;
            if (kind >= 2 && !layered) return -13n;
            if (width < 1 || height < 1 || width > maxSize || height > maxSize ||
                (kind === 1 && (width !== height || width > maxCube)) ||
                (kind === 3 && (width > max3d || height > max3d)) ||
                (kind === 2 ? layers < 1 || layers > Math.min(64, maxLayers) :
                  kind === 3 ? layers < 1 || layers > Math.min(64, max3d) : layers !== (kind === 1 ? 6 : 1)) ||
                (kind === 4 ? colors < 2 || colors > Math.min(4, maxColors) : colors !== 1)) return -12n;
            const descriptor = { kind, id, width, height, layers, colors, depth };
            let bytes = targetCost(descriptor);
            for (const other of planned.values()) bytes += targetCost(other);
            if (bytes > byteBudget) return -12n;
            planned.set(id, descriptor);
            return descriptor;
          }
          const [id, layer, color, x, y, width, height, output] = values;
          const target = planned.get(id);
          if (opcode === 10) {
            if (id ? !target || layer >= target.layers : layer !== 0) return -12n;
            active = id;
          } else if (opcode === 11) {
            if (!target || layer >= target.layers || color >= target.colors || width < 1 || height < 1 ||
                x + width > target.width || y + height > target.height || output + width * height * 4 > 6291456) return -12n;
            const first = 2097152 + output;
            const last = first + width * height * 4;
            if (last > packet.buffer.byteLength || (first < packet.byteOffset + packet.byteLength && packet.byteOffset < last)) return -12n;
          } else {
            if (!target || active === id) return -12n;
            planned.delete(id);
          }
          return values;
        },
      };
    },
  };
}

const HOST_IMPORTS = new Set([
  "host_time", "host_input_read", "host_frame_present", "host_asset_read", "host_audio_write"
]);
const ABI_VERSION = 1;

function importsFor(module, namespace, available, allowed) {
  const imports = Object.create(null);
  for (const entry of WebAssembly.Module.imports(module)) {
    if (entry.kind !== "function" || entry.module !== namespace ||
        !allowed(entry.name) || typeof available[entry.name] !== "function") {
      throw new WebAssembly.LinkError("unsupported ABI import");
    }
    imports[entry.name] = available[entry.name];
  }
  return { [namespace]: imports };
}

function verifyVersion(instance) {
  const version = instance.exports.uxe_abi_version;
  if (typeof version !== "function" || version() !== ABI_VERSION) {
    throw new WebAssembly.LinkError("UXE ABI version");
  }
}

export function linkUxe(baseModule, productModule, host) {
  if (!(baseModule instanceof WebAssembly.Module) || !(productModule instanceof WebAssembly.Module)) {
    throw new WebAssembly.LinkError("missing UXE module");
  }
  const base = new WebAssembly.Instance(baseModule,
    importsFor(baseModule, "env", host, name => HOST_IMPORTS.has(name)));
  verifyVersion(base);
  const product = new WebAssembly.Instance(productModule,
    importsFor(productModule, "uxe_v1", base.exports, name => name === "uxe_dispatch"));
  verifyVersion(product);
  if (!(base.exports.memory instanceof WebAssembly.Memory) ||
      !(product.exports.memory instanceof WebAssembly.Memory) ||
      base.exports.memory === product.exports.memory) {
    throw new WebAssembly.LinkError("UXE memory ownership");
  }
  return { base: base.exports, product: product.exports };
}

export function createBrowserHost(canvas, keys, asset = new Uint8Array(), audioContext = null, pointerEnabled = false) {
  let memory, pressed = 0n, audio = audioContext, audioEnd = 0;
  let pointerId, pointerBits = 0n;
  const context = canvas.getContext("2d");
  const keyBits = new Map(keys.map((key, i) => [key, 1n << BigInt(i)]));
  if (keys.length > (pointerEnabled ? 32 : 63) || keyBits.size !== keys.length) throw new RangeError("input codes");
  const listeners = new AbortController();
  const options = { signal: listeners.signal };
  const keydown = event => {
    const bit = keyBits.get(event.code);
    if (bit !== undefined) {
      event.preventDefault();
      pressed |= bit;
    }
  };
  const keyup = event => {
    const bit = keyBits.get(event.code);
    if (bit !== undefined) pressed &= ~bit;
  };
  addEventListener("keydown", keydown, options);
  addEventListener("keyup", keyup, options);
  const clearPointer = () => { pointerId = undefined; pointerBits = 0n; };
  addEventListener("blur", () => { pressed = 0n; clearPointer(); }, options);
  if (pointerEnabled) {
    const sample = event => {
      if (event.pointerId !== pointerId) return;
      const bounds = canvas.getBoundingClientRect();
      const x = bounds.width ? (event.clientX - bounds.left) / bounds.width : 0.5;
      const quantized = Math.round(1024 * Math.max(0, Math.min(1, x)));
      pointerBits = (1n << 32n) | (BigInt(quantized) << 33n);
    };
    canvas.addEventListener("pointerdown", event => {
      if (pointerId !== undefined) return;
      pointerId = event.pointerId;
      canvas.setPointerCapture(pointerId);
      sample(event);
    }, options);
    canvas.addEventListener("pointermove", sample, options);
    canvas.addEventListener("lostpointercapture", event => {
      if (event.pointerId === pointerId) clearPointer();
    }, options);
  }
  const unlock = () => {
    if (!audio && typeof AudioContext === "function") audio = new AudioContext({ sampleRate: 48000 });
    if (audio?.state === "suspended") void audio.resume();
  };
  canvas.addEventListener("pointerdown", unlock, options);
  addEventListener("keydown", unlock, options);

  const span = (ptr, count) => {
    const buffer = memory.buffer;
    if (ptr < 0n || count < 0n || ptr + count > BigInt(buffer.byteLength)) {
      throw new WebAssembly.RuntimeError("host memory span");
    }
    return [buffer, Number(ptr), Number(count)];
  };
  return {
    attachMemory(owner) { memory = owner; },
    destroy() {
      listeners.abort();
      if (audio && audio !== audioContext) void audio.close();
    },
    host_time: () => performance.now(),
    host_input_read: () => pressed | pointerBits,
    host_frame_present(ptr, width, height) {
      if (width < 1n || height < 1n || width * height > 1048576n) return -1n;
      const pixels = new Uint8ClampedArray(...span(ptr, width * height * 4n));
      const w = Number(width), h = Number(height);
      if (canvas.width !== w) canvas.width = w;
      if (canvas.height !== h) canvas.height = h;
      context.putImageData(new ImageData(pixels, w, h), 0, 0);
      return width * height;
    },
    host_asset_read(offset, ptr, count) {
      if (offset < 0n || offset + count > BigInt(asset.byteLength)) return -1n;
      new Uint8Array(...span(ptr, count)).set(asset.subarray(Number(offset), Number(offset + count)));
      return count;
    },
    host_audio_write(ptr, count) {
      if (!audio || (!audioContext && audio.state !== "running")) return 0n;
      if (ptr % 4n || count < 1n || count > 48000n) return -1n;
      const [buffer, offset] = span(ptr, count * 4n);
      const samples = new Float32Array(buffer, offset, Number(count));
      const pcm = audio.createBuffer(1, samples.length, 48000);
      pcm.copyToChannel(samples, 0);
      const source = audio.createBufferSource();
      source.buffer = pcm;
      source.connect(audio.destination);
      audioEnd = Math.max(audio.currentTime, audioEnd);
      source.start(audioEnd);
      audioEnd += pcm.duration;
      return count;
    }
  };
}

async function fetchBytes(path) {
  const response = await fetch(path);
  if (!response.ok) throw new Error("UXE resource " + response.status);
  return response.arrayBuffer();
}

export async function boot(canvas) {
  const { uxeBase, uxeProduct, uxeAsset, uxeKeys = "", uxePointer } = canvas.dataset;
  const [base, product, asset] = await Promise.all([
    fetchBytes(uxeBase).then(WebAssembly.compile),
    fetchBytes(uxeProduct).then(WebAssembly.compile),
    uxeAsset ? fetchBytes(uxeAsset).then(bytes => new Uint8Array(bytes)) : new Uint8Array()
  ]);
  const host = createBrowserHost(canvas, uxeKeys.split(/\s+/).filter(Boolean), asset, null, uxePointer === "1");
  let pending;
  try {
    const linked = linkUxe(base, product, host);
    host.attachMemory(linked.base.memory);
    const frame = now => {
      try {
        linked.product.uxe_frame(BigInt(Math.round(now * 1000)));
        pending = requestAnimationFrame(frame);
      } catch (error) {
        stop();
        throw error;
      }
    };
    const stop = () => {
      cancelAnimationFrame(pending);
      removeEventListener("pagehide", stop);
      host.destroy();
    };
    addEventListener("pagehide", stop, { once: true });
    pending = requestAnimationFrame(frame);
  } catch (error) {
    host.destroy();
    throw error;
  }
}

const canvas = document.querySelector("canvas[data-uxe-base][data-uxe-product]");
if (canvas) void boot(canvas);

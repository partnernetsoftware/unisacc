/** Browser implementation of Host ABI v0. */
import { createWebGLRenderer } from "./renderer-webgl.js";
import { createWebGPURenderer } from "./renderer-webgpu.js";
import { Scene, PerspectiveCamera } from "./scene.js";
import { HOST_ABI_VERSION } from "./host-abi.js";
import { decodeRenderPacket } from "./packet.js";
import { encodeInputSnapshot, INPUT_BYTES } from "./input.js";

/**
 * @param {HTMLCanvasElement} canvas
 * @param {{ baseURL?: URL, prefer?: "auto" | "webgpu" | "webgl" }} [opts]
 * @returns {Promise<import("./host-abi.js").HostAbi & { backend: string, version: number }>}
 */
export async function createBrowserHost(canvas, opts = {}) {
  const baseURL = opts.baseURL || new URL(".", import.meta.url);
  // auto: WebGPU if available & healthy, else WebGL (both are first-class).
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

  const keys = Object.create(null);
  const onKey = (down) => (e) => {
    keys[e.code] = down;
    if (e.code === "Space") e.preventDefault();
  };
  addEventListener("keydown", onKey(true));
  addEventListener("keyup", onKey(false));

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
      const name = "c" + (i++);
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

  return {
    version: HOST_ABI_VERSION,
    backend: renderer.backend,

    host_time() {
      return performance.now();
    },

    /**
     * @param {ArrayBuffer|ArrayBufferView} [buf]
     * @returns {object|number} object if no buf; byte length written if buf
     */
    host_input_read(buf) {
      let ix = 0, iy = 0;
      if (keys.KeyA || keys.ArrowLeft) ix -= 1;
      if (keys.KeyD || keys.ArrowRight) ix += 1;
      // Chase cam sits above looking forward: raw +iy felt inverted on screen.
      if (keys.KeyS || keys.ArrowDown) iy += 1;
      if (keys.KeyW || keys.ArrowUp) iy -= 1;
      const snap = {
        ix, iy,
        fire: keys.Space ? 1 : 0,
        keys: { ...keys },
      };
      if (buf != null) {
        const ab = buf instanceof ArrayBuffer
          ? buf
          : buf.buffer.slice(buf.byteOffset, buf.byteOffset + buf.byteLength);
        encodeInputSnapshot(snap, ab);
        return INPUT_BYTES;
      }
      return snap;
    },

    host_frame_begin() {},

    /** Prefer ArrayBuffer (wasm-ready); object form kept for debug only. */
    host_gpu_submit(packetOrBuf) {
      let packet = packetOrBuf;
      if (packetOrBuf instanceof ArrayBuffer || ArrayBuffer.isView(packetOrBuf)) {
        lastPacketBytes = ArrayBuffer.isView(packetOrBuf)
          ? packetOrBuf.byteLength
          : packetOrBuf.byteLength;
        packet = decodeRenderPacket(packetOrBuf);
      } else {
        lastPacketBytes = 0;
      }
      applyObjectPacket(packet);
    },

    host_frame_present() {},

    async host_asset_read(path) {
      const url = /^(https?:|data:)/i.test(path) || path.startsWith("/")
        ? path
        : new URL(path, baseURL).href;
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

    _stats() {
      return { clouds: lastPacketClouds, bytes: lastPacketBytes, backend: renderer.backend };
    },
  };
}

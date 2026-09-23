/** Browser implementation of Host ABI v0 (+ UXIN v2 pointer). */
import { createWebGLRenderer } from "./renderer-webgl.js";
import { createWebGPURenderer } from "./renderer-webgpu.js";
import { Scene, PerspectiveCamera } from "./scene.js";
import { HOST_ABI_VERSION } from "./host-abi.js";
import { decodeRenderPacket } from "./packet.js";
import {
  encodeInputSnapshot, INPUT_BYTES,
  BTN_LEFT, BTN_RIGHT, BTN_MIDDLE, FLAG_POINTER_IN,
} from "./input.js";

/**
 * @param {HTMLCanvasElement} canvas
 * @param {{ baseURL?: URL, prefer?: "auto" | "webgpu" | "webgl" }} [opts]
 */
export async function createBrowserHost(canvas, opts = {}) {
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

  const keys = Object.create(null);
  const onKey = (down) => (e) => {
    keys[e.code] = down;
    if (e.code === "Space") e.preventDefault();
  };
  addEventListener("keydown", onKey(true));
  addEventListener("keyup", onKey(false));

  let mx = 0, my = 0, buttons = 0, flags = 0;
  let movAccX = 0, movAccY = 0;

  function syncPointerFromEvent(e) {
    const r = canvas.getBoundingClientRect();
    if (r.width <= 0 || r.height <= 0) return;
    const nx = ((e.clientX - r.left) / r.width) * 2 - 1;
    const ny = -(((e.clientY - r.top) / r.height) * 2 - 1);
    mx = Math.max(-1, Math.min(1, nx));
    my = Math.max(-1, Math.min(1, ny));
    const inside =
      e.clientX >= r.left && e.clientX <= r.right &&
      e.clientY >= r.top && e.clientY <= r.bottom;
    flags = inside ? FLAG_POINTER_IN : 0;
  }

  canvas.addEventListener("pointermove", (e) => {
    if (document.pointerLockElement === canvas) {
      movAccX += e.movementX || 0;
      movAccY += e.movementY || 0;
      flags = FLAG_POINTER_IN;
      return;
    }
    syncPointerFromEvent(e);
  });
  canvas.addEventListener("pointerdown", (e) => {
    if (document.pointerLockElement !== canvas) {
      canvas.requestPointerLock?.();
    }
    canvas.setPointerCapture?.(e.pointerId);
    if (document.pointerLockElement !== canvas) syncPointerFromEvent(e);
    if (e.button === 0) buttons |= BTN_LEFT;
    if (e.button === 1) buttons |= BTN_MIDDLE;
    if (e.button === 2) buttons |= BTN_RIGHT;
    e.preventDefault();
  });
  canvas.addEventListener("pointerup", (e) => {
    if (document.pointerLockElement !== canvas) syncPointerFromEvent(e);
    if (e.button === 0) buttons &= ~BTN_LEFT;
    if (e.button === 1) buttons &= ~BTN_MIDDLE;
    if (e.button === 2) buttons &= ~BTN_RIGHT;
  });
  canvas.addEventListener("pointerleave", () => {
    if (document.pointerLockElement !== canvas) flags = 0;
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

    host_input_read(buf) {
      let ix = 0, iy = 0;
      if (keys.KeyA || keys.ArrowLeft) ix -= 1;
      if (keys.KeyD || keys.ArrowRight) ix += 1;
      if (keys.KeyS || keys.ArrowDown) iy += 1;
      if (keys.KeyW || keys.ArrowUp) iy -= 1;
      const fireKey = keys.Space ? 1 : 0;
      const fireBtn = (buttons & BTN_LEFT) ? 1 : 0;
      let outMx = mx, outMy = my;
      let outFlags = flags;
      if (keys.KeyF) outFlags |= 2; // FLAG_SUICIDE
      if (document.pointerLockElement === canvas) {
        outMx = Math.max(-1, Math.min(1, movAccX / 48));
        outMy = Math.max(-1, Math.min(1, -movAccY / 48));
        movAccX = 0;
        movAccY = 0;
      }
      const snap = {
        ix, iy,
        fire: fireKey || fireBtn,
        mx: outMx, my: outMy, buttons, flags: outFlags,
        keys: { ...keys },
      };
      if (buf != null) {
        const ab = buf instanceof ArrayBuffer
          ? buf
          : buf.buffer.slice(buf.byteOffset, buf.byteOffset + buf.byteLength);
        encodeInputSnapshot(snap, ab);
        return ab.byteLength >= INPUT_BYTES ? INPUT_BYTES
          : (ab.byteLength >= 20 ? 20 : 0);
      }
      return snap;
    },

    host_frame_begin() {},

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

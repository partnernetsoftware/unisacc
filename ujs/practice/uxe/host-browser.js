/** Browser implementation of Host ABI v0 (+ UXIN v2 pointer / touch stick). */
import { createWebGLRenderer } from "./renderer-webgl.js";
import { createWebGPURenderer } from "./renderer-webgpu.js";
import { Scene, PerspectiveCamera } from "./scene.js";
import { HOST_ABI_VERSION } from "./host-abi.js";
import { decodeRenderPacket, summarizeRenderPacket } from "./packet.js";
import {
  encodeInputSnapshot, INPUT_BYTES,
  BTN_LEFT, BTN_RIGHT, BTN_MIDDLE,
  FLAG_POINTER_IN, FLAG_SUICIDE, FLAG_TOUCH, FLAG_LOOK_STICK,
  axesFromDrag, analogFromDrag,
} from "./input.js";

const INPUT_RING_CAP = 32;

/**
 * @param {HTMLCanvasElement} canvas
 * @param {{
 *   baseURL?: URL,
 *   prefer?: "auto" | "webgpu" | "webgl",
 *   pointerLock?: boolean,
 * }} [opts]
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

  // Mobile: keep gestures for the game, not the browser chrome.
  canvas.style.touchAction = "none";
  canvas.style.userSelect = "none";
  canvas.style.webkitUserSelect = "none";

  const keys = Object.create(null);
  const onKey = (down) => (e) => {
    keys[e.code] = down;
    if (e.code === "Space") e.preventDefault();
  };
  addEventListener("keydown", onKey(true));
  addEventListener("keyup", onKey(false));

  let mx = 0, my = 0, buttons = 0, flags = 0;
  let movAccX = 0, movAccY = 0;
  // Opt-in: FPS look (drone mouse). Asteroid / touch stay unlocked.
  let pointerLockEnabled = opts.pointerLock === true;
  /**
   * Active pointers. Touch: multi; mouse: one.
   * @type {Map<number, { x: number, y: number, ox: number, oy: number, touch: boolean }>}
   */
  const ptrs = new Map();

  function ndcFromEvent(e) {
    const r = canvas.getBoundingClientRect();
    if (r.width <= 0 || r.height <= 0) return null;
    const nx = ((e.clientX - r.left) / r.width) * 2 - 1;
    const ny = -(((e.clientY - r.top) / r.height) * 2 - 1);
    return {
      x: Math.max(-1, Math.min(1, nx)),
      y: Math.max(-1, Math.min(1, ny)),
      inside:
        e.clientX >= r.left && e.clientX <= r.right &&
        e.clientY >= r.top && e.clientY <= r.bottom,
    };
  }

  function syncPointerFromEvent(e) {
    const p = ndcFromEvent(e);
    if (!p) return;
    mx = p.x;
    my = p.y;
    flags = p.inside ? FLAG_POINTER_IN : 0;
  }

  const ptrOpts = { passive: false };

  canvas.addEventListener("pointermove", (e) => {
    if (document.pointerLockElement === canvas) {
      movAccX += e.movementX || 0;
      movAccY += e.movementY || 0;
      flags = FLAG_POINTER_IN;
      return;
    }
    const p = ndcFromEvent(e);
    if (!p) return;
    const rec = ptrs.get(e.pointerId);
    if (rec) {
      rec.x = p.x;
      rec.y = p.y;
    }
    if (!rec || !rec.touch) {
      mx = p.x;
      my = p.y;
      flags = p.inside ? FLAG_POINTER_IN : 0;
    } else {
      flags = FLAG_POINTER_IN;
    }
    if (e.cancelable) e.preventDefault();
  }, ptrOpts);

  canvas.addEventListener("pointerdown", (e) => {
    const isTouch = e.pointerType === "touch";
    if (
      pointerLockEnabled &&
      !isTouch &&
      document.pointerLockElement !== canvas
    ) {
      canvas.requestPointerLock?.();
    }
    const p = ndcFromEvent(e);
    if (p) {
      ptrs.set(e.pointerId, {
        x: p.x, y: p.y, ox: p.x, oy: p.y, touch: isTouch,
      });
      mx = p.x;
      my = p.y;
      flags = p.inside ? FLAG_POINTER_IN : 0;
    }
    canvas.setPointerCapture?.(e.pointerId);
    if (
      e.isPrimary !== false &&
      (isTouch || e.button === 0 || e.button === -1 || (e.buttons & 1))
    ) {
      buttons |= BTN_LEFT;
    }
    if (e.button === 1) buttons |= BTN_MIDDLE;
    if (e.button === 2) buttons |= BTN_RIGHT;
    if (e.cancelable) e.preventDefault();
  }, ptrOpts);

  canvas.addEventListener("pointerup", (e) => {
    ptrs.delete(e.pointerId);
    const isTouch = e.pointerType === "touch";
    if (isTouch || e.button === 0 || e.button === -1) buttons &= ~BTN_LEFT;
    if (e.button === 1) buttons &= ~BTN_MIDDLE;
    if (e.button === 2) buttons &= ~BTN_RIGHT;
    if (ptrs.size === 0) buttons &= ~BTN_LEFT;
    else flags = FLAG_POINTER_IN;
    if (e.cancelable) e.preventDefault();
  }, ptrOpts);

  canvas.addEventListener("pointercancel", (e) => {
    ptrs.delete(e.pointerId);
    if (ptrs.size === 0) buttons &= ~BTN_LEFT;
  }, ptrOpts);

  canvas.addEventListener("pointerleave", () => {
    if (document.pointerLockElement !== canvas && ptrs.size === 0) flags = 0;
  });
  canvas.addEventListener("contextmenu", (e) => e.preventDefault());
  document.addEventListener("pointerlockchange", () => {
    if (document.pointerLockElement === canvas) flags = FLAG_POINTER_IN;
  });

  /** Pick move (leftmost / only) and look (rightmost when ≥2) contacts. */
  function touchSticks() {
    const list = [];
    for (const rec of ptrs.values()) {
      if (rec.touch) list.push(rec);
    }
    if (list.length === 0) return { move: null, look: null };
    if (list.length === 1) return { move: list[0], look: null };
    list.sort((a, b) => a.x - b.x);
    return { move: list[0], look: list[list.length - 1] };
  }
  let lastPacketClouds = 0;
  let lastPacketBytes = 0;
  /** @type {ReturnType<typeof summarizeRenderPacket>} */
  let lastPacketSummary = null;
  /** @type {object[]} */
  const inputRing = [];
  /** @type {object|null} */
  let lastDebugSnap = null;

  function publishDebugSnap(extraInput) {
    const input = extraInput || lastSnap;
    const snap = {
      t: performance.now(),
      backend: renderer.backend,
      input: input
        ? {
          ix: input.ix, iy: input.iy, fire: input.fire,
          mx: input.mx, my: input.my, buttons: input.buttons, flags: input.flags,
        }
        : null,
      inputRing: inputRing.slice(),
      uxe: (typeof window !== "undefined" && window.__UXE__) || null,
      packet: lastPacketSummary,
    };
    lastDebugSnap = snap;
    if (typeof window !== "undefined") window.__UXE_SNAP__ = snap;
    return snap;
  }

  function pushInputRing(snap) {
    inputRing.push({
      ix: snap.ix, iy: snap.iy, fire: snap.fire,
      mx: snap.mx, my: snap.my, buttons: snap.buttons, flags: snap.flags,
    });
    while (inputRing.length > INPUT_RING_CAP) inputRing.shift();
  }

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
    lastPacketSummary = summarizeRenderPacket(packet);
    renderer.render(scene, camera);
    publishDebugSnap();
  }

  function stickAxes(dx, dy) {
    return axesFromDrag(dx, dy);
  }

  /** @type {object|null} */
  let lastSnap = null;

  return {
    version: HOST_ABI_VERSION,
    backend: renderer.backend,

    host_time() {
      return performance.now();
    },

    host_input_read(buf) {
      let ix = 0, iy = 0;
      // UXIN: A/← ix=-1 · D/→ ix=+1 · W/↑ iy=-1 · S/↓ iy=+1
      // (matches touch stick & drone thrust; asteroid sim negates iy→py)
      if (keys.KeyA || keys.ArrowLeft) ix -= 1;
      if (keys.KeyD || keys.ArrowRight) ix += 1;
      if (keys.KeyS || keys.ArrowDown) iy += 1;
      if (keys.KeyW || keys.ArrowUp) iy -= 1;
      const fireKey = keys.Space ? 1 : 0;
      const { move, look } = touchSticks();
      const anyPtr = ptrs.size > 0;
      const contact = anyPtr || (buttons & BTN_LEFT) !== 0;
      const fireBtn = contact ? 1 : 0;
      let outMx = mx, outMy = my;
      let outFlags = flags;
      if (keys.KeyF) outFlags |= FLAG_SUICIDE;
      if (document.pointerLockElement === canvas) {
        outMx = Math.max(-1, Math.min(1, movAccX / 48));
        outMy = Math.max(-1, Math.min(1, -movAccY / 48));
        movAccX = 0;
        movAccY = 0;
      } else if (move || look) {
        outFlags |= FLAG_TOUCH;
        if (move && ix === 0 && iy === 0) {
          const a = stickAxes(move.x - move.ox, move.y - move.oy);
          ix = a.ix;
          iy = a.iy;
        }
        if (look) {
          const a = analogFromDrag(look.x - look.ox, look.y - look.oy);
          outMx = a.x;
          outMy = a.y;
          outFlags |= FLAG_LOOK_STICK;
        } else {
          outMx = 0;
          outMy = 0;
        }
      } else if (contact && ix === 0 && iy === 0) {
        // mouse drag as move stick (asteroid)
        const mouse = [...ptrs.values()].find((r) => !r.touch);
        if (mouse) {
          const a = stickAxes(mouse.x - mouse.ox, mouse.y - mouse.oy);
          ix = a.ix;
          iy = a.iy;
        }
      }
      const outButtons = contact ? (buttons | BTN_LEFT) : buttons;

      const snap = {
        ix, iy,
        fire: fireKey || fireBtn,
        mx: outMx, my: outMy, buttons: outButtons, flags: outFlags,
        keys: { ...keys },
        pointerLock: document.pointerLockElement === canvas,
      };
      lastSnap = snap;
      pushInputRing(snap);
      publishDebugSnap(snap);
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

    host_frame_present() {
      publishDebugSnap();
    },

    /** One-frame JSON for agents / CDP (also mirrored on window.__UXE_SNAP__). */
    host_debug_snapshot() {
      // Refresh input so ring/flags match the call site, then return live snap.
      const input = this.host_input_read();
      return publishDebugSnap(input);
    },

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
      // rAF preferred; setTimeout fallback keeps the loop alive in headless
      // (macOS CVDisplayLink often fails without a display).
      let fired = false;
      const once = (t) => {
        if (fired) return;
        fired = true;
        cb(typeof t === "number" ? t : performance.now());
      };
      requestAnimationFrame(once);
      setTimeout(once, 50);
    },

    setPointerLockEnabled(on) {
      pointerLockEnabled = !!on;
      if (!pointerLockEnabled && document.pointerLockElement === canvas) {
        document.exitPointerLock?.();
      }
    },

    _lastInput() {
      return lastSnap;
    },

    _stats() {
      return { clouds: lastPacketClouds, bytes: lastPacketBytes, backend: renderer.backend };
    },
  };
}

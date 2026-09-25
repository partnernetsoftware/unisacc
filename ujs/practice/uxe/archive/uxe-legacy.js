
/** @deprecated Prefer createBrowserHost + core-*.js. Kept for release-artifacts. */
import { Scene, PerspectiveCamera } from "./scene.js";
import { createWebGLRenderer } from "./renderer-webgl.js";
import { createWebGPURenderer } from "./renderer-webgpu.js";

/**
 * Optional thin facade (not the Host-game path).
 * @param {HTMLCanvasElement} canvas
 * @param {{ prefer?: "auto" | "webgl" | "webgpu" }} [opts]
 */
export async function createEngine(canvas, opts = {}) {
  const prefer = opts.prefer || "auto";
  const scene = new Scene();
  const camera = new PerspectiveCamera();

  let renderer = null;
  if (prefer !== "webgl") {
    try {
      renderer = await createWebGPURenderer(canvas);
    } catch {
      renderer = null;
    }
  }
  if (!renderer) renderer = createWebGLRenderer(canvas);

  renderer.resize();
  addEventListener("resize", () => renderer.resize());

  return {
    scene,
    camera,
    renderer,
    backend: renderer.backend,
    resize: () => renderer.resize(),
    render: () => renderer.render(scene, camera),
  };
}

export { Scene, PerspectiveCamera, InstanceCloud } from "./scene.js";
export * as math from "./math.js";

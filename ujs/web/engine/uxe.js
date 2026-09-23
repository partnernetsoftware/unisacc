/** UXE public facade — createEngine(canvas). */
import { Scene, PerspectiveCamera } from "./scene.js";
import { createWebGLRenderer } from "./renderer-webgl.js";
import { createWebGPURenderer } from "./renderer-webgpu.js";

/**
 * @param {HTMLCanvasElement} canvas
 * @param {{ prefer?: "webgl" | "webgpu" }} [opts]
 */
export async function createEngine(canvas, opts = {}) {
  const prefer = opts.prefer || "webgpu";
  const scene = new Scene();
  const camera = new PerspectiveCamera();

  let renderer = null;
  if (prefer === "webgpu") {
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

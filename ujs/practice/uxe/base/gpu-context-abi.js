import { createGpuSubmit } from "./gpu-host-abi.js";
import { createGpuTargets } from "./gpu-target-abi.js";

export function createGpuContext(gl, memory, initialGeneration = 1) {
  if (!Number.isInteger(initialGeneration) || initialGeneration < 1 || initialGeneration > 0xffffffff) {
    throw new RangeError("invalid GPU generation");
  }
  const canvas = gl.canvas;
  let generation = initialGeneration;
  let lost = gl.isContextLost();
  const makeSubmit = () => createGpuSubmit(gl, memory, () => generation, createGpuTargets(gl));
  let submit = lost ? null : makeSubmit();

  function onLost(event) {
    event.preventDefault();
    lost = true;
    // Objects from the lost context cannot be used or explicitly recovered.
    submit = null;
  }

  function onRestored() {
    if (!lost || gl.isContextLost() || generation === 0xffffffff) return;
    const resumed = makeSubmit();
    generation++;
    submit = resumed;
    lost = false;
  }

  canvas.addEventListener("webglcontextlost", onLost);
  canvas.addEventListener("webglcontextrestored", onRestored);
  return {
    get generation() { return generation; },
    get ready() { return submit !== null && !gl.isContextLost(); },
    submit(offset, length) {
      if (submit === null || gl.isContextLost()) return -6n;
      return submit(offset, length);
    },
    detach() {
      canvas.removeEventListener("webglcontextlost", onLost);
      canvas.removeEventListener("webglcontextrestored", onRestored);
      submit = null;
    },
  };
}

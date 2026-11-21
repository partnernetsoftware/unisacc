export function bindTransformXInput(element, command) {
  const controller = new AbortController();
  const options = { signal: controller.signal };
  const send = (action, event) => {
    const bounds = element.getBoundingClientRect();
    return command(action, BigInt(event.pointerId),
      event.clientX - bounds.left, event.clientY - bounds.top);
  };

  element.addEventListener("pointerdown", event => {
    if (event.button === 0 && send(1n, event) > 0) element.setPointerCapture(event.pointerId);
  }, options);
  element.addEventListener("pointermove", event => send(event.buttons & 1 ? 2n : 0n, event), options);
  element.addEventListener("pointerup", event => {
    if (event.button === 0) send(3n, event);
  }, options);
  for (const type of ["pointercancel", "lostpointercapture"]) {
    element.addEventListener(type, event => send(3n, event), options);
  }
  return () => controller.abort();
}

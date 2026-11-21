export function bindOrbitInput(element, command) {
  const controller = new AbortController();
  const options = { signal: controller.signal };
  const touchAction = element.style.touchAction;
  element.style.touchAction = "none";

  const send = (event, action) => {
    const touch = event.pointerType === "touch";
    const shifted = event.ctrlKey || event.metaKey || event.shiftKey;
    const binding = action === 1 ? event.button + (shifted ? 3 : 0) : devicePixelRatio | 0;
    return command(BigInt(action + (touch ? 10 : 0)), BigInt(event.pointerId),
      event.pageX, event.pageY, BigInt(binding));
  };
  element.addEventListener("pointerdown", event => {
    if (send(event, 1) > 0) element.setPointerCapture(event.pointerId);
  }, options);
  element.addEventListener("pointermove", event => send(event, 2), options);
  for (const type of ["pointerup", "pointercancel", "lostpointercapture"]) {
    element.addEventListener(type, event => send(event, 3), options);
  }
  element.addEventListener("wheel", event => {
    if (command(4n, 0n, event.pageX, event.deltaY, BigInt(devicePixelRatio | 0)) > 0) {
      event.preventDefault();
    }
  }, { ...options, passive: false });
  element.addEventListener("contextmenu", event => event.preventDefault(), options);
  return () => {
    controller.abort();
    element.style.touchAction = touchAction;
  };
}

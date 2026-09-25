/**
 * UJS core — product API (closed script VM).
 * Not UXE: no canvas / WebGL / Host ABI here.
 */
export {
  bootRuntime,
  wasm_run,
  compile,
  unwrap,
  HOST_EXPORTS,
  TAG,
} from "./wasm_run.js";

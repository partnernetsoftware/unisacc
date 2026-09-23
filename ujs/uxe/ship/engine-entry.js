/**
 * Shared UXE ship engine — Host + GPU backends.
 * Built to engine.js; games import { createBrowserHost } from "./engine.js"
 * (or ../engine.js on Pages).
 */
export { createBrowserHost } from "../host-browser.js";
export { HOST_ABI_VERSION } from "../host-abi.js";
export {
  encodeInputSnapshot, decodeInputSnapshot, INPUT_BYTES,
  BTN_LEFT, BTN_RIGHT, BTN_MIDDLE,
  FLAG_POINTER_IN, FLAG_SUICIDE, FLAG_TOUCH, FLAG_LOOK_STICK,
} from "../input.js";
export {
  encodeRenderPacket, decodeRenderPacket, summarizeRenderPacket,
  MESH_OCTA, MESH_SHIP,
} from "../packet.js";
export { MESH_BOX } from "../meshes.js";

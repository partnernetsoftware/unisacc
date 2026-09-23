/** Host ABI v0 — contracts only (no browser types). */

/** @typedef {{
 *   clear: [number, number, number, number],
 *   camera: { fovy: number, near: number, far: number, eye: number[], target: number[] },
 *   clouds: Array<{ color: number[], count: number, xyz: Float32Array, scale: Float32Array }>
 * }} RenderPacket */

/** @typedef {{
 *   ix: number, iy: number, fire: number,
 *   keys: Record<string, boolean>
 * }} InputSnapshot */

/**
 * @typedef {object} HostAbi
 * @property {() => number} host_time
 * @property {(buf?: ArrayBuffer) => InputSnapshot|number} host_input_read
 *   no buf → InputSnapshot (JS compat); with buf → encode UXIN, return byteLength
 * @property {() => void} host_frame_begin
 * @property {() => void} host_frame_present
 * @property {(packet: RenderPacket) => void} host_gpu_submit
 * @property {(path: string) => Promise<Uint8Array>} host_asset_read
 * @property {(level: string, msg: string) => void} host_log
 * @property {(cb: (t: number) => void) => void} host_request_frame
 */

export const HOST_ABI_VERSION = 0;

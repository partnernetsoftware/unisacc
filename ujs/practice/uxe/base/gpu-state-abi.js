const equations = [32774, 32778, 32779, 32775, 32776];
const factors = [0, 1, 768, 769, 770, 771, 772, 773, 774, 775, 776, 32769, 32770, 32771, 32772];
const depths = [512, 519, 513, 515, 514, 518, 516, 517];
const stencilOps = new Set([0, 7680, 7681, 7682, 7683, 34055, 34056, 5386]);
const faces = [0, 1029, 1028, 1032];

export function readRenderState(gl, packet, at, span) {
  if (span !== 100) return -4n;
  const state = Array.from({ length: 23 }, (_, i) => packet.getUint32(at + 8 + i * 4, true));
  const [depth, depthWrite, blend, eqRgb, eqAlpha, srcRgb, dstRgb, srcAlpha, dstAlpha, ,
    stencil, ref, readMask, writeMask, fail, zFail, pass, cull, clockwise, offset, , , colorMask] = state;
  if (depth > 8 || depthWrite > 1) return -9n;
  if (blend > 1 || eqRgb < 100 || eqRgb > 104 || eqAlpha < 100 || eqAlpha > 104 ||
      [srcRgb, dstRgb, srcAlpha, dstAlpha].some(value => value < 200 || value > 214) ||
      [dstRgb, srcAlpha, dstAlpha].includes(210)) return -8n;
  const constantColor = value => value === 211 || value === 212;
  const constantAlpha = value => value === 213 || value === 214;
  if ((constantColor(srcRgb) && constantAlpha(dstRgb)) ||
      (constantAlpha(srcRgb) && constantColor(dstRgb))) return -8n;
  if ((eqRgb >= 103 || eqAlpha >= 103) &&
      gl.MIN === undefined && !gl.getExtension("EXT_blend_minmax")) return -8n;
  if ((stencil !== 0 && (stencil < 512 || stencil > 519 || !gl.getContextAttributes().stencil)) ||
      ref > 255 || readMask > 255 || writeMask > 255 ||
      [fail, zFail, pass].some(value => !stencilOps.has(value))) return -10n;
  if (cull > 3 || clockwise > 1) return -11n;
  state[20] = packet.getFloat32(at + 88, true);
  state[21] = packet.getFloat32(at + 92, true);
  if (offset > 1 || colorMask > 15 || !Number.isFinite(state[20]) || !Number.isFinite(state[21])) return -4n;
  return state;
}

export function applyRenderState(gl, state) {
  const [depth, depthWrite, blend, eqRgb, eqAlpha, srcRgb, dstRgb, srcAlpha, dstAlpha, color,
    stencil, ref, readMask, writeMask, fail, zFail, pass, cull, clockwise, offset, factor, units, mask] = state;
  const toggle = (capability, on) => on ? gl.enable(capability) : gl.disable(capability);
  toggle(gl.DEPTH_TEST, depth !== 8);
  if (depth !== 8) gl.depthFunc(depths[depth]);
  gl.depthMask(depthWrite !== 0);
  toggle(gl.BLEND, blend !== 0);
  gl.blendEquationSeparate(equations[eqRgb - 100], equations[eqAlpha - 100]);
  gl.blendColor((color & 255) / 255, (color >>> 8 & 255) / 255, (color >>> 16 & 255) / 255, (color >>> 24) / 255);
  gl.blendFuncSeparate(factors[srcRgb - 200], factors[dstRgb - 200], factors[srcAlpha - 200], factors[dstAlpha - 200]);
  toggle(gl.STENCIL_TEST, stencil !== 0);
  if (stencil !== 0) gl.stencilFunc(stencil, ref, readMask);
  gl.stencilMask(writeMask);
  gl.stencilOp(fail, zFail, pass);
  toggle(gl.CULL_FACE, cull !== 0);
  if (cull !== 0) gl.cullFace(faces[cull]);
  gl.frontFace(clockwise ? gl.CW : gl.CCW);
  toggle(gl.POLYGON_OFFSET_FILL, offset !== 0);
  gl.polygonOffset(factor, units);
  gl.colorMask(Boolean(mask & 1), Boolean(mask & 2), Boolean(mask & 4), Boolean(mask & 8));
}

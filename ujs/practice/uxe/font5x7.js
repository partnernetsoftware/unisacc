/* 自绘 5x7,与 font5x7.tsv 同步. */
export const FONT_W = 5;
export const FONT_H = 7;
export const FONT = {
  "0": [62, 65, 65, 65, 62],
  "1": [0, 66, 127, 64, 0],
  A: [126, 9, 9, 9, 126],
  I: [0, 65, 127, 65, 0],
};
export function glyphBits(ch) {
  return FONT[ch] || null;
}

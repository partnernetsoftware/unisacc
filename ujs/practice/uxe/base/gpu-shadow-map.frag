precision mediump float;
varying vec4 v_color;
uniform sampler2D u_texture;
float occluded(vec2 uv, float depth) {
  return depth > texture2D(u_texture, uv).r ? 1.0 : 0.0;
}
void main() {
  float depth = fract(v_color.z);
  float coverage = occluded(v_color.xy, depth);
  if (v_color.z >= 1.0) {
    coverage = 0.0;
    for (int y = -2; y <= 2; y++) {
      for (int x = -2; x <= 2; x++) {
        bool outsideRow = y == -2 || y == 2;
        bool outerColumn = x == -2 || x == 2;
        if (y == 0 || (outsideRow && (outerColumn || x == 0)) || (!outsideRow && !outerColumn)) {
          coverage += occluded(v_color.xy + vec2(float(x), float(y)) / 16.0, depth);
        }
      }
    }
    coverage /= 17.0;
  }
  float opacity = coverage * v_color.a;
  gl_FragColor = vec4(0.0, 0.0, 0.0, opacity);
}

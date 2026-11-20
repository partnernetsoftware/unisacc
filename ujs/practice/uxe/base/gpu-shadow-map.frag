precision highp float;
varying vec4 v_color;
uniform sampler2D u_texture;
float occluded(vec2 uv, float depth) {
  return depth > texture2D(u_texture, uv).r ? 1.0 : 0.0;
}
void main() {
  float depth = fract(v_color.z);
  float coverage = occluded(v_color.xy, depth);
  if (v_color.z >= 3.0) {
    float sum = 0.0;
    float squares = 0.0;
    for (int y = -1; y <= 1; y++) {
      for (int x = -1; x <= 1; x++) {
        vec2 rg = texture2D(u_texture, v_color.xy + vec2(float(x), float(y)) / 16.0).rg;
        float z = dot(rg, vec2(65280.0, 255.0) / 65535.0);
        sum += z;
        squares += z * z;
      }
    }
    float mean = sum / 9.0;
    float variance = max(0.0, squares / 9.0 - mean * mean);
    float delta = depth - mean;
    coverage = 0.0;
    if (delta > 0.0) {
      float probability = variance / (variance + delta * delta);
      coverage = 1.0 - clamp((probability - 0.3) / 0.65, 0.0, 1.0);
    }
  } else if (v_color.z >= 2.0) {
    vec2 phase = fract(16.0 * v_color.xy + 0.5);
    vec2 anchor = v_color.xy - phase / 16.0;
    coverage = 0.0;
    for (int y = -1; y <= 2; y++) {
      float wy = y == -1 ? 1.0 - phase.y : y == 2 ? phase.y : 1.0;
      for (int x = -1; x <= 2; x++) {
        float wx = x == -1 ? 1.0 - phase.x : x == 2 ? phase.x : 1.0;
        coverage += wx * wy * occluded(anchor + vec2(float(x), float(y)) / 16.0, depth);
      }
    }
    coverage /= 9.0;
  } else if (v_color.z >= 1.0) {
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

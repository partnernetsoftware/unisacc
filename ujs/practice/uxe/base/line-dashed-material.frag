#version 300 es
precision highp float;
precision highp int;
in highp float v_lineDistance;
flat in highp vec3 v_dash;
out vec4 fragmentColor;

void main() {
  if (mod(v_lineDistance, v_dash.y) > v_dash.x) discard;
  int color = int(v_dash.z);
  vec3 srgb = vec3(float(color / 65536), float((color / 256) % 256), float(color % 256));
  fragmentColor = vec4(srgb / 255.0, 1.0);
}

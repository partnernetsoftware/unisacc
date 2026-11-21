#version 300 es
in vec3 a_position;
in vec4 a_color;
out highp float v_lineDistance;
flat out highp vec3 v_dash;

void main() {
  gl_Position = vec4(a_position, 1.0);
  v_lineDistance = a_color.x;
  v_dash = a_color.yzw;
}

attribute vec3 a_position;
attribute vec4 a_color;
varying vec2 uv;
void main() {
  uv = a_color.xy;
  gl_Position = vec4(a_position, 1.0);
}

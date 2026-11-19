attribute vec3 a_position;
attribute vec4 a_color;
varying vec3 direction;
void main() {
  direction = a_color.xyz;
  gl_Position = vec4(a_position, 1.0);
}

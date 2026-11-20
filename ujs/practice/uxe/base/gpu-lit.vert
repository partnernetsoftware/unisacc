attribute vec3 a_position;
attribute vec4 a_color;
varying vec3 worldPosition;
varying vec3 worldNormal;
varying float lightingMode;
void main() {
  worldPosition = a_position;
  worldNormal = a_color.xyz;
  lightingMode = a_color.w;
  gl_Position = vec4(a_position.xy, -a_position.z, 1.0);
}

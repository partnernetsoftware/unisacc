precision mediump float;
varying vec4 v_color;
uniform sampler2D u_texture;
void main() {
  float stored = texture2D(u_texture, v_color.xy).r;
  float opacity = v_color.z > stored ? v_color.a : 0.0;
  gl_FragColor = vec4(0.0, 0.0, 0.0, opacity);
}

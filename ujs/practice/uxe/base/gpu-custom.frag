precision mediump float;
varying vec4 v_color;
void main() {
  gl_FragColor = vec4(v_color.b, (v_color.r + v_color.g) * 0.5, v_color.g, v_color.a);
}

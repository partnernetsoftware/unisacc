precision mediump float;
varying vec4 v_color;
void main() {
  float visibility = clamp(v_color.r, 0.0, 1.0);
  gl_FragColor = vec4(0.0, 0.0, 0.0, v_color.a * (1.0 - visibility));
}

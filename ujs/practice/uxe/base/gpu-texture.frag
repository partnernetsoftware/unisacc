precision mediump float;
varying vec2 uv;
uniform sampler2D u_texture;
void main() {
  gl_FragColor = texture2D(u_texture, uv);
}

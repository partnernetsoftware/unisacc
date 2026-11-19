precision mediump float;
varying vec3 direction;
uniform samplerCube u_texture;
void main() {
  gl_FragColor = textureCube(u_texture, direction);
}

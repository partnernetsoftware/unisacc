#version 300 es
precision highp float;
uniform sampler2D source;
uniform float u_amount;
out vec4 color;
void main() {
  vec4 pixel = texelFetch(source, ivec2(gl_FragCoord.xy), 0);
  color = vec4(pixel.rgb * u_amount, pixel.a);
}

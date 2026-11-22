#version 300 es
precision highp float;
uniform sampler2D source;
uniform float u_amount;
out vec4 color;
void main() {
  color = texelFetch(source, ivec2(gl_FragCoord.xy), 0) * u_amount;
}

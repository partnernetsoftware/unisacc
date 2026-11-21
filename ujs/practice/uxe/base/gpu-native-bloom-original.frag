#version 300 es
precision highp float;
uniform highp sampler2D source;
out vec4 result;
void main() {
  result = vec4(texelFetch(source, ivec2(gl_FragCoord.xy), 0).rgb, 1.0);
}

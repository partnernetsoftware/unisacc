#version 300 es
precision highp float;
uniform highp sampler2D source;
out vec4 result;

vec3 uxeToneMapping(vec3 color, float exposure);

void main() {
  vec4 inputColor = texelFetch(source, ivec2(gl_FragCoord.xy), 0);
  float exposure = uintBitsToFloat(UXE_TONE_EXPOSURE_BITS);
  result = vec4(uxeToneMapping(clamp(inputColor.rgb, 0.0, 1.0), exposure), inputColor.a);
}

#version 300 es
precision highp float;
precision highp int;
uniform highp sampler2D source;
out vec4 result;

#ifndef UXE_BLOOM_RADIUS
#define UXE_BLOOM_RADIUS 3
#endif
#ifndef UXE_BLOOM_VERTICAL
#define UXE_BLOOM_VERTICAL 0
#endif
#if UXE_BLOOM_RADIUS != 3 && UXE_BLOOM_RADIUS != 5 && UXE_BLOOM_RADIUS != 7 && UXE_BLOOM_RADIUS != 9 && UXE_BLOOM_RADIUS != 11
#error Unsupported Bloom kernel radius
#endif
#if UXE_BLOOM_VERTICAL != 0 && UXE_BLOOM_VERTICAL != 1
#error Invalid Bloom blur axis
#endif

vec3 samplePixel(ivec2 at, ivec2 edge) {
  return texelFetch(source, clamp(at, ivec2(0), edge), 0).rgb;
}

void main() {
  ivec2 at = ivec2(gl_FragCoord.xy);
  ivec2 edge = textureSize(source, 0) - ivec2(1);
  ivec2 axis = ivec2(1 - UXE_BLOOM_VERTICAL, UXE_BLOOM_VERTICAL);
  float radius = float(UXE_BLOOM_RADIUS);
  float weightSum = 1.0;
  vec3 diffuse = samplePixel(at, edge);
  for (int i = 1; i < UXE_BLOOM_RADIUS; ++i) {
    float x = float(i) / radius;
    float weight = exp(-0.5 * x * x);
    ivec2 offset = axis * i;
    diffuse += (samplePixel(at + offset, edge) + samplePixel(at - offset, edge)) * weight;
    weightSum += 2.0 * weight;
  }
  result = vec4(diffuse / weightSum, 1.0);
}

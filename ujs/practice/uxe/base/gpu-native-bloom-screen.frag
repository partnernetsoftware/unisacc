#version 300 es
precision highp float;
precision highp int;
uniform highp sampler2D source;
in highp vec2 uv;
out vec4 result;

void main() {
  ivec2 size = textureSize(source, 0);
  ivec2 edge = size - ivec2(1);
  vec2 at = uv * vec2(size) - 0.5;
  ivec2 base = ivec2(floor(at));
  vec2 f = fract(at);
  ivec2 low = clamp(base, ivec2(0), edge);
  ivec2 high = clamp(base + ivec2(1), ivec2(0), edge);
  vec4 bottom = mix(texelFetch(source, low, 0), texelFetch(source, ivec2(high.x, low.y), 0), f.x);
  vec4 top = mix(texelFetch(source, ivec2(low.x, high.y), 0), texelFetch(source, high, 0), f.x);
  result = mix(bottom, top, f.y);
}

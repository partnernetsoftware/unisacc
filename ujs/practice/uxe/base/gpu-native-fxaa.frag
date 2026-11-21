#version 300 es
// FXAA edge rule adapted from the NVIDIA shader bundled with three.js r160.
// Copyright (c) 2014-2015, NVIDIA CORPORATION. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// (1) Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
// (2) Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
// (3) Neither the name of NVIDIA CORPORATION nor the names of its contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

precision highp float;
precision highp int;
uniform highp sampler2D source;
out vec4 result;

vec4 samplePixel(ivec2 at, ivec2 limit) {
  return texelFetch(source, clamp(at, ivec2(0), limit), 0);
}

float contrast(vec4 a, vec4 b) {
  vec4 delta = abs(a - b);
  return max(max(delta.r, delta.g), max(delta.b, delta.a));
}

vec4 filterPixel(ivec2 at, ivec2 limit) {
  vec4 center = samplePixel(at, limit);
  vec4 north = samplePixel(at + ivec2(0, -1), limit);
  vec4 south = samplePixel(at + ivec2(0, 1), limit);
  vec4 east = samplePixel(at + ivec2(1, 0), limit);
  vec4 west = samplePixel(at + ivec2(-1, 0), limit);
  float cn = contrast(center, north);
  float cs = contrast(center, south);
  float ce = contrast(center, east);
  float cw = contrast(center, west);
  if (max(max(cn, cs), max(ce, cw)) < 0.2) return center;

  float tilt = ((cn + cs) - (ce + cw)) * 5.0;
  if (abs(tilt) < 0.3) {
    int dx = ce > cw ? 1 : -1;
    int dy = cs > cn ? 1 : -1;
    vec4 across = samplePixel(at + ivec2(dx, -dy), limit);
    vec4 along = samplePixel(at + ivec2(-dx, dy), limit);
    tilt = (contrast(center, along) - contrast(center, across)) * 5.0;
    if (abs(tilt) < 0.3) return mix(center, (north + south + east + west) * 0.25, 0.4);
  }
  bool horizontal = tilt > 0.0;
  vec4 edge = horizontal ? (cn > cs ? north : south) : (cw > ce ? west : east);
  ivec2 tangent = horizontal ? ivec2(1, 0) : ivec2(0, 1);
  int distance = 0;
  for (int step = 1; step <= 5; ++step) {
    distance += step;
    vec4 a = samplePixel(at + tangent * distance, limit);
    vec4 b = samplePixel(at - tangent * distance, limit);
    if (contrast(a, center) > contrast(a, edge) || contrast(b, center) > contrast(b, edge)) {
      float weight = (1.0 - sqrt(float(step - 1) * 0.25)) * 0.5;
      return mix(center, edge, weight);
    }
  }
  return center;
}

void main() {
  result = filterPixel(ivec2(gl_FragCoord.xy), textureSize(source, 0) - ivec2(1));
}

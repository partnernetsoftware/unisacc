#version 300 es
precision highp float;
uniform highp sampler2D source;
out vec4 result;

#ifndef UXE_COLOR_POW_RGB
#define UXE_COLOR_POW_RGB vec3(2.0)
#endif
#ifndef UXE_COLOR_MUL_RGB
#define UXE_COLOR_MUL_RGB vec3(1.0)
#endif
#ifndef UXE_COLOR_ADD_RGB
#define UXE_COLOR_ADD_RGB vec3(0.0)
#endif

void main() {
  vec4 pixel = texelFetch(source, ivec2(gl_FragCoord.xy), 0);
  result = vec4(UXE_COLOR_MUL_RGB * pow(pixel.rgb + UXE_COLOR_ADD_RGB, UXE_COLOR_POW_RGB), pixel.a);
}

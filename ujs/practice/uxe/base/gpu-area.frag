precision highp float;
varying vec4 v_color;

vec3 edge(vec3 from, vec3 to) {
  float cosine = clamp(dot(from, to), -0.999999, 0.999999);
  float angularWeight = acos(cosine) / (6.283185307179586 * sqrt(1.0 - cosine * cosine));
  return cross(to, from) * angularWeight;
}

float srgb(float linear) {
  if (linear <= 0.0031308) return 12.92 * linear;
  return 1.055 * pow(linear, 1.0 / 2.4) - 0.055;
}

void main() {
  vec2 relative = -v_color.xy;
  vec3 a = normalize(vec3(relative + vec2(0.5, -0.5), 1.0));
  vec3 b = normalize(vec3(relative + vec2(-0.5, -0.5), 1.0));
  vec3 c = normalize(vec3(relative + vec2(-0.5, 0.5), 1.0));
  vec3 d = normalize(vec3(relative + vec2(0.5, 0.5), 1.0));
  vec3 integral = edge(a,b) + edge(b,c) + edge(c,d) + edge(d,a);
  float magnitude = length(integral);
  float irradiance = max(0.0, (magnitude * magnitude + integral.z) / (magnitude + 1.0));
  if (v_color.z < 0.5) irradiance = 0.0;
  vec3 color = vec3(0.2140411405, 0.0508760882, 0.0024282159) * irradiance;
  gl_FragColor = vec4(srgb(color.r), srgb(color.g), srgb(color.b), 1.0);
}

vec3 uxeToneMapping(vec3 color, float exposure) {
  vec3 t = max(color * exposure, vec3(0.0));
  return vec3(t.r * t.r, t.g / (1.0 + t.g), (t.b * t.b + t.r) * 0.5);
}

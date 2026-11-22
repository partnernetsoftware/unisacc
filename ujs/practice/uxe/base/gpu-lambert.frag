precision highp float;
varying vec3 worldNormal;
uniform vec3 diffuse;
uniform vec3 emissive;
uniform float opacity;
void main() {
  vec3 normal = normalize(worldNormal);
  vec3 light = normalize(vec3(1.0, 0.0, 2.0));
  float irradiance = max(dot(normal, light), 0.0);
  gl_FragColor = vec4(emissive + diffuse * (irradiance / 3.141592653589793), opacity);
}

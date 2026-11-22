precision highp float;
varying vec3 worldPosition;
varying vec3 worldNormal;
uniform vec3 diffuse;
uniform vec3 emissive;
uniform vec3 specular;
uniform float opacity;
uniform float shininess;
void main() {
  vec3 normal = normalize(worldNormal);
  vec3 light = normalize(vec3(1.0, 0.0, 2.0));
  vec3 eye = normalize(vec3(0.0, 0.0, 2.0) - worldPosition);
  float irradiance = max(dot(normal, light), 0.0);
  vec3 halfway = normalize(eye + light);
  float highlight = pow(max(dot(normal, halfway), 0.0), max(shininess, 0.0001));
  gl_FragColor = vec4(emissive + irradiance * (diffuse / 3.141592653589793 + specular * highlight), opacity);
}

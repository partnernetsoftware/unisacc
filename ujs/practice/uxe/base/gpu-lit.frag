#extension GL_OES_standard_derivatives : enable
precision highp float;
varying vec3 worldPosition;
varying vec3 worldNormal;
varying float lightingMode;

const vec3 diffuse = vec3(0.2140411405, 0.0508760882, 0.0024282159);
const float invPi = 0.31830988618;

float fresnel(float f0, float vh) {
  return f0 + (1.0 - f0) * exp2((-5.55473 * vh - 6.98316) * vh);
}

vec3 direct(vec3 normal, vec3 light, float power, float roughness, bool standard) {
  float nl = max(0.0, dot(normal, light));
  if (nl == 0.0) return vec3(0.0);
  vec3 halfway = normalize(light + vec3(0.0, 0.0, 1.0));
  float nh = clamp(dot(normal, halfway), 0.0, 1.0);
  float vh = clamp(halfway.z, 0.0, 1.0);
  float specular;
  if (standard) {
    float nv = max(0.0, normal.z);
    float alpha2 = pow(roughness, 4.0);
    float denom = 1.0 + nh * nh * (alpha2 - 1.0);
    float distribution = invPi * alpha2 / (denom * denom);
    float visible = nl * sqrt(alpha2 + (1.0 - alpha2) * nv * nv)
                  + nv * sqrt(alpha2 + (1.0 - alpha2) * nl * nl);
    specular = fresnel(0.04, vh) * distribution * 0.5 / max(1e-6, visible);
  } else {
    specular = fresnel(0.08, vh) * invPi * pow(nh, 6.0);
  }
  return nl * power * (diffuse * invPi + vec3(specular));
}

float srgb(float value) {
  if (value <= 0.0031308) return 12.92 * value;
  return 1.055 * pow(value, 1.0 / 2.4) - 0.055;
}

void main() {
  vec3 normal = normalize(worldNormal);
  if (lightingMode >= 10.0) {
    vec3 view = normalize(vec3(0.0, 0.0, 1.0) - worldPosition);
    vec3 horizontal = normalize(vec3(view.z, 0.0, -view.x));
    float vertical = dot(cross(view, horizontal), normal);
    float neutralCap = 0.5 + 0.297 * vertical;
    gl_FragColor = vec4(vec3(srgb(neutralCap)), 1.0);
    return;
  }
  if (lightingMode >= 8.0) {
    gl_FragColor = vec4(vec3(1.0 - gl_FragCoord.z), 1.0);
    return;
  }
  if (lightingMode >= 6.0) {
    gl_FragColor = vec4(normal * 0.5 + 0.5, 1.0);
    return;
  }
  vec3 variation = max(abs(dFdx(normal)), abs(dFdy(normal)));
  float roughness = min(1.0, 0.5 + max(max(variation.x, variation.y), variation.z));
  bool standard = mod(lightingMode, 2.0) > 0.5;
  vec3 color;
  if (lightingMode > 1.5) {
    vec3 delta = vec3(0.4, -0.3, 1.6) - worldPosition;
    color = direct(normal, normalize(delta), 3.2 / max(0.01, dot(delta, delta)), roughness, standard);
  } else {
    color = direct(normal, normalize(vec3(1.0, 0.0, 2.0)), 2.0, roughness, standard)
          + direct(normal, normalize(vec3(-1.0, 1.0, 1.0)), 1.0, roughness, standard);
  }
  gl_FragColor = vec4(srgb(color.r), srgb(color.g), srgb(color.b), 1.0);
}

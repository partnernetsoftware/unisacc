precision highp float;
uniform vec3 diffuse;
uniform vec3 emissive;
uniform float opacity;
uniform float roughness;
uniform float metalness;
uniform float ior;
uniform vec3 specularColor;
uniform float specularIntensity;
uniform float clearcoat;
uniform float clearcoatRoughness;
varying vec3 worldNormal;

float ggx(float rough, float nl, float nv, float nh) {
    float alpha = clamp(rough, 0.0525, 1.0);
    alpha *= alpha;
    float alpha2 = alpha * alpha;
    float denom = nh * nh * (alpha2 - 1.0) + 1.0;
    float visibility = 0.5 / max(
        nl * sqrt(alpha2 + (1.0 - alpha2) * nv * nv) +
        nv * sqrt(alpha2 + (1.0 - alpha2) * nl * nl), 1e-6);
    return visibility * alpha2 / (3.141592653589793 * denom * denom);
}

float schlickWeight(float cosine) {
    return exp2((-5.55473 * cosine - 6.98316) * cosine);
}

void main() {
    vec3 n = normalize(worldNormal);
    vec3 light = vec3(0.0, 0.0, 1.0);
    vec3 view = vec3(0.0, 0.0, 1.0);
    vec3 halfDirection = normalize(light + view);
    float nl = clamp(dot(n, light), 0.0, 1.0);
    float nv = clamp(dot(n, view), 0.0, 1.0);
    float nh = clamp(dot(n, halfDirection), 0.0, 1.0);
    float vh = clamp(dot(view, halfDirection), 0.0, 1.0);
    float eta = (ior - 1.0) / (ior + 1.0);
    vec3 f0 = mix(min(eta * eta * specularColor, vec3(1.0)) * specularIntensity, diffuse, metalness);
    float f90 = mix(specularIntensity, 1.0, metalness);
    vec3 fresnel = mix(f0, vec3(f90), schlickWeight(vh));
    vec3 base = diffuse * (1.0 - metalness) / 3.141592653589793
        + fresnel * ggx(roughness, nl, nv, nh);
    float coatFresnel = mix(0.04, 1.0, schlickWeight(nv));
    float coatLight = mix(0.04, 1.0, schlickWeight(vh)) * ggx(clearcoatRoughness, nl, nv, nh);
    gl_FragColor = vec4(emissive + 2.0 * nl * (base * (1.0 - clearcoat * coatFresnel) + clearcoat * coatLight), opacity);
}

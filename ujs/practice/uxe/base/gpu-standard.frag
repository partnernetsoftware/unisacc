precision highp float;
uniform vec3 diffuse;
uniform vec3 emissive;
uniform float opacity;
uniform float roughness;
uniform float metalness;
varying vec3 worldNormal;
void main() {
    vec3 n = normalize(worldNormal);
    vec3 l = normalize(vec3(0.3, 0.6, 1.0));
    vec3 v = vec3(0.0, 0.0, 1.0);
    vec3 h = normalize(l + v);
    float nl = max(dot(n, l), 0.0);
    float nv = max(dot(n, v), 0.001);
    float nh = max(dot(n, h), 0.0);
    float vh = max(dot(v, h), 0.0);
    float a = max(roughness * roughness, 0.002);
    float a2 = a * a;
    float d = nh * nh * (a2 - 1.0) + 1.0;
    float distribution = a2 / (3.14159265 * d * d);
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float visibility = 1.0 / (4.0 * (nv * (1.0 - k) + k) * (nl * (1.0 - k) + k));
    vec3 f = mix(vec3(0.04), diffuse, metalness);
    f += (1.0 - f) * pow(1.0 - vh, 5.0);
    vec3 base = (1.0 - f) * diffuse * (1.0 - metalness) / 3.14159265;
    gl_FragColor = vec4(emissive + nl * (base + f * distribution * visibility), opacity);
}

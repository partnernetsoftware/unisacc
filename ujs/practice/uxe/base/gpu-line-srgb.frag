precision highp float;
varying vec4 v_color;
void main() {
  vec3 linear = v_color.rgb;
  vec3 nonlinear = pow(max(linear, vec3(0.0)), vec3(0.41666)) * 1.055 - vec3(0.055);
  vec3 low = vec3(lessThanEqual(linear, vec3(0.0031308)));
  gl_FragColor = vec4(mix(nonlinear, linear * 12.92, low), v_color.a);
}

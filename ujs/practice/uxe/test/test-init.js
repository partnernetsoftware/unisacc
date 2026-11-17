/* 测试页统一 fog/ambient/lights,避免默认雾. */
export function applyTestScene(scene, clear) {
  scene.clearColor = clear || [0, 0, 0, 1];
  scene.fog = { density: 0, color: [0, 0, 0] };
  scene.ambient = { color: [0, 0, 0], intensity: 0 };
  scene.lights = [];
  scene.ink = 0;
}

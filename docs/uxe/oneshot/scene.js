import { FONT, FONT_W, FONT_H } from "./font5x7.js";
import { MESH_POINTS } from "./meshes.js";

/** UXE scene — closed, data-oriented. No materials graph. */

export class PerspectiveCamera {
  constructor(fovy = Math.PI / 3, near = 0.1, far = 300) {
    this.fovy = fovy;
    this.near = near;
    this.far = far;
    this.eye = [0, 3, 12];
    this.target = [0, 0, -18];
  }
  setEye(x, y, z) { this.eye[0] = x; this.eye[1] = y; this.eye[2] = z; }
  setTarget(x, y, z) { this.target[0] = x; this.target[1] = y; this.target[2] = z; }
}

/** CPU-side instance cloud; renderer may mirror into GPU buffer. */
export class InstanceCloud {
  constructor(capacity, color = [0.55, 0.58, 0.62]) {
    this.capacity = capacity;
    this.count = capacity;
    this.xyz = new Float32Array(capacity * 3);
    this.scale = new Float32Array(capacity);
    this.yaw = new Float32Array(capacity);
    this.pitch = new Float32Array(capacity);
    this.roll = new Float32Array(capacity);
    this.color = color;
    this.emissive = [0, 0, 0];
    this.metalness = 0.2;
    this.roughness = 0.5;
    this.meshId = 0;
    this.mapId = 0;
    this.billboard = 0;
    this.dirty = true;

  }
  /** yaw Y, then pitch X, then roll Z. omitted angles stay 0 / previous. */
  set(i, x, y, z, s, yaw, pitch, roll) {
    const o = i * 3;
    this.xyz[o] = x; this.xyz[o + 1] = y; this.xyz[o + 2] = z;
    this.scale[i] = s;
    if (yaw !== undefined) this.yaw[i] = yaw;
    if (pitch !== undefined) this.pitch[i] = pitch;
    if (roll !== undefined) this.roll[i] = roll;
    this.dirty = true;
  }
  markClean() { this.dirty = false; }
}

export class Scene {
  constructor() {
    this.clouds = new Map();
    this.clearColor = [0.02, 0.024, 0.04, 1];
    this.fog = { density: 0.018, color: [0.02, 0.024, 0.04] };
    this.ambient = { color: [0.25, 0.38, 0.5], intensity: 0.55 };
    this.lights = [
      { dir: [0.35, 0.9, 0.25], color: [1.0, 0.9, 0.78], intensity: 1.15 },
      { dir: [-0.4, 0.2, -0.5], color: [0.4, 0.53, 1.0], intensity: 0.45 },
    ];
  }
  instances(name, capacity, color) {
    const c = new InstanceCloud(capacity, color);
    this.clouds.set(name, c);
    return c;
  }
  text(str, x, y, scale, color) {
    const s = scale || 4;
    const xs = [], ys = [], sc = [];
    const t = String(str || "");
    for (let gi = 0; gi < t.length; gi++) {
      const cols = FONT[t[gi]];
      if (!cols) continue;
      for (let cx = 0; cx < FONT_W; cx++) {
        const bits = cols[cx] | 0;
        for (let cy = 0; cy < FONT_H; cy++) {
          if (!((bits >> cy) & 1)) continue;
          xs.push(x + gi * FONT_W * s + cx * s + s / 2);
          ys.push(y + cy * s + s / 2);
          sc.push(s);
        }
      }
    }
    const cnt = xs.length;
    const name = "text:" + t;
    const c = this.instances(name, cnt, color || [1, 1, 1]);
    c.meshId = MESH_POINTS;
    c.count = cnt;
    c.screenSpace = 1;
    c.flags = 8;
    for (let i = 0; i < cnt; i++) {
      c.xyz[i * 3] = xs[i];
      c.xyz[i * 3 + 1] = ys[i];
      c.xyz[i * 3 + 2] = 0;
      c.scale[i] = sc[i];
    }
    return c;
  }

  get(name) { return this.clouds.get(name); }
}

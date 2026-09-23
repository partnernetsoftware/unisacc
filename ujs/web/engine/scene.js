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
    this.color = color;
    this.dirty = true;
  }
  set(i, x, y, z, s) {
    const o = i * 3;
    this.xyz[o] = x; this.xyz[o + 1] = y; this.xyz[o + 2] = z;
    this.scale[i] = s;
    this.dirty = true;
  }
  markClean() { this.dirty = false; }
}

export class Scene {
  constructor() {
    this.clouds = new Map();
    this.clearColor = [0.02, 0.024, 0.04, 1];
  }
  instances(name, capacity, color) {
    const c = new InstanceCloud(capacity, color);
    this.clouds.set(name, c);
    return c;
  }
  get(name) { return this.clouds.get(name); }
}

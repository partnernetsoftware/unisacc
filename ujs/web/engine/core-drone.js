/**
 * FP drone battlefield — cockpit view, 2 missiles (lock+fire), suicide ram.
 */
import { bootRuntime, wasm_run, unwrap } from "../wasm_run.js";
import { encodeRenderPacket, MESH_OCTA } from "./packet.js";
import { MESH_BOX } from "./meshes.js";
import {
  decodeInputSnapshot, INPUT_BYTES, BTN_RIGHT, FLAG_SUICIDE,
} from "./input.js";

const NT = 8;
const MAGAZINE = 2;
const LOCK_ALIGN = 0.965;
const LOCK_MAX_DIST = 85;
const LOCK_HOLD = 0.18;
const MSL_SPEED = 55;
const SUICIDE_PROX = 5.5;
const SUICIDE_BLAST = 18;

function basis(yaw, pitch) {
  const cp = Math.cos(pitch), sp = Math.sin(pitch);
  const cy = Math.cos(yaw), sy = Math.sin(yaw);
  const fx = cp * sy, fy = sp, fz = cp * cy;
  const rx = cy, ry = 0, rz = -sy;
  return { fx, fy, fz, rx, ry, rz };
}

function freshTargets() {
  const txs = [], tys = [], tzs = [], thp = [], tang = [];
  for (let i = 0; i < NT; i++) {
    const ang = (i / NT) * Math.PI * 2 + 0.35;
    const r = 18 + (i % 3) * 7;
    tang.push(ang);
    txs.push(Math.sin(ang) * r);
    tys.push(5 + (i % 3) * 2.5);
    tzs.push(Math.cos(ang) * r - 10);
    thp.push(1);
  }
  return { txs, tys, tzs, thp, tang };
}

function freshState() {
  return {
    px: 0, py: 12, pz: 32,
    score: 0, alive: 1, ammo: MAGAZINE,
    ...freshTargets(),
  };
}

function bestLock(state, B) {
  let best = -1, bestAlign = LOCK_ALIGN;
  for (let i = 0; i < NT; i++) {
    if (state.thp[i] <= 0) continue;
    const dx = state.txs[i] - state.px;
    const dy = state.tys[i] - state.py;
    const dz = state.tzs[i] - state.pz;
    const dist = Math.sqrt(dx * dx + dy * dy + dz * dz);
    if (dist < 2.5 || dist > LOCK_MAX_DIST) continue;
    const inv = 1 / dist;
    const align = dx * inv * B.fx + dy * inv * B.fy + dz * inv * B.fz;
    if (align > bestAlign) {
      bestAlign = align;
      best = i;
    }
  }
  return best;
}

function faceTargetIdx(state, i) {
  const dx = state.txs[i] - state.px;
  const dy = state.tys[i] - state.py;
  const dz = state.tzs[i] - state.pz;
  const yaw = Math.atan2(dx, dz);
  const horiz = Math.sqrt(dx * dx + dz * dz) || 1;
  const pitch = Math.atan2(dy, horiz);
  return { yaw, pitch };
}

/**
 * @param {import("./host-abi.js").HostAbi} host
 * @param {{
 *   simUrl?: string, wasmUrl: string, precompiled?: object, onHud?: Function,
 *   controls?: "keyboard"|"mouse"
 * }} opts
 */
export async function runDroneCore(host, opts) {
  host.host_log("info", "drone FP cockpit boot");
  let controls = opts.controls === "mouse" ? "mouse" : "keyboard";
  host.setPointerLockEnabled?.(controls === "mouse");
  const wasmBytes = await host.host_asset_read(opts.wasmUrl);
  await bootRuntime(wasmBytes.buffer.slice(
    wasmBytes.byteOffset,
    wasmBytes.byteOffset + wasmBytes.byteLength,
  ));

  let fnImage, fnBlob;
  if (opts.precompiled?.image) {
    const img = opts.precompiled.image;
    fnImage = img instanceof Uint8Array ? img : new Uint8Array(img);
    fnBlob = opts.precompiled.blob || {};
  } else {
    if (typeof __UXE_SHIP__ !== "undefined" && __UXE_SHIP__) {
      throw new Error("drone ship requires precompiled sim");
    }
    const { compile } = await import("../compiler.js");
    const simText = new TextDecoder().decode(await host.host_asset_read(opts.simUrl));
    const compiled = compile(simText);
    fnImage = compiled.image;
    fnBlob = compiled.blob;
  }

  let state = freshState();
  let yaw = Math.PI, pitch = -0.06;
  let lastFire = 0;
  let kills = 0;
  let lockIdx = -1;
  let lockTime = 0;
  let locked = false;
  let suicideArm = false;
  let endReason = ""; // "" | "win" | "suicide" | "crash"
  let last = host.host_time();
  let accFrames = 0, lastHud = last;
  const inputBuf = new ArrayBuffer(INPUT_BYTES);
  let lastAbsMx = 0, lastAbsMy = 0, haveAbs = false;
  /** @type {{x:number,y:number,z:number,vx:number,vy:number,vz:number,life:number,ti:number}[]} */
  let missiles = [];
  let flash = 0;
  let tOrbit = 0;
  let forceFire = false;

  const gN = 64;
  const gXYZ = new Float32Array(gN * 3);
  const gS = new Float32Array(gN);
  let gi = 0;
  for (let gz = -4; gz <= 3; gz++) {
    for (let gx = -4; gx <= 3; gx++) {
      if (gi >= gN) break;
      gXYZ[gi * 3] = gx * 14;
      gXYZ[gi * 3 + 1] = 0;
      gXYZ[gi * 3 + 2] = gz * 14;
      gS[gi] = 13.2;
      gi++;
    }
  }

  async function stepWith(dt, ix, iy, B, thit, suicide, speedMul) {
    const r = await wasm_run({ image: fnImage, blob: fnBlob }, {
      px: state.px, py: state.py, pz: state.pz,
      score: state.score, alive: state.alive, ammo: state.ammo,
      ix, iy, dt, suicide, speed_mul: speedMul,
      fx: B.fx, fy: B.fy, fz: B.fz,
      rx: B.rx, ry: B.ry, rz: B.rz,
      txs: state.txs, tys: state.tys, tzs: state.tzs, thp: state.thp, thit,
    }, {});
    if (r.err) {
      host.host_log("error", "ujs " + JSON.stringify(r.err));
      return;
    }
    const out = unwrap(r);
    state = {
      px: out.px, py: out.py, pz: out.pz,
      score: out.score, alive: out.alive, ammo: state.ammo,
      txs: out.txs, tys: out.tys, tzs: out.tzs, thp: out.thp,
      tang: state.tang,
    };
    kills += out.nk || 0;
  }

  function orbitTargets(dt) {
    tOrbit += dt;
    for (let i = 0; i < NT; i++) {
      if (state.thp[i] <= 0) continue;
      state.tang[i] += dt * (0.22 + (i % 3) * 0.04);
      const r = 18 + (i % 3) * 7;
      state.txs[i] = Math.sin(state.tang[i]) * r;
      state.tzs[i] = Math.cos(state.tang[i]) * r - 10;
      state.tys[i] = 5 + (i % 3) * 2.5 + Math.sin(tOrbit * 1.4 + i) * 0.6;
    }
  }

  function cockpitClouds(B) {
    // denser canopy/frame — reads as sitting inside the craft
    const pieces = [
      [-0.62, -0.42, 0.88, 0.11],
      [0.62, -0.42, 0.88, 0.11],
      [0, -0.52, 0.82, 0.62],
      [-0.78, 0.05, 0.92, 0.09],
      [0.78, 0.05, 0.92, 0.09],
      [-0.45, 0.38, 0.9, 0.08],
      [0.45, 0.38, 0.9, 0.08],
      [0, 0.42, 0.95, 0.5],
      [-0.9, -0.15, 1.05, 0.07],
      [0.9, -0.15, 1.05, 0.07],
    ];
    const xyz = [], sc = [];
    for (const [lx, ly, lz, s] of pieces) {
      const wx = state.px + B.fx * lz + B.rx * lx;
      const wy = state.py + B.fy * lz + ly;
      const wz = state.pz + B.fz * lz + B.rz * lx;
      xyz.push(wx, wy, wz);
      sc.push(s);
    }
    return {
      color: suicideArm ? [0.18, 0.04, 0.04] : [0.04, 0.06, 0.08],
      count: sc.length,
      xyz: new Float32Array(xyz), scale: new Float32Array(sc),
      meshId: MESH_BOX, metalness: 0.65, roughness: 0.32,
      emissive: suicideArm ? [0.12, 0.01, 0.01] : [0.01, 0.025, 0.035],
    };
  }

  function buildPacket(B) {
    const aXYZ = [], aS = [], lockXYZ = [], lockS = [];
    for (let i = 0; i < NT; i++) {
      if (state.thp[i] <= 0) continue;
      const arr = (locked && i === lockIdx) ? lockXYZ : aXYZ;
      const sc = (locked && i === lockIdx) ? lockS : aS;
      arr.push(state.txs[i], state.tys[i], state.tzs[i]);
      sc.push(locked && i === lockIdx ? 2.0 : 1.4);
    }
    const eye = [state.px, state.py, state.pz];
    const target = [
      state.px + B.fx * 80,
      state.py + B.fy * 80,
      state.pz + B.fz * 80,
    ];
    const sky = suicideArm
      ? [0.42, 0.22, 0.2, 1]
      : flash > 0
        ? [0.55, 0.5, 0.35, 1]
        : [0.32, 0.46, 0.58, 1];
    const clouds = [
      {
        color: [0.2, 0.24, 0.18], count: gN, xyz: gXYZ, scale: gS,
        meshId: MESH_BOX, metalness: 0.08, roughness: 0.92, emissive: [0, 0, 0],
      },
      cockpitClouds(B),
    ];
    if (aS.length) {
      clouds.push({
        color: [0.78, 0.18, 0.1], count: aS.length,
        xyz: new Float32Array(aXYZ), scale: new Float32Array(aS),
        meshId: MESH_OCTA, metalness: 0.25, roughness: 0.48, emissive: [0.18, 0.03, 0],
      });
    }
    if (lockS.length) {
      clouds.push({
        color: [0.15, 1.0, 0.4], count: lockS.length,
        xyz: new Float32Array(lockXYZ), scale: new Float32Array(lockS),
        meshId: MESH_OCTA, metalness: 0.3, roughness: 0.3, emissive: [0.06, 0.4, 0.1],
      });
    }
    if (missiles.length) {
      const mXYZ = [], mS = [];
      for (const m of missiles) {
        mXYZ.push(m.x, m.y, m.z);
        mS.push(0.35);
      }
      clouds.push({
        color: [1, 0.85, 0.25], count: mS.length,
        xyz: new Float32Array(mXYZ), scale: new Float32Array(mS),
        meshId: MESH_OCTA, metalness: 0.4, roughness: 0.25, emissive: [0.5, 0.35, 0.05],
      });
    }
    // aim pip ahead of nose
    clouds.push({
      color: locked ? [0.15, 1, 0.45] : suicideArm ? [1, 0.35, 0.2] : [1, 0.92, 0.25],
      count: 1,
      xyz: new Float32Array([
        state.px + B.fx * 11,
        state.py + B.fy * 11,
        state.pz + B.fz * 11,
      ]),
      scale: new Float32Array([locked ? 0.16 : 0.1]),
      meshId: MESH_BOX, metalness: 0.2, roughness: 0.4,
      emissive: locked ? [0.05, 0.45, 0.12] : [0.35, 0.28, 0.02],
    });
    return encodeRenderPacket({
      clear: sky,
      camera: { fovy: Math.PI / 2.2, near: 0.06, far: 280, eye, target },
      fog: {
        density: suicideArm ? 0.016 : 0.012,
        color: suicideArm ? [0.45, 0.28, 0.22] : [0.38, 0.5, 0.58],
      },
      ambient: { color: [0.42, 0.48, 0.55], intensity: 0.72 },
      lights: [
        { dir: [0.25, 1, 0.15], color: [1, 0.95, 0.85], intensity: 1.2 },
        { dir: [-0.4, 0.15, -0.35], color: [0.35, 0.45, 0.65], intensity: 0.42 },
      ],
      clouds,
    });
  }

  function resetRun() {
    state = freshState();
    yaw = Math.PI; pitch = -0.06;
    kills = 0; lockIdx = -1; lockTime = 0; locked = false;
    suicideArm = false; endReason = "";
    missiles = []; flash = 0; tOrbit = 0;
    haveAbs = false;
  }

  function spawnMissile(ti, B) {
    missiles.push({
      x: state.px + B.fx * 1.2,
      y: state.py + B.fy * 1.2,
      z: state.pz + B.fz * 1.2,
      vx: B.fx * MSL_SPEED,
      vy: B.fy * MSL_SPEED,
      vz: B.fz * MSL_SPEED,
      life: 2.2,
      ti,
    });
  }

  function updateMissiles(dt, thit) {
    const next = [];
    for (const m of missiles) {
      m.life -= dt;
      if (m.life <= 0) continue;
      // mild homing toward assigned target if still alive
      if (m.ti >= 0 && state.thp[m.ti] > 0) {
        const dx = state.txs[m.ti] - m.x;
        const dy = state.tys[m.ti] - m.y;
        const dz = state.tzs[m.ti] - m.z;
        const d = Math.sqrt(dx * dx + dy * dy + dz * dz) || 1;
        const pull = 38 * dt;
        m.vx += (dx / d) * pull;
        m.vy += (dy / d) * pull;
        m.vz += (dz / d) * pull;
        const sp = Math.sqrt(m.vx * m.vx + m.vy * m.vy + m.vz * m.vz) || 1;
        const want = MSL_SPEED;
        m.vx = (m.vx / sp) * want;
        m.vy = (m.vy / sp) * want;
        m.vz = (m.vz / sp) * want;
        if (d < 2.2) {
          thit[m.ti] = 1;
          flash = 0.25;
          continue;
        }
      }
      m.x += m.vx * dt;
      m.y += m.vy * dt;
      m.z += m.vz * dt;
      // proximity to any live target
      let hit = false;
      for (let i = 0; i < NT; i++) {
        if (state.thp[i] <= 0) continue;
        const dx = state.txs[i] - m.x;
        const dy = state.tys[i] - m.y;
        const dz = state.tzs[i] - m.z;
        if (dx * dx + dy * dy + dz * dz < 4.5) {
          thit[i] = 1;
          flash = 0.25;
          hit = true;
          break;
        }
      }
      if (!hit) next.push(m);
    }
    missiles = next;
  }

  function armSuicideBlast(thit) {
    for (let j = 0; j < NT; j++) {
      if (state.thp[j] <= 0) continue;
      const ex = state.txs[j] - state.px;
      const ey = state.tys[j] - state.py;
      const ez = state.tzs[j] - state.pz;
      if (ex * ex + ey * ey + ez * ez < SUICIDE_BLAST * SUICIDE_BLAST) thit[j] = 1;
    }
  }

  const api = {
    faceNearest() {
      let best = -1, bestD = 1e9;
      for (let i = 0; i < NT; i++) {
        if (state.thp[i] <= 0) continue;
        const dx = state.txs[i] - state.px;
        const dy = state.tys[i] - state.py;
        const dz = state.tzs[i] - state.pz;
        const d = dx * dx + dy * dy + dz * dz;
        if (d < bestD) { bestD = d; best = i; }
      }
      if (best < 0) return false;
      const f = faceTargetIdx(state, best);
      yaw = f.yaw; pitch = Math.max(-1.1, Math.min(1.1, f.pitch));
      lockIdx = best; lockTime = LOCK_HOLD; locked = true;
      return true;
    },
    faceTarget(i) {
      if (i < 0 || i >= NT || state.thp[i] <= 0) return false;
      const f = faceTargetIdx(state, i);
      yaw = f.yaw; pitch = Math.max(-1.1, Math.min(1.1, f.pitch));
      lockIdx = i; lockTime = LOCK_HOLD; locked = true;
      return true;
    },
    /** Probe/helper: fire immediately if locked + ammo, else queue next tick. */
    fire() {
      if (state.alive && locked && state.ammo > 0 && !suicideArm && lockIdx >= 0) {
        spawnMissile(lockIdx, basis(yaw, pitch));
        state.ammo -= 1;
        locked = false;
        lockTime = 0;
        flash = 0.12;
        return { ok: true, ammo: state.ammo, missiles: missiles.length };
      }
      forceFire = true;
      return { ok: false, locked, ammo: state.ammo, suicideArm };
    },
    setSuicideArm(v) { suicideArm = !!v; return suicideArm; },
    /** Instant proximity blast for probe (armed suicide path). */
    detonateNow() {
      if (!state.alive) return false;
      suicideArm = true;
      // warp onto nearest live target so blast is deterministic for CDP
      let best = -1, bestD = 1e9;
      for (let i = 0; i < NT; i++) {
        if (state.thp[i] <= 0) continue;
        const dx = state.txs[i] - state.px;
        const dy = state.tys[i] - state.py;
        const dz = state.tzs[i] - state.pz;
        const d = dx * dx + dy * dy + dz * dz;
        if (d < bestD) { bestD = d; best = i; }
      }
      if (best >= 0) {
        state.px = state.txs[best];
        state.py = state.tys[best];
        state.pz = state.tzs[best];
      }
      const thit = Array(NT).fill(0);
      armSuicideBlast(thit);
      let nk = 0;
      for (let i = 0; i < NT; i++) {
        if (thit[i] && state.thp[i] > 0) {
          state.thp[i] = 0;
          nk++;
          state.score += 500;
        }
      }
      kills += nk;
      state.alive = 0;
      state.score += 150;
      endReason = state.thp.every((h) => h <= 0) ? "win" : "suicide";
      if (endReason === "win") state.score += 1000;
      flash = 0.5;
      return { nk, endReason, kills, remaining: state.thp.filter((h) => h > 0).length };
    },
    reset: resetRun,
    getSnapshot() {
      return {
        ammo: state.ammo, kills, locked, lockIdx, suicideArm,
        remaining: state.thp.filter((h) => h > 0).length,
        alive: state.alive, endReason, missiles: missiles.length,
        px: state.px, py: state.py, pz: state.pz, yaw, pitch,
      };
    },
  };
  if (typeof globalThis !== "undefined") globalThis.__DRONE_API__ = api;

  async function tick(now) {
    host.host_frame_begin();
    const dt = Math.min(0.05, (now - last) / 1000);
    last = now;
    if (flash > 0) flash -= dt;

    let mx = 0, my = 0, buttons = 0, flags = 0, ix = 0, iy = 0, fire = 0;
    let keys = {};
    let pointerLock = false;
    const nIn = host.host_input_read(inputBuf);
    if (nIn === INPUT_BYTES) {
      const input = decodeInputSnapshot(inputBuf);
      ix = input.ix; iy = input.iy; fire = input.fire;
      mx = input.mx; my = input.my; buttons = input.buttons; flags = input.flags;
    }
    const snapObj = host.host_input_read();
    if (snapObj && typeof snapObj === "object") {
      keys = snapObj.keys || {};
      pointerLock = !!snapObj.pointerLock;
    }

    // Look: keyboard (default) = IJKL；mouse = 指针（lock 增量 / 未 lock 绝对差分）
    if (controls === "keyboard") {
      let lookX = 0, lookY = 0;
      if (keys.KeyJ || keys.ArrowLeft) lookX -= 1;
      if (keys.KeyL || keys.ArrowRight) lookX += 1;
      if (keys.KeyI || keys.ArrowUp) lookY -= 1;
      if (keys.KeyK || keys.ArrowDown) lookY += 1;
      ix = 0; iy = 0;
      if (keys.KeyA) ix -= 1;
      if (keys.KeyD) ix += 1;
      if (keys.KeyS) iy += 1;
      if (keys.KeyW) iy -= 1;
      fire = keys.Space ? 1 : 0;
      yaw += lookX * 2.4 * dt;
      pitch -= lookY * 2.0 * dt;
      haveAbs = false;
    } else if (pointerLock) {
      yaw += mx * 2.6;
      pitch += my * 2.2;
      haveAbs = false;
    } else if (flags & 1 /* FLAG_POINTER_IN */ || haveAbs) {
      if (haveAbs) {
        yaw += (mx - lastAbsMx) * 2.8;
        pitch += (my - lastAbsMy) * 2.4;
      }
      lastAbsMx = mx;
      lastAbsMy = my;
      haveAbs = true;
    }
    if (pitch > 1.2) pitch = 1.2;
    if (pitch < -1.2) pitch = -1.2;

    const B = basis(yaw, pitch);
    const fireEdge = (fire && !lastFire) || forceFire;
    forceFire = false;
    lastFire = fire;

    const wantSuicide = controls === "mouse"
      ? (!!(flags & FLAG_SUICIDE) || !!(buttons & BTN_RIGHT) || !!keys.KeyF)
      : (!!keys.KeyF || !!(flags & FLAG_SUICIDE));
    if (wantSuicide && state.alive) suicideArm = true;

    const cand = bestLock(state, B);
    if (cand >= 0 && cand === lockIdx) lockTime += dt;
    else if (!(locked && lockIdx >= 0 && cand < 0)) {
      // sticky: if currently locked, allow brief miss before dropping
      if (locked && lockIdx >= 0 && state.thp[lockIdx] > 0) {
        lockTime = Math.max(0, lockTime - dt * 0.5);
        if (lockTime <= 0) { locked = false; lockIdx = cand; }
      } else {
        lockIdx = cand;
        lockTime = 0;
        locked = false;
      }
    } else {
      lockIdx = cand;
      lockTime = 0;
      locked = false;
    }
    if (lockIdx >= 0 && lockTime >= LOCK_HOLD) locked = true;

    const thit = Array(NT).fill(0);
    let suicide = 0;
    const boost = !!(keys.ShiftLeft || keys.ShiftRight);
    let speedMul = (suicideArm ? 2.05 : 1) * (boost ? 1.35 : 1);

    if (state.alive && fireEdge && locked && state.ammo > 0 && !suicideArm) {
      spawnMissile(lockIdx, B);
      state.ammo -= 1;
      locked = false;
      lockTime = 0;
      flash = 0.12;
    }

    if (state.alive) updateMissiles(dt, thit);

    // orbit after combat so probe face+fire same frame stays valid
    if (state.alive) orbitTargets(dt);

    // suicide detonation on proximity while armed
    if (state.alive && suicideArm) {
      for (let i = 0; i < NT; i++) {
        if (state.thp[i] <= 0) continue;
        const dx = state.txs[i] - state.px;
        const dy = state.tys[i] - state.py;
        const dz = state.tzs[i] - state.pz;
        const d2 = dx * dx + dy * dy + dz * dz;
        if (d2 < SUICIDE_PROX * SUICIDE_PROX) {
          armSuicideBlast(thit);
          suicide = 1;
          endReason = "suicide";
          flash = 0.5;
          break;
        }
      }
      if (!suicide && state.py <= 2.55 && iy >= 0) {
        armSuicideBlast(thit);
        suicide = 1;
        endReason = "suicide";
        flash = 0.5;
      }
    }

    if (!state.alive) {
      if (fireEdge) resetRun();
    } else {
      await stepWith(dt, ix, iy, B, thit, suicide, speedMul);
      const rem = state.thp.filter((h) => h > 0).length;
      if (rem === 0 && state.alive) {
        endReason = "win";
        state.alive = 0;
        state.score += 1000;
      }
      if (!state.alive && !endReason) endReason = suicide ? "suicide" : "crash";
    }

    const remaining = state.thp.filter((h) => h > 0).length;
    host.host_gpu_submit(buildPacket(basis(yaw, pitch)));
    host.host_frame_present();

    accFrames++;
    if (now - lastHud >= 160) {
      const mode = !state.alive
        ? (endReason === "win" ? "任务完成 — 全歼"
          : endReason === "suicide" ? "自爆出击"
            : "失联")
        : suicideArm
          ? "模式 B：自爆冲撞 — 撞向目标！"
          : locked
            ? "模式 A：导弹锁定 — 射击！"
            : state.ammo <= 0
              ? "弹仓空 — F/右键武装自爆"
              : "模式 A：搜索锁定目标";
      opts.onHud?.({
        ready: true, drone: true, fp: true, abi: true,
        score: state.score, kills, remaining, alive: state.alive,
        ammo: state.ammo, magazine: MAGAZINE,
        locked, lockIdx, suicideArm, endReason, mode,
        controls,
        mx, my, buttons, flags, pointerLock,
        missiles: missiles.length,
        pointer: controls === "mouse",
        fps: (accFrames * 1000) / (now - lastHud),
      });
      accFrames = 0;
      lastHud = now;
    }
    host.host_request_frame(tick);
  }

  opts.onHud?.({
    ready: true, drone: true, fp: true, abi: true,
    score: 0, kills: 0, remaining: NT, alive: 1,
    ammo: MAGAZINE, magazine: MAGAZINE,
    locked: false, suicideArm: false, mode: "模式 A：搜索锁定目标",
    controls,
    mx: 0, my: 0, buttons: 0, flags: 0,
    pointer: controls === "mouse", missiles: 0,
  });
  host.host_request_frame(tick);
  return {
    nt: NT,
    magazine: MAGAZINE,
    api,
    getControls: () => controls,
    setControls(next) {
      controls = next === "mouse" ? "mouse" : "keyboard";
      host.setPointerLockEnabled?.(controls === "mouse");
      haveAbs = false;
    },
  };
}

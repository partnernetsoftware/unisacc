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
const LOCK_ALIGN = 0.992;
const LOCK_MAX_DIST = 70;
const LOCK_HOLD = 0.28;

function basis(yaw, pitch) {
  const cp = Math.cos(pitch), sp = Math.sin(pitch);
  const cy = Math.cos(yaw), sy = Math.sin(yaw);
  const fx = cp * sy, fy = sp, fz = cp * cy;
  const rx = cy, ry = 0, rz = -sy;
  return { fx, fy, fz, rx, ry, rz };
}

function freshTargets() {
  const txs = [], tys = [], tzs = [], thp = [];
  for (let i = 0; i < NT; i++) {
    const ang = (i / NT) * Math.PI * 2 + 0.4;
    const r = 22 + (i % 3) * 8;
    txs.push(Math.sin(ang) * r);
    tys.push(4 + (i % 3) * 3);
    tzs.push(Math.cos(ang) * r - 14);
    thp.push(1);
  }
  return { txs, tys, tzs, thp };
}

function freshState() {
  return {
    px: 0, py: 14, pz: 36,
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
    if (dist < 2 || dist > LOCK_MAX_DIST) continue;
    const inv = 1 / dist;
    const align = dx * inv * B.fx + dy * inv * B.fy + dz * inv * B.fz;
    if (align > bestAlign) {
      bestAlign = align;
      best = i;
    }
  }
  return best;
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
  let yaw = Math.PI, pitch = -0.08;
  let lastFire = 0;
  let kills = 0;
  let lockIdx = -1;
  let lockTime = 0;
  let locked = false;
  let suicideArm = false;
  let endReason = ""; // "" | "win" | "ammo" | "suicide" | "crash"
  let last = host.host_time();
  let accFrames = 0, lastHud = last;
  const inputBuf = new ArrayBuffer(INPUT_BYTES);

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
    };
    kills += out.nk || 0;
  }

  function cockpitClouds(B) {
    // frame pieces in view space → world
    const pieces = [
      [-0.55, -0.35, 0.9, 0.12],
      [0.55, -0.35, 0.9, 0.12],
      [0, -0.48, 0.85, 0.55],
      [-0.7, 0.1, 0.95, 0.1],
      [0.7, 0.1, 0.95, 0.1],
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
      color: [0.05, 0.07, 0.09], count: sc.length,
      xyz: new Float32Array(xyz), scale: new Float32Array(sc),
      meshId: MESH_BOX, metalness: 0.6, roughness: 0.35, emissive: [0.01, 0.02, 0.03],
    };
  }

  function buildPacket(B) {
    const aXYZ = [], aS = [], lockXYZ = [], lockS = [];
    for (let i = 0; i < NT; i++) {
      if (state.thp[i] <= 0) continue;
      const arr = (locked && i === lockIdx) ? lockXYZ : aXYZ;
      const sc = (locked && i === lockIdx) ? lockS : aS;
      arr.push(state.txs[i], state.tys[i], state.tzs[i]);
      sc.push(locked && i === lockIdx ? 1.8 : 1.35);
    }
    const eye = [state.px, state.py, state.pz];
    const target = [
      state.px + B.fx * 80,
      state.py + B.fy * 80,
      state.pz + B.fz * 80,
    ];
    const clouds = [
      {
        color: [0.22, 0.26, 0.2], count: gN, xyz: gXYZ, scale: gS,
        meshId: MESH_BOX, metalness: 0.08, roughness: 0.92, emissive: [0, 0, 0],
      },
      cockpitClouds(B),
    ];
    if (aS.length) {
      clouds.push({
        color: [0.75, 0.2, 0.12], count: aS.length,
        xyz: new Float32Array(aXYZ), scale: new Float32Array(aS),
        meshId: MESH_OCTA, metalness: 0.25, roughness: 0.5, emissive: [0.15, 0.02, 0],
      });
    }
    if (lockS.length) {
      clouds.push({
        color: [0.2, 1.0, 0.35], count: lockS.length,
        xyz: new Float32Array(lockXYZ), scale: new Float32Array(lockS),
        meshId: MESH_OCTA, metalness: 0.3, roughness: 0.35, emissive: [0.05, 0.35, 0.08],
      });
    }
    // far aim pip
    clouds.push({
      color: locked ? [0.2, 1, 0.4] : [1, 0.9, 0.2],
      count: 1,
      xyz: new Float32Array([
        state.px + B.fx * 12,
        state.py + B.fy * 12,
        state.pz + B.fz * 12,
      ]),
      scale: new Float32Array([locked ? 0.14 : 0.1]),
      meshId: MESH_BOX, metalness: 0.2, roughness: 0.4,
      emissive: locked ? [0.05, 0.4, 0.1] : [0.35, 0.3, 0.02],
    });
    return encodeRenderPacket({
      clear: [0.35, 0.48, 0.58, 1],
      camera: { fovy: Math.PI / 2.35, near: 0.08, far: 260, eye, target },
      fog: { density: 0.011, color: [0.4, 0.52, 0.6] },
      ambient: { color: [0.45, 0.5, 0.55], intensity: 0.7 },
      lights: [
        { dir: [0.25, 1, 0.15], color: [1, 0.95, 0.85], intensity: 1.15 },
        { dir: [-0.4, 0.15, -0.35], color: [0.35, 0.45, 0.65], intensity: 0.4 },
      ],
      clouds,
    });
  }

  function resetRun() {
    state = freshState();
    yaw = Math.PI; pitch = -0.08;
    kills = 0; lockIdx = -1; lockTime = 0; locked = false;
    suicideArm = false; endReason = "";
  }

  async function tick(now) {
    host.host_frame_begin();
    const dt = Math.min(0.05, (now - last) / 1000);
    last = now;

    let mx = 0, my = 0, buttons = 0, flags = 0, ix = 0, iy = 0, fire = 0;
    let keys = {};
    const nIn = host.host_input_read(inputBuf);
    if (nIn === INPUT_BYTES) {
      const input = decodeInputSnapshot(inputBuf);
      ix = input.ix; iy = input.iy; fire = input.fire;
      mx = input.mx; my = input.my; buttons = input.buttons; flags = input.flags;
    }
    // also peek object for KeyF if needed
    const snapObj = host.host_input_read();
    if (snapObj && typeof snapObj === "object") keys = snapObj.keys || {};

    // Look: keyboard (default) = IJKL；mouse = 指针（飞行杆：后拉抬头）
    let lookX = 0, lookY = 0;
    if (controls === "mouse") {
      lookX = mx;
      lookY = my;
    } else {
      if (keys.KeyJ || keys.ArrowLeft) lookX -= 1;
      if (keys.KeyL || keys.ArrowRight) lookX += 1;
      // I = nose up (flight), K = nose down
      if (keys.KeyI || keys.ArrowUp) lookY -= 1;
      if (keys.KeyK || keys.ArrowDown) lookY += 1;
      // Arrows also feed ix/iy in Host — zero strafe/thrust from arrows by
      // re-deriving move from WASD only.
      ix = 0; iy = 0;
      if (keys.KeyA) ix -= 1;
      if (keys.KeyD) ix += 1;
      if (keys.KeyS) iy += 1;
      if (keys.KeyW) iy -= 1;
      fire = keys.Space ? 1 : 0;
    }
    yaw += lookX * 2.4 * dt;
    pitch -= lookY * 2.0 * dt;
    if (pitch > 1.15) pitch = 1.15;
    if (pitch < -1.15) pitch = -1.15;

    const B = basis(yaw, pitch);
    const fireEdge = fire && !lastFire;
    lastFire = fire;

    const wantSuicide = controls === "mouse"
      ? (!!(flags & FLAG_SUICIDE) || !!(buttons & BTN_RIGHT) || !!keys.KeyF)
      : (!!keys.KeyF || !!(flags & FLAG_SUICIDE));
    if (wantSuicide && state.alive) suicideArm = true;

    // acquire lock
    const cand = bestLock(state, B);
    if (cand >= 0 && cand === lockIdx) lockTime += dt;
    else {
      lockIdx = cand;
      lockTime = 0;
      locked = false;
    }
    if (lockIdx >= 0 && lockTime >= LOCK_HOLD) locked = true;

    const thit = Array(NT).fill(0);
    let suicide = 0;
    let speedMul = suicideArm ? 1.85 : 1;

    if (state.alive && fireEdge && locked && state.ammo > 0) {
      thit[lockIdx] = 1;
      state.ammo -= 1;
      locked = false;
      lockTime = 0;
    }

    // suicide detonation on proximity while armed
    if (state.alive && suicideArm) {
      for (let i = 0; i < NT; i++) {
        if (state.thp[i] <= 0) continue;
        const dx = state.txs[i] - state.px;
        const dy = state.tys[i] - state.py;
        const dz = state.tzs[i] - state.pz;
        const d2 = dx * dx + dy * dy + dz * dz;
        if (d2 < 6.5) {
          // wipe nearby
          for (let j = 0; j < NT; j++) {
            if (state.thp[j] <= 0) continue;
            const ex = state.txs[j] - state.px;
            const ey = state.tys[j] - state.py;
            const ez = state.tzs[j] - state.pz;
            if (ex * ex + ey * ey + ez * ez < 100) thit[j] = 1;
          }
          suicide = 1;
          endReason = "suicide";
          break;
        }
      }
      // ground kamikaze
      if (state.py <= 3.05 && iy >= 0) {
        for (let j = 0; j < NT; j++) {
          if (state.thp[j] <= 0) continue;
          const ex = state.txs[j] - state.px;
          const ez = state.tzs[j] - state.pz;
          if (ex * ex + ez * ez < 120) thit[j] = 1;
        }
        suicide = 1;
        endReason = "suicide";
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
      } else if (state.ammo <= 0 && !suicideArm && rem > 0) {
        // can still suicide
      }
      if (!state.alive && !endReason) endReason = suicide ? "suicide" : "crash";
    }

    const remaining = state.thp.filter((h) => h > 0).length;
    host.host_gpu_submit(buildPacket(basis(yaw, pitch)));
    host.host_frame_present();

    accFrames++;
    if (now - lastHud >= 180) {
      const mode = !state.alive
        ? (endReason === "win" ? "任务完成" : endReason === "suicide" ? "自爆出击" : "失联")
        : suicideArm
          ? "模式：自爆冲撞（撞上去）"
          : locked
            ? "模式：导弹锁定 — 射击！"
            : "模式：搜索锁定目标";
      opts.onHud?.({
        ready: true, drone: true, fp: true, abi: true,
        score: state.score, kills, remaining, alive: state.alive,
        ammo: state.ammo, magazine: MAGAZINE,
        locked, lockIdx, suicideArm, endReason, mode,
        controls,
        mx, my, buttons, flags,
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
    locked: false, suicideArm: false, mode: "模式：搜索锁定目标",
    controls,
    mx: 0, my: 0, buttons: 0, flags: 0, pointer: controls === "mouse",
  });
  host.host_request_frame(tick);
  return {
    nt: NT,
    magazine: MAGAZINE,
    getControls: () => controls,
    setControls(next) {
      controls = next === "mouse" ? "mouse" : "keyboard";
      host.setPointerLockEnabled?.(controls === "mouse");
    },
  };
}

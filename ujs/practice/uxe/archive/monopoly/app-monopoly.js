/**
 * City Monopoly core — ARCHIVED (not on Pages / not in test:uxe:all).
 * See web/uxe/archive/README.md. Host ABI only; UJS rules in monopoly.ujs.
 * Dice RNG lives in host (UJS-1 has no random).
 */
import { bootRuntime, wasm_run, unwrap } from "../core/wasm_run.js";
import { encodeRenderPacket, MESH_OCTA } from "./packet.js";
import { MESH_BOX } from "./meshes.js";
import { decodeInputSnapshot, INPUT_BYTES } from "./input.js";

const CELLS = 16;
const SIDE = 4;
const AI_MS = 650;

const CELL_NAME = [
  "起点", "港湾路", "市场街", "车站",
  "公园", "滨江道", "剧院", "银行",
  "广场", "科技园", "大学城", "机场",
  "码头", "老街", "会展", "酒店",
];

/** Map cell index → XZ on a square ring. */
export function cellXZ(i) {
  const s = SIDE;
  if (i < s) return { x: i - (s - 1) / 2, z: (s - 1) / 2 };
  if (i < s * 2) return { x: (s - 1) / 2, z: (s - 1) / 2 - (i - s) };
  if (i < s * 3) return { x: (s - 1) / 2 - (i - s * 2), z: -(s - 1) / 2 };
  return { x: -(s - 1) / 2, z: -(s - 1) / 2 + (i - s * 3) };
}

function freshState() {
  const owned = [], houses = [];
  for (let i = 0; i < CELLS; i++) { owned.push(-1); houses.push(0); }
  return {
    turn: 0, phase: 0,
    pos0: 0, pos1: 0,
    money0: 1500, money1: 1500,
    owned, houses,
    msg: 0, dice: 0, winner: -1,
  };
}

function cellPrice(c) {
  if (c === 0 || c === 4 || c === 8 || c === 12) return 0;
  return 60 + (c % 4) * 40 + Math.floor(c / 4) * 20;
}

function buildCost(c, houses) {
  return 50 + (houses[c] || 0) * 30;
}

/**
 * @param {import("./host-abi.js").HostAbi} host
 * @param {{ simUrl?: string, wasmUrl: string, precompiled?: object, onHud?: Function }} opts
 */
export async function runMonopolyCore(host, opts) {
  host.host_log("info", "monopoly core boot");
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
    // Ship build defines __UXE_SHIP__=true so this branch is DCE'd (no compiler).
    if (typeof __UXE_SHIP__ !== "undefined" && __UXE_SHIP__) {
      throw new Error("monopoly ship requires precompiled sim");
    }
    const { compile } = await import("../compiler.js");
    const simText = new TextDecoder().decode(await host.host_asset_read(opts.simUrl));
    const compiled = compile(simText);
    fnImage = compiled.image;
    fnBlob = compiled.blob;
  }

  let state = freshState();
  const warm = await wasm_run({ image: fnImage, blob: fnBlob }, {
    ...state, action: 4, dice: 0,
  }, {});
  if (warm.err) throw new Error(JSON.stringify(warm.err));
  state = { ...state, ...unwrap(warm) };

  const inputBuf = new ArrayBuffer(INPUT_BYTES);
  let lastFire = 0;
  let lastSkip = 0;
  let aiLock = false;
  let nextAiAt = 0;
  let accFrames = 0, lastHud = host.host_time();
  const t0 = host.host_time();

  async function step(action, dice) {
    const r = await wasm_run({ image: fnImage, blob: fnBlob }, {
      turn: state.turn, phase: state.phase,
      pos0: state.pos0, pos1: state.pos1,
      money0: state.money0, money1: state.money1,
      owned: state.owned, houses: state.houses,
      action, dice: dice || 0,
    }, {});
    if (r.err) {
      host.host_log("error", "ujs " + JSON.stringify(r.err));
      return;
    }
    const out = unwrap(r);
    state = {
      turn: out.turn, phase: out.phase,
      pos0: out.pos0, pos1: out.pos1,
      money0: out.money0, money1: out.money1,
      owned: out.owned, houses: out.houses,
      msg: out.msg, dice: out.dice, winner: out.winner,
    };
  }

  function buildPacket(now) {
    const ang = ((now - t0) / 1000) * 0.12;
    const eyeR = 18, eyeY = 13;
    const eye = [Math.sin(ang) * eyeR, eyeY, Math.cos(ang) * eyeR];

    const freeXyz = [], freeS = [], cornerXyz = [], cornerS = [];
    const owned0Xyz = [], owned0S = [], owned1Xyz = [], owned1S = [];
    for (let i = 0; i < CELLS; i++) {
      const { x, z } = cellXZ(i);
      const px = x * 2.2, pz = z * 2.2;
      const corner = i === 0 || i === 4 || i === 8 || i === 12;
      if (state.owned[i] === 0) { owned0Xyz.push(px, 0.08, pz); owned0S.push(1.9); }
      else if (state.owned[i] === 1) { owned1Xyz.push(px, 0.08, pz); owned1S.push(1.9); }
      else if (corner) { cornerXyz.push(px, 0.08, pz); cornerS.push(1.95); }
      else { freeXyz.push(px, 0.08, pz); freeS.push(1.9); }
    }
    const b0xyz = [], b0s = [], b1xyz = [], b1s = [];
    for (let i = 0; i < CELLS; i++) {
      if (state.owned[i] < 0) continue;
      const { x, z } = cellXZ(i);
      const h = 0.45 + (state.houses[i] || 0) * 0.42;
      const arr = state.owned[i] === 0 ? b0xyz : b1xyz;
      const sc = state.owned[i] === 0 ? b0s : b1s;
      arr.push(x * 2.2, 0.16 + h / 2, z * 2.2);
      sc.push(0.55 + h * 0.22);
    }
    const bob = 0.04 * Math.sin((now - t0) / 180);
    const p0 = cellXZ(state.pos0);
    const p1 = cellXZ(state.pos1);
    const t0a = new Float32Array([p0.x * 2.2, 0.55 + bob, p0.z * 2.2]);
    const t1a = new Float32Array([p1.x * 2.2 + 0.28, 0.55 - bob, p1.z * 2.2 + 0.28]);
    const sTok = new Float32Array([0.38]);

    const boxCloud = (color, xyz, s, em = [0, 0, 0]) => ({
      color, count: s.length, xyz: new Float32Array(xyz), scale: new Float32Array(s),
      meshId: MESH_BOX, metalness: 0.15, roughness: 0.8, emissive: em,
    });
    const clouds = [
      // center plaza
      {
        color: [0.14, 0.18, 0.24], count: 1,
        xyz: new Float32Array([0, -0.05, 0]), scale: new Float32Array([5.2]),
        meshId: MESH_BOX, metalness: 0.05, roughness: 0.95, emissive: [0, 0, 0],
      },
    ];
    if (freeS.length) clouds.push(boxCloud([0.22, 0.28, 0.36], freeXyz, freeS));
    if (cornerS.length) clouds.push(boxCloud([0.42, 0.36, 0.22], cornerXyz, cornerS, [0.05, 0.04, 0.01]));
    if (owned0S.length) clouds.push(boxCloud([0.2, 0.42, 0.7], owned0Xyz, owned0S, [0.02, 0.05, 0.1]));
    if (owned1S.length) clouds.push(boxCloud([0.7, 0.32, 0.22], owned1Xyz, owned1S, [0.1, 0.03, 0.02]));
    clouds.push(
      {
        color: [0.3, 0.75, 1.0], count: 1, xyz: t0a, scale: sTok,
        meshId: MESH_OCTA, metalness: 0.4, roughness: 0.3, emissive: [0.05, 0.15, 0.25],
      },
      {
        color: [1.0, 0.45, 0.35], count: 1, xyz: t1a, scale: sTok,
        meshId: MESH_OCTA, metalness: 0.4, roughness: 0.3, emissive: [0.25, 0.08, 0.05],
      },
    );
    if (b0s.length) {
      clouds.push({
        color: [0.25, 0.55, 0.95], count: b0s.length,
        xyz: new Float32Array(b0xyz), scale: new Float32Array(b0s),
        meshId: MESH_BOX, metalness: 0.35, roughness: 0.4, emissive: [0.02, 0.05, 0.12],
      });
    }
    if (b1s.length) {
      clouds.push({
        color: [0.95, 0.4, 0.28], count: b1s.length,
        xyz: new Float32Array(b1xyz), scale: new Float32Array(b1s),
        meshId: MESH_BOX, metalness: 0.35, roughness: 0.4, emissive: [0.12, 0.03, 0.02],
      });
    }
    return encodeRenderPacket({
      clear: [0.05, 0.07, 0.1, 1],
      camera: {
        fovy: Math.PI / 3.2, near: 0.1, far: 90,
        eye, target: [0, 0.2, 0],
      },
      fog: { density: 0.014, color: [0.05, 0.07, 0.1] },
      ambient: { color: [0.32, 0.36, 0.42], intensity: 0.7 },
      lights: [
        { dir: [0.35, 1.0, 0.25], color: [1.0, 0.95, 0.88], intensity: 1.15 },
        { dir: [-0.4, 0.3, -0.35], color: [0.35, 0.48, 0.75], intensity: 0.45 },
      ],
      clouds,
    });
  }

  async function maybeAi(now) {
    if (aiLock || state.phase === 2 || state.turn !== 1) return;
    if (now < nextAiAt) return;
    aiLock = true;
    try {
      if (state.phase === 0) {
        const dice = 1 + ((Math.random() * 6) | 0) + 1 + ((Math.random() * 6) | 0);
        await step(1, dice);
      } else if (state.phase === 1) {
        const price = cellPrice(state.pos1);
        if (state.money1 >= price && Math.random() > 0.25) await step(2, 0);
        else await step(3, 0);
      } else if (state.phase === 3) {
        const cost = buildCost(state.pos1, state.houses);
        if (state.money1 >= cost && Math.random() > 0.35) await step(5, 0);
        else await step(3, 0);
      }
      nextAiAt = now + AI_MS;
    } finally {
      aiLock = false;
    }
  }

  function humanCell() {
    return state.turn === 0 ? state.pos0 : state.pos1;
  }

  async function tick(now) {
    host.host_frame_begin();
    const nIn = host.host_input_read(inputBuf);
    if (nIn === INPUT_BYTES) {
      const input = decodeInputSnapshot(inputBuf);
      const skipEdge = input.ix < 0 && !lastSkip;
      if (state.turn === 0 && (state.phase === 1 || state.phase === 3) && skipEdge) {
        await step(3, 0);
        nextAiAt = now + AI_MS;
      } else if (input.fire && !lastFire) {
        if (state.phase === 2) {
          await step(4, 0);
        } else if (state.turn === 0 && state.phase === 0) {
          const dice = 1 + ((Math.random() * 6) | 0) + 1 + ((Math.random() * 6) | 0);
          await step(1, dice);
          nextAiAt = now + AI_MS;
        } else if (state.turn === 0 && state.phase === 1) {
          const price = cellPrice(state.pos0);
          if (state.money0 >= price) await step(2, 0);
          else await step(3, 0);
          nextAiAt = now + AI_MS;
        } else if (state.turn === 0 && state.phase === 3) {
          const cost = buildCost(state.pos0, state.houses);
          if (state.money0 >= cost) await step(5, 0);
          else await step(3, 0);
          nextAiAt = now + AI_MS;
        }
      }
      lastFire = input.fire;
      lastSkip = input.ix < 0 ? 1 : 0;
    }

    await maybeAi(now);

    host.host_gpu_submit(buildPacket(now));
    host.host_frame_present();

    accFrames++;
    if (now - lastHud >= 350) {
      const msgs = [
        "", "等待操作", "可购买", "付租金", "购入", "破产", "胜负已定", "可建房", "建房完成",
      ];
      const cell = humanCell();
      const phaseHint =
        state.phase === 0 ? "空格掷骰" :
        state.phase === 1 ? "空格买 · A 跳过" :
        state.phase === 3 ? "空格建房 · A 跳过" :
        "空格重开";
      opts.onHud?.({
        ready: true, monopoly: true,
        turn: state.turn, phase: state.phase,
        money0: state.money0, money1: state.money1,
        pos0: state.pos0, pos1: state.pos1,
        dice: state.dice, msg: msgs[state.msg] || "",
        winner: state.winner,
        cellName: CELL_NAME[cell] || "",
        houses: state.houses[cell] || 0,
        phaseHint,
        owned0: state.owned.filter((o) => o === 0).length,
        owned1: state.owned.filter((o) => o === 1).length,
        fps: (accFrames * 1000) / (now - lastHud),
      });
      accFrames = 0;
      lastHud = now;
    }
    host.host_request_frame(tick);
  }

  opts.onHud?.({
    ready: true, monopoly: true, turn: 0, phase: 0,
    money0: 1500, money1: 1500, pos0: 0, pos1: 0, dice: 0,
    phaseHint: "空格掷骰", owned0: 0, owned1: 0,
  });
  host.host_request_frame(tick);
  return { cells: CELLS };
}

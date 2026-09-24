/**
 * Game core (JS stand-in for future single wasm module).
 * Talks ONLY via Host ABI + wasm_run for UJS sim — no canvas/WebGL imports.
 */
import { bootRuntime, wasm_run, unwrap } from "../core/wasm_run.js";
import { encodeRenderPacket } from "./packet.js";
import { decodeInputSnapshot, INPUT_BYTES } from "./input.js";

const N = 480;

function freshState() {
  const xs = [], ys = [], zs = [], vxs = [], vys = [], vzs = [], rs = [];
  for (let i = 0; i < N; i++) {
    let sx = ((i * 13) % 37) - 18;
    let sy = ((i * 7) % 25) - 12;
    if (sx > -3.5 && sx < 3.5 && sy > -3.5 && sy < 3.5) sx += 7;
    xs.push(sx); ys.push(sy); zs.push(-40 - i * 1.55);
    vxs.push(((i % 5) - 2) * 0.55);
    vys.push(((i % 3) - 1) * 0.4);
    vzs.push(8 + (i % 11) * 0.35);
    rs.push(0.55 + (i % 5) * 0.22);
  }
  return { xs, ys, zs, vxs, vys, vzs, rs, px: 0, py: 0, pz: 0, score: 0, alive: 1 };
}

/**
 * @param {import("./host-abi.js").HostAbi} host
 * @param {{
 *   simUrl?: string,
 *   wasmUrl: string,
 *   precompiled?: { image: Uint8Array|number[], blob: { globals?: string[], locals?: string[] } },
 *   onHud?: (s: object) => void
 * }} opts
 */
export async function runAsteroidCore(host, opts) {
  host.host_log("info", "core boot abi");
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
      throw new Error("ship build requires precompiled sim embed");
    }
    const { compile } = await import("../core/compiler.js");
    const simText = new TextDecoder().decode(await host.host_asset_read(opts.simUrl));
    const compiled = compile(simText);
    fnImage = compiled.image;
    fnBlob = compiled.blob;
  }

  let state = freshState();
  let alive = true;
  let lastFire = 0;
  let tapMs = 0;

  const warm = await wasm_run({ image: fnImage, blob: fnBlob }, {
    ...state, ix: 0, iy: 0, dt: 0.016,
  }, {});
  if (warm.err) throw new Error(JSON.stringify(warm.err));
  state = { ...state, ...unwrap(warm) };
  alive = !!state.alive;

  const rockXYZ = new Float32Array(N * 3);
  const rockS = new Float32Array(N);
  const shipXYZ = new Float32Array(3);
  const shipS = new Float32Array([0.7]);
  const inputBuf = new ArrayBuffer(INPUT_BYTES);

  let last = host.host_time();
  let accUjs = 0, accGpu = 0, accFrames = 0, lastHud = last;

  function buildPacket() {
    const { xs, ys, zs, rs, px, py, pz } = state;
    for (let i = 0; i < N; i++) {
      rockXYZ[i * 3] = xs[i];
      rockXYZ[i * 3 + 1] = ys[i];
      rockXYZ[i * 3 + 2] = zs[i];
      rockS[i] = rs[i];
    }
    shipXYZ[0] = px; shipXYZ[1] = py; shipXYZ[2] = pz;
    return encodeRenderPacket({
      clear: [0.02, 0.024, 0.04, 1],
      camera: {
        fovy: Math.PI / 3, near: 0.1, far: 300,
        eye: [px * 0.15, py * 0.15 + 2.8, pz + 11],
        target: [px * 0.05, py * 0.05, pz - 18],
      },
      clouds: [
        {
          color: [0.55, 0.58, 0.62], count: N, xyz: rockXYZ, scale: rockS,
          meshId: 0, metalness: 0.15, roughness: 0.75, emissive: [0, 0, 0],
        },
        {
          color: [0.35, 0.75, 1.0], count: 1, xyz: shipXYZ, scale: shipS,
          meshId: 1, metalness: 0.45, roughness: 0.35, emissive: [0.05, 0.12, 0.2],
        },
      ],
    });
  }

  async function tick(now) {
    host.host_frame_begin();
    const dt = Math.min(0.05, (now - last) / 1000);
    last = now;
    // ArrayBuffer roundtrip proves UXIN ABI each frame.
    const nIn = host.host_input_read(inputBuf);
    if (nIn !== INPUT_BYTES) throw new Error("host_input_read bytes " + nIn);
    const input = decodeInputSnapshot(inputBuf);

    // Restart: keyboard Space = single edge; touch = double-tap within 450ms
    const fireEdge = !!(input.fire && !lastFire);
    lastFire = input.fire;
    if (!alive && fireEdge) {
      const touch = !!(input.flags & 4);
      if (!touch) {
        state = freshState();
        alive = true;
        tapMs = 0;
      } else {
        const nowTap = host.host_time();
        if (tapMs > 0 && nowTap - tapMs < 450) {
          state = freshState();
          alive = true;
          tapMs = 0;
        } else {
          tapMs = nowTap;
        }
      }
    }
    if (alive) tapMs = 0;

    let ujsMs = 0;
    if (alive) {
      const t0 = host.host_time();
      const r = await wasm_run({ image: fnImage, blob: fnBlob }, {
        xs: state.xs, ys: state.ys, zs: state.zs,
        vxs: state.vxs, vys: state.vys, vzs: state.vzs, rs: state.rs,
        px: state.px, py: state.py, pz: state.pz,
        ix: input.ix, iy: input.iy, dt, score: state.score, alive: 1,
      }, {});
      ujsMs = host.host_time() - t0;
      if (r.err) {
        host.host_log("error", "ujs " + JSON.stringify(r.err));
        host.host_request_frame(tick);
        return;
      }
      const out = unwrap(r);
      state = {
        xs: out.xs, ys: out.ys, zs: out.zs,
        vxs: out.vxs, vys: out.vys, vzs: out.vzs, rs: out.rs,
        px: out.px, py: out.py, pz: out.pz,
        score: out.score, alive: out.alive,
      };
      if (!out.alive || out.hit) alive = false;
    }

    const g0 = host.host_time();
    host.host_gpu_submit(buildPacket());
    host.host_frame_present();
    const gpuMs = host.host_time() - g0;

    accUjs += ujsMs; accGpu += gpuMs; accFrames += 1;
    if (now - lastHud >= 500) {
      const fps = (accFrames * 1000) / (now - lastHud);
      const snap = {
        ready: true, abi: true, n: N, alive, fps,
        ujsMs: accUjs / accFrames,
        drawMs: accGpu / accFrames,
        score: state.score,
      };
      opts.onHud?.(snap);
      accUjs = 0; accGpu = 0; accFrames = 0; lastHud = now;
    }

    host.host_request_frame(tick);
  }

  opts.onHud?.({ ready: true, abi: true, n: N, score: state.score, alive });
  host.host_request_frame(tick);
  return { n: N };
}

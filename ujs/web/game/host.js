/** Host: Three.js render + input; UJS/wasm runs sim.ujs each frame. */
import * as THREE from "three";
import { bootRuntime, compile, wasm_run, unwrap } from "../wasm_run.js";

const N = 480;
const hud = document.getElementById("hud");
const banner = document.getElementById("banner");
const finalEl = document.getElementById("final");
const canvas = document.getElementById("c");
hud.textContent = "init three…";

const keys = new Set();
addEventListener("keydown", (e) => {
  keys.add(e.code);
  if (e.code === "Space") {
    e.preventDefault();
    if (!alive) resetGame();
  }
});
addEventListener("keyup", (e) => keys.delete(e.code));

function inputAxes() {
  let ix = 0, iy = 0;
  if (keys.has("KeyA") || keys.has("ArrowLeft")) ix -= 1;
  if (keys.has("KeyD") || keys.has("ArrowRight")) ix += 1;
  if (keys.has("KeyS") || keys.has("ArrowDown")) iy -= 1;
  if (keys.has("KeyW") || keys.has("ArrowUp")) iy += 1;
  return { ix, iy };
}

function freshState() {
  const xs = [], ys = [], zs = [], vxs = [], vys = [], vzs = [], rs = [];
  for (let i = 0; i < N; i++) {
    let sx = ((i * 13) % 37) - 18;
    let sy = ((i * 7) % 25) - 12;
    if (sx > -3.5 && sx < 3.5 && sy > -3.5 && sy < 3.5) sx += 7;
    xs.push(sx);
    ys.push(sy);
    zs.push(-40 - i * 1.55);
    vxs.push(((i % 5) - 2) * 0.55);
    vys.push(((i % 3) - 1) * 0.4);
    vzs.push(8 + (i % 11) * 0.35);
    rs.push(0.55 + (i % 5) * 0.22);
  }
  return {
    xs, ys, zs, vxs, vys, vzs, rs,
    px: 0, py: 0, pz: 0,
    score: 0, alive: 1,
  };
}

let state = freshState();
let alive = true;
let fnImage = null;
let fnBlob = null;

function resetGame() {
  state = freshState();
  alive = true;
  banner.classList.remove("show");
}

let renderer, scene, camera, player, rocks;
const _m = new THREE.Matrix4();
const _q = new THREE.Quaternion();
const _s = new THREE.Vector3();
const _p = new THREE.Vector3();

function buildScene() {
  renderer = new THREE.WebGLRenderer({ canvas, antialias: true, powerPreference: "high-performance" });
  renderer.setPixelRatio(Math.min(devicePixelRatio, 2));
  renderer.setSize(innerWidth || 800, innerHeight || 600, false);
  renderer.outputColorSpace = THREE.SRGBColorSpace;

  scene = new THREE.Scene();
  scene.background = new THREE.Color(0x05060a);
  scene.fog = new THREE.FogExp2(0x05060a, 0.018);

  camera = new THREE.PerspectiveCamera(60, (innerWidth || 800) / (innerHeight || 600), 0.1, 300);
  camera.position.set(0, 3, 12);

  scene.add(new THREE.AmbientLight(0x406080, 0.55));
  const sun = new THREE.DirectionalLight(0xffe6c8, 1.15);
  sun.position.set(4, 10, 6);
  scene.add(sun);
  const rim = new THREE.DirectionalLight(0x6688ff, 0.45);
  rim.position.set(-6, 2, -4);
  scene.add(rim);

  const starGeo = new THREE.BufferGeometry();
  const starN = 800;
  const starPos = new Float32Array(starN * 3);
  for (let i = 0; i < starN; i++) {
    starPos[i * 3] = (Math.random() - 0.5) * 160;
    starPos[i * 3 + 1] = (Math.random() - 0.5) * 100;
    starPos[i * 3 + 2] = -Math.random() * 200;
  }
  starGeo.setAttribute("position", new THREE.BufferAttribute(starPos, 3));
  scene.add(new THREE.Points(starGeo, new THREE.PointsMaterial({ color: 0xaaccff, size: 0.08, sizeAttenuation: true })));

  player = new THREE.Mesh(
    new THREE.ConeGeometry(0.55, 1.6, 6),
    new THREE.MeshStandardMaterial({ color: 0x5ec8ff, emissive: 0x123048, metalness: 0.35, roughness: 0.4 }),
  );
  player.rotation.x = Math.PI / 2;
  scene.add(player);

  const rockGeo = new THREE.IcosahedronGeometry(1, 0);
  const rockMat = new THREE.MeshStandardMaterial({
    color: 0x8a909c, roughness: 0.85, metalness: 0.1, flatShading: true,
  });
  rocks = new THREE.InstancedMesh(rockGeo, rockMat, N);
  rocks.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
  scene.add(rocks);
}

function syncMeshes() {
  const { xs, ys, zs, rs, px, py, pz } = state;
  player.position.set(px, py, pz);
  for (let i = 0; i < N; i++) {
    _p.set(xs[i], ys[i], zs[i]);
    _q.setFromEuler(new THREE.Euler(xs[i] * 0.1, ys[i] * 0.13, zs[i] * 0.02));
    _s.setScalar(rs[i]);
    _m.compose(_p, _q, _s);
    rocks.setMatrixAt(i, _m);
  }
  rocks.instanceMatrix.needsUpdate = true;
  camera.position.lerp(new THREE.Vector3(px * 0.15, py * 0.15 + 2.8, pz + 11), 0.12);
  camera.lookAt(px * 0.05, py * 0.05, pz - 18);
}

addEventListener("resize", () => {
  if (!camera || !renderer) return;
  camera.aspect = innerWidth / innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(innerWidth, innerHeight, false);
});

async function main() {
  let hasWebGL = true;
  try {
    buildScene();
  } catch (e) {
    hasWebGL = false;
    hud.innerHTML = `<span class="warn">WebGL unavailable</span> — sim-only<br>${String(e.message || e)}`;
    console.warn("three init failed, continuing sim-only", e);
  }
  hud.textContent = (hasWebGL ? "" : "[sim-only] ") + "loading wasm + compiling sim.ujs…";
  await bootRuntime(new URL("../ujs_full.wasm", import.meta.url));
  const src = await fetch(new URL("./sim.ujs", import.meta.url)).then((r) => {
    if (!r.ok) throw new Error("sim.ujs " + r.status);
    return r.text();
  });
  const compiled = compile(src);
  fnImage = compiled.image;
  fnBlob = compiled.blob;

  const warm = await wasm_run({ image: fnImage, blob: fnBlob }, {
    ...state, ix: 0, iy: 0, dt: 0.016,
  }, {});
  if (warm.err) throw new Error(JSON.stringify(warm.err));
  state = { ...state, ...unwrap(warm) };
  alive = !!state.alive;
  window.__UJS_GAME__ = { ready: true, n: N, webgl: hasWebGL };

  let last = performance.now();
  let accUjs = 0, accFrames = 0, fps = 0, lastFpsT = last;
  let ujsMs = 0;

  function schedule(fn) {
    if (typeof requestAnimationFrame === "function") requestAnimationFrame(fn);
    else setTimeout(() => fn(performance.now()), 16);
  }

  async function frame(now) {
    const dt = Math.min(0.05, (now - last) / 1000);
    last = now;
    const { ix, iy } = inputAxes();

    if (alive) {
      const t0 = performance.now();
      const r = await wasm_run({ image: fnImage, blob: fnBlob }, {
        xs: state.xs, ys: state.ys, zs: state.zs,
        vxs: state.vxs, vys: state.vys, vzs: state.vzs, rs: state.rs,
        px: state.px, py: state.py, pz: state.pz,
        ix, iy, dt, score: state.score, alive: 1,
      }, {});
      ujsMs = performance.now() - t0;
      if (r.err) {
        hud.innerHTML = `<span class="warn">UJS trap</span><br>${r.err.message || JSON.stringify(r.err)}`;
        schedule(frame);
        return;
      }
      const out = unwrap(r);
      state = {
        xs: out.xs, ys: out.ys, zs: out.zs,
        vxs: out.vxs, vys: out.vys, vzs: out.vzs, rs: out.rs,
        px: out.px, py: out.py, pz: out.pz,
        score: out.score, alive: out.alive,
      };
      if (!out.alive || out.hit) {
        alive = false;
        finalEl.textContent = out.score.toFixed(0);
        banner.classList.add("show");
      }
    }

    if (hasWebGL) {
      syncMeshes();
      renderer.render(scene, camera);
    }

    accUjs += ujsMs;
    accFrames += 1;
    if (now - lastFpsT >= 500) {
      fps = (accFrames * 1000) / (now - lastFpsT);
      const avg = accUjs / Math.max(1, accFrames);
      hud.innerHTML =
        `<b>Asteroid Rush</b> · UJS-1 in wasm${hasWebGL ? "" : " · sim-only"}<br>` +
        `entities <b>${N}</b><br>` +
        `ujs step <b>${avg.toFixed(2)} ms</b> (last ${ujsMs.toFixed(2)})<br>` +
        `fps <b>${fps.toFixed(0)}</b><br>` +
        `score <b>${state.score.toFixed(0)}</b>` +
        (alive ? "" : `<br><span class="warn">crashed — Space</span>`);
      window.__UJS_GAME__ = {
        ready: true, n: N, fps, ujsMs: avg, score: state.score, alive, webgl: hasWebGL,
      };
      accUjs = 0;
      accFrames = 0;
      lastFpsT = now;
    }
    schedule(frame);
  }
  setTimeout(() => frame(performance.now()), 0);
}

main().catch((e) => {
  hud.innerHTML = `<span class="warn">boot failed</span><br>${String(e.message || e)}<br>run <code>python3 -m ujs web-build</code>`;
  window.__UJS_GAME__ = { ready: false, error: String(e.message || e) };
  console.error(e);
});

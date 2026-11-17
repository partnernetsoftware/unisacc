import { spawn } from "child_process";
import { setTimeout as sleep } from "timers/promises";
import http from "http";
import path from "path";
import { fileURLToPath } from "url";
import {
  spawnChrome, makeCdp, connectPage, directEnv,
} from "../../_cdp.mjs";

const WALL_MS = 25_000;
const PORT_HTTP = 8844;
const ORIGIN = "http://127.0.0.1:" + PORT_HTTP;
const URL = ORIGIN + "/demo/gpu-tri/index.html?auto=1&t=" + Date.now();
const PORT = 9344;
const PROFILE = "/tmp/uxe-gpu-tri-" + process.pid;
const WEB_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../..");
const deadline = Date.now() + WALL_MS;
const left = () => {
  const ms = deadline - Date.now();
  if (ms <= 0) throw new Error("probe wall");
  return ms;
};

const child = spawn("python3", ["-m", "http.server", String(PORT_HTTP), "--bind", "127.0.0.1", "--directory", WEB_ROOT], {
  stdio: ["ignore", "ignore", "ignore"],
  env: directEnv(),
});
const chrome = spawnChrome({ port: PORT, profile: PROFILE, startUrl: "about:blank" });
const cdp = makeCdp(left);

function stop() {
  try { chrome.kill("SIGKILL"); } catch {}
  try { child.kill("SIGKILL"); } catch {}
}

try {
  for (let i = 0; i < 40; i++) {
    try {
      await new Promise((res, rej) => {
        const req = http.get({ hostname: "127.0.0.1", port: PORT_HTTP, path: "/demo/gpu-tri/index.html", timeout: 400, agent: false }, (r) => {
          r.resume();
          if (r.statusCode === 200) res();
          else rej(new Error("http " + r.statusCode));
        });
        req.on("error", rej);
        req.on("timeout", () => { req.destroy(); rej(new Error("t")); });
      });
      break;
    } catch {
      await sleep(100);
      if (i === 39) throw new Error("http not up");
    }
  }
  const ws = await connectPage(PORT, left);
  await cdp(ws, 1, "Runtime.enable");
  await cdp(ws, 2, "Page.enable");
  await cdp(ws, 3, "Page.navigate", { url: URL });
  let n = 0;
  while (Date.now() < deadline) {
    n++;
    const r = await cdp(ws, 100 + n, "Runtime.evaluate", {
      expression: `({ t: document.title, uxe: window.__UXE__ || null })`,
      returnByValue: true,
    });
    const v = r.result.value || {};
    if (v.t && v.t !== "gpu-tri" && v.uxe) {
      console.log("GPU_TRI_PROBE", v.t, JSON.stringify(v.uxe));
      ws.close(); stop();
      process.exit(v.t === "GPU_TRI_FAIL" ? 1 : 0);
    }
    await sleep(Math.min(400, left()));
  }
  throw new Error("never titled");
} catch (e) {
  stop();
  console.error("GPU_TRI_PROBE_FAIL", e.message || e);
  process.exit(2);
}

/** CDP probe for UXE demo — hard wall inside. */
import { setTimeout as sleep } from "timers/promises";
import {
  DEMO_ORIGIN, spawnChrome, makeCdp, connectPage, ensureDemoServer,
} from "../_cdp.mjs";

const WALL_MS = 40_000;
const URL = DEMO_ORIGIN + "/engine/demo/?t=" + Date.now();
const PORT = 9336;
const PROFILE = "/tmp/uxe-chrome-" + process.pid;
const deadline = Date.now() + WALL_MS;
const left = () => {
  const ms = deadline - Date.now();
  if (ms <= 0) throw new Error("probe wall exceeded");
  return ms;
};

const server = await ensureDemoServer();
const chrome = spawnChrome({ port: PORT, profile: PROFILE });
const cdp = makeCdp(left);

try {
  const ws = await connectPage(PORT, left);
  await cdp(ws, 1, "Runtime.enable");
  await cdp(ws, 2, "Page.enable");
  await cdp(ws, 3, "Page.navigate", { url: URL });

  let n = 0;
  while (Date.now() < deadline) {
    n++;
    const r = await cdp(ws, 100 + n, "Runtime.evaluate", {
      expression: `({ uxe: window.__UXE__ || null, hud: document.getElementById('hud')?.innerText || '' })`,
      returnByValue: true,
    });
    const v = r.result.value || {};
    if (v.uxe?.ready && v.uxe.ujsMs != null && v.uxe.abi) {
      console.log("OK_UXE", v.uxe);
      ws.close(); chrome.kill("SIGKILL"); server.stop(); process.exit(0);
    }
    if (v.uxe?.ready === false) throw new Error(v.uxe.error);
    if (/boot failed|UJS trap/i.test(v.hud || "")) throw new Error(v.hud);
    await sleep(Math.min(350, left()));
  }
  throw new Error("never ready");
} catch (e) {
  try { chrome.kill("SIGKILL"); } catch {}
  try { server.stop(); } catch {}
  console.error("FAIL_UXE", e.message || e);
  process.exit(1);
}

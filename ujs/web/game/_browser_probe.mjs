/** Headless Chrome CDP probe with hard wall clock. */
import { spawn } from "child_process";
import { setTimeout as sleep } from "timers/promises";
import fs from "fs";

const WALL_MS = 40_000;
const CHROME = "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome";
const URL = "http://127.0.0.1:8765/game/?t=" + Date.now();
const PORT = 9334;
const PROFILE = "/tmp/ujs-game-chrome-profile-" + process.pid;

const deadline = Date.now() + WALL_MS;
function left() {
  const ms = deadline - Date.now();
  if (ms <= 0) throw new Error("probe wall clock exceeded");
  return ms;
}

fs.rmSync(PROFILE, { recursive: true, force: true });
fs.mkdirSync(PROFILE, { recursive: true });

const chrome = spawn(CHROME, [
  `--remote-debugging-port=${PORT}`,
  "--headless=new",
  "--disable-gpu",
  "--use-angle=swiftshader",
  "--enable-webgl",
  "--ignore-gpu-blocklist",
  "--no-first-run",
  "--disable-extensions",
  `--user-data-dir=${PROFILE}`,
  "--autoplay-policy=no-user-gesture-required",
  URL,
], { stdio: ["ignore", "ignore", "pipe"] });

let stderr = "";
chrome.stderr.on("data", (d) => {
  stderr += d.toString();
  if (stderr.length > 12000) stderr = stderr.slice(-6000);
});

async function waitPort() {
  while (Date.now() < deadline) {
    try {
      const r = await fetch(`http://127.0.0.1:${PORT}/json/version`);
      if (r.ok) return;
    } catch (_) {}
    await sleep(Math.min(200, left()));
  }
  throw new Error("chrome debug port not up");
}

function cdp(ws, id, method, params = {}) {
  return new Promise((resolve, reject) => {
    const t = setTimeout(() => reject(new Error("cdp timeout " + method)), Math.min(12000, left()));
    const onMsg = (ev) => {
      let msg;
      try { msg = JSON.parse(ev.data); } catch { return; }
      if (msg.id !== id) return;
      clearTimeout(t);
      ws.removeEventListener("message", onMsg);
      if (msg.error) reject(new Error(JSON.stringify(msg.error)));
      else resolve(msg.result);
    };
    ws.addEventListener("message", onMsg);
    ws.send(JSON.stringify({ id, method, params }));
  });
}

const logs = [];
try {
  await waitPort();
  let page;
  for (;;) {
    const tabs = await (await fetch(`http://127.0.0.1:${PORT}/json/list`)).json();
    page = tabs.find((t) => t.type === "page") || tabs[0];
    if (page?.webSocketDebuggerUrl) break;
    await sleep(Math.min(200, left()));
  }

  const ws = new WebSocket(page.webSocketDebuggerUrl);
  await new Promise((res, rej) => {
    ws.addEventListener("open", res);
    ws.addEventListener("error", rej);
    setTimeout(() => rej(new Error("ws open timeout")), Math.min(8000, left()));
  });

  await cdp(ws, 1, "Runtime.enable");
  await cdp(ws, 2, "Console.enable").catch(() => {});
  ws.addEventListener("message", (ev) => {
    let msg;
    try { msg = JSON.parse(ev.data); } catch { return; }
    if (msg.method === "Runtime.consoleAPICalled") {
      const args = (msg.params.args || []).map((a) => a.value ?? a.description).join(" ");
      logs.push((msg.params.type || "log") + ": " + args);
    }
    if (msg.method === "Runtime.exceptionThrown") {
      logs.push("exception: " + (msg.params.exceptionDetails?.text || JSON.stringify(msg.params)));
    }
  });

  // force navigate in case about:blank
  await cdp(ws, 3, "Page.enable");
  await cdp(ws, 4, "Page.navigate", { url: URL });

  let last = "";
  let n = 0;
  while (Date.now() < deadline) {
    n++;
    const r = await cdp(ws, 100 + n, "Runtime.evaluate", {
      expression: `({
        url: location.href,
        hud: document.getElementById('hud')?.innerText || '',
        game: window.__UJS_GAME__ || null,
        engine: document.querySelector('canvas')?.dataset?.engine || ''
      })`,
      returnByValue: true,
    });
    const v = r.result.value || {};
    last = JSON.stringify(v);
    if (v.game?.ready && (v.game.ujsMs != null || /ujs step|entities/i.test(v.hud))) {
      console.log("OK_BROWSER", {
        n,
        webgl: v.game.webgl,
        ujsMs: v.game.ujsMs,
        fps: v.game.fps,
        score: v.game.score,
        hud: (v.hud || "").replace(/\n/g, " | "),
      });
      ws.close();
      chrome.kill("SIGKILL");
      process.exit(0);
    }
    if (v.game && v.game.ready === false) throw new Error("boot error: " + v.game.error);
    if (/boot failed|UJS trap|lex bad|CompileError/i.test(v.hud || "")) throw new Error("hud error: " + v.hud);
    // ready after warm but before hud refresh
    if (v.game?.ready === true && v.game.n === 480) {
      // wait a bit more for first hud stats; accept ready alone after 2s of seeing it
      if (n > 8) {
        console.log("OK_BROWSER_READY", { n, game: v.game, hud: (v.hud || "").replace(/\n/g, " | ") });
        ws.close();
        chrome.kill("SIGKILL");
        process.exit(0);
      }
    }
    await sleep(Math.min(350, left()));
  }
  throw new Error("never ready; last=" + last + " logs=" + JSON.stringify(logs.slice(-8)) + " stderr_tail=" + stderr.slice(-400));
} catch (e) {
  try { chrome.kill("SIGKILL"); } catch (_) {}
  console.error("FAIL_BROWSER", e.message || e);
  if (logs.length) console.error("console", logs.slice(-12));
  process.exit(1);
}

/**
 * Shared headless Chrome CDP helpers for UXE probes.
 * Always bypass HTTP proxies (local 8765 is often shadowed by http_proxy).
 */
import { spawn } from "child_process";
import { setTimeout as sleep } from "timers/promises";
import fs from "fs";
import http from "http";
import path from "path";
import { fileURLToPath } from "url";

export const CHROME =
  process.env.UXE_CHROME ||
  "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome";

export const DEMO_ORIGIN = "http://127.0.0.1:8765";

const WEB_ROOT = path.resolve(
  path.dirname(fileURLToPath(import.meta.url)),
  "..",
);

/** Strip proxy vars so Node fetch + Chrome hit localhost directly. */
export function directEnv(extra = {}) {
  const env = { ...process.env, ...extra };
  for (const k of [
    "http_proxy", "https_proxy", "HTTP_PROXY", "HTTPS_PROXY",
    "ALL_PROXY", "all_proxy", "ftp_proxy", "FTP_PROXY",
  ]) {
    delete env[k];
    env[k] = "";
  }
  env.NO_PROXY = "*";
  env.no_proxy = "*";
  return env;
}

/**
 * @param {{
 *   port: number,
 *   profile: string,
 *   startUrl?: string,
 * }} opts
 */
export function spawnChrome(opts) {
  const profile = opts.profile;
  fs.rmSync(profile, { recursive: true, force: true });
  fs.mkdirSync(profile, { recursive: true });
  const chrome = spawn(CHROME, [
    `--remote-debugging-port=${opts.port}`,
    "--headless=new",
    "--use-angle=swiftshader",
    "--enable-webgl",
    "--ignore-gpu-blocklist",
    "--no-first-run",
    "--no-proxy-server",
    "--proxy-bypass-list=*",
    `--user-data-dir=${profile}`,
    opts.startUrl || "about:blank",
  ], { stdio: ["ignore", "ignore", "pipe"], env: directEnv() });
  return chrome;
}

export function makeCdp(leftMs) {
  return function cdp(ws, id, method, params = {}) {
    const budget = typeof leftMs === "function" ? leftMs() : leftMs;
    return new Promise((resolve, reject) => {
      const t = setTimeout(
        () => reject(new Error(method)),
        Math.min(12000, budget || 1),
      );
      const onMsg = (ev) => {
        let msg; try { msg = JSON.parse(ev.data); } catch { return; }
        if (msg.id !== id) return;
        clearTimeout(t);
        ws.removeEventListener("message", onMsg);
        if (msg.error) reject(new Error(JSON.stringify(msg.error)));
        else resolve(msg.result);
      };
      ws.addEventListener("message", onMsg);
      ws.send(JSON.stringify({ id, method, params }));
    });
  };
}

/** GET body as text, never via HTTP_PROXY. */
export function httpGetText(url, ms = 2000) {
  return new Promise((resolve, reject) => {
    const u = new URL(url);
    const req = http.get({
      hostname: u.hostname,
      port: u.port || 80,
      path: u.pathname + u.search,
      timeout: ms,
      agent: false,
    }, (res) => {
      const chunks = [];
      res.on("data", (c) => chunks.push(c));
      res.on("end", () => resolve({
        status: res.statusCode || 0,
        text: Buffer.concat(chunks).toString("utf8"),
      }));
    });
    req.on("error", reject);
    req.on("timeout", () => { req.destroy(); reject(new Error("timeout")); });
  });
}

/** Wait until Chrome remote debugging answers. */
export async function waitDebugger(port, leftMs) {
  const left = typeof leftMs === "function" ? leftMs : () => leftMs;
  while (left() > 0) {
    try {
      const r = await httpGetText(`http://127.0.0.1:${port}/json/version`, 800);
      if (r.status === 200) return;
    } catch { /* retry */ }
    await sleep(Math.min(200, left() || 1));
  }
  throw new Error("chrome debugger timeout :" + port);
}

/** Open CDP session on the first page target. */
export async function connectPage(port, leftMs) {
  const left = typeof leftMs === "function" ? leftMs : () => leftMs;
  await waitDebugger(port, left);
  const r = await httpGetText(`http://127.0.0.1:${port}/json/list`, 2000);
  const tabs = JSON.parse(r.text);
  const page = tabs.find((t) => t.type === "page") || tabs[0];
  if (!page?.webSocketDebuggerUrl) throw new Error("no page target");
  const ws = new WebSocket(page.webSocketDebuggerUrl);
  await new Promise((res, rej) => {
    ws.addEventListener("open", res);
    ws.addEventListener("error", rej);
    setTimeout(() => rej(new Error("ws")), Math.min(8000, left() || 1));
  });
  return ws;
}

/** GET that never uses HTTP_PROXY. */
export function fetchDirect(url, ms = 2000) {
  return new Promise((resolve, reject) => {
    const u = new URL(url);
    const req = http.get({
      hostname: u.hostname,
      port: u.port || 80,
      path: u.pathname + u.search,
      timeout: ms,
      agent: false,
    }, (res) => {
      res.resume();
      resolve(res.statusCode || 0);
    });
    req.on("error", reject);
    req.on("timeout", () => { req.destroy(); reject(new Error("timeout")); });
  });
}

/**
 * Ensure static server on 8765 serving ujs/web.
 * @returns {Promise<{ stop: () => void, started: boolean }>}
 */
export async function ensureDemoServer() {
  try {
    const code = await fetchDirect(DEMO_ORIGIN + "/engine/");
    if (code === 200 || code === 301 || code === 302) {
      return { started: false, stop() {} };
    }
  } catch { /* start */ }

  const child = spawn("python3", ["-m", "http.server", "8765", "--directory", WEB_ROOT], {
    stdio: ["ignore", "ignore", "ignore"],
    env: directEnv(),
    detached: false,
  });
  for (let i = 0; i < 30; i++) {
    await sleep(100);
    try {
      const code = await fetchDirect(DEMO_ORIGIN + "/engine/");
      if (code === 200) {
        return {
          started: true,
          stop() {
            try { child.kill("SIGKILL"); } catch { /* */ }
          },
        };
      }
    } catch { /* retry */ }
  }
  try { child.kill("SIGKILL"); } catch { /* */ }
  throw new Error("could not start demo server on 8765");
}

import { createBrowserHost } from "../../browser-host.js";
import { runMonopolyCore } from "../../core-monopoly.js";

const hud = document.getElementById("hud");
const canvas = document.getElementById("c");
hud.textContent = "Host ABI · monopoly…";

const q = new URLSearchParams(location.search).get("gpu");
const prefer = q === "webgl" || q === "webgpu" ? q : "auto";

const host = await createBrowserHost(canvas, {
  baseURL: new URL("../../../", import.meta.url),
  prefer,
});

function paint(s) {
  window.__UXE__ = { ...s, backend: host.backend, ship: false };
  if (!s.ready) return;
  const who = s.turn === 0 ? "你" : "AI";
  const win =
    s.phase === 2
      ? (s.winner === 0 ? "你赢了" : s.winner === 1 ? "AI 赢了" : "结束")
      : "";
  hud.innerHTML =
    `<b>城市大富翁</b> · 单机 16 格<br>` +
    `backend <b>${host.backend}</b> · fps <b>${(s.fps || 0).toFixed(0)}</b><br>` +
    `回合 <b>${who}</b> · ${s.phaseHint || ""}` +
    (win ? ` · <span class="warn">${win}</span>` : "") + `<br>` +
    `你 💰<b>${(s.money0 || 0).toFixed(0)}</b> · 地 ${s.owned0 ?? 0}` +
    ` · AI 💰<b>${(s.money1 || 0).toFixed(0)}</b> · 地 ${s.owned1 ?? 0}<br>` +
    (s.cellName ? `当前格 <b>${s.cellName}</b>` : "") +
    (s.houses ? ` · 房 ${s.houses}` : "") +
    (s.dice ? ` · 骰 <b>${s.dice}</b>` : "") +
    (s.msg ? `<br><span class="warn">${s.msg}</span>` : "");
}

try {
  await runMonopolyCore(host, {
    wasmUrl: "ujs_full.wasm",
    simUrl: "game/monopoly.ujs",
    onHud: paint,
  });
} catch (e) {
  hud.innerHTML = `<span class="warn">boot failed</span><br>${e.message || e}`;
  window.__UXE__ = { ready: false, error: String(e.message || e) };
  console.error(e);
}

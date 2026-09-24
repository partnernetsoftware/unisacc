# ujs2wasm suite

独立门禁：`./tests/ujs2wasm.sh`（`./tests/ujs.sh` 有 node 时会调用）。

| 路径 | 作用 |
|---|---|
| `corpus/*.ujs` | 直出语料（含 setidx / f64） |
| `expect.json` | 期望值 |
| `neg/*.ujs` | 必须失败 |
| `run_wasm.mjs` | Node：`main_export` / tag / i64\|f64\|str |
| suite 内 game-ready | `sim.ujs`/`drone.ujs` 必须 `can_emit_direct` |

另：`./tests/uxe_ship_js.sh` — ship-js 合同（无 eng_*、无 C `asteroid.wasm`）。

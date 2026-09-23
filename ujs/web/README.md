# UJS web

产品 ESM + playground + 游戏/引擎演示。

| 跟踪（git） | |
|---|---|
| `wasm_run.js` | **库入口**：`bootRuntime` / `wasm_run` |
| `compiler.js` | 页内编译（`//` 注释与 Python front 对齐） |
| `BUILD.json` | 指纹：version / sha256 / ABI（无二进制） |
| `index.html` `demo.js` `style.css` | playground |
| `jspi.js` | JSPI 探针（非主路径） |
| `engine/` | **UXE 主线**：Host ABI + Asteroid demo（见 `engine/README.md`） |
| `game/` | Three 对照 + `exp/`（非 API 依赖） |

| 不跟踪 → **GitHub Release** / 本地 `web-build` | |
|---|---|
| `ujs_full.wasm` · `compiler.gen.js` | 产品核（~152KB） |
| `ujs_rt.wasm` · `progs/*` | 可选演示 |
| `game/three.module.js` | 按需拉取（见 `game/README.md`） |

本地：`npm run build` / `python3 -m ujs web-build`。  
发版：核 + 可选 `--with-demos`（含 `game/` 与 `engine/` 源）→ Release；哈希对 `BUILD.json`。

地图：[`../DOCS.md`](../DOCS.md)。

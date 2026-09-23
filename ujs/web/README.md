# UJS web

产品 ESM + playground + UXE / 对照演示。

| 跟踪（git） | |
|---|---|
| `wasm_run.js` | **库入口**：`bootRuntime` / `wasm_run` |
| `compiler.js` | 页内编译（与 Python front 对齐） |
| `BUILD.json` | 指纹：version / sha256 / ABI（无二进制） |
| `index.html` `demo.js` `style.css` | playground |
| `jspi.js` | JSPI 探针（非主路径） |
| `engine/` | **UXE**：`demo/` 源码测试 · `ship/` 发布脚本与源 |
| `game/` | Three 对照 + `exp/`（非 API 依赖） |

| 不跟踪 → **Release** / 本地 `web-build` · `ship:engine` | |
|---|---|
| `ujs_full.wasm` · `compiler.gen.js` | 产品核（≈88KB wasm） |
| `engine/ship/asteroid.wasm` · `engine.wasm` | UXE 发布面（`npm run ship:engine`） |
| `engine/ship/index.html` · `uxe-host.js` | 由 `bake-html` 生成 |
| `ujs_rt.wasm` · `progs/*` | 可选演示 |
| `game/three.module.js` | 按需拉取 |

本地：`npm run build` / `npm run ship:engine`。  
发版核：`./scripts/release-artifacts.sh`；UXE 演示源随 `--with-demos`。

地图：[`../DOCS.md`](../DOCS.md)。

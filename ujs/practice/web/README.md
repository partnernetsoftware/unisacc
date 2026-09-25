# UJS web — playground &对照

静态演示根的一部分（`npm run demo` 服务于整个 `ujs/`）。**path-A / playground**，不是 Pages 出货脊。

| 路径 | 角色 |
|---|---|
| `index.html` · `demo.js` · `style.css` | playground（`wasm_run` + `ujs_full`） |
| `game/` | Three / 裸 WebGL **对照**（非产品 API） |
| `progs/` · `demos.json` | 可选演示产物 |

**产品核**在 [`../core/`](../core/)（出货编译默认 **`compiler_core`**）。**UXE Host / ship**在 [`../uxe/`](../uxe/)（Pages → `sim.wasm`）。两脊见 [`../prd.md`](../prd.md) · Paper B [`../../research/ujs-paper.md`](../../research/ujs-paper.md)。

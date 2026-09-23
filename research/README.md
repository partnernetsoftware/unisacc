# research/

论文与 prior-art 草稿；**不是**产品规格（规格在 `ujs/prd.md`）。

| 文件 | 角色 |
|---|---|
| [`unisacc-paper.md`](unisacc-paper.md) | **Paper A** — UNISA SH / C 自举与构造方法 |
| [`ujs-paper-outline.md`](ujs-paper-outline.md) | **Paper B** 一页提纲 |
| [`ujs-paper.md`](ujs-paper.md) | **Paper B** 正文草稿 — UJS-1 + `wasm_run` |
| [`prior-art.md`](prior-art.md) | 对抗性相关工作（两文共享） |

交叉引用：B 凡方法命题一律 **[cite A]**；不编造 latency / 准确率小数；验收只绑 `tests/ujs.sh` 与已命名门禁。

## 可选定性产物（截图 / 链接，非数字表）

| 路径 | 说明 |
|---|---|
| `ujs/web/` playground | `wasm_run` 开箱 |
| `ujs/web/engine/demo/` | UXE 源码测试（Host ABI） |
| `ujs/web/engine/ship/` | UXE **发布面**：`index.html` + `{game}.wasm` + `gameEngine.wasm`（不合包） |
| `ujs/web/game/` | Three 对照（非主线） |

产品目录地图：[`ujs/DOCS.md`](../ujs/DOCS.md)。

# research/

论文与 prior-art 草稿；**不是**产品规格（Paper A 的规格是仓库根的 `prd.md`，Paper B 的是 `ujs/prd.md`）。

| 文件 | 角色 |
|---|---|
| [`unisacc-paper.md`](unisacc-paper.md) | **Paper A** — UNISA SH / C 自举与构造方法（§3.4 形式化义务表） |
| [`ujs-paper-outline.md`](ujs-paper-outline.md) | **Paper B** 一页提纲 |
| [`ujs-paper.md`](ujs-paper.md) | **Paper B** 正文草稿 — UJS-1 + `wasm_run` |
| [`prior-art.md`](prior-art.md) | 对抗性相关工作（两文共享） |
| [`paper-c-intent.md`](paper-c-intent.md) | **Paper C** 意向书 — 种子机与迭代脱离：把确定性模型推理推广为管道方法（引用 A、B） |
| [`formalization-roadmap.md`](formalization-roadmap.md) | **Paper A 形式化义务** — Lean 4 L0–L3；B/C 引用钩子 |
| [`lean/`](lean/) | Lean 4 契约内核（`lake build`） |
| [`figures/`](figures/) | Paper A 插图（图 1 网络结构，IEEE 线稿） |

交叉引用：B 凡方法命题一律 **[cite A]**；不编造 latency / 准确率小数；验收只绑 `tests/ujs.sh` 与已命名门禁。

## 可选定性产物（截图 / 链接，非数字表）

| 路径 | 说明 |
|---|---|
| `ujs/uxe/demo/` · `ship/` | UXE 源码测 / 发布面 |
| `ujs/web/game/` | Three 对照（非主线） |

产品规格：[`ujs/prd.md`](../ujs/prd.md)。

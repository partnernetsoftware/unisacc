# UJS 地图：种子 · 可复用权重 · 自迭代 · 实践

与仓库根 [`ARCHITECTURE.md`](../ARCHITECTURE.md) 同口径（Paper C）：**造权重的脚本属种子，造出来的才是可复用权重**；自迭代用孪生字节脱离出货种子；实践是 Web 实验场。规格 [`prd.md`](prd.md)，**版本计划** [`plans/`](plans/)，论文 B [`../research/ujs-paper.md`](../research/ujs-paper.md)。

**出货主产物**：**`ujs.com`（APE）**——`./unisacc.com -run ujs/product/complete.cx`（APE＋预言＋**验面**＋六目标）。源 [`product/ujs_main.cx`](product/ujs_main.cx)+[`seed/stage0/compiler_min.cx`](seed/stage0/compiler_min.cx)；验面 [`product/verify-host.cx`](product/verify-host.cx)／`--verify`。产品链 **只许 `unisacc.com`+`.cx`**。细则见 [`prd.md`](prd.md) §0.1。

**工具链纪律（主人 2026-10-25）**：ujs 内新动态逻辑／构造／验收胶水逐步走 **`unisacc.com` + `.cx`**（默认 `-run`），与根仓库零 Python 目标同向；逐步脱离 `python3 -m ujs` 日常依赖。`.cx` 转调 Python 不算完成。见 [`prd.md`](prd.md)「何时可弃 Python」。

**直出热路径（Paper B 床实证）**：`.ujs→emit_wat→wat2wasm` 中语料秒级税曾是 **每文件 fork wat2wasm**；常驻／进程内组装后地板在 **parseWat**，不在四张决策网。细节与 REJECT 表见 [`prd.md`](prd.md)「加速实证」。不并表 Paper A 七流十八网。

```
  ┌──────────── 种子 seed/ ─────────────────────────────────────────────┐
  │  stage0/      compiler_min.cx（host1）                               │
  │  construct/ · vm/   占位；.py/.c 已迁 archive/ujs-legacy-py-c/       │
  └──────────────────────────────┬──────────────────────────────────────┘
                                 │ 编 .ujs
                                 ▼
  ┌── product/ ujs.com ──┐     ┌── 自迭代 iterate/ ──────────────────┐
  │ APE·六目标·预言·验面  │     │ compiler.ujs · compiler_core.wasm   │
  └──────────────────────┘     │ compile.mjs（实践脊 · 另标签）        │
                               └──────────────┬───────────────────────┘
                                              ▼
                               ┌── 实践 practice/ ───────────────────┐
                               │ core · uxe · web                    │
                               └─────────────────────────────────────┘
```

**产品目录**：出货只认 [`product/`](product/) + [`seed/stage0/compiler_min.cx`](seed/stage0/compiler_min.cx)。  
**`ujs/` 内无 `.c`／`.py`（0.0.3）**；遗留见 [`../archive/ujs-legacy-py-c/`](../archive/ujs-legacy-py-c/)。  
旧链接 `core`·`uxe`·`web`·`compile.mjs` 仍指向实践脊；**`construct` 链接已删**。

## 1　可复用权重 `weights/`

IC 表 `.c` 生成物已迁 archive。新权重经 `.cx` 路径（续迁）；共享底座仍可用根 `weights/built.uns2`。

## 2　种子 `seed/`

| 位置 | 作用 |
|---|---|
| [`seed/stage0/`](seed/stage0/) | host1 `compiler_min.cx` |
| [`seed/construct/`](seed/construct/) · [`seed/vm/`](seed/vm/) | 占位 README；源在 archive |

## 3　自迭代 `iterate/`（M3 出货编译器）

| 文件 | 作用 |
|---|---|
| `compiler.ujs` | UJS 写的编译器核（UJS-1_ship） |
| `compiler.wasm` | stage0 产物 |
| `compiler_core.wasm` | stage1；`compile.mjs` 默认 bridge |
| `compile.mjs` 等 | 产品编译与 splice |

门禁：`./tests/ujs2wasm_compiler.sh`。

## 4　实践 `practice/`

| 位置 | 作用 |
|---|---|
| `practice/core/` | `wasm_run` · path-A `ujs_full` · npm 入口 |
| `practice/uxe/` | Host ABI · demo/ship · Pages |
| `practice/web/` | playground / Three 对照 |

门禁：`./tests/ujs.sh` · `uxe_ship_js` · UXE 另门。

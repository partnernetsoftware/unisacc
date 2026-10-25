# UJS 地图：种子 · 可复用权重 · 自迭代 · 实践

与仓库根 [`ARCHITECTURE.md`](../ARCHITECTURE.md) 同口径（Paper C）：**造权重的脚本属种子，造出来的才是可复用权重**；自迭代用孪生字节脱离出货种子；实践是 Web 实验场。规格 [`prd.md`](prd.md)，论文 B [`../research/ujs-paper.md`](../research/ujs-paper.md)。

**出货主产物**：**`ujs.com`（APE）**——`./unisacc.com -run product/build-ape.cx`（或 `complete.cx`）。源 [`product/ujs_main.cx`](product/ujs_main.cx)+[`seed/stage0/compiler_min.cx`](seed/stage0/compiler_min.cx)；打包 [`product/ape-pack.cx`](product/ape-pack.cx)。另：`build.cx` 本机原生、`build-six.cx` 六目标。产品链 **只许 `unisacc.com`+`.cx`**。`compile.mjs`／`compiler_core` 为更全表实践脊（待嵌入）。细则见 [`prd.md`](prd.md) §0.1。

**工具链纪律（主人 2026-10-25）**：ujs 内新动态逻辑／构造／验收胶水逐步走 **`unisacc.com` + `.cx`**（默认 `-run`），与根仓库零 Python 目标同向；逐步脱离 `python3 -m ujs` 日常依赖。`.cx` 转调 Python 不算完成。见 [`prd.md`](prd.md)「何时可弃 Python」。

**直出热路径（Paper B 床实证）**：`.ujs→emit_wat→wat2wasm` 中语料秒级税曾是 **每文件 fork wat2wasm**；常驻／进程内组装后地板在 **parseWat**，不在四张决策网。细节与 REJECT 表见 [`prd.md`](prd.md)「加速实证」。不并表 Paper A 七流十八网。

```
  ┌──────────── 种子 seed/ ─────────────────────────────────────────────┐
  │  construct/   Python：catalog·gold·oracle·front·jtape·emit·build…    │
  │  stage0/      compiler_min.cx（host）· iterate/compiler.wasm（已提交） │
  │  vm/          ujs_vm.c 等 path-A VM 源                              │
  └──────┬───────────────────────────────┬──────────────────────────────┘
         │ build-weights / emit ic_c     │ 编 compiler.ujs
         ▼                               ▼
  ┌── 可复用 weights/ ──┐         ┌── 自迭代 iterate/ ──────────────────┐
  │ ujs_ic_net.c        │         │ compiler.ujs · compiler_core.wasm   │
  │ (built.json 可写入) │         │ compile.mjs · run-compiler-core…    │
  └─────────────────────┘         │ stage2≡stage1 · body≡stage0         │
                                  └──────────────┬──────────────────────┘
                                                 │ 编 sim.ujs
                                                 ▼
                                  ┌── 实践 practice/ ───────────────────┐
                                  │ core/  wasm_run · ujs_full（path-A）│
                                  │ uxe/   Host · ship · sim.wasm       │
                                  │ web/   playground · Three 对照      │
                                  └─────────────────────────────────────┘
```

**产品目录**：出货只认 [`product/`](product/) + [`seed/stage0/compiler_min.cx`](seed/stage0/compiler_min.cx)。  
旧路径 `construct` · `core` · `uxe` · `web` · `compile.mjs` · `native/*` 为**兼容符号链接**；新代码写板块路径，勿在链接名下堆新逻辑。

## 1　可复用权重 `weights/`（数据）

| 文件 | 内容 | 由谁写出 |
|---|---|---|
| [`weights/ujs_ic_net.c`](weights/ujs_ic_net.c) | IC 阶段扁平表（`IC_TABLE`） | `construct/build/ic_c.py` |
| （可选）`weights/built.json` | `ujs build-weights` 构造网快照 | `python3 -m ujs build-weights` |

共享底座仍用仓库根 `weights/built.uns2`（Paper A）；UJS 表阶段经 `unisa.intnet` 构造。

## 2　种子 `seed/`（第 0 代程序）

| 位置 | 作用 |
|---|---|
| [`seed/construct/`](seed/construct/) | 表规格、走查、emit、`web-build`、`ujs2wasm` CLI（`python3 -m ujs`） |
| [`seed/stage0/`](seed/stage0/) | `compiler_min.cx`（host）+ 已提交 `iterate/compiler.wasm`；`build-compiler-wasm.cx` 只验产物 |
| [`seed/vm/`](seed/vm/) | path-A C VM / 遗留游戏核源 |

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

# UJS — 开箱

> 板块地图：[`ARCHITECTURE.md`](ARCHITECTURE.md)（种子 · 权重 · 自迭代 · 实践）  
> 规格：[`prd.md`](prd.md) · Paper B：[`../research/ujs-paper.md`](../research/ujs-paper.md)

**出货主产物**：**`ujs.com`**（类 `unisacc.com`）。**host1**：`./unisacc.com -run ujs/product/build.cx`（`ujs_main.cx` 无 `#include`；见 [`product/README.md`](product/README.md)）；APE 六目标尚未做。下面其余命令是**子功能／实践脊**。新胶水一律 **`unisacc.com` + `.cx`（`-run`）**（见 [`prd.md`](prd.md)）。

```
seed/       第 0 代：construct/ · stage0/ · vm/
weights/    可复用产物：ujs_ic_net.c
iterate/    M3：compiler.ujs · compiler_core · compile.mjs
practice/   core · uxe · web
```

兼容链接（旧脚本仍可用）：`construct` → `seed/construct`，`core` → `practice/core`，`uxe`/`web`，`compile.mjs` → `iterate/compile.mjs`。

```bash
# 今日子功能（将来由 ujs.com 调度）
python3 -m ujs web-build          # → practice/core（path-A）
python3 -m ujs ujs2wasm …         # .ujs → \0asm（内部可经 wat2wasm）
node ujs/compile.mjs foo.ujs -o foo.wasm   # 默认 iterate/compiler_core
./tests/ujs.sh
./tests/ujs2wasm_compiler.sh
```

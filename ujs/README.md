# UJS — 开箱

> 板块地图：[`ARCHITECTURE.md`](ARCHITECTURE.md)（种子 · 权重 · 自迭代 · 实践）  
> 规格：[`prd.md`](prd.md) · Paper B：[`../research/ujs-paper.md`](../research/ujs-paper.md)

```
seed/       第 0 代：construct/ · stage0/ · vm/
weights/    可复用产物：ujs_ic_net.c
iterate/    M3：compiler.ujs · compiler_core · compile.mjs
practice/   core · uxe · web
```

兼容链接（旧脚本仍可用）：`construct` → `seed/construct`，`core` → `practice/core`，`uxe`/`web`，`compile.mjs` → `iterate/compile.mjs`。

```bash
python3 -m ujs web-build          # → practice/core（path-A）
node ujs/compile.mjs foo.ujs -o foo.wasm   # 默认 iterate/compiler_core
./tests/ujs.sh
./tests/ujs2wasm_compiler.sh
```

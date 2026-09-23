# construct — 构造侧（非应用运行时）

Python：gold 表、走查、jtape/VM、IC、`web-build`、CLI。

```bash
python3 -m ujs web-build   # → ../core/ujs_full.wasm + compiler.gen.js + BUILD.json
python3 -m ujs acc
from ujs.construct import Runtime
```

应用请用 [`../package.json`](../package.json) / [`../core/`](../core/)。  
C 源在 [`../native/`](../native/)（VM + UXE 游戏核）。  
本目录**不是**训练入口；出货权重由构造代数枚举生成。

地图：[`../DOCS.md`](../DOCS.md)。

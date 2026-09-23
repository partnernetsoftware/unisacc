# construct — 构造侧（非应用运行时）

Python：gold 表、走查、jtape/VM、IC/`web-build`、CLI。

```bash
python3 -m ujs web-build   # → ../web 产物
python3 -m ujs acc
from ujs.construct import Runtime
```

应用请用上层 [`../package.json`](../package.json) / [`../web/`](../web/)。
本目录**不是**训练入口；出货权重由构造代数枚举生成（与仓规一致）。

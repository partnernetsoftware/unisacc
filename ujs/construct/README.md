# construct — 构造侧（非应用运行时）

Python：gold、走查、jtape/VM、`web-build`、`ujs2wasm`、CLI。规格见 [`../prd.md`](../prd.md)。

```bash
python3 -m ujs web-build    # → ../core/
python3 -m ujs ujs2wasm foo.ujs -o foo.wasm
python3 -m ujs acc
```

应用用 [`../core/`](../core/)。C 源在 [`../native/`](../native/)。**不是**训练入口。

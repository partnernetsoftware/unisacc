# construct — 构造侧（方法脊 · 非应用运行时）

Python：gold、走查、jtape/VM、`web-build`、全表 `ujs2wasm`、`acc` / fold。规格见 [`../prd.md`](../prd.md)。

**角色**：Paper B 的 **构造/jtape 脊**（表→IntNet、方法验收）。  
**不是**：Pages/ship 出货编译器（那是 M3 `compiler_core.wasm`）。Python **只许缩或持平**，禁止新增「仅 Python 才出货」边。

```bash
python3 -m ujs web-build    # → ../core/ujs_full（path A；非 ship 必经）
python3 -m ujs ujs2wasm foo.ujs -o foo.wasm   # 全表对照 emit
python3 -m ujs acc
```

出货编译用 [`../compile.mjs`](../compile.mjs)（默认 core）。应用 API 在 [`../core/`](../core/)。C 源在 [`../native/`](../native/)。**不是**训练入口。

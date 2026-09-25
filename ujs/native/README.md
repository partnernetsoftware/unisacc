# native — C 运行时与 stage0

| 文件 | 产物 | 角色 |
|---|---|---|
| `ujs_vm.c` + `ujs_ic_net.c` | `core/ujs_full.wasm`（path-A；交付名曾作 **`engine.wasm`**） | UJS VM：`host_run` / list·dict；**非** P0 ship 必经（仅 `FORCE_WEB_BUILD` / `ujs.sh` 重建） |
| `compiler_min.c` | `core/compiler.wasm` | **M2/M3 stage0**（C）：编出 `compiler.ujs` → core；产品默认是 **`compiler_core.wasm`** |
| `uxe_asteroid.c` + `sim_embed.h` | 遗留；出货玩法核已是 path-B **`sim.wasm`** | 旧 `{game}.wasm` 调度；**不**再作 Pages 主线 |
| `sim_embed.h` | （生成） | 旧 ship 写入；已 gitignore |

**出货脊**：`compile.mjs` → `compiler_core` → `sim.wasm` / drone；门禁 `./tests/ujs2wasm_compiler.sh` · `uxe_ship_js`。  
**构造脊 / path-A**：`web-build` → `ujs_full`（方法演示与 VM，非 ship emit）。

```bash
./ujs/scripts/build-compiler-wasm.sh   # → core/compiler.wasm（需 zig；stage0）
# 出货 Pages（默认 core，无 python emit）：
cd ujs && npm run ship:pages
# path-A VM（非 ship 必经）：
python3 -m ujs web-build               # → ujs_full.wasm
```

规格与两脊：[`../prd.md`](../prd.md)。Paper B：[`../../research/ujs-paper.md`](../../research/ujs-paper.md)。

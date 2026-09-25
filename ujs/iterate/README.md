# iterate/ — 自迭代（M3 出货编译器脊）

| 文件 | 说明 |
|---|---|
| `compiler.ujs` | UJS 源编译器 |
| `compiler.wasm` | stage0（`../seed/stage0` 构建） |
| `compiler_core.wasm` | stage1；产品默认 |
| `compiler_rt_stub.*` | splice 模板 |
| `compile.mjs` | Node 桥（仓库根也可 `ujs/compile.mjs` symlink） |
| `run-compiler-core.mjs` · `rebuild-main.mjs` | 运行与拼接 |

**不是** IntNet。孪生：stage2≡stage1 · sim/drone body≡stage0。

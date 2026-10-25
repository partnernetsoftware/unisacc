# ujs2wasm 夹具（非出货闸入口）

本目录保留 **语料／拒例／历史 Node 探针**。  
**`ujs.com` 产品闸**请走：

```bash
./unisacc.com -run tests/ujs/run.cx -- --smoke
./unisacc.com -run tests/ujs/run.cx -- --complete
```

见 [`../ujs/README.md`](../ujs/README.md)。旧 `./tests/ujs2wasm.sh`／`ujs.sh` 已迁  
[`archive/testing-c43-legacy/tests/`](../../archive/testing-c43-legacy/tests/)（Python／Node；非 host1 合同）。

| 路径 | 作用 |
|---|---|
| `corpus/*.ujs` | host1 PASS 列表真源（与 `ujs/product/corpus-host.cx` 对齐，28 项） |
| `expect.json` | 历史全表期望（≠ host1 合同；含曾在 construct 侧的探针） |
| `neg/*.ujs` | 必须失败的探针（构造／边界） |
| `run_wasm.mjs` · `run_step.mjs` | 历史 Node 读回；实践脊可用，非 `complete.cx` 必经 |
| `step_corpus/` · `step_expect.json` | 步进 inject 夹具 |
| `gate_*.sh` | 遗留分类壳；新产品闸不依赖 |

**勿**把「`expect.json` 有某文件」读成 host1／M3 已覆盖。host1 覆盖面以 `corpus-host.cx` 的 PASS 数组为准。

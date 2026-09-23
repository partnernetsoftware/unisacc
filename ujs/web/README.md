# UJS web

产品 ESM + playground。生成物（`*.wasm` / `compiler.gen.js` / demos）由
`npm run build` / `python3 -m ujs web-build` 写出（gitignore）。

| 跟踪 | |
|---|---|
| `wasm_run.js` | **库入口**：`bootRuntime` / `wasm_run` |
| `compiler.js` | 页内编译 |
| `index.html` `demo.js` `style.css` | 站点 |
| `jspi.js` | JSPI 探针（非主路径） |

包根见 [`../package.json`](../package.json)、[`../README.md`](../README.md)。

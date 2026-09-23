# UJS core

闭合脚本机（产品 API）。**不是** UXE / 游戏 Host。

| 文件 | 角色 |
|---|---|
| `index.js` / `wasm_run.js` | `bootRuntime` · `wasm_run` · `compile` · `unwrap` |
| `compiler.js` (+ `compiler.gen.js`) | 页内 / 工具链编译 |
| `ujs_full.wasm` | 交付 VM（`web-build` 生成） |
| `BUILD.json` | 指纹 |
| `jspi.js` | JSPI 探针（非主路径） |

```js
import { bootRuntime, wasm_run } from "ujs"; // → core/
await bootRuntime(new URL("./ujs_full.wasm", import.meta.url));
```

构造：`python3 -m ujs web-build` → 本目录。  
嵌入网页的 Host 在 [`../uxe/`](../uxe/)。

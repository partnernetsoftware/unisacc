# UJS core

闭合脚本机（产品 API）。**不是** UXE / 游戏 Host。

| 文件 | 角色 |
|---|---|
| `index.js` / `wasm_run.js` | `bootRuntime` · `wasm_run` · `compile` · `unwrap` |
| `compiler.js` (+ `compiler.gen.js`) | 页内 / 工具链编译（path A 面） |
| `ujs_full.wasm` | path-A VM（`web-build`；**非** P0 ship 必经） |
| `compiler.wasm` | **M2 stage0**（C `compiler_min`；zig 一次性） |
| `compiler.ujs` | **M3 v16**：UJS 写的编译器核（UJS-1_ship；stage2≡ · body≡） |
| `compiler_core.wasm` | stage1 入树；**`compile.mjs` / ship 默认** |
| `compiler_rt_stub.wasm` | M3 splice 模板（host ABI + 可换 main） |
| `BUILD.json` | 指纹 |
| `jspi.js` | JSPI 探针（非主路径） |

```js
import { bootRuntime, wasm_run } from "ujs"; // → core/
await bootRuntime(new URL("./ujs_full.wasm", import.meta.url));
```

出货：`node ../compile.mjs foo.ujs -o foo.wasm` → 默认 **`compiler_core.wasm`**（不是 IntNet；手写子集自举）。  
构造 / path-A：`python3 -m ujs web-build` → 本目录。  
嵌入网页的 Host 在 [`../uxe/`](../uxe/)。规格：[`../prd.md`](../prd.md)。

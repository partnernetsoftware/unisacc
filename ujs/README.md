# UJS — 开箱 JS 产品 / 库

> **产品**：`wasm_run(code, globals, locals)`（浏览器 · Node · Bun）  
> **构造**：`ujs/construct/`（Python；出表 / `web-build` / 验收）  
> **规格**：[`prd.md`](prd.md)

先出产物：`npm run build`（即 `python3 -m ujs web-build`）。需要本机 `python3` + `zig`。

## 网页

```html
<script type="module">
  import { bootRuntime, wasm_run } from "./wasm_run.js";
  await bootRuntime(new URL("./ujs_full.wasm", import.meta.url));
  const r = await wasm_run("return 1+2;", {}, {});
  document.body.textContent = r.ok;  // 3
</script>
```

Playground：`npm run demo` → http://127.0.0.1:8765/ （或静态托管 `web/`）。

## Node / Bun

```bash
# 在仓库内：以 ujs/ 为包根
cd ujs && npm run build
```

```js
import { bootRuntime, wasm_run } from "ujs";
// 或相对：import { bootRuntime, wasm_run } from "./web/wasm_run.js";

await bootRuntime(new URL("./web/ujs_full.wasm", import.meta.url));
const r = await wasm_run("return 1+2*3;", {}, {});
console.log(r.ok);  // 7
// 或：import { unwrap } from "ujs"; unwrap(r)
```

失败看 `r.err`。产品 wasm 是 **`ujs_full.wasm`**（`ujs_rt.wasm` 仅 ask 演示）。

## 构造侧（Python，非应用依赖）

```python
from ujs.construct import Runtime   # 或兼容：from ujs import Runtime
print(Runtime().run("return 1+2;").unwrap())
```

```bash
python3 -m ujs web-build
python3 -m ujs run --code 'return 1+2;'
python3 -m ujs js2wasm prog.ujs -o prog.wasm
./tests/ujs.sh
```

## 布局

```
ujs/
├── package.json         # Node/Bun 包（入口 web/wasm_run.js）
├── web/                 # 站点 + ESM 库
│   ├── wasm_run.js      # bootRuntime / wasm_run
│   ├── compiler.js      # 页内编译（手写）
│   ├── index.html …     # playground
│   └── *.wasm / gen.*   # web-build 生成（gitignore）
├── native/              # C VM（js2wasm / full wasm）
├── construct/           # Python：gold · front · build · CLI
└── prd.md
```

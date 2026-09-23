# UJS — 开箱 JS 产品 / 库

> **产品**：`wasm_run(code, globals, locals)`（浏览器 · Node · Bun）  
> **构造**：`ujs/construct/`（Python；出表 / `web-build` / 验收）  
> **规格**：[`prd.md`](prd.md)（v1.1 活规格；条款全表见 [`archive/prd-v1.0.md`](archive/prd-v1.0.md)）

先出产物（开发机）：`npm run build`（即 `python3 -m ujs web-build`；要 `python3` + `zig`）。

**分发策略**：`*.wasm` / `compiler.gen.js` 等**二进制产物进 GitHub Release**，不进 git 主树（避免大文件与无关 diff）。  
仓内 `core/BUILD.json` 只作指纹（version / sha256 / ABI），用来核对 Release 附件是否与某次绿测一致。  
`package.json` 的 `version` 为语义版本；语言、VM ABI 或字节码变了再 bump。

## 网页

```html
<script type="module">
  import { bootRuntime, wasm_run } from "./core/wasm_run.js";
  await bootRuntime(new URL("./core/ujs_full.wasm", import.meta.url));
  const r = await wasm_run("return 1+2;", {}, {});
  document.body.textContent = r.ok;  // 3
</script>
```

Playground：`cd ujs && npm run demo` → http://127.0.0.1:8765/web/

## Node / Bun

```bash
# 在仓库内：以 ujs/ 为包根
cd ujs && npm run build
```

```js
import { bootRuntime, wasm_run } from "ujs";
// 或相对：import { bootRuntime, wasm_run } from "./core/wasm_run.js";

await bootRuntime(new URL("./core/ujs_full.wasm", import.meta.url));
const r = await wasm_run("return 1+2*3;", {}, {});
console.log(r.ok);  // 7
```

工具索引与发版脚本：[TOOLS.md](TOOLS.md)。  
文档地图：[DOCS.md](DOCS.md)。  
路线：[FUTURE.md](FUTURE.md)。

### 演示（可选）

| URL | 角色 |
|---|---|
| `/web/` | playground |
| **`/uxe/demo/`** | UXE asteroid 源码测试 |
| **`/uxe/demo/drone/`** | UXE 无人机源码测试 |
| **`/uxe/ship/`** | asteroid 发布面 |
| **`/uxe/ship/drone/`** | 无人机发布面 |
| Pages `docs/` | 外网索引 |
| `/web/game/` | Three 对照 |

契约：[`uxe/HOST_ABI.md`](uxe/HOST_ABI.md) · [`uxe/PLATFORM.md`](uxe/PLATFORM.md) · [`core/README.md`](core/README.md)。  
Pages：`npm run ship:pages` · 门禁：`npm run test:uxe:all`。

## 构造侧（Python，非应用依赖）

```python
from ujs.construct import Runtime
print(Runtime().run("return 1+2;").unwrap())
```

```bash
python3 -m ujs web-build   # → ujs/core/
./tests/ujs.sh
```

## 布局

```
ujs/
├── package.json         # 入口 → core/
├── core/                # 产品 API：wasm_run · compiler · ujs_full.wasm
├── uxe/                 # Host ABI · demo/ · ship/ · archive/
├── web/                 # playground + Three 对照
├── construct/           # Python gold / web-build
├── native/              # C：VM + {game}.wasm
├── cloudflare/          # 旁路（live 信令等）
└── scripts/
```

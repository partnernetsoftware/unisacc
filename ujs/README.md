# UJS — 开箱

> **产品**：`wasm_run(code, globals, locals)`（浏览器 · Node · Bun）  
> **构造**：`ujs/construct/`（Python；出表 / `web-build` / 验收）  
> **规格真源**：[`prd.md`](prd.md)（v1.3 · 思维树 · 记忆宫殿）· Host：[`uxe/HOST_ABI.md`](uxe/HOST_ABI.md)

先出产物：`npm run build`（`python3 -m ujs web-build`；要 `python3` + `zig`）。

**分发**：`*.wasm` / `compiler.gen.js` 进 **GitHub Release**，不进 git 主树。  
仓内 `core/BUILD.json` 只作指纹。`package.json` 的 `version` 为语义版本。

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

```js
import { bootRuntime, wasm_run } from "ujs";
await bootRuntime(new URL("./core/ujs_full.wasm", import.meta.url));
console.log((await wasm_run("return 1+2*3;", {}, {})).ok);  // 7
```

## 演示 URL

| URL | 角色 |
|---|---|
| `/web/` | playground |
| `/uxe/demo/` · `/uxe/demo/drone/` | UXE 源码测 |
| `/uxe/ship/` · `/uxe/ship/drone/` | 发布对照 |
| Pages `docs/` | 外网 |

```bash
npm run ship:pages && npm run test:uxe:all
./tests/ujs.sh
```

## 布局

```
ujs/
├── prd.md               # ★规格 + 思维树 + 记忆宫殿
├── package.json
├── core/                # 产品 API
├── uxe/                 # Host · demo/ · ship/
├── web/                 # playground · Three 对照
├── construct/           # Python 构造
├── native/              # C VM / 过渡游戏核
├── scripts/
└── archive/             # 旧文档
```

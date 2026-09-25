# UJS — 开箱

> **产品**：`wasm_run`（path A 演示）· Pages/ship 步进（path B `sim.wasm`）· 出货编译 `compile.mjs` → **`compiler_core.wasm`（M3）**  
> **构造**：`ujs/construct/`（Python；gold / fold / `web-build` / 全表 `ujs2wasm`）— **方法脊，非 ship 必经**  
> **规格真源**：[`prd.md`](prd.md)（v1.3 · 两条脊 · 门禁）· Host：[`uxe/HOST_ABI.md`](uxe/HOST_ABI.md)  
> **论文**：[`../research/ujs-paper.md`](../research/ujs-paper.md)（Paper B）

**两条脊**（同纪律、不同产物）：构造/jtape（表→IntNet）∥ M3 出货（手写子集自举）。详见 `prd.md`。

先出 VM 演示产物：`npm run build`（`python3 -m ujs web-build`；要 `python3` + `zig`）— path A。  
出货玩法核：`npm run ship:*` → `compile.mjs` 默认 **core**，无 python emit。

**分发**：`*.wasm` / `compiler.gen.js` 进 **GitHub Release**，不进 git 主树。  
仓内 `core/BUILD.json` 只作指纹。`package.json` 的 `version` 为语义版本。

## 网页（path A · `wasm_run`）

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

出货编译（M3）：`node ujs/compile.mjs foo.ujs -o foo.wasm`（默认 `compiler_core.wasm`）。

## 演示 URL

| URL | 角色 |
|---|---|
| `/web/` | playground（A） |
| `/uxe/demo/` · `/uxe/demo/drone/` | UXE 源码测 |
| `/uxe/ship/` · `/uxe/ship/drone/` | 发布对照（B 步进） |
| Pages `docs/` | 外网 |

```bash
npm run ship:pages && npm run test:uxe:all   # UXE 另门
./tests/ujs.sh                               # 语言/构造
./tests/ujs2wasm_compiler.sh                 # M3/P0
./tests/uxe_ship_js.sh                       # ship-js 合同
```

## 布局

```
ujs/
├── prd.md               # ★规格 + 思维树 + 记忆宫殿
├── package.json
├── core/                # 产品 API · compiler_core · ujs_full
├── compile.mjs          # 出货编译桥（默认 core）
├── uxe/                 # Host · demo/ · ship/
├── web/                 # playground · Three 对照
├── construct/           # Python 构造（方法脊）
├── native/              # C VM / stage0 compiler_min
├── scripts/
└── archive/             # 旧文档
```

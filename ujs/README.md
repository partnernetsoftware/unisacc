# UJS — 开箱 JS 产品 / 库

> **产品**：`wasm_run(code, globals, locals)`（浏览器 · Node · Bun）  
> **构造**：`ujs/construct/`（Python；出表 / `web-build` / 验收）  
> **规格**：[`prd.md`](prd.md)

先出产物（开发机）：`npm run build`（即 `python3 -m ujs web-build`；要 `python3` + `zig`）。

**分发策略**：`*.wasm` / `compiler.gen.js` 等**二进制产物进 GitHub Release**，不进 git 主树（避免大文件与无关 diff）。  
仓内 `web/BUILD.json` 只作指纹（version / sha256 / ABI），用来核对 Release 附件是否与某次绿测一致。  
`package.json` 的 `version` 为语义版本；语言、VM ABI 或字节码变了再 bump。

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
// globals 可绑 list/dict（未声明名走 G）：
const r2 = await wasm_run("return xs[1] + d.a;", { xs: [1, 2, 3], d: { a: 3 } }, {});
console.log(r2.ok); // 5
// 或：import { unwrap } from "ujs"; unwrap(r)
```

工具索引与发版脚本：[TOOLS.md](TOOLS.md)。  
文档地图：[DOCS.md](DOCS.md)。  
未来节点（小游戏检验、A/B/C/D）：[FUTURE.md](FUTURE.md)。  
论文 B 提纲 / 草稿：[research/ujs-paper-outline.md](../research/ujs-paper-outline.md)、[research/ujs-paper.md](../research/ujs-paper.md)。

### 演示（可选）

| URL | 角色 |
|---|---|
| `/` | playground |
| **`/engine/demo/`** | UXE asteroid 源码测试 |
| **`/engine/ship/`** | asteroid 发布面 |
| `/game/` | Three 对照 |
| `/game/exp/` | 裸 WebGL 对照 |

契约：[`web/engine/HOST_ABI.md`](web/engine/HOST_ABI.md)（**Host API**）、[`web/engine/PLATFORM.md`](web/engine/PLATFORM.md)、[`web/engine/README.md`](web/engine/README.md)、[`DOCS.md`](DOCS.md)。

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
├── DOCS.md              # 文档地图（先读）
├── TOOLS.md  FUTURE.md  prd.md
├── scripts/             # release-artifacts · ship-engine
├── web/                 # 站点 + ESM 库
│   ├── wasm_run.js      # bootRuntime / wasm_run
│   ├── compiler.js      # 页内编译
│   ├── BUILD.json       # 产物指纹
│   ├── engine/          # UXE：demo/ + ship/
│   ├── game/            # Three 对照 + exp/
│   └── *.wasm / gen.*   # web-build 生成（gitignore / Release）
├── native/              # C：VM + {game}.wasm 源
└── construct/           # Python：gold · front · build · CLI
```

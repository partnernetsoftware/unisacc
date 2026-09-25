# UXE — UJS eXperimental Engine

> **不是** Three 移植。闭合契约、薄胶水、玩法走 UJS、GPU 经 Host。  
> **规格 / 目标 / 树 / 宫殿**：[`../prd.md`](../prd.md) · **Host 真源**：[`HOST_ABI.md`](HOST_ABI.md) · **归档**：[`archive/`](archive/)  
> **门禁**：语言/M3 绿 ≠ UXE 绿（`test:uxe:*` **另门**，不挡 `ujs.sh` / `ujs2wasm_compiler`）。

| | 开发 `/uxe/demo/` | 发布 `/uxe/ship/` · Pages |
|---|---|---|
| 玩法 | 页内 compile / 源码测 | 预编译 path-B |
| 核 | `app-asteroid.js` · `app-drone.js` | **`sim.wasm`**（`compile.mjs` → **core**） |
| 引擎 | 可选 `ujs_full`（path-A 演示） | **无** ship 必经 `engine.wasm`（P0） |
| 页 | 多模块 ESM | `engine.js`（共享 Host）+ `game.js` |

```bash
npm run ship:pages                 # 默认 compiler_core，无 python emit
./tests/uxe_ship_js.sh             # ship-js 合同
npm run test:uxe:all               # UXE 无人（另门）
```

大富翁等下架车 → [`archive/`](archive/)。文件职责与架构见 prd 思维树 T2 / T4；两脊叙事见 [`../../research/ujs-paper.md`](../../research/ujs-paper.md)。

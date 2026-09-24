# UXE — UJS eXperimental Engine

> **不是** Three 移植。闭合契约、薄胶水、玩法走 UJS、GPU 经 Host。  
> **规格 / 目标 / 树 / 宫殿**：[`../prd.md`](../prd.md) · **Host 真源**：[`HOST_ABI.md`](HOST_ABI.md) · **归档**：[`archive/`](archive/)

| | 开发 `/uxe/demo/` | 发布 `/uxe/ship/` · Pages |
|---|---|---|
| 玩法 | 页内 `compile(*.ujs)` | 预编译 / embed |
| 核 | `app-asteroid.js` · `app-drone.js` | `asteroid.wasm` / drone ship-js |
| 引擎 | `ujs_full.wasm` | **`engine.wasm`** |
| 页 | 多模块 ESM | `engine.js`（共享）+ `game.js` |

```bash
npm run ship:pages && npm run test:uxe:all
```

大富翁等下架车 → [`archive/`](archive/)。文件职责与架构见 prd 思维树 T2 / T4。

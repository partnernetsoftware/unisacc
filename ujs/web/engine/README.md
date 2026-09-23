# UXE — UJS eXperimental Engine

> **不是** Three 移植。闭合契约、薄胶水、玩法走 UJS-1、GPU 经 Host API。  
> **Host API 真源**：[`HOST_ABI.md`](HOST_ABI.md) · **平台方向**：[`PLATFORM.md`](PLATFORM.md) · **归档**：[`archive/`](archive/)

## 开发 vs 发布

| | 开发 `/engine/demo/` | 发布 `/engine/ship/` · Pages `/docs/uxe/` |
|---|---|---|
| 玩法 | 页内 `compile(*.ujs)` | 预编译进核或 embed |
| 核 | `core-asteroid.js` · `core-drone.js` | `asteroid.wasm` · drone ship-js |
| 引擎 | `ujs_full.wasm` | **`engine.wasm`** |
| 页 | 多模块 ESM | `index.html` + **`engine.js`（共享）** + `game.js` |

```bash
npm run ship:pages             # asteroid + drone → ship/ 与 docs/uxe/
npm run test:uxe:all           # 无人门禁（含 drone）
```

大富翁已归档，见 [`archive/`](archive/)。

## 命题

```
Shell = {game} 核（调度 + packet）
      + engine.wasm（UJS VM）
      + 极薄 JS（Host GPU / 输入 / 双 wasm 桥）
      + （规划）用户自带 LLM → host_llm_*
```

| 层 | 现状 | 约束 |
|---|---|---|
| 玩法 | `game/sim.ujs` · `game/drone.ujs` | UJS-1；表外永久拒绝 |
| 游戏核 | asteroid: C wasm；drone: ship-js + embed | 只调 `host_*`（+ ship 的 `eng_*`） |
| 引擎 | `engine.wasm` | = `ujs_full`；**不合进**游戏 wasm |
| Host | `browser-host.js` | WebGL / WebGPU 自适应 |
| 提交 | UXEP → `host_gpu_submit` | 一次一包 |
| 输入 | UXIN v2 | `host_input_read(buf)` · 指针/触屏 |

## 架构

**demo**

```
demo/ · demo/drone/  →  core-*.js  →  wasm_run(ujs_full)
                              ↓ host_*
                        browser-host → WebGL | WebGPU
```

**ship**

```
index.html
    ├─ ../engine.js     # 共享 Host + GPU（docs/uxe/engine.js）
    ├─ ./game.js        # 本游戏胶水 / 核
    ├─ engine.wasm
    └─ asteroid.wasm（仅 asteroid）
```

## 文件职责

| 文件 | 职责 |
|---|---|
| `HOST_ABI.md` | **Host API**（符号 · UXEP/UXIN · 扩展 · LLM 规划） |
| `PLATFORM.md` | 多游戏平台 + BYO LLM |
| `host-abi.js` | `HOST_ABI_VERSION` + JSDoc |
| `packet.js` / `input.js` | UXEP / UXIN |
| `meshes.js` | 预置几何 |
| `browser-host.js` | 浏览器 `host_*` |
| `core-asteroid.js` / `core-drone.js` | 游戏核（demo；drone 亦进 ship） |
| `math.js` `scene.js` | 矩阵 / InstanceCloud |
| `renderer-webgl.js` / `renderer-webgpu.js` | GPU 后端 |
| `_cdp.mjs` | 无人探针共享 Chrome/CDP |
| `demo/` · `ship/` | 源码测试 · 发布构建 |
| `uxe.js` | 可选 `createEngine` 门面（非 Host 游戏主路径） |
| `archive/` | 下架实践车与一次性探针 |

## 跑与门禁

```bash
cd ujs && npm run demo
# /engine/demo/            asteroid
# /engine/demo/drone/      无人机
# /engine/ship/            asteroid 发布
# /engine/ship/drone/      无人机发布
# ?gpu=webgl|webgpu|auto

npm run test:uxe:all         # 默认绿线
```

## 与 `game/`

| 路径 | 角色 |
|---|---|
| **`web/engine/`** | **产品化主线** |
| `web/game/` | Three 对照 + `*.ujs` 玩法源 |
| `web/game/exp/` | 裸 GL 对照 |

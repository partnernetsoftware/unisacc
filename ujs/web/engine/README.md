# UXE — UJS eXperimental Engine

> **不是** Three 移植。闭合契约、薄胶水、玩法走 UJS-1、GPU 经 Host API。  
> **Host API 真源**：[`HOST_ABI.md`](HOST_ABI.md) · **平台方向**：[`PLATFORM.md`](PLATFORM.md)

## 开发 vs 发布

| | 开发 `/engine/demo/` | 发布 `/engine/ship/` · Pages `/docs/uxe/` |
|---|---|---|
| 玩法 | 页内 `compile(*.ujs)` | 预编译进核或 embed |
| 核 | `core-asteroid.js` / `core-monopoly.js` | `asteroid.wasm` · monopoly ship-js |
| 引擎 | `ujs_full.wasm` | **`gameEngine.wasm`** |
| 页 | 多模块 ESM | **一张** `index.html` + 极薄胶水 |

```bash
npm run ship:engine              # asteroid + monopoly → ship/ 与 docs/
npm run test:uxe:ship
npm run test:uxe:monopoly:ship
```

## 命题

```
Shell = {game} 核（调度 + packet）
      + gameEngine.wasm（UJS VM）
      + 极薄 JS（Host GPU / 输入 / 双 wasm 桥）
      + （规划）用户自带 LLM → host_llm_*
```

| 层 | 现状 | 约束 |
|---|---|---|
| 玩法 | `game/sim.ujs` · `game/monopoly.ujs` | UJS-1；表外永久拒绝 |
| 游戏核 | ship: `asteroid.wasm`；monopoly: ship-js | 只调 `host_*`（+ ship 的 `eng_*`） |
| 引擎 | `gameEngine.wasm` | = `ujs_full`；**不合进**游戏 wasm |
| Host | `browser-host.js` | WebGL / WebGPU 自适应 |
| 提交 | UXEP → `host_gpu_submit` | 一次一包 |
| 输入 | UXIN | `host_input_read(buf)` |

## 架构

**demo**

```
demo/{asteroid,monopoly}/host.js → core-*.js → wasm_run(ujs_full)
                                        ↓ host_*
                                  browser-host → WebGL | WebGPU
```

**ship**

```
index.html（内联胶水）
    ├─ gameEngine.wasm
    ├─ asteroid.wasm  或  monopoly 打包核 + embed sim
    └─ browser-host → WebGL | WebGPU
```

## 文件职责

| 文件 | 职责 |
|---|---|
| `HOST_ABI.md` | **Host API**（符号 · UXEP/UXIN · 扩展 · LLM 规划） |
| `PLATFORM.md` | 多游戏平台 + BYO LLM 叙事 |
| `host-abi.js` | `HOST_ABI_VERSION` + JSDoc |
| `packet.js` / `input.js` | UXEP / UXIN |
| `meshes.js` | 预置几何（octa / ship / box） |
| `browser-host.js` | 浏览器 `host_*` |
| `core-asteroid.js` / `core-monopoly.js` | demo（及 monopoly ship-js）核 |
| `math.js` `scene.js` | 矩阵 / InstanceCloud |
| `renderer-webgl.js` / `renderer-webgpu.js` | GPU 后端 |
| `demo/` | 源码测试 + 探针 |
| `ship/` | 发布构建与产物 |
| `_packet_selftest*` / `_input_selftest*` | 二进制门禁 |

## 跑与门禁

```bash
cd ujs && npm run demo
# /engine/demo/                 asteroid 源码
# /engine/demo/monopoly/        大富翁源码
# /engine/ship/                 asteroid 发布
# /engine/ship/monopoly/        大富翁发布
# ?gpu=webgl|webgpu|auto

npm run test:uxe:packet
npm run test:uxe:input
npm run test:uxe
npm run test:uxe:ship
npm run test:uxe:monopoly
npm run test:uxe:monopoly:ship
```

## 与 `game/`

| 路径 | 角色 |
|---|---|
| **`web/engine/`** | **产品化主线** |
| `web/game/` | Three 对照 + `*.ujs` 玩法源 |
| `web/game/exp/` | 裸 GL 对照 |

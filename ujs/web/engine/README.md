# UXE — UJS eXperimental Engine

> **不是** Three 移植。闭合契约、薄胶水、玩法走 UJS-1、GPU 经 Host ABI。  
> 设计真源：[`HOST_ABI.md`](HOST_ABI.md)。本文与目录文件一一对应。

## 开发 vs 发布

| | 开发 `/engine/demo/` | 发布 `/engine/ship/` |
|---|---|---|
| 玩法 | 页内 `compile(sim.ujs)` | 预编译进 `asteroid.wasm` |
| 核 | `core-asteroid.js` | **`asteroid.wasm`**（`native/uxe_asteroid.c`） |
| 引擎 | `ujs_full.wasm` | **`gameEngine.wasm`**（同产物，交付名） |
| 页 | 多模块 ESM | **一张** `index.html` + 极薄胶水 |

```bash
npm run ship:engine      # 生成 ship 三件套
npm run test:uxe:ship    # CDP 验收
```

## 命题

```
Shell = {game}.wasm（调度 + packet）
      + gameEngine.wasm（UJS VM）
      + 极薄 JS（Host GPU / 输入 / 双 wasm 桥）
```

| 层 | 现状 | 约束 |
|---|---|---|
| 玩法 | `game/sim.ujs` | UJS-1；表外永久拒绝 |
| 游戏核 | ship: `asteroid.wasm` | 只调 `host_*` + `eng_*`；**不含** VM |
| 引擎 | `gameEngine.wasm` | = `ujs_full.wasm`；**不合进**游戏 wasm |
| Host | `browser-host.js` | WebGL / WebGPU 自适应 |
| 提交 | UXEP → `host_gpu_submit` | 一次一包，非逐 draw |
| 输入 | UXIN | `host_input_read(buf)` |

## 架构

**demo（源码测试）**

```
demo/host.js → core-asteroid.js → wasm_run(ujs_full)
                    ↓ host_*
              browser-host → WebGL | WebGPU
```

**ship（交付）**

```
index.html（内联胶水）
    ├─ instantiate gameEngine.wasm
    ├─ instantiate asteroid.wasm  (imports: host_* + eng_boot/eng_sim_step)
    └─ browser-host → WebGL | WebGPU
```

## 文件职责

| 文件 | 职责 |
|---|---|
| `HOST_ABI.md` | 契约（MVP 六类、UXEP/UXIN、反模式） |
| `host-abi.js` | ABI 版本常量 |
| `packet.js` / `input.js` | UXEP / UXIN |
| `browser-host.js` | 浏览器 `host_*` |
| `core-asteroid.js` | demo 用 JS 核 |
| `math.js` `scene.js` | 矩阵 / InstanceCloud |
| `renderer-webgl.js` / `renderer-webgpu.js` | GPU 后端 |
| `uxe.js` | `createEngine` 底层门面（非主路径） |
| `demo/` | 源码测试 + `_probe.mjs` |
| `ship/` | 发布面：`build-asteroid.mjs` · `host-entry.js` · `bake-html.mjs` |
| `_packet_selftest*` / `_input_selftest*` | 二进制门禁 |

## 跑与门禁

```bash
cd ujs && npm run demo
# /engine/demo/          源码测试
# /engine/ship/          发布面
# ?gpu=webgl|webgpu|auto

npm run test:uxe:packet
npm run test:uxe:input
npm run test:uxe
npm run test:uxe:ship
```

## 与 `game/`

| 路径 | 角色 |
|---|---|
| **`web/engine/`** | **产品化主线** |
| `web/game/` | Three 对照 |
| `web/game/exp/` | 裸 GL 对照 |

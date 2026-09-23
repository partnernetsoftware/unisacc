# UXE — UJS eXperimental Engine

> **不是** Three 移植。按 UJS / UNISA 思路：闭合契约、薄胶水、玩法可走 `wasm_run`、GPU 经 Host ABI。  
> 设计真源：[`HOST_ABI.md`](HOST_ABI.md)。本文与目录文件一一对应；过时即改。

## 命题

```
Shell = 引擎核（经典调度） + UJS 脚本（玩法/决策） + Host ABI → GPU
```

| 层 | 现状（代码） | 方向 |
|---|---|---|
| 玩法 | `../game/sim.ujs` → `wasm_run` | 保持 UJS-1 |
| 核 | `core-asteroid.js`（JS 占位） | → 单 wasm，仍只调 `host_*` |
| Host | `browser-host.js` | 可换 Native Host |
| GPU 提交 | `packet.js` 二进制 **UXEP** → `host_gpu_submit` | buffer 驻留 |
| 输入 | `input.js` 二进制 **UXIN** | 手柄/指针后加 |
| GPU 后端 | **WebGL + WebGPU 自适应**（`prefer: auto\|webgl\|webgpu`） | compute / 驻留 buffer |
| 胶水 | `demo/host.js` | 仅 canvas + HUD + `createBrowserHost` |

## 架构（已实现）

```
demo/host.js          # 胶水
      ↓
core-asteroid.js      # 核：UJS 步进 + encodeRenderPacket
      ↓ host_*
browser-host.js       # 浏览器宿主（auto: WebGPU→WebGL）
      ↓ decode UXEP → WebGPU 或 WebGL
renderer-webgpu.js | renderer-webgl.js
```

`createEngine` / `uxe.js`：底层场景 API，供宿主内部或实验用；**演示主路径走 Host ABI**。

## 文件职责

| 文件 | 职责 |
|---|---|
| `HOST_ABI.md` | 契约说明（MVP 六类、packet、反模式） |
| `host-abi.js` | ABI 版本常量 / typedef |
| `packet.js` | UXEP encode/decode |
| `input.js` | UXIN encode/decode |
| `browser-host.js` | 浏览器 `host_*`；**WebGL / WebGPU 自适应** |
| `core-asteroid.js` | Asteroid 核（不知 canvas） |
| `math.js` `scene.js` | 矩阵 / InstanceCloud |
| `renderer-webgl.js` | WebGL1 实例化后端 |
| `renderer-webgpu.js` | WebGPU 实例化绘制（失败则宿主回退 WebGL） |
| `uxe.js` | `createEngine` 门面（底层） |
| `demo/` | 产品演示页 + CDP `_probe.mjs` |
| `_packet_selftest*` | packet 门禁（alarm） |

## 跑与门禁

```bash
cd ujs && npm run demo
# http://127.0.0.1:8765/engine/demo/           # prefer=auto
# …/engine/demo/?gpu=webgl | ?gpu=webgpu       # 强制

npm run test:uxe:packet   # alarm 20
npm run test:uxe:input    # alarm 15
npm run test:uxe          # alarm 55，需本机 Chrome
```

发版：`./ujs/scripts/release-artifacts.sh --with-demos` 包含本目录（无自测脚本亦可）。

## 与 `game/` 

| 路径 | 角色 |
|---|---|
| **`web/engine/`** | **产品化主线** |
| `web/game/` | Three 对照 |
| `web/game/exp/` | 裸 GL 对照实验 |

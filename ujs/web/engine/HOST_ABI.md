# UXE Host ABI v0

> WASM 游戏核 **不**知道自己在浏览器里。只认本 ABI。  
> 浏览器 / 原生各自实现同一套 `host_*`。

```
WASM Game Core
      ↓
  Host ABI  (本文件)
      ↓
Browser Host | Native Host
      ↓
WebGPU 和/或 WebGL（浏览器自适应）| Vulkan/Metal/D3D12/SDL …
```

## 开发 vs 发布（交付面）

| | 开发 | 发布（目标） |
|---|---|---|
| 源码 | `engine/*.js` 分文件 | **不**把拆分源当交付物 |
| 页面 JS | `demo/` 多模块 + compiler | **极薄胶水**（GPU Host + 双 wasm 桥） |
| 引擎 | `ujs_full.wasm` | **`gameEngine.wasm`**（同产物，交付名） |
| 游戏 | `core-asteroid.js` | **`{gameName}.wasm`**（如 `asteroid.wasm`） |
| 资源 | `sim.ujs` 可外置 | 编进 `{game}.wasm` |

今日：`index.html` + `asteroid.wasm` + `gameEngine.wasm`（**不合包**）。  
明日：胶水再收；Host 只留 DOM 接线。

## 浏览器 GPU 后端（一等公民 · 自适应）

宿主必须同时具备两套主流 Web 图形接口，按能力选择，**不是二选一放弃**：

| 后端 | 定位 |
|---|---|
| **WebGL** | 上一代；OpenGL ES 思路；成熟、兼容面宽 |
| **WebGPU** | 新一代；接近 Vulkan / Metal / D3D12；更现代的提交模型，并可做通用 GPU 计算 |

选择策略（`createBrowserHost({ prefer })`）：

- `auto`（默认）：有可用 WebGPU 则用，否则 **WebGL**
- `webgpu` / `webgl`：强制；`webgpu` 失败时仍回退 WebGL（并打日志）
- 演示页：`?gpu=auto|webgpu|webgl`

约束：`createWebGPURenderer` **仅在** pipeline 建造成功后才 `getContext("webgpu")`，避免锁死 canvas 导致无法回退 WebGL。  
核只提交 **UXEP packet**；译成 WebGL 或 WebGPU command 是宿主的事。

## MVP 六类（够跑通一帧游戏）

| 符号 | 作用 |
|---|---|
| `host_time()` | 单调毫秒（或秒）时间戳 |
| `host_input_read(buf?)` | 无 buf → 对象；有 ArrayBuffer → 写入 **UXIN**，返回 byteLength |
| `host_frame_begin()` / `host_frame_present()` | 与宿主事件循环对齐；present 换缓冲 |
| `host_gpu_submit(packet)` | **一次**提交 render packet（非逐 draw） |
| `host_asset_read(path) → bytes` | 只读资源 |
| `host_log(level, msg)` | 诊断 |

后加（v0 未做；路线见 `FUTURE.md` P0.4）：雾/光进 UXEP、材质、多 mesh、指针、`host_audio` / `host_net` / `host_storage`。

## 反模式

- 不要把 `canvas.getContext` / `fetch` / `addEventListener` 直接塞进核。
- 不要 `host_draw_mesh()` 每物体跨边界一次；聚成 **packet** 再 `host_gpu_submit`。
- 不要 `import three`；**能力对标**靠加厚本 ABI，不是搬库。

## Render packet（**已线性化**，`packet.js`）

逻辑视图（**v1** 已实现；**v2** = v1 + fog + lights，见下）：

```
Packet v1 {
  clear: [r,g,b,a]
  camera: { fovy, near, far, eye[3], target[3] }
  clouds: [
    { color[3], count, xyz: Float32Array, scale: Float32Array }
  ]
}
```

**v2 增量**（H1）：fog + ambient + lights。  
**v3 增量**（H2）：每 cloud 增加 `emissive[3]`、`metalness`、`roughness`、`mesh_id`（0=八面体岩，1=船楔，2=盒）。

**线上格式**（LE，magic `UXEP`）见 `packet.js`。decode 接受 v1–v3。  
`host_gpu_submit` 主路径吃 **ArrayBuffer**；对象仅调试兼容。

宿主：解码 packet → 译成 **WebGL 或 WebGPU** command → **一次** `present`。
预置几何：`meshes.js`。

## 输入快照（**已线性化**，`input.js`）

逻辑视图（JS 兼容对象仍可带 `keys` 诊断字段）：

```
Input {
  ix: i32   // -1/0/1 横向
  iy: i32   // -1/0/1 纵向
  fire: u32 // 0/1
  // mx, my, buttons — 后加（reserved）
}
```

**线上格式**（LE，magic `UXIN`，version=1，固定 20 字节 + 可选 reserved）：

```
u32 magic "UXIN" | u32 version=1 | i32 ix | i32 iy | u32 fire | [reserved…]
```

见 `input.js` `encodeInputSnapshot` / `decodeInputSnapshot`。  
`host_input_read(buf)` 主路径写 **ArrayBuffer**（wasm-ready）；无参返回对象仅兼容。

## 与 UJS

- **玩法步进**：`gameEngine.wasm`（=`ujs_full`）跑预编译 sim image；demo 仍可页内 `compile` + `wasm_run`。
- **游戏核**：`{game}.wasm` 只调 Host ABI + `eng_boot`/`eng_sim_step`（胶水桥到引擎）；**不**与引擎合包。
- 数据流：UJS 产出状态 → 核写入 UXEP → `host_gpu_submit`。

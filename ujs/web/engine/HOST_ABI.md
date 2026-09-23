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
WebGPU/WebGL | Vulkan/Metal/SDL …
```

## MVP 六类（够跑通一帧游戏）

| 符号 | 作用 |
|---|---|
| `host_time()` | 单调毫秒（或秒）时间戳 |
| `host_input_read(buf?)` | 无 buf → 对象；有 ArrayBuffer → 写入 **UXIN**，返回 byteLength |
| `host_frame_begin()` / `host_frame_present()` | 与宿主事件循环对齐；present 换缓冲 |
| `host_gpu_submit(packet)` | **一次**提交 render packet（非逐 draw） |
| `host_asset_read(path) → bytes` | 只读资源 |
| `host_log(level, msg)` | 诊断 |

后加（刻意不做进 v0）：`host_audio` / `host_net` / `host_storage` / 剪贴板等。

## 反模式

- 不要把 `canvas.getContext` / `fetch` / `addEventListener` 直接塞进核。
- 不要 `host_draw_mesh()` 每物体跨边界一次；聚成 **packet** 再 `host_gpu_submit`。

## Render packet（**已线性化**，`packet.js`）

逻辑视图：

```
Packet {
  clear: [r,g,b,a]
  camera: { fovy, near, far, eye[3], target[3] }
  clouds: [
    { color[3], count, xyz: Float32Array, scale: Float32Array }
  ]
}
```

**线上格式**（LE，magic `UXEP`，version=1）见 `packet.js` `encodeRenderPacket` / `decodeRenderPacket`。  
`host_gpu_submit` 主路径吃 **ArrayBuffer**；对象仅调试兼容。

宿主：解码 packet → 译成 WebGL/WebGPU command → **一次** `present`。  
浏览器：`createBrowserHost({ prefer: "webgpu"|"webgl" })` 默认先 WebGPU 实例化路径，失败回退 WebGL。

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

- **玩法步进**仍可 `wasm_run(sim.ujs)`（构造/验收路径）。
- **引擎核**目标：单 wasm 只调 Host ABI；UJS 产出状态 → 核写入 packet → `host_gpu_submit`。

# UXE Host API

> **真源**：本文件。实现：`host-browser.js` · 类型草稿：`host-abi.js` · 二进制：`packet.js` / `input.js`。  
> 版本常量：`HOST_ABI_VERSION`（当前 **0**）。改 `host_*` / UXEP / UXIN 须同会话改本文件与门禁。

游戏核（`{game}.wasm` 或 demo 的 JS 核）**只认本 API**，不知道自己在浏览器、Node 还是原生壳里。  
浏览器 Host / 原生 Host 各自实现同一套符号；GPU、键盘、密钥、网络都停在 Host 这一侧。

```
┌─────────────────────┐
│  {game}.wasm / 核    │  调度 · 组 UXEP · 读 UXIN
└──────────┬──────────┘
           │  host_*  （本文件）
┌──────────▼──────────┐
│  Host 实现           │  Browser · Native · （将来）平台壳
└──────────┬──────────┘
           │
    WebGL / WebGPU · 输入 · 资源 · （将来）LLM / 音频 / 存储
```

配套引擎（path-A 演示）：交付名曾用 **`engine.wasm`**（=`ujs_full.wasm`）。  
**P0 出货 Pages 不依赖它**：步进默认 path-B **`sim.wasm`**（`compiler_core` 编）。  
命名不用 `gameEngine`：同一 Host 还要跑 **wasm app**（AI harness、工具壳等），不只是游戏。  
与 `{game|app}.wasm` **不合包**（path-A 双核场景）；path-B 热路径甚至不再加载 `ujs_full`。

---

## 0. Ship 胶水（ship-js 现状）

Asteroid / 无人机 **Pages 已走 ship-js**：`game.js` + 共享 `engine.js` + **`sim.wasm`**（`directSim`），**无** 出货 `engine.wasm`、**无** `{game}.wasm` 必经、**无** `eng_boot` / `eng_sim_step` 双 memory 搬砖。  
编译：`compile.mjs` 默认 **`compiler_core.wasm`**（M3；非 IntNet）。门禁：`./tests/ujs2wasm_step.sh` · `./tests/uxe_ship_js.sh` · `./tests/ujs2wasm_compiler.sh` — 见 [`prd.md`](../prd.md) M1–M3 / P0。

| 块 | 落点 |
|---|---|
| WebGL / WebGPU 译包 | Host（不可沉） |
| UXEP/UXIN | Host 编解码 · 核只调 `host_*` |
| 玩法调度 | `app-*.js`（Asteroid/drone：`directSim`+`sim.wasm`；禁拉 `compiler.gen`） |
| 遗留 C 双 wasm + eng_* | `native/uxe_asteroid.c` · 旧 host-entry — **不再出货** |
| path-A `ujs_full` | demo / `FORCE_WEB_BUILD`；**非** ship 必经 |

门禁：`./tests/uxe_ship_js.sh`（无 eng_* · 无强制 asteroid.wasm · game-ready emit）。

---

## 1. 设计原则

| 要 | 不要 |
|---|---|
| 一次 `host_gpu_submit` 交一整包画面 | 每物体一次 `host_draw_*` |
| 输入写成 **UXIN** 字节（wasm-ready） | 核里 `addEventListener` |
| 资源经 `host_asset_read` | 核里直接 `fetch` / 读盘 |
| 密钥、LLM、账号只在 Host / 页面壳 | API Key 进 wasm、进 UJS globals、进日志 |
| 能力对标 Three **常用子集** | `import three` / 搬库进主线 |

---

## 2. 调用面一览（v0 已实现）

| 符号 | 签名（逻辑） | 作用 |
|---|---|---|
| `host_time` | `() → f64` | 单调时间（浏览器：`performance.now()` 毫秒） |
| `host_input_read` | `(buf?) → Input \| i32` | 无 buf → JS 对象（调试）；有 `ArrayBuffer` → 写入 **UXIN**，返回字节数 |
| `host_frame_begin` | `() → void` | 帧起点（与宿主事件环对齐；浏览器可为空） |
| `host_frame_present` | `() → void` | 帧终点 / 换缓冲钩子（浏览器可为空；ship 胶水可在此刷 HUD） |
| `host_gpu_submit` | `(packet) → void` | 提交 **UXEP**（主路径 `ArrayBuffer`；对象仅调试） |
| `host_debug_snapshot` | `() → object` | **浏览器调试**：一帧 JSON（input · packet 摘要 · `__UXE__`）；同步写 `window.__UXE_SNAP__` |
| `host_asset_read` | `(path) → bytes` | 只读资源（浏览器：`fetch`；相对路径相对 `baseURL`） |
| `host_log` | `(level, msg) → void` | 诊断：`info` / `warn` / `error` |
| `host_request_frame` | `(cb) → void` | 下一帧回调（浏览器：`requestAnimationFrame`） |

浏览器实现：`createBrowserHost(canvas, { prefer, baseURL })` → 上表 + `backend`（`"webgl"` \| `"webgpu"`）+ `version`。

**代理 / 门禁**：优先读 `__UXE__` 与 `host_input_read()`（及规划中的 snapshot，见 [`prd.md`](../../prd.md) §6）；不要用截图当默认验收。

**GPU 选择**（`prefer`）：

- `auto`（默认）：能起 WebGPU 则用，否则 WebGL  
- `webgpu` / `webgl`：强制；`webgpu` 失败仍回退 WebGL  
- 页参：`?gpu=auto|webgpu|webgl`  
- 约束：WebGPU pipeline **建造成功**后才锁 `getContext("webgpu")`，否则无法回退

出货路径为 **ship-js**（见 §0）。遗留 C 双 wasm 曾用页内 `eng_boot` / `eng_sim_step`（`ship/host-entry.js`），**不再出货**。

---

## 3. 帧循环约定

典型一帧（核侧）：

```
host_frame_begin()
host_input_read(uxin_buf)     // 或无参调试
… 玩法步进（UJS / 本地状态）…
host_gpu_submit(uxep_buf)     // 一包画完
host_frame_present()
host_request_frame(下一帧)
```

- **异步**：`host_asset_read` 在浏览器是 `async`；boot 阶段 await，热路径避免每帧 fetch。  
- **同步 wasm import**：C `{game}.wasm` 里的 `host_*` 由胶水做成同步包装；真正的 `fetch` / LLM 须走「请求槽 + 下帧取结果」或 Host 侧预取。

---

## 4. UXEP — 渲染包

Magic `UXEP`（LE `0x50455855`）。编码：`encodeRenderPacket` / `decodeRenderPacket`（`packet.js`）。

| 版本 | 内容 |
|---|---|
| **v1** | clear · camera · clouds（color / count / xyz / scale） |
| **v2** | + fog · ambient · directional lights[] |
| **v3（当前编码）** | + 每 cloud：`emissive[3]` · `metalness` · `roughness` · `mesh_id` |

逻辑视图（v3）：

```
Packet {
  clear: [r,g,b,a]
  camera: { fovy, near, far, eye[3], target[3] }
  fog: { density, color[3] }
  ambient: { color[3], intensity }
  lights: [ { dir[3], color[3], intensity }, … ]
  clouds: [ {
    color[3], emissive[3], metalness, roughness, mesh_id,
    count, xyz: f32[count*3], scale: f32[count]
  }, … ]
}
```

**预置 `mesh_id`**（`meshes.js`）：

| id | 几何 |
|---|---|
| 0 | 八面体（岩） |
| 1 | 船楔 |
| 2 | 盒 |

加几何 = 扩表 + 双后端支持 + 版本策略；**不加** `host_draw_mesh`。

线上布局细节以 `packet.js` 头注释为准；decode 接受 v1–v3。

---

## 5. UXIN — 输入快照

Magic `UXIN`（LE `0x4e495855`）。

| 版本 | 大小 | 字段 |
|---|---|---|
| **v1** | 20 B | `ix` `iy` `fire` |
| **v2（当前）** | **36 B**（`INPUT_BYTES`） | + **`mx` `my`**（指针 NDC ∈ [-1,1]，中心原点，+y 向上）· **`buttons`** · **`flags`** |

```
u32 magic | u32 version | i32 ix | i32 iy | u32 fire
f32 mx | f32 my | u32 buttons | u32 flags     ← v2
```

| 字段 | 含义（浏览器） |
|---|---|
| `ix` / `iy` | −1/0/1 ← WASD / 方向键；**或** 按住后相对按下点拖动合成的虚拟摇杆（死区约 0.32 NDC） |
| `fire` | Space **或** 主触点按下（鼠标左键 / 手指） |
| `mx` / `my` | 相对 canvas 的 NDC；Pointer Events（含 touch）更新 |
| `buttons` | bit0 主触点 · bit1 右 · bit2 中 |
| `flags` | bit0 = 指针在 canvas 内 · bit1 = suicide（KeyF）· **bit2 = touch** · **bit3 = look-stick**（`mx/my` 为右摇杆轴） |

**触屏约定（Host 侧）：**

- canvas `touch-action: none`，拦截默认滚动
- **不**对 touch 请求 Pointer Lock
- **单指**：相对按下点拖动 → `ix/iy`（移动 / 油门）
- **双指**：左侧指 → `ix/iy`；右侧指 → `mx/my` 连续看轴 + **`FLAG_LOOK_STICK`**（bit3）
- Asteroid 撞毁后：**双击**重开（键盘空格仍单击即可）

`flags`：bit0 指针在内 · bit1 suicide · bit2 touch · **bit3 look-stick**。

decode 接受 v1 与 v2。`encode` 在缓冲 ≥36 时写 v2，仅 20 时写 v1（兼容旧 `{game}.wasm` 小缓冲）。

见 `input.js`。门禁：`npm run test:uxe:input`。

---

## 6. 交付面与 Host 的关系

| | 开发 `uxe/demo/` | 发布 `uxe/ship/` · Pages `docs/uxe/` |
|---|---|---|
| 玩法 | 页内 compile / 读 `.ujs` | 预编译 **path-B** |
| 核 | `app-*.js` | **`sim.wasm`** + `directSim`（UJS-1_ship · `compiler_core`） |
| 引擎 VM | 可选 `ujs_full`（path-A） | **无** ship 必经 `engine.wasm`（P0） |
| Host | 分模块 `host-browser.js` | 共享 `engine.js` + 薄页 |

Pages 只镜像 **ship 静态面** + 游戏索引；不放源码 demo。

```
docs/uxe/engine.js          # 共享 Host + GPU
docs/uxe/{game}/index.html  # 薄页
docs/uxe/{game}/game.js     # 本游戏胶水（directSim）
docs/uxe/{game}/sim.wasm    # 玩法核（core 编）
```

```bash
cd ujs && npm run ship:pages    # 默认 compiler_core；无 python emit / 无 A 核
```

两款均为 ship-js：`game.js` + **`sim.wasm`** + 共享 `../engine.js`（无出货 `engine.wasm`、无强制 `{game}.wasm`）。

---

## 7. 扩展路线（Host 加厚）

对标 Three 常用子集 → 见 [`prd.md`](../prd.md) 目标 #5。摘要：

| 档 | 能力 | 落点 |
|---|---|---|
| H1 ✓ | 雾 / 环境光 / 方向光 | UXEP v2 |
| H2 ✓ | 金属粗糙自发光 / 多 mesh | UXEP v3 · `meshes.js` |
| **H3 ✓ 指针** | 鼠标/触屏 NDC + 按键位 + Host 虚拟摇杆 | **UXIN v2**（`mx/my/buttons/flags`，`FLAG_TOUCH`） |
| H3 余 | 点精灵 · 更多 action bit | UXEP points · UXIN 再扩 |
| **H4** | 贴图 · buffer 驻留 | `map_id` · `host_asset` |
| 后加 | `host_audio` · `host_storage` · `host_net` | 独立符号，仍经 Host |

### 7.1 平台向：`host_llm`（规划，未实现）

目标形态：**用户自带 LLM API**（Bring Your Own Key）——平台不代持密钥、不做「谁付钱」的中心；壳收集 endpoint + key，Host 代发请求，核只看见结构化结果。

```
用户（页面壳）
  └─ 填 base URL / model / API Key（仅存 Host 内存或 host_storage）
游戏核
  └─ host_llm_request(req_bytes) → 请求 id
  └─ host_llm_poll(id) → { pending | ok(bytes) | err }
       或：host_llm_ask 异步槽，下帧取结果
```

| 约束 | 原因 |
|---|---|
| Key **永不**进入 `{game}.wasm` / UJS 返回值 / `host_log` | 防泄漏、可审计 |
| 协议先定 **OpenAI-compatible chat** 子集 | 覆盖多数网关；特化模型后加 |
| 请求/响应走 **bytes + 小 schema**（类 UXEP） | 核不知 `fetch`；可换原生壳 |
| 限流 / 超时 / 取消在 Host | 核保持同步帧友好 |
| 与玩法解耦：LLM 是 **Host 能力**，不是 UJS 语言特性 | 无 API 的部署仍可跑纯本地游戏 |

规格与目标见 [`../prd.md`](../prd.md)。

---

## 8. 反模式（再列一次）

1. 核里碰 `canvas` / DOM / `fetch` / 文件系统。  
2. 逐 draw 跨边界。  
3. 为单一游戏开旁路 API（缺口进本表版本化）。  
4. 出货 Pages 再引入 `engine.wasm` / 把玩法核与 path-A VM 合包。  
5. 把 LLM Key 或用户隐私写进 packet / sim globals。
6. 把 M3 `compiler_core` 叙述成 IntNet，或把全表 UJS-1 写成已由 core 交付。

---

## 9. 验收门禁

```bash
# 语言 / 构造脊
./tests/ujs.sh
# 出货脊（M3/P0）+ ship-js 合同
./tests/ujs2wasm_compiler.sh
./tests/uxe_ship_js.sh
# UXE 无人（另门；不挡上两者）
cd ujs && npm run test:uxe:all
```

改 ABI 而未改本文件 / 门禁 = 文档债务。两脊与子集边界见 [`../prd.md`](../prd.md) · Paper B [`../../research/ujs-paper.md`](../../research/ujs-paper.md)。

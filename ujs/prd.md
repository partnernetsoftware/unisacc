# UJS —— 产品规格 v1.1

> **活规格**（与代码同会话更新）。完整条款表见归档 [`archive/prd-v1.0.md`](archive/prd-v1.0.md)。  
> Host 细节真源：[`web/engine/HOST_ABI.md`](web/engine/HOST_ABI.md) · 地图：[`DOCS.md`](DOCS.md) · 路线：[`FUTURE.md`](FUTURE.md)

---

## 0. 一句话

**UJS** = 可构造的闭合脚本语言（UJS-1）+ `wasm_run` 产品 API + **UXE**（Host ABI · 可换游戏核 · 外网 Pages）。  
构造权重，不训练填表；验收以门禁与**可机读 snapshot**为准，不以截图猜 UI。

---

## 1. 三层交付

| 层 | 交付 | 不是 |
|---|---|---|
| **产品 API** | `bootRuntime` / `wasm_run`（`ujs/web` · Node/Bun/浏览器） | 页内训练 · 完整 ES |
| **UXE** | Host + `{game}` 核 + `engine.wasm` + 共享 **`engine.js`** | Three 移植 · 合包 |
| **构造侧** | `construct/` gold → 权重 / `web-build` | 应用运行时依赖 |

### 1.1 外网游戏面（Pages · `docs/`）

```
docs/uxe/engine.js           # 共享 Host + GPU（两游戏共用）
docs/uxe/{game}/index.html   # 薄页
docs/uxe/{game}/game.js      # 本游戏胶水 / 核
docs/uxe/{game}/*.wasm
docs/index.html              # 索引（Asteroid · 无人机）
```

本地：`/engine/demo/` 分源 · `/engine/ship/` 对照。  
重建：`npm run ship:pages` · 无人门禁：`npm run test:uxe:all`。

### 1.2 不交付

完整 ECMAScript · 开放原型 · `eval` · async 一等公民 · 把 Three 搬进 wasm · 用户 LLM key 进核/packet。

---

## 2. 语言 UJS-1（摘要）

闭合子集：**表内全功能，表外永久拒绝。**  
有：字面量、list/dict、`let`/`const`、if/while/for/switch、function/箭头、rest/spread、算术比较、索引成员、`len`/`keys`、只读外层闭包。  
无：`undefined` 双无值、`var`/hoisting、`this`/`class`/prototype、`eval`、async/generator、RegExp 字面量。

名字解析：**locals → 词法外层 → globals**；未命中硬错。

完整 [L-*][V-*][A-*][G-*][X-*] 条款表 → [`archive/prd-v1.0.md`](archive/prd-v1.0.md)。  
改语言行为须同步该归档或升版 prd，并跑 `./tests/ujs.sh`。

---

## 3. 产品 API

```text
compile(src) → Fn
wasm_run(fn|src, globals, locals) → { ok } | { err }
```

- 浏览器 / Node / Bun 同源；二进制与 `BUILD.json` 指纹对齐（Release）。  
- **不在页面训练**；构造：`python3 -m ujs web-build` / `build-weights`。  
- UXE **不得**偷偷改 `wasm_run` 契约；Host 符号另见 HOST_ABI。

---

## 4. UXE 嵌入约束

| 约束 | 说明 |
|---|---|
| **双 wasm** | `{game}.wasm`（或 ship-js 核）≠ `engine.wasm`（=`ujs_full`）；永不合包 |
| **共享 engine.js** | Host + WebGL/WebGPU；游戏只交 `game.js` |
| **核只调 host_*** | 不知 canvas / fetch / 密钥 |
| **一次一包** | UXEP → `host_gpu_submit`；UXIN → `host_input_read` |
| **触屏** | 单指移动摇杆；双指左飞右看（`FLAG_LOOK_STICK`） |

契约细节：[`HOST_ABI.md`](web/engine/HOST_ABI.md)。平台叙事：[`PLATFORM.md`](web/engine/PLATFORM.md)。

---

## 5. 验收

| 门 | 命令 | 要求 |
|---|---|---|
| **语言 / 构造** | `./tests/ujs.sh` · `npm test` | acc/fold/icfold/difftest/ship … |
| **UXE 无人** | `cd ujs && npm run test:uxe:all` | packet · input · demo · ship · drone*（CDP · 禁代理 · 自启 8765） |
| **Pages** | `npm run ship:pages` 后人工抽玩感 | 索引可点；cache-bust `?v=` |

CI 是第二意见；本地门禁先绿。人只审手感，不点门禁。

---

## 6. 代理可观测性（开发加速）

### 6.1 原则

- **断言写在数上**；浏览器截图对代理又贵又脆，不作默认验收。  
- 已有路径：CDP → `__UXE__` / `__UXE_HOST__.host_input_read()` / `__DRONE_API__`。  
- 目标：每个 ship/demo 暴露**同一套 snapshot 契约**，门禁与代理共用。

### 6.2 已有（须保持）

| 句柄 | 内容 |
|---|---|
| `window.__UXE__` | 玩法 HUD 态：`ready` · `backend` · 分数/弹药/锁定/… |
| `window.__UXE_HOST__` | `host_input_read()` → UXIN 对象（含双指 flags） |
| `window.__DRONE_API__` 等 | 探针专用确定性动作（face/fire/detonate） |
| `_cdp.mjs` + `test:uxe:*` | 无头 Chrome · 禁代理 · wall clock |

### 6.3 契约：`__UXE_SNAP__` / `host_debug_snapshot()`

一帧一 JSON，字段稳定、可 diff（Host 每帧提交后刷新 `window.__UXE_SNAP__`）：

```text
{
  t: number,                 // host_time
  backend: "webgl"|"webgpu",
  input: { ix, iy, fire, mx, my, buttons, flags },   // 当前 UXIN
  inputRing?: Input[],       // 最近 N 帧
  uxe: object,               // 与 __UXE__ 同形
  packet?: {                 // 上一包 UXEP 摘要（解码后）
    clear, eye, target, fogDensity,
    clouds: [{ meshId, count, yMin, yMax }]
  }
}
```

| 用途 | 怎么断言 |
|---|---|
| 天地方向 | `eye[1]` vs 地面 `yMin`/`yMax`；勿靠截图色块 |
| 触屏飞行 | 注入双指后 `flags & LOOK_STICK` · `iy === -1` |
| 回归 | `uxe.ammo` / `locked` / `alive` 与探针脚本 |

门禁：`npm run test:uxe:snap`（Asteroid + Drone ship 互动，走 snapshot）。

可选辅助（非主路径）：上/下半屏平均色。仍禁止以「看图说话」代替上表。

### 6.4 代理开发环（约定）

```
改 Host / 核 / 玩法
  → 本地 npm run test:uxe:all（或单测）
  → 失败则 CDP 拉 snapshot JSON 定位
  → 绿后再 ship:pages；人只审手感
```

新游戏进索引前：至少一条 CDP 探针读 snapshot，禁止「仅人工点鼠标验收」。

---

## 7. 与归档 / 其他文档

| 文档 | 角色 |
|---|---|
| **本文件** | 产品活规格 v1.1 |
| [`archive/prd-v1.0.md`](archive/prd-v1.0.md) | 原条款全表（L/V/A/G/X/IC/…） |
| [`HOST_ABI.md`](web/engine/HOST_ABI.md) | Host / UXEP / UXIN 真源 |
| [`FUTURE.md`](FUTURE.md) | 产品化下一刀（eng 下沉 · 模板 · snapshot 落地） |
| [`DOCS.md`](DOCS.md) | 阅读地图 |

升版：破坏性语言/API 变更 → bump 本文件版本号，旧版移入 `archive/`。

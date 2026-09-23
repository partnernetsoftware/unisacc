# UJS / UNISA — 未来考虑节点

> 不是当前里程碑；用来避免「造万物」冲动散焦。  
> 实践检验优先于再开一条语言线。

---

## 近期实践（优先用效果说话）

### P0 · 网页小游戏 demo（宿主 HTML/JS + UJS 脚本） — 已有

`ujs/web/game/`：**Asteroid Rush 3D**（`sim.ujs` 仿真 + Three.js 渲染 + HUD 显示每帧 ujs ms）。

概念边界见下文「思路整理」：

- 游戏逻辑 = **UJS-1 源码**（闭合子集），不是任意浏览器 JS
- 页面壳 = **普通 HTML/JS**（Three.js、键盘、HUD）
- 桥：一次 `compile`，每帧 `wasm_run(fn, G, L)`（今日绑定面仍偏数据）

验收：`npm run demo` → `/game/` 能飞、能撞、HUD 有 ujs step ms。

### P0.1 · 小实验（胆大、范围小）

`ujs/web/game/exp/`：**同 `sim.ujs`，宿主换成裸 WebGL 实例化**（不造引擎）。

假设：`ujs ms` ≈ Three 对照页；若差在 `draw ms`，说明该抠边界/编组，而不是先写「wasm Three」。

（已测定性：N=480 时 ujs≈0.5ms 量级两侧接近；draw 单独可报。）

### P0.2 · 交付面 / GPU 绑定（调研已收口）

**平台事实**（浏览器）：

```
WASM 核心  →  极薄 JS 绑定  →  WebGPU/WebGL  →  GPU
```

- 胶水级 JS **必要**（instantiate + 把 API 暴露给 wasm）；目标是 JS 退化成 bootstrap，业务不散落成可读 `.js` 树。
- Wasm **不内嵌** GPU：通过 import 调 WebGL / **WebGPU**（更现代：渲染 + 通用计算）。
- Shader **不在** wasm 里跑：WebGPU 下为 **WGSL**，由浏览器/驱动编译上 GPU。三层：`WASM(CPU) · WebGPU API · WGSL(GPU)`。
- 「看起来像 wasm 自己操作 GPU」= 绑定做得很薄（Rust/C++/… → wasm + generated bindings）。

**性能关键点（比「有没有 GPU」更要紧）**：少搬数据——大状态长期驻留 **GPU buffer**，每帧只下发小控制参数。这与「GPU 上大量 cell/neuron 并行」同构：算力在 GPU，瓶颈常在 CPU↔GPU 拷贝。

**与 UJS 的分轨：**

| 层 | 倾向 |
|---|---|
| UJS-1 / `wasm_run` | 可构造、可验收的脚本/玩法 |
| **UXE**（`web/engine/`） | 自研引擎主线：场景 + GPU 后端；非 Three |
| 下一刀 | WebGPU 实例绘制、buffer 驻留、引擎核收进单 wasm |

**信心边界：** 不移植 `three.module.js`；自研闭合子集 + WebGPU/WebGL 绑定。

### P0.3 · UXE 自研引擎 — **已开工**

路径：`ujs/web/engine/`。

- v0：闭合场景 API + WebGL 绘制 + WebGPU 探测/驻留 stub
- **Host ABI v0**（`HOST_ABI.md`）：`time/input/frame/gpu_submit/asset_read/log`；演示经 `browser-host` + `core-asteroid`（核不知 canvas）
- GPU：UXEP → **WebGL / WebGPU 自适应**（`prefer: auto`；`?gpu=` 可强制）；输入 UXIN
- 发版：`--with-demos` 含 `web/engine/`
- 验收：`test:uxe:packet` · `test:uxe:input` · `test:uxe`（均 alarm）

下一刀：GPU buffer 驻留少拷贝；核收进单 wasm（同 ABI）。

---

## 中期产品加深（A）

| 节点 | 内容 | 为何 |
|---|---|---|
| A1 | C 线：自举缺口 / 多文件 / libc 地板 | unisacc「能用」 |
| A2 | UJS：G 注入 host fn、`print`、错误行号 | 小游戏与嵌入刚需 |
| A3 | UJS 语言面克制扩展（模板串等） | 每项双 front + gold + parity |

## 第三同构实例（B）

选**天生有限表**的领域再做一条短线，回答「是不是只会写编译器」：

- 某 ISA 编码/重定位表（C 线已有雏形可挖）
- 正则 → DFA 表
- 字节码校验器 / 迷你类型检查器

判据：能较快看到 `acc=1.000` 的玩具 + 与 A/UJS 共享构造叙事。

## 元层（C，险）

从规格/形式描述**生成 gold**，减少手填表。挡在发版与 P0 之后。

## 先不做（D）

- 通用 npm 生态发布（UJS 未成熟）
- 完整 ECMAScript / 开放原型 / `eval` / async 当一等公民
- 用训练/LLM 填决策表（毁「无回退构造」命题）

---

## 发版与预置（已定方向）

- 二进制进 **GitHub Release**（脚本：`ujs/scripts/release-artifacts.sh`）
- git 留源码 + `BUILD.json` 指纹
- 工具索引：[`TOOLS.md`](TOOLS.md)

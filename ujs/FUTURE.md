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
| 下一刀 | WebGPU buffer 驻留；胶水 `eng_*` 再收（双 wasm 不变） |

**信心边界：** 不移植 `three.module.js`；自研闭合子集 + WebGPU/WebGL 绑定。

### P0.3 · UXE 自研引擎 — **交付面已验证**

路径：`ujs/web/engine/`。

- Host ABI v0：`time/input/frame/gpu_submit/asset_read/log`；UXEP / UXIN
- GPU：WebGL / WebGPU 自适应
- **交付（不合包）**：`index.html` + `{game}.wasm` + `gameEngine.wasm`（=`ujs_full`）
- 开发：`/engine/demo/` 分源；发布：`npm run ship:engine` → `/engine/ship/`
- 验收：`test:uxe:packet` · `test:uxe:input` · `test:uxe` · `test:uxe:ship`

下一刀：GPU buffer 驻留少拷贝；胶水再收（`eng_*` 桥可下沉，仍保持双 wasm）。

**已否决**：把游戏核与 VM 打成单个 wasm（验证过可做，但交付边界错误）。

### P0.4 · Host API 加厚，**能力对标** Three（不是搬库）

先前说「不可能」指的是：**Three-in-wasm / 合包 / 开 ES**。那些没做；做的是另一条——且已跑通。  
正确读法：**逐步加厚 Host ABI + UXEP/UXIN**，使画面与交互能力逼近 Three 对照页所用子集；宿主仍译包，核仍不知 canvas。

对标对象（`web/game/host.js` 实际用到的）：

| Three 用法 | UXE 落点 | 阶段 |
|---|---|---|
| InstancedMesh + PerspectiveCamera | UXEP clouds + camera（**已有**） | — |
| FogExp2 | UXEP fog | **H1 ✓** |
| Ambient + DirectionalLight | UXEP lights | **H1 ✓** |
| MeshStandard（金属/粗糙/自发光） | cloud material 字段 | **H2 ✓** |
| 多几何（船 vs 岩） | mesh_id 预置表 | **H2 ✓** |
| Points（星空） | UXEP points / kind=points | H3 |
| 指针 / 多键 | UXIN 扩 reserved | H3 |
| 贴图 / 环境光 | `host_asset` + map_id；buffer 驻留 | H4 |
| 阴影 / 后处理 | 很晚；有证据再开 | H5+ |

原则：

- **加字段进 packet**，不加 `host_draw_mesh` 逐物体 API。
- 版本 bump（UXEP v2…）；旧包拒绝或显式兼容层。
- 每档：双后端（WebGL+WebGPU）+ packet/input 自测 + demo/ship 探针。
- **永不** `import three.module.js` 进主线。

---

## 平台怎么长出来（不谈变现）

命题：**Host 能力逼近 Three 常用子集 → 做好游戏移植与 demo → 自然长成游戏平台。**  
不先设计「谁付钱」；交付面与玩法密度到位，平台身份是结果不是 KPI。

### 增长链

```
加厚 Host/UXEP（对标 Three 能力）
        ↓
同一套 ship：html + {game}.wasm + gameEngine.wasm
        ↓
多款 demo / 移植（证明可换核、可换玩法）
        ↓
别人按模板打自己的 {game}.wasm  →  平台
```

### 当下该堆的

| 优先 | 内容 |
|---|---|
| **H2+** | 材质、多 mesh、点精灵、指针……按 P0.4 表继续 |
| **第二款游戏** | **城市大富翁（单机）** · `demo/monopoly/` · 验证 Host 模板可复制 |
| **移植** | 选小型 Three/经典小游戏，逻辑收进 UJS-1 + `{game}.wasm` |
| **模板** | `ship:engine` 文档化：换 sim / 换核的最短路径 |
| **A2** | host fn / 错误行号——写第二款玩法时会卡住 |

### 实践车：三维城市大富翁（单机）

路径：`web/game/monopoly.ujs` + `engine/core-monopoly.js` + `engine/demo/monopoly/`。

| 已通 | 证据 |
|---|---|
| 16 格环盘 + BOX/OCTA 实例 | UXEP v3 clouds；`MESH_BOX=2` |
| 掷骰/走格/买/跳过/租金/破产 | UJS sim；骰子在 Host（无 RNG） |
| 人机对战 | 人空格掷/买 · A 跳过；AI 自动 |
| 探针 | `npm run test:uxe:monopoly` · `test:uxe:monopoly:rules` |

**本游戏卡住 Host 的真实缺口（驱动 H3+）**：

| 缺口 | Three 对照 | 落点 |
|---|---|---|
| 多键语义（买/跳过/建房） | `keydown` 多码 | UXIN reserved / action bits → **H3** |
| 点选格子 / 轨道相机 | Raycaster + OrbitControls | 指针 + 相机模式 → **H3** |
| 地块色带 / 牌面字 | 贴图或 CanvasTexture | map_id / 字形 → **H4** |
| 掷骰音效 | Audio | `host_audio` → 后加 |
| ship `{monopoly}.wasm` | 同 asteroid 模板 | **暂用 ship-js**（预编译 sim + 打包核 + gameEngine）；C 核后补 |
| **GitHub Pages** | 外网测 ship | `docs/index.html` 游戏索引 + `docs/uxe/{asteroid,monopoly}/` |

原则：缺口进 `HOST_ABI` / 本表，**不**为 Monopoly 特开旁路 API。

### 仍不做（技术边界，不是商业话术）

- 搬 `three.module.js`；游戏与引擎合包；开放完整 ES；页内训练填表。

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

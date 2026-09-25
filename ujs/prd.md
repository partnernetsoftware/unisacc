# UJS —— 产品规格 v1.3

> **唯一耦合活文档**（规格 · 目标 · 思维树 · 记忆宫殿）。与代码同会话更新。  
> Host 契约真源：[`uxe/HOST_ABI.md`](uxe/HOST_ABI.md) · 开箱：[`README.md`](README.md)  
> 归档：[`archive/`](archive/)（旧条款 / 旧路线 / 旧地图）

---

## 0. 一句话

**UJS** = 尽量贴 ES 表面的**闭合**脚本子集 + **构造法确定性推理**（gold → IntNet，不训练填表）+ 产品 API `wasm_run` + **UXE**（Host ABI · 双 wasm · Pages）。  
验收：门禁与可机读 snapshot；不以截图猜 UI。

| 层 | 交付 | 不是 |
|---|---|---|
| **core** | `bootRuntime` / `wasm_run` / `compile` · `ujs_full.wasm` | 页内训练 · 完整 ES |
| **UXE** | Host + `{app}` 核 + `engine.wasm` + 共享 `engine.js` | Three 移植 · 合包 |
| **构造** | `construct/` gold → 权重；`ujs2wasm` → `\0asm`；`web-build` → `core/` | 应用运行时依赖 |

**路径 A**（共享 VM）：`web-build` → `ujs_full.wasm` ≡ UXE **`engine.wasm`**（已交付；内核仍 C+zig）。  
**路径 B**（直出）：`python3 -m ujs ujs2wasm foo.ujs -o foo.wasm`（已交付；`isel`/`enc`→WAT→wasm）。  
开发可先 B 写 `.ujs`，再把 UXE `{app}` 接到 B（§目标）。

**语言（闭合）**：表内全功能，表外永久拒绝。有字面量 / list·dict / let·const / if·while·for·switch / function·箭头 / rest·spread / 算术比较 / 索引成员 / `len`·`keys` / 只读外层闭包。无 `undefined` 双轨、`var`/hoisting、`this`/`class`/prototype、`eval`、async/generator、RegExp 字面量。条款全表见 [`archive/prd-v1.0.md`](archive/prd-v1.0.md)。

**不交付**：完整 ES · Three-in-wasm · 合包 · 用户 LLM key 进核/packet · UJS 直播主线。

---

## 目标（按杠杆）

别人能按模板交 `{app}`，外网可玩，验收无人。**主线夯 0–4；门禁先于功能。** 不开新门面游戏。

**北极星：UJS 自举** — 最终由 UJS 写的编译器把 UJS→wasm；**逐步脱离 Python**（今仅构造臂可忍）。本应用 **unisacc** 作宿主/工具链底座，但它仍在进化、尚未稳到扛日常开发，故 Python 暂顶；unisacc 稳后优先把 construct 迁回去，再收束到 UJS 自举。Python 最终只留 gold/oracle/acc 对照，非 ship 必经。

**域分工**：unisacc = C 实践 · ujs = JS+wasm 实践 · **同理论**（构造法确定性推理=编译）。

### 自举阶梯

1. **今** Python construct 出 A（`engine`）+ B 工具（`ujs2wasm`）；Asteroid + drone Pages 步进默认 B
2. **#2b / M1 + M1b** 玩法步进直出（B）— **Asteroid ✓ · drone ✓**；build 仍一次 Python emit（M2 去掉）
3. 更多 construct 面迁入 UJS/wasm（front/emit 可在 wasm host 上用）
4. **自举** UJS 编译器：UJS→wasm；Python 仍作构造/对照，不挡出货

**反漂移**：下一步必须把能力推进 JS/wasm（或会打成 wasm 的 UJS 源）；Python construct **只许缩或持平**，禁止新增「仅 Python 才出货」的依赖。

### 何时可弃 Python（退出条件）

分两层，**先弃出货、后弃构造**。没达到门闩就还不能喊「脱离 Python」。

| 层 | 弃什么 | 做成什么之后可以弃 | 现在卡在 |
|---|---|---|---|
| **P0 弃出货 Python** | `web-build` / `ujs2wasm` CLI / 预编译 embed **不再是 ship 必经** | ① Pages 玩法步进只加载直出 `.wasm`（B），不再 `wasm_run`+bytecode ② 改 `.ujs` 用 **JS/wasm 宿主**重编译，而不是 `python3 -m ujs …` ③ `npm run ship:*` 里无 `python3` | **✓** ship 停拷 `ujs_full`→`engine.wasm`；A 核仅 `FORCE_WEB_BUILD` / `ujs.sh` 重建 |
| **P1 弃构造 Python** | gold / oracle / acc 也不靠 Python | 构造臂迁到 **unisacc**（或自举后的构造器）；枚举门禁仍在 | unisacc 未稳；刻意后置 |

**P0 可检里程碑（每步弃一点）**

| 里程碑 | 做成什么 | 门闩 | 弃掉什么 |
|---|---|---|---|
| **M1** | 运行时步进只跑 B-wasm | `ujs2wasm_step` 绿 · asteroid+drone **默认** B · 热路径无 `wasm_run` | 运行时对 bytecode/`wasm_run`（A）的依赖（**Asteroid+drone ✓**） |
| **M2** | 编译器本身是 wasm，在 Node/浏览器调用 | 输入 `.ujs`→`\0asm`；fold；**ship 脚本无 python3** · `./tests/ujs2wasm_compiler.sh` | 开发/CI 出货对 `python3 -m ujs` 的依赖（生成该编译器 wasm 的一次性 zig/C 尚可忍） — **✓** |
| **M3** | 编译器用 UJS 写并自举 | stage0→stage1→stage2 **字节一致** | Python 作为编译器实现语言 |

**一句话**：能扔掉出货 Python ≠ emit.py 更全，而是 **M1→M2→M3 门闩全绿**；P1 等 unisacc。

### 产物清单（理论 → 交什么）

同理论在 JS+wasm 域的实践物，**按里程碑交件**；每件都问：最终跑在 JS/wasm 上吗？

| 里程碑 | 必须落地的产物 | 不是产物（别做厚） | 状态 |
|---|---|---|---|
| **已有脚手架** | `engine.wasm`（A VM）· ship-js Host · `ujs2wasm` **Python CLI**（把 `.ujs`→用户程序 `.wasm`）· `ujs2wasm_step` 门禁 | 再扩 Python 专属出货链 | 脚手架 ✓；非自举 |
| **M1** | ① **`sim.wasm`**（直出玩法核，含 `host_*`/`run_step`）② Pages/ship **默认**加载它步进 ③ `direct_step.js` 热路径 ④ 合同：`uxe_ship_js` 验 sim.wasm | 只 fold 绿却仍默认 A | **Asteroid ✓ · drone ✓** |
| **M2** | ① **`compiler.wasm`**：Node/浏览器 `compile(ujs)→wasm` ② **`compile.mjs`** ③ **`ship:*` 无 python3 emit** ④ sim/drone 经 compiler.wasm 出货 | 继续把 emit 只留在 `.py` 当日常编译器 | **✓** `ujs2wasm_compiler.sh`（含 ship emit 无 python3）。一次性构建 compiler.wasm 仍用 zig |
| **M3** | ① **`compiler.ujs`**（编译器用 UJS 写）② **stage0/1/2**：stage2 字节 ≡ stage1 ③ 自举门禁脚本 | 「差不多能编」无字节一致 | **v11✓** sim+drone body≡stage0；**P0✓** ship 无 A 核 |
| **P1** | gold/oracle/acc 在 **unisacc**（或自举构造器） | 在 ujs 里把 Python 构造成产品 | 后置 |

**两条产物线不要混**

```
用户程序线（玩法）     .ujs  ──M1──►  sim.wasm     ──ship──►  浏览器跑步进
编译器线（工具）       （今 Python）──M2──►  compiler.wasm ──M3──►  compiler.ujs 自举
构造线（理论真源）     gold/acc     ──P1──►  unisacc
```

今日 `ujs2wasm` 只是 **用户程序线的 Python 工厂**；自举要的是 **编译器线** 上的 wasm/UJS 产物。

```
0 emit 齐 · 2a ship-js     ✓
        ↓
M1 (=#2b) 步进默认 B       **Asteroid ✓ · drone ✓**
        ↓
M1b drone 切 B             ✓（对称产物）
        ↓
M2 编译器 wasm 化          **✓**（compile+host+sim+ship emit 无 python3）
        ↓
M3 UJS 自举                **v12✓** sim+drone body≡ · core→meta · ship via core
        ↓
P0 ship 无 A 核            **✓** 停拷 ujs_full；web-build 非 ship 必经
        ↓
P1 构造迁 unisacc          可选
```

（换核模板 / 门禁契约 / Host 加厚仍按杠杆穿插；不改变上表弃 Python 顺序与产物线。）

#### M3（v11 ✓ sim+drone body≡stage0）

**协议**：stage0=`compiler.wasm`(C) 编 `compiler.ujs` → stage1 core；stage1 再编 `compiler.ujs` → stage2；**main body 字节一致**。host 拼进 `compiler_rt_stub.wasm`。

**已交付（v11）**

| 件 | 说明 |
|---|---|
| `compiler.ujs` | v10 + id>8 drain · GKEYS64 · f64 `(i-j)*TWO52/j` 无溢出 |
| `compiler.wasm` | len u32 · f64 lex ratio · MEM 128 |
| 门禁 | **sim+drone body≡stage0** · stage2≡stage1 |

**P0（✓ ship）**：Pages/ship 只依赖 `compiler_core.wasm` + `sim.wasm`；`compile.mjs` **默认** core。

#### M3 v13（✓ `[…]` + `dict.field` · fold corpus on core）

| 件 | 说明 |
|---|---|
| `compiler.ujs` | v13：`[e,…]` → MKLIST/BOX/LSET；`.` → SCONST+DOT；`CST[slot]` 区分 dict/f64-list |
| 门禁 | fold 子集经 **compiler_core**；stage2≡stage1 · sim+drone body≡stage0；**setidx_globals 默认 core** |
| 回落 stage0 | `UJS_COMPILER=wasm` 或 `UJS_REQUIRE_COMPILER_WASM=1`（body≡ 对照 / 重建 core） |
| splice | `rebuild-main.mjs` 把 stub 的 `host_set/get_global` 边界从 ng=1 补到 MAXG=64（否则 inject 只写得进 idx0） |

**下一刀**：v14 视缺口（更大子集 / IC / 错误信息）；setidx_globals 已关。

#### M3 v12（✓ core→meta · default compile=core）

| 件 | 说明 |
|---|---|
| `compiler.ujs` | v12：`OUT` 尾部名表 + `MG`；供 host 写 `.meta.json` |
| `compiler_core.wasm` | stage1 入树；`compile.mjs` **默认** |
| ship | asteroid+drone bridge=`compiler_core.wasm` |

#### M2（✓）

**已交付**

| 件 | 路径 | 说明 |
|---|---|---|
| `compiler.wasm` | `ujs/core/compiler.wasm` | C 子集编译器（`native/compiler_min.c`）→ zig 一次性构建 |
| 重建 | `./ujs/scripts/build-compiler-wasm.sh` | 需 zig；**不是**运行时依赖 |
| 宿主 | `ujs/compile.mjs` | 有 artifact → wasm（`UJS_REQUIRE_COMPILER_WASM=1` 禁止回落 Python） |
| ship | `build-{asteroid,drone}-pages.mjs` | `compile.mjs` → `sim.wasm`+meta；**无 python3 emit** |
| 门禁 | `./tests/ujs2wasm_compiler.sh` | PATH 无 python3 · fold · setidx_globals · sim · **ship builders 无 emit_wasm** |

覆盖：`let` / `while` / `if`/`else` / `===` / `return` / i64+f64（混算+比较）/ list·`len`·下标·`setidx` / dict·dot / globals inject / `host_*`·`run_step` / **Asteroid+drone ship Pages**。

**仍非 M2 范围（可后移）**

- 未烘焙 gold/catalog；无通用长 str / fn
- `web-build` / `ujs_full.wasm` 仍一次 Python+zig（引擎 Host，非玩法核 emit）
- 吐出模块与完整 `emit_wasm` 未字节同构

Python 直出管线（对照 / 全量仍用）：

```
.ujs → front + jtape → emit_wat → wat2wasm → \0asm + meta
```

**`compiler.wasm` ABI**（已钉）：

```
// JS — 产品契约
compile(src: Uint8Array|string) →
  { wasm: Uint8Array, meta: { locals:string[], globals:string[], slots:string[] } }
  | throw Error(message)

// wasm 导出
// memory; alloc(n)→ptr; compile(src_ptr, src_len)→i32 status
// status=0: out_ptr/out_len + meta_ptr/meta_len（UTF-8 JSON）
// status≠0: err_ptr/err_len
```

| # | 做什么 | 完成判据 / 测试 | 状态 |
|---|---|---|---|
| **0** | `emit_wasm`：`setidx` · `f64` · game-ready | `./tests/ujs2wasm.sh` | ✓ |
| **2a** | Asteroid ship-js（砍 `uxe_*.c` 出货） | `./tests/uxe_ship_js.sh` · `npm run ship:engine` | ✓ |
| **2b / M1** | Asteroid 步进默认 `sim.wasm`（path B） | `ujs2wasm_step` + `uxe_ship_js`（含 sim.wasm） | **✓** |
| **M1b** | drone 对称切 B（`ship/drone/sim.wasm`） | `uxe_ship_js`（drone sim.wasm + directSim） | **✓** |
| **M2** | `compiler.wasm` + 无 python ship | `ujs2wasm_compiler.sh`；ship 无 `python3` emit | **✓** |
| **M3** | `compiler.ujs` 自举 | body≡stage0 → stage2≡stage1 | **v11✓** sim+drone body≡stage0 |
| **P0** | ship 无 web-build / 无 `engine.wasm`（A） | `ujs2wasm_compiler.sh` · `ship-engine` 拒 A 核 | **✓** |
| **3** | 换核模板 | 按下表抄路径绿 | 骨架 ✓ |
| **4** | 门禁即契约 | `ujs.sh` 含 ujs2wasm；ship-js 合同；uxe 无人 | 进行中 |
| **5–6** | Host / llm | 有玩法再开 | 搁置 |

### 换核模板（#3 可抄）

```
1. 写 ujs/web/game/{name}.ujs          # 只算状态，不画
2. 写 ujs/uxe/app-{name}.js            # 只调 host_* · wasm_run/预编译
3. 写 ujs/uxe/ship/{name}-host-entry.js + build-{name}-pages.mjs
4. package.json 加 ship:{name}；scripts/ship-pages.sh 挂上
5. ./tests/uxe_ship_js.sh 扩展检查 · npm run test:uxe:all
```

对照实现：`asteroid-host-entry.js` · `build-asteroid-pages.mjs` · `drone-host-entry.js` · `build-drone-pages.mjs`。

**已交付**：Host ABI v0 · UXEP/UXIN · WebGL/WebGPU · Pages（Asteroid+无人机 ship-js）· `test:uxe:all` · H1–H3 · snapshot · **ujs2wasm 直出（setidx/f64 + game-ready）** · 双 ship-js 无 C 游戏核。

**门禁（必绿）**

| 门 | 命令 |
|---|---|
| 语言 / 构造 / **ujs2wasm** | `./tests/ujs.sh`（含 `ujs2wasm.sh` → `ujs2wasm_step.sh`） |
| **ship-js 合同** | `./tests/uxe_ship_js.sh` |
| UXE 无人 | `cd ujs && npm run test:uxe:all` |
| Pages | `npm run ship:pages` 后抽玩感 |

**工具（摘）**：`python3 -m ujs web-build` · `ujs2wasm` · `./ujs/scripts/release-artifacts.sh` · `ship-pages.sh` · `uxe-gate.sh`。  
指纹：`core/BUILD.json`。二进制走 GitHub Release，不进主树。

**代理观测**：断言写在数上。`__UXE__` · `__UXE_HOST__` · `host_debug_snapshot` / `__UXE_SNAP__` · `test:uxe:snap`。

---

# 一、思维树（markdown-tree-dag）

> 开工用。约定：`├─` 包含（树），`══>` 跨树依赖（DAG），`⟦门禁⟧` 检查点。

## T1. 系统树 —— 三层切分

任何一行代码必须能归到其中一类，归不了就是走偏。

```
UJS
│
├─ ① 经典代码（algebra）—— 绝不「训练填表」
│   ├─ 前端走查          construct/front/{lex,compile}
│   ├─ jtape / VM        jtape.py · vm.py · bc_vm.py
│   ├─ UXE Host 胶水     host-browser · packet · input · renderer-*
│   ├─ C VM               native/ujs_vm.c（路径 A）；uxe_asteroid.c 遗留不出货
│   └─ 页壳 / ship-js     demo/ · ship/{asteroid,drone} · engine.js
│
├─ ② 模型数据（constructed tables）—— 离散 key → 类
│   ├─ gold 表           catalog → gold 笛卡尔积
│   ├─ IntNet 权重       构造代数（非 SGD 出货）
│   └─ isel / enc …     ujs2wasm 决策表（WAT 形态）
│
└─ ③ 出货面（artifacts）
    ├─ core/ujs_full.wasm (+ compiler.gen.js)   路径 A
    ├─ engine.wasm                              = A 的 UXE 名
    ├─ *.wasm from ujs2wasm                     路径 B
    └─ docs/uxe/{engine.js,{game}/}             Pages
```

**DAG 边**

```
gold ══构造══> 权重 / isel·enc
② ══驱动══> ① 走查 / emit（Oracle 只回类名）
① ══产出══> ③ 出货面
A (engine) ══Host══> {app}     双 wasm，永不合包
B (ujs2wasm) ══目标══> {app}.wasm   ← 目标 #2
```

## T2. 目录树 —— 文件到职责

```
ujs/
├─ package.json          npm scripts 真源（demo / ship / test:uxe）
├─ prd.md                ★本文件
├─ README.md             开箱
├─ core/                 产品 API
│   ├─ wasm_run.js       bootRuntime · wasm_run · compile
│   ├─ ujs_full.wasm     交付 VM
│   └─ BUILD.json        指纹
├─ construct/            构造侧（非应用依赖）
│   ├─ catalog.py · gold.py · oracle.py
│   ├─ front/            lex · compile
│   ├─ ujs2wasm.py · emit_wasm.py · lower_wasm.py
│   ├─ jtape.py · vm.py · bc_*.py
│   └─ cli.py            web-build / ujs2wasm / acc / …
├─ uxe/                  UXE 嵌入
│   ├─ HOST_ABI.md       ★Host 契约真源
│   ├─ host-browser.js · packet.js · input.js · meshes.js
│   ├─ app-asteroid.js · app-drone.js
│   ├─ demo/ · ship/     源码测 · 发布对照
│   └─ archive/          下架实践车（大富翁…）
├─ native/               C：ujs_vm · uxe_asteroid（过渡）
├─ web/                  playground · Three 对照（非产品主线）
├─ scripts/              release-artifacts · ship-pages · uxe-gate
└─ archive/              旧规格 / 旧地图 / 旧路线
```

**易错 DAG**

```
web-build ══> core/ujs_full.wasm     （A-demo / FORCE_WEB_BUILD；非 ship）
ship B ══> compiler.wasm → sim.wasm  （无 engine.wasm / 无 python3）
{app} ══只调══> host_*          （不知 canvas / fetch / key）
UXEP ══一次一包══> host_gpu_submit
UXIN ══> host_input_read
tests/ujs.sh ══含══> tests/ujs2wasm.sh
```

## T3. 出货 DAG —— 两条路径

```
.ujs 源
 │
 ├──── 路径 A（共享 VM）────────────────────────────┐
 │    construct front → bytecode → zig/C → ujs_full │
 │    UXE：engine.wasm + 页内 compile 或 embed      │
 │    ⟦门禁⟧ ./tests/ujs.sh · test:uxe:all          │
 │                                                  │
 └──── 路径 B（直出程序）───────────────────────────┤
      jtape → isel/enc → WAT → wat2wasm → foo.wasm  │
      ⟦门禁⟧ ./tests/ujs2wasm.sh                    │
      目标：接成 {app}.wasm，砍 uxe_*.c              │
                                                    ▼
                              npm run ship:pages → docs/
                              ⟦门禁⟧ test:uxe:all · 人审手感
```

## T4. UXE 积木树

```
UXE Shell
├─ {app|game} 核     调度 · 组 UXEP · 读 UXIN · 只调 host_*
├─ engine.wasm       UJS VM（= ujs_full）；eng_* 待下沉（目标 #1）
├─ Host（engine.js） GPU / 输入 / 时间 / 资源 /（规划）llm
└─ Pages             docs/uxe/engine.js + docs/uxe/{name}/
```

```
demo：*.ujs → compile(ujs_full) → host_*
ship：index.html → engine.js + game.js + engine.wasm [+ app.wasm]
```

## T5. 验收 DAG

```
语言绿 ── ./tests/ujs.sh ──┬── acc / fold / icfold / difftest
                           ├── front parity · in-page wasm_run
                           └── ujs2wasm（direct · fold · neg · tinyvm?）

UXE 绿 ── test:uxe:all ────┬── CDP 探针 · snapshot 数
                           └── 禁代理猜 UI

出货 ──── ship:pages ─────── docs/ 与 BUILD 指纹对齐 Release
```

## T6. 三条红线

```
① 双 wasm 永不合包                    → {app} ≠ engine
② 核只调 host_*                       → 无 canvas / key / fetch
③ 出货权重构造、不训练填表             → --drive 默认 built；勿擅跑 train
```

---

# 二、记忆宫殿（mermaid-flowchart-memory-palace）

> 记全局用。按 **门厅 → 书库 → 工坊 → 熔炉 → 展厅 → 哨所 → 装船口** 走一圈。

## 宫殿全景

```mermaid
flowchart TD
    subgraph R1["门厅 · 命题"]
        A1["UJS = 闭合子集 + 构造推理 + wasm_run + UXE"]
        A2["三层：core · UXE · construct"]
        A3["两条出货：A 共享 VM · B ujs2wasm 直出"]
        A1 --> A2 --> A3
    end

    subgraph R2["书库 · 语言与契约"]
        B1["闭合：表内全功能 / 表外永久拒绝"]
        B2["HOST_ABI：host_* · UXEP · UXIN"]
        B3["核不知宿主；GPU/密钥停在 Host"]
        B1 --> B2 --> B3
    end

    subgraph R3["工坊 · 构造"]
        C1["catalog → gold 笛卡尔积"]
        C2["Oracle：ask → 类名；不读 logits"]
        C3["web-build → core/ · ujs2wasm → \\0asm"]
        C1 --> C2 --> C3
    end

    subgraph R4["熔炉 · UXE 运行时"]
        D1["{app} 核 ══host_*══> Host"]
        D2["engine.wasm = ujs_full"]
        D3["一次一包 UXEP · UXIN 读输入"]
        D1 --> D2 --> D3
    end

    R1 --> R2 --> R3 --> R4

    classDef room fill:#1f2937,stroke:#6366f1,color:#e5e7eb
    class A1,A2,A3,B1,B2,B3,C1,C2,C3,D1,D2,D3 room
```

```mermaid
flowchart TD
    subgraph R5["展厅 · Pages"]
        E1["docs/uxe/engine.js 共享"]
        E2["docs/uxe/asteroid · drone"]
        E3["索引 docs/index.html"]
        E1 --> E2 --> E3
    end

    subgraph R6["哨所 · 观测与门禁"]
        F1["__UXE_SNAP__ 一帧 JSON"]
        F2["test:uxe:all · ujs.sh"]
        F3["截图非默认；人只审手感"]
        F1 --> F2 --> F3
    end

    subgraph R7["装船口 · 发版"]
        G1["release-artifacts → GitHub Release"]
        G2["BUILD.json 指纹对齐"]
        G3["ship:pages → docs/"]
        G1 --> G2 --> G3
    end

    R5 --> R6 --> R7

    classDef room fill:#1f2937,stroke:#f59e0b,color:#e5e7eb
    class E1,E2,E3,F1,F2,F3,G1,G2,G3 room
```

## 管线真图

```mermaid
flowchart LR
    SRC[".ujs"] --> FRONT["front lex/compile"]
    FRONT --> JT["jtape"]
    JT --> VM["路径 A：C VM → ujs_full / engine"]
    JT --> DIR["路径 B：isel/enc → WAT → wasm"]

    VM --> CORE["core/ + engine.wasm"]
    DIR --> APP["{app}.wasm 目标"]

    CORE --> HOST["Host engine.js"]
    APP -.目标.-> HOST
    GAME["app-*.js / 过渡 C 核"] --> HOST
    HOST --> GPU["WebGL | WebGPU"]
    HOST --> IN["UXIN"]
    GAME --> PK["UXEP"]
    PK --> HOST

    HOST --> PAGES["docs/uxe/{game}"]
    SNAP["__UXE_SNAP__"] -.门禁.-> GATE["test:uxe:all"]
    PAGES --> GATE

    classDef net fill:#312e81,stroke:#818cf8,color:#e0e7ff
    classDef cls fill:#064e3b,stroke:#34d399,color:#d1fae5
    class FRONT,JT,DIR net
    class VM,CORE,HOST,GPU,GATE cls
```

**读图**：紫 = 构造/决策表路径；绿 = 经典运行时与 Host。`{app}` 接路径 B 是当前主杠杆。

## 门禁状态机

```mermaid
stateDiagram-v2
    [*] --> Lang
    Lang: 语言/构造 ujs.sh
    Wasm: ujs2wasm 套件
    Uxe: test:uxe:all
    Ship: ship:pages
    Feel: 人审手感

    Lang --> Wasm: ujs2wasm 语料绿
    Wasm --> Uxe: 改 UXE / Host / 输入
    Uxe --> Ship: snapshot 断言过
    Ship --> Feel: docs 可玩
    Feel --> [*]

    Lang --> Lang: 卡住改语言/gold/编码，勿训练填表
```

## 目标推进图

```mermaid
flowchart TD
    U2W["ujs2wasm 语料"] --> E0["0 setidx+f64+game-ready ✓"]
    E0 --> SJS["2a ship-js 砍 C ✓"]
    SJS --> B2["2b sim→ujs2wasm"]
    B2 --> TPL["3 换核模板"]
    TPL --> GATE["4 门禁即契约"]
    GATE --> HOST["5 Host · 6 llm"]

    classDef now fill:#064e3b,stroke:#34d399,color:#d1fae5
    classDef next fill:#1e3a5f,stroke:#60a5fa,color:#e0f2fe
    class E0,SJS now
    class B2,TPL,GATE next
```

## 随身卡

| 记 | 是什么 |
|---|---|
| **A / B** | 共享 VM（engine） / ujs2wasm 直出 |
| **双 wasm** | `{app}` ≠ `engine`，永不合包 |
| **host_*** | 核唯一出入口；GPU/key 停 Host |
| **UXEP / UXIN** | 一次一包提交 / 输入快照 |
| **ujs.sh · test:uxe:all** | 语言门 · UXE 无人门 |
| **BUILD.json** | 与 Release 对指纹 |
| **构造不训练** | 出货权重由 gold 代数生成 |
| **1→4** | emit → ship-js → ujs2wasm 步进 → 模板/门禁 |


## 走出宫殿前默念

```mermaid
flowchart LR
    L1["① 不合包"] --> R1["app ≠ engine"]
    L2["② 核只调 host_*"] --> R2["无 canvas/key"]
    L3["③ 构造出货"] --> R3["勿训练填表"]

    classDef red fill:#7f1d1d,stroke:#f87171,color:#fee2e2
    class L1,L2,L3 red
```

---

## 文档角色（收束后）

| 文档 | 角色 |
|---|---|
| **本文件** | 规格 · 目标 · 思维树 · 记忆宫殿 |
| [`uxe/HOST_ABI.md`](uxe/HOST_ABI.md) | Host 符号 / UXEP·UXIN / LLM 规划 |
| [`README.md`](README.md) | 开箱与分发 |
| 各层薄 README | 目录文件表（`core/` · `construct/` · `native/` · `web/` · `uxe/`） |
| [`archive/`](archive/) | 历史；不当真源 |

升版：破坏性语言/API → bump；旧活规格摘入 `archive/`。

# Paper A 材料：0.0.43 晚绑定 + 流推导措辞边界（非 A seal-before）

- **拍点：** 2026-10-14（UTC）；**机核 tip** `acef20da93d2a4783df24d3ffeff2b39d7fd783c`
- **性质：** 研究笔记；梳 0.0.43 重构进展与 Paper A 侧可用措辞；**不发布、不投稿、不改正文**
- **外部身份：** 最新公开/Latest 封存候选仍为 **v0.0.42**（`release/candidate.json`）；0.0.43 开发树未签、未公开（见 `plans/v0.0.43.md`）

---

## 主张 / 证据 / 不升格（三联）

| 维 | 内容 |
|---|---|
| **主张（A 根，不变）** | 基于神经网络（**构造**网络与权重，非训练）的编译器；声明域上 **T1**（逐键精确）；实证载体为 unisacc（POSIX C99），在编译器外调用已出货/封存身份做表与门禁，不把训练写进产品路径。 |
| **证据（本拍可引用）** | tip `acef20da`：晚绑定 **2c1** 库数据 lower 模型（`librarydataspan` / `librarydataprov` TSV 族 + `unisa/librarydatalower.py`）；仓外/模型闸 `exec-librarydata-model`（39 例）与收据 [`research/c43-librarydata-lower-receipt.md`](../c43-librarydata-lower-receipt.md)。更早 **2a 合同 + 2b 普通函数** 路径已在树内（`exec/lower/libraryfn-*`、`research/c43-libraryfn-2b-receipt.md` 等）。产品架构口径：**流推导 + 显式字节流 + 执行器+有限表**（`prd.md` 流推导小节、`spec.md` FLOW-1）。 |
| **不升格** | 不把晚绑定/流接口迁移记成 A **seal-before**；不把「路径未接齐/拧不开」读成构造+T1 被否证；不把 0.0.43 工程进度写成已封存实证或投稿键数义务；**8509/8769、§8.1#3 同身份矩阵、Ο1|Ο2** 仍按既有 gap-nail 开放。更深「整管道流方法」归意向 **Paper C**，非 A 封口闸。 |

---

## 1. 晚绑定：已落地 vs 仍观察/未接线

### 1.1 时间线与 tip 锚点

| 切口 | 状态（tip `acef20da`） | 库内锚点 |
|---|---|---|
| 首刀 + UNISYM1 侧流合同 | 已在树（方案 A 前置验域与晚绑定授权见 `prd.md`） | `research/c43-late-binding-first-cut.md`、`research/c43-late-binding-step0.md` |
| **2a** 合同/观察对齐 | 在树；E3 侧合同流推进（parse2 library contract） | `research/c43-step3-parse2-contract-step0.md`、`exec/parse2/libraryfunctionimports.md` |
| **2b** 普通函数 `kind=0,callkind=0` | **已验收切片**（片段级无地址 + relocate 绑定）；非整份 lower 无地址 | `exec/lower/libraryfn-*`、`research/c43-libraryfn-2b-receipt.md`、`exec/c/librarylatebind.h` |
| **2c1** 库**数据** `kind=1` | **模型先行**：span + readonly provenance 有限表进 lower 模型；**commit `acef20da`** | `exec/lower/librarydataspan-*`、`exec/lower/librarydataprov-*`、`unisa/librarydatalower.py`、`research/c43-late-binding-cut2c1-step0.md`、`research/c43-librarydata-lower-receipt.md` |
| **2c** 其余（typed/variadic/callback/运行桥/Windows） | **只是设计**或旧路径仍带地址 | `research/c43-late-binding-cut2c-step0.md` |

### 1.2 `acef20da` 具体落了什么（模型证据，非整产品绿）

- **核心模型产物：** `librarydataspan`（数据引用 / UNILOW1 role=0 span 标定）与 `librarydataprov`（readonly 写入溯源与 fusion 修复见证）；构造入口 `unisa/librarydatalower.py`。
- **验收形状（收据原文）：** 27+ 夹具 tbl/net 与独立参考收拒一致；readonly-write 等在未绑定 lower 或绑定层具名拒绝；**未接库 API 注入后的执行返回证明**、**未开整闸**、动态 C 运行器仅为观察工具（见收据 § 首段与 § 入仓检查）。
- **合同约束（2c1 step0）：** 宿主地址**不得进入核心模型**；provider 地址只在 **relocate** 层填入；`us_library_data_lower_unbound` 与 2b 同：**先 relocate 再读 lower 收据**，不是惰性执行接口。
- **仍观察 / 未当作 Paper A 封存实证：**
  - 整包 0.0.43 产品身份、CI 候选格仍绑 **0.0.42** 字节（`plans/v0.0.43.md`）。
  - mapping/argv/process_slots、exit/mmap、callable 服务、Windows L-4 等仍在 `libunisacc.c` 旧接线（2c step0 表）。
  - 库 API `us_declare_injected_data` / `us_register_data_provider` 等：**接口草案**，fresh 黑盒与全库回归未在本收据内闭。

### 1.3 Paper A 可说的 vs 须写清的债

**现在可以说（与 §4 字节流阶段叙事一致，不新造根主张）：**

- lowering 侧正在把**外部库合同**从「宿主地址立即数」拆成「**显式合同流 + 模型标定片段 + 绑定层填址**」；2b/2c1 是 lowering/库边界上的**构造网络切片**证据，不是第二套编译理论。
- **拒绝**仍按阶段归属：未覆盖键拒绝、readonly/extent/type 等具名诊断，与 A 的偏函数/拒绝边界同族（不升成新产品 UI 定理）。

**现在不能说（或必须加「开发树/模型闸 only」）：**

- 「0.0.43 已出货」「全 lower 无宿主地址」「晚绑定已产品绿」「库数据注入后运行时已证」。
- 把 2c1 模型闸通过等同于 release gate 全绿或 Table 4/5 同身份重测完成。

**拧不开 ≠ 否证：** 某 C 路径、某目标或某 API 尚未接线，只记 **实现债**；不反向削弱「表构造 + T1 + 已封存身份上的 unisacc 实证」根主张（与 `paper-a-derive-purify-product-maturity-not-theory-*` 同边界）。

---

## 2. 流推导（政委定调）与 Paper A 措辞边界

### 2.1 产品/规格侧定调（非新 Paper A 根主张）

权威表述已写入：

- `prd.md`「流推导与模块复用口径」「流推导的默认可复用切面」
- `spec.md` **FLOW-1**（流推导可复用模式）

要点（Paper A 材料用**复述**，不扩成定理）：

- **IO = 显式字节流**；阶段间通道形态可类比 iostream，**不要求 C++**。
- **函数调用 = 调用模型做流式推导**（语义在构造网络+执行器，不在宿主隐式状态）。
- **可复用 = 共用执行器 + 有限表 + 显式流接口**；**拒绝按阶段归属**，不合并成隐式「大阶段」。
- **外部合同**（符号、ABI 等）作**显式输入**；宿主地址/环境只在**适配与绑定层**。
- 设计方向**不宣称**旧路径已全部迁移，**不自动摘钩**历史测试结果（FLOW-1 末句）。

### 2.2 Paper A 正文：现在写什么、什么留给 C / 工程

| 层级 | Paper A（基石 A） | 不放进 A seal-before / 另纸 |
|---|---|---|
| 已有 §4.2 七阶段字节流 + 通用执行器 + 构造表 | 继续作为**实例叙事**；表 1 短名 pp/lex/parse/…/lower/enc | 不要求 Fig.2 管线图完稿才成立（已有 derive-purify 登记） |
| FLOW-1 / 流推导口号 | 可作 **§4 实现观感的单向脚注**（「阶段接口为字节流；拒绝分阶段」），与 README 七阶段描述一致 | **「种子机→脱离→≥3 场景」完整管道方法** -> 意向 **Paper C**（`paper-c-intent.md`、`paper-a-derive-purify-pipeline-method-paper-c-*`） |
| 中段流身份 / P3 提取 / modelstreamextract | 工程与 exec 迁移材料（`research/c43-middle-*`、`tests/modelstreamextract.cx` 在 ungated） | 不得升成 T1 或「同输入同模型」定理；须完整流+合同实证 |
| 0.0.43 晚绑定 | 开发树进展；引用 **模型收据 + commit SHA**，不绑新键数 | 不等同 §8.1 矩阵/8509-8769 封口 |

**优先序（本笔记立场）：** 摘要/构造/T1/unisacc 实证 **高于** 0.0.43 未封存工程状态；写 Latest 时继续区分 **冻结投稿身份** vs **开发 tip**（既有 §8.1 Latest drift 笔记），不把 tip 上的 2c1 自动写进表 2/表 4 数字位。

### 2.3 与晚绑定的接句（供日后正文微调时三语同步）

若将来要在 §4 加一句，建议形态（**本 PR 不改正文**）：

- 中文：外部库符号与数据对象通过**显式合同字节流**进入管线；构造网络标定 lowering 片段，**宿主解析与地址填充**留在绑定适配层，不进入核心阈值网络输入。
- 对应英文/arXiv 须与 `research/unisacc-paper.md`、`research/unisacc-paper.en.md`、`research/arxiv-paper-a/` **同句同步**；无明确不一致前，**维持 notes-only**。

---

## 3. seal-before 与 DAG 父节点

- **父节点仍为 A**；本笔记是 0.0.43 产品 refactor 的 **论文侧旁支地图**，类比 [`paper-a-gate722-suite-to-stage-model-map-20261013.md`](paper-a-gate722-suite-to-stage-model-map-20261013.md)（release 结构映射，非 seal）。
- **不得**因晚绑定或流推导定调而新增投稿闸门（与大量 `paper-a-derive-purify-*-not-a-gate-*` 一致）。
- **仍 seal-blocking（未本笔记重钉）：** 测量身份 8509/8769；§8.1#3 同身份平台矩阵；Ο1|Ο2 决策卡——见 `research/paper-notes-20261009.md` 封口索引与 2026-10-13 gap-nail tip-refresh。

---

## 4. 机核路径清单（tip `acef20da`）

| 类别 | 路径 |
|---|---|
| 提交 | `acef20da93d2a4783df24d3ffeff2b39d7fd783c` — `fix: model late-bound library data spans and readonly provenance` |
| 2c1 收据 | `research/c43-librarydata-lower-receipt.md` |
| 2c1 设计 step0 | `research/c43-late-binding-cut2c1-step0.md` |
| 2b 收据 | `research/c43-libraryfn-2b-receipt.md` |
| 2c 剩余 step0 | `research/c43-late-binding-cut2c-step0.md` |
| lower 表 | `exec/lower/librarydataspan-*.tsv`、`exec/lower/librarydataprov-*.tsv`、`exec/lower/libraryfn-*.tsv` |
| 闸 | `tests/librarydatacheck.cx`、`tests/gate.sh`（含 `exec-librarydata-model`） |
| 流推导口径 | `prd.md`（流推导两小节）、`spec.md` FLOW-1 |
| 0.0.43 计划 | `plans/v0.0.43.md` |

---

## 5. 本笔记动作

- 只增本文件 + `research/paper-notes-20261009.md` 登记一行。
- **未改** `research/unisacc-paper.md`、`research/unisacc-paper.en.md`、`research/arxiv-paper-a/` 正文（无发现「训练产品化」或「0.0.43 已封存实证」类低风险必修补丁）。

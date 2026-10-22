# CSIH CX 模块化短页（2145）— CXIF 共构设计

- **拍点：** 2026-10-11 ≈21:45 Asia/Singapore（UTC+8）；政委加码 ≈22:00「先协作构思再写码」
- **仓 tip（只读核对 / before）：** `2779b4bde6a8e5200eaa6e9c6c8f6e7acde08b55`
- **依据：** `docs/cx-spec-draft.md` §2–3 / §3.1 / M 片；`apps/csih/docs/module-decision.md`；政委「把 rust 优点用在 unisacc(posix c99)」；加码五轴共构
- **状态：** **设计短页 + 可验收旁注授刀**（材料 only；**不**接编译器实现、**不**改产品路径、**不**挡 K6-9 / **不**抢 K6-10）；续页见 checker-reject-N1N3
- **授刀名：** `cxif-cols-boundary-旁注-2145`

---

## 0. 本刀钉死（并授）

| 钉 | 值 |
|---|---|
| **文件域写** | `research/csih-cx-modularity-2145.md`；`research/cxif-cols-boundary/**`；`archive/plans/v0.0.41.md`（仅追加「CXIF 旁注 / 41↔42 分账」小节，不改 K6 主表） |
| **文件域只读** | `docs/cx-spec-draft.md`；`apps/csih/cols.cx`；`apps/csih/csih_cols.h`；`apps/csih/docs/module-decision.md`；`include/cx.h`；现 tip |
| **禁写** | `src/`（含 `version.h`）、`exec/`、`kernel/`、`weights/`、`release/`、`seed/`、`tests/gate.sh`、既有 K6-1..9 闭包材料、tag、Latest、签、Draft、bump |
| **预算** | 墙钟 **≤30 min**（设计短页 + 样例材料 + 计划旁注）；超则首红停或拆刀 |
| **首红停** | 越文件域 / 误动 K6-9 主叶 / 把样例挂进产品 gate / 口称「已实现 CXIF」→ **立即停** |
| **产出** | 本短页（五轴冻结）+ cols 公开入口×消费者×反例材料树 + 41/42 分账旁注 |
| **不做** | 编译器 M0/E2 接线；改 `csih.sh`；删 `#include` 全树；所有权/借用检查器；六目标重测；bump / Latest / 签 |

---

## 1. 目标一句话

在 **不增加 C99 语法糖** 的前提下，用 **CXIF 接口工件** 表达 Rust 式「公开集合 / 零泄漏 / 模块边界」；本刀只交 **可对审的设计 + cols 样例旁注**，证明形状可验收；工具链真接与门闩归账另版。

---

## 2. 五轴共构（设计冻结；实现另授）

### 2.1 语法形态

| 项 | 冻结 |
|---|---|
| 源码表层 | **仍是 C99**。不增 `mod`/`use`/`export`/`own` 等关键字。 |
| 身份 / 依赖 | 可选 `#pragma unisa cx 1` / `module` / `use`（见 cx-spec §2）；与**旁置包清单**互校；脚本入口可无 pragma。 |
| 公开集合 | **清单 / 公开头投影选出已有 C 声明**；不新造 export 语法。cols 先例：`csih_cols.h` 头注释列出 `csih_next_cp, csih_cp_cols`。 |
| CXIF 本体 | **构建产物**，不是人手维护的第二套源码语言。v0 样例可用可读 JSON/文本 stub 代替二进制，schema 键必须稳定。 |
| 与现况关系 | csih 现状 `-include` 公开头 = **过渡投影**；CXIF 目标是消费者 **装接口记录、不拼接提供者 token**（cx-spec §3）。本刀样例同时展示「头投影」与「CXIF stub」对照，不宣称 `-include` 已等于 CXIF。 |

**CXIF stub 最小键（v0；本刀冻结名称，值可为占位）：**

```text
cxif_version, module, target, abi,
public[] { name, kind, signature_norm, decl_id },
opaque_types[],            # 公开不完整类型（本样例可空）
effects[] (optional v0),   # 契约表；本刀不填满
source_hash, generator
```

### 2.2 公开集合检查

| 检查 | 预期 |
|---|---|
| P1 集合一致 | 提供者外部链接名 ∩ 清单公开集 = 公开集；CXIF `public[]` 与公开头声明 **同名同签名** |
| P2 消费者可见 | 消费者只装 CXIF / 公开头；**不得**看见提供者源内未公开符号 |
| P3 无宏出口 | 公开投影禁止导出 `#define`；宏不得绕过集合 |
| P4 签名不泄私 | 公开函数参数/返回类型不得引用未公开完整类型（须不透明句柄或同模块公开类型） |
| P5 身份稳定 | `decl_id` 不随私有声明增删重编号（cx-spec §3.1） |

**cols 钉死公开集（本刀样例唯一合法出口）：** `csih_next_cp`、`csih_cp_cols`。

### 2.3 零泄漏反例（材料必须红）

故意违规；**设计预期 = 拒**。本刀只落源码/说明，不要求今日工具链已拒（验收口径写「材料可审；执行器接线后同拒」）。

| ID | 反例 | 预期诊断要点 |
|---|---|---|
| N1 | 消费者 `#include "cols.cx"` / 拼接提供者源 | 严格 `.cx`：文本 include 硬错 |
| N2 | 消费者调用未公开 `static`/内部辅助（若提供者故意留外部链接私有名） | 未声明标识符 / 不在公开集 |
| N3 | 公开头或 CXIF 混入函数体 / 私有类型完整定义 | 投影合法性失败 |
| N4 | 公开签名参数用仅提供者可见的完整 struct | 签名泄漏私有类型 |
| N5 | 两模块重复公开同一 C 名 | 冲突 |
| N6 | 模块图环依赖 | 拒环 |

本刀材料树至少交 **N1、N2、N3** 三份可读反例；N4–N6 表列保留，下刀可补。

### 2.4 与 `.cx` 编译管线接点

```text
包清单 / 模块图
    → M0：解析依赖、公开集、生成/校验 CXIF（接口键）
    → E2：.cx 拒 #include；保留 pragma / 宏位置
    → 消费者符号环境 ← 装 CXIF（不重放提供者 token）
    → E1/E3：既有 C99 路径；安全检查另片（P/B/F…）
    → tape（.c 路径字节不变）
```

| 接点 | 本刀 | 另授 |
|---|---|---|
| M0 生成 CXIF | 手写 stub + 与 `csih_cols.h` 对照表 | 解析器导出入口 |
| 消费者装载 | 文档规定「只见公开集」；样例 consumer 只调二入口 | E2 前装环境 |
| 旧 `.cx` `-run` | **不混充**已检查模块（cx-spec §1）；csih 仍 build-then-run | run 模式 `.cx` 缺陷另案 |
| `.c` 六目标 | **不得**因本旁注改变既有字节 | 回归闸保持 |

### 2.5 41 样例 vs 42 门闩分账

| 账本 | 归属 | 本刀 |
|---|---|---|
| **0.0.41（K6 主叶）** | `unisa/` → C 参考对拍；K6-9 等真审主叶 | **旁注并行、不挡、不改 gate 身份** |
| **41 样例旁注** | CXIF / `.cx` 模块边界 **材料**（本刀） | research 可验收；**不**进 `tests/gate.sh` 产品套件 |
| **0.0.42（K7 + 无 py/cc/sh 全链）** | 清点 + 全链验收 | **CXIF 执行器接线 / 模块门闩** 候选挂 **42+**，须另授；**不得**把本旁注样例静默升成 41 产品闸 |
| **Paper A** | 构造 + T1 | 已有 derive-purify：cx-lab/CXIF 完稿 **≠ A 闸**（`research/notes/paper-a-derive-purify-cx-lab-include-free-not-a-gate-20261011-0139.md`）；本刀不重切 |

**分账句（可摘进计划旁注）：**  
> CXIF cols 边界样例 = **41 可验收旁注材料**；**不是** 0.0.41 K6 产品义务，**不是** Softguess/T1/矩阵闸；工具链门闩与全链验收候选记 **42+ 另授**。

---

## 3. 样例树布局（本刀材料）

```text
research/cxif-cols-boundary/
  README.md                 # 如何审、何谓绿/红（材料口径）
  provider/
    cols.public.set         # 公开集名单
    cols.cxif.json          # CXIF stub（与公开集一致）
    cols.public.h           # 与 apps/csih/csih_cols.h 同形对照（复制声明，不改 apps）
  consumer/
    use_cols_ok.c           # 只调公开二入口（正例）
  neg/
    N1_include_provider.c   # 文本 include 提供者
    N2_call_private.c       # 调用未公开符号
    N3_body_in_projection.h # 投影藏函数体
```

正例/反例 **不**要求本刀用 unisacc 跑通检查器；验收 = 人/审阅者能按 §2.2–2.3 对照材料判「应当过/应当拒」。

---

## 4. 与 csih 现状的关系（不破坏）

- `apps/csih/cols.cx` + `csih_cols.h` + `-include` 注入 **保持不动**。
- 本刀材料在 `research/cxif-cols-boundary/`，是 **对照实验室**，不是替换 csih 构建。
- `module-decision.md`「不采用 `.cx` 后缀作模块标记（run 丢定义）」仍然有效；CXIF 单元模式落地前，产品侧继续 build-then-run。

---

## 5. 实现序（设计后；未授执行）

1. **已授（本刀）：** 设计短页 + 样例材料 + 计划旁注。  
2. **未授：** stub→真 CXIF 导出；M0 接线；把 N1–N3 挂进检查器；csih 迁离 `-include`；所有权片 P。  
3. **禁并行抢主叶：** 任何实现刀不得改 K6-9 / gate 主路径，除非政委另授且显式分账。

---

## 6. 卡点（写页时）

| 卡点 | 态 |
|---|---|
| K6-9 真审主叶 | **勿挡**；本刀 research-only |
| CXIF schema 未进 `spec.md` | 本页 v0 stub 键冻结；升 spec 另授 |
| 检查器尚不能拒 N1–N3 | 材料口径；执行器接线后复跑 |
| `.cx` run 模式缺陷 | 已知；本刀不修 |
| 三窗共构 | 本短页为共构底稿；续页 [`csih-cx-modularity-2145-checker-reject-N1N3.md`](csih-cx-modularity-2145-checker-reject-N1N3.md) 钉「如何实现检查器拒 N1–N3」（语法/接线/门闩 vs 样例分账）；不另开第二份打架权威 |

---

## 7. 禁项自检

- 未 bump、未动 Latest、未签、未 Draft、未改 `version.h`
- 未改 `apps/csih/**` 产品源、未改 `tests/gate.sh`、未动 K6-1..9
- 未称「CXIF 已实现」或「模块安全已达标」
- 未把样例升格为 Paper A / Softguess / 41 产品闸

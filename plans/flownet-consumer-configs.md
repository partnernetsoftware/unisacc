# flownet 的消费者配置：两类需求下的「流 / 网」梳理

> 2026-10-23 · 起因：主人指出 unisacc（论文 a）与 ujs（论文 b）是两类需求，什么流什么网必须认真梳理
> 依据：`prd.md` §3.0a/3.0b/**§3.0c 关系总表**、`exec/pipeline/image-stages.tsv`、`unisa/gold.py`、`research/ujs-paper.md`、`research/m4-flow-net-contract-draft.md`

## 1. flow 就是字节管道阶段——设计上是同一个东西

`flownet/contract.cx` 里 `flow` 行的定义是 `flow FLOW NET...`：**一条流，加上决定它的网（wired）**。流就是字节管道阶段，网挂在流上。三处配置只是名单不同：

| 需求 | flow 名单 | 挂网 |
|---|---|---|
| **unisacc（论文 a）** | 7 条：`e2 / e1 / e3 / e4 / prune / lower / elf`（`exec/pipeline/image-stages.tsv`） | e2→pp；e1→lex；e3→parse,type,scope,irsel,tyinfo,pfconv,binsel,prec；e4→opinfo,peep；prune→无；lower→abi,regmap,reloc（enc ◐）；elf→enc |
| **ujs / m4（论文 b）** | 4 条：`lex / front / select / encode`（`research/m4-flow-net-contract-draft.md` §2） | lex→lex；front→parse（type/scope/shape 侧信道）；select→isel；encode→enc |
| **tdescprobe（闸）** | 3 条：`declare / context / size` | declare→adj；context→ap；size→sz |

**m4 的「4 条流」与「4 张真决策网」是数字巧合**：`m4/contract.py` 的 `FLOWS` 是 4 条**字节段**（lex/front/select/encode），而"决策网恰好 4 张（lex/parse/isel/enc）"是另一个维度的实测结论。合同 §2 白纸黑字："m4 的 4 不是「缩小版 7」；是 wasm 向重新切的字节合同，仅**角色**与七段对照。"

**红线（prd.md:174）的原意**是「7 流（字节管道）≠ 18 网（ask 事实表）」——指流与网不可互换，**不是**说 flow 与字节管道是两个东西。此前把它读成后者是误读。

## 2. 两类需求的配置对照

| 项 | unisacc（论文 a） | ujs（论文 b，= m4 方法臂） |
|---|---|---|
| 字节管道流 | **7**：e2 → e1 → e3 → e4 → prune → lower → elf | **4 段**（研究口径）；现状出货是 1 条 M3 主脊 |
| 网 | **18**，合计 **8770 键** | **10**，合计 **3677 键** |
| 最大网 | type 4275、peep 1632、enc/abi/combo 各 522 | type 1863、ic 975 |
| 产出 | 原生（六平台） | `\0asm` |
| 流×网关系 | **一网多流**：enc 在 elf 是 ✓、在 lower 是 ◐；e3 内嵌 8 张网 | **一网一流**：每网只属一段 |
| 对照臂 | isel / combo 有金表但主脊不问（⊘） | 无此概念 |
| flownet 现状 | ❌ 模型不匹配（见 §3） | ✅ **已实证**：33/33 逐字节 + trace 相同 |
| 键空间上限 | 18 < `FNC_MAX` 32 ✅ | 10 < 32 ✅ |

## 3. 硬发现：flownet 的 contract 模型与 unisacc 的矩阵**不同构**

PRD §3.0c 的关系总表是四态（✓ 主消费 / ◐ 次要旁路 / ⊘ 对照臂 / · 不相关），而 `flownet/contract.cx` 现在只有：

- `cflowof[net]` —— **单值**，「该网属于哪一条流」，且 `net named twice` 会直接报错
- 角色三态：`1 wired / 2 reserved / 3 observed`

对照 PRD 矩阵的实际形态：

| PRD 需要 | flownet 现在 | 差距 |
|---|---|---|
| 一网可被多条流查询（lower→enc ◐ + elf→enc ✓） | 一网只能挂一条 flow，重复即报错 | **必须支持多流归属**，每条带角色 |
| 四态 ✓/◐/⊘/· | 三态 wired/reserved/observed | `◐` 与 `⊘` 无对应；`observed` 是「问了但结果被丢」，不等于「旁路」或「对照」 |
| 「·不相关」是可断言的**零 ask** | 无表达（不声明≈不相关） | 需要显式断言：该 (phase,net) 的 hits 恒 0 |
| 对照臂 must-not-be-asked | `reserved` 近义但不完全等价 | isel/combo 是「有金表、构造存在、主脊不问」，需要独立角色 |
| 逐格矩阵（7×18） | 由 flow 行隐式推导 | 必须**照抄**矩阵，不得由网名单猜 |

**结论：论文 b 的复用是既成事实；论文 a 的复用不是「改个配置」，而是先扩 contract 语义——一网多流 + 四态角色 + 零 ask 断言。**

## 4. unisacc 的流×网矩阵（照抄 §3.0c，供 contract.tsv 逐行落）

| 流 | ✓ 主消费 | ◐ 次要 | ⊘ 对照（零 ask） |
|---|---|---|---|
| e2 | pp | — | isel, combo |
| e1 | lex | — | isel, combo |
| e3 | parse, type, scope, irsel, tyinfo, pfconv, binsel, prec | — | isel, combo |
| e4 | opinfo, peep | — | isel, combo |
| prune | （无金网） | — | isel, combo |
| lower | abi, regmap, reloc | enc | isel, combo |
| elf | enc | — | isel, combo |

**注意 prune**：不消费任何金网——它必须作为一条**没有任何 net 行的 phase**存在于 contract 中，否则 7 个阶段会被压成 6 个。

## 5. 复用前置条件（给 W6 之后新增 W7）

| 编号 | 要求 | 判据 |
|---|---|---|
| C1 | contract 支持**一网多流**，每条带角色（primary / secondary / control-only / unrelated） | 同一网在两条 flow 行出现不再报错，且两条都记入 `cflowof` 集合 |
| C2 | 四态角色可断言 | ✓ 需 hits>0；◐ 允许 hits>0 但不作为主账；⊘ 必须 hits==0；· 必须 hits==0 |
| C3 | 无网阶段合法 | `prune` 只有 phase 行、无 net 行，`fn_check_contract` 通过 |
| C4 | 矩阵照抄可机检 | `contract.tsv` 的每个 ✓/◐/⊘ 都能追溯到 `prd.md §3.0c` 的一格；缺格即红 |
| C5 | dry-run 实证 | 18 网 / 8770 键 / 7 phase：`fn_build` + `fn_acc` 全 18 网 exact，0 gold 回退 |
| C6 | 与 ujs 不互相退化 | ujs 10 网 33/33 逐字节仍绿（回归） |

**顺序**：C1 → C2 → C3 → C4（只落表、不接编译器）→ C5（dry-run）→ 最后才谈接产品流。

## 6. 为什么这份梳理必须先于写代码

m4 出生时只有一个消费方、一网一流、四段，所以 contract 的单值模型够用。unisacc 的 18 网一开始就是**多流共享 + 对照臂 + 无网阶段**三种形态叠加。若不先把 contract 的语义扩到能表达 PRD §3.0c，任何「18 网复用」都会以**静默失真**的方式通过：网被挂到错误的流上、◐ 被当 ✓ 记账、⊘ 的对照臂被当成真消费——而这三种错都不会让编译失败，只会让结论错。

# Paper A 衍生净化：有限性 / 有限控制表的系统构造

- **拍点：** 2026-10-10 ~16:29 Asia/Shanghai（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `c3c361954f9c221b1c959822db6e9748f3bb13b7`
- **父节点：** **意向 Paper E**（有限控制表的系统构造；[`../paper-e-intent.md`](../paper-e-intent.md)；不挡 A 投稿）→ 最终回 **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；实证载体 unisacc POSIX C99 跨架构）
- **状态：** **note only**（登记未来独立「有限性／哪些阶段天然是有限控制表 + 系统造表方法」命题；**不起稿正文**；**不改** A 根主张、键数身份、product/kernel/weights/facts；**不重写** [`../paper-e-intent.md`](../paper-e-intent.md)）
- **Softguess：** tip 四文件 **NONE×4**（本拍机扫：`research/unisacc-paper.md` / `unisacc-paper.en.md` / `arxiv-paper-a/main.tex` / `abstract.txt` 均 0 hit）
- **测量身份：** 8509/8769 **仍开** — 引用既有钉 [`paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)；本拍**不重钉、不叠**路线Ι
- **已有意向（本拍不重写）：**
  - [`../paper-e-intent.md`](../paper-e-intent.md)（E：从文法、类型属性规则、显式上下文与状态不变量系统产生控制表；首实验声明符簇；网络=表单列≠语言语义证明）
  - A 正文已有「Relationship to Paper E」分工句（EN §2 附近）：A 用给定有限表做精确构造与实证；系统造表与 C99 语义充分性属 E — **keep**；本拍升格为 derive-purify 登记，不改正文

---

## 主张一句（本页唯一）

**把「哪些编译阶段天然／必然是有限控制表」的分类定理，以及「如何从文法／属性规则／不变量系统造出这些表」的完备造表方法，不得搭乘 Paper A 投稿；属意向 Paper E（有限控制表系统构造，不挡 A）。A 只保留 unisacc 里已经构造出来的表作为载体证据，以及「已造表 → 精确权重（T1）」；不主张「凡编译阶段皆有限表」，也不主张系统造表完备性或表≡C99。**

---

## 切什么（从 A 的主张负担里切出）

切出对象是 **把「有限控制分解」或「表形决策」升成 A 必须携带的分类／完备造表定理**，以及任何把「凡阶段皆有限表」「系统造表已完备」读成 A 已证命题的投稿级负荷；登记为挂回 **意向 E** 的未来独立课题：

| 稿内／笔记位点（tip `c3c36195`） | 摘句要点 | 分类 |
| --- | --- | --- |
| A 摘要／§2 有限决策 | 表形决策可设计为有限笛卡尔积；无界工作分解为有限控制+通用存储 | **keep 方法边界 in A**；**「阶段分类定理／凡阶段皆有限」migrate** |
| A「Relationship to Paper E」 | A 用给定表做精确构造与实证；系统造表属 E | **keep**（已分工）；本拍升格登记，不改正文 |
| `paper-e-intent.md` | 从文法／属性规则／上下文／不变量系统产生表；首实验声明符簇 | **正式方法宿主（意向）**；本衍生不改意向书正文 |
| `paper-notes`「有限性」段 | 「E 问哪些阶段本来就是有限控制表、表怎么系统造出来；A 用已造表做实证；没造出来的阶段不要提前写进 A」— 仍 INTENT，无 derive-purify 笔记 | **本拍升格登记** → 正式 derive-purify 笔记 |
| 先验已登记衍生 | combo-merge／width／partial-fn／locality／target-param／table4／table5／bootstrap | **正交**；不 redo |

**不切：** 根主张（构造非训练、T1、神经编译器命名）、表 1 键数、已造表的 T1 穷举证据、A 正文已有的「与 E 分工」句（仅引用）、宽度竞赛（已登记）、阶段合并（已登记）、偏函数定义边界（已登记）、局部性（已登记）、目标参数化（已登记）、表 4/5 历史测量（已登记）、自举不动点（已登记）。

---

## 为何不挡 A 投稿

1. **A 根主张不依赖分类／造表完备性定理。** 核心是「unisacc 里已构造的表 → 精确权重（T1）」；哪些阶段*必然*可写成有限表、如何*系统*从规格生成尚未造出的表，是 E 的方法问题，不是 A 的存在性定理。
2. **A 正文已显式把系统造表推给 E。** 「Relationship to Paper E」与 `paper-e-intent.md` 已写清分工；审稿人仍可能把「有限控制分解」读成「凡编译阶段皆有限表」或「造表方法已完备」——故升格登记衍生以卸负担。
3. **没造出来的阶段不要提前写进 A。** `paper-notes` 有限性段已钉死；本拍只登记，不把未造阶段塞进 A。
4. **本拍零正文 diff。** 不改正文 CN/EN/TeX/abstract、不改键数、不重写 paper-e-intent；只登记。

---

## 父节点如何回 A

- **方法回 E→A：** 阶段有限性分类、可检查声明规格、有限观察充分性、造表探针与覆盖报告属意向 E；结果可收窄 A 是否保留任何「凡阶段皆有限／造表已系统完备」软读法，但不能长成第二套编译器理论。
- **实证回 A：** unisacc 已造表仍是 A 的载体证据；T1（网络=已声明表）仍是根主张。
- **DAG：** A → E（有限控制表系统构造）→（未来）finiteness / systematic table construction；**不是**第二根，也不是 A2 RQ1 宽度／合并衍生或测量侧注。

---

## 切后 A 哪一句更硬

切出后，A 可把有限性叙事收成一句硬边界：

> **unisacc 里已经构造出来的有限决策／转移表，经构造器导出权重后，在各表声明的有限域上与表逐键一致（T1）。本文用这些已造表作实证载体，不主张凡编译阶段皆可分类为有限控制表，亦不主张从文法／属性规则／不变量系统造表的完备性；该分类与造表方法留给意向 Paper E。网络=表不等于表=C99。**

攻击面从「是不是在证明所有阶段都是有限表／造表方法已完备？」缩回 **「已造表 → 精确权重」+ T1**，与根主张正交；与既有 A↔E 分工句同向。

---

## 衍生课题提案（不起稿）

| 项 | 内容 |
| --- | --- |
| **暂定题** | Finite Control-Table Systematic Construction: Which Compilation Stages Are Inherently Finite Tables, and How to Construct Them from Specs |
| **一行主张** | Whether every compilation stage is inherently a finite control table, and how to systematically construct such tables from grammars, attribute rules, explicit context and state invariants, is an independent proposition owned by intentional Paper E, not a Paper A theorem; Paper A only keeps already-constructed unisacc tables as empirical carrier evidence and T1 (constructed weights equal the declared table). |
| **上游** | Paper A（T1；已造表实证；正文 A↔E 分工句）；**直接父节点** 意向 E（[`../paper-e-intent.md`](../paper-e-intent.md)） |
| **协议指针** | 系统造表与语义充分性已意向于 paper-e-intent；本课题若起稿须沿 E 意向书另开，不得改 A 键数或「已造表→T1」表述；不得把未造阶段提前写进 A；网络=表≠表=C99 |
| **状态** | **note only** — 无独立 TeX/中英草稿；不改 paper-e-intent；不改正文 A↔E 句 |

---

## 与既有衍生／硬缺口的正交

| 主题 | 关系 |
| --- | --- |
| 测量身份 8509/8769 | **仍开硬缺口**；本拍只引用 12:39 钉，不重论 |
| 表 4 速度区间（~14:25 E） | 正交 |
| 自举不动点（~14:40 E） | 正交 |
| 表 5 历史耗时（~14:57 E） | 正交 |
| 偏函数／partial-fn（~15:12 E） | **相邻但正交**：partial-fn 切「≡偏函数」定理；本拍切「阶段有限性分类 + 系统造表」 |
| 局部性／edit-locality（~15:43 E） | 正交 |
| 阶段组合／combo-merge（~15:52 E） | **相邻但正交**：combo-merge 切「已有表如何合并」；本拍切「哪些阶段*是*有限表、如何*造*表」。共用「表」语汇，命题不同；combo 宿主 A2/C，本拍宿主 E |
| 宽度／W*（~16:15 E） | 正交（宿主 A2 RQ1） |
| 目标参数化 | 正交 |
| paper-e-intent | **直接父节点／方法宿主**；本拍升格登记，不重写 |
| Softguess | **NONE×4**；非硬缺口 |

---

## 明确不做

- 不改正文 CN/EN/TeX/abstract（含不改 A↔E 分工句、不重写 paper-e-intent）
- 不改键数身份 8509/8769/9174；不改 product/kernel/weights/facts
- 不起稿；不叠决策卡；不重钉 8509/8769
- 不把未造阶段提前写进 A；不发明「凡阶段皆有限表」定理句塞回 A
- 不与 ~15:52 combo-merge 合并（相邻语汇、不同命题与宿主）

---

## 证据

- tip（before）：`c3c361954f9c221b1c959822db6e9748f3bb13b7`
- Paper A blobs：CN `f40070830bc44538ac15c3734dcc8257a8e3d7ee` · EN `7878d8072a35438db9653cb21c257b362c7a0665` · TeX `eacae1a1dafa8b4d81eaaa4283d73e1792636a48` · abs `43d7ea7e7297ed8be9a96e3950e014e359467973`（相对 ~16:15／PR #43 后产品 tip **SAME** 正文）
- Softguess：**NONE×4**（四正文文件无 Softguess 字面）
- 先验：`research/paper-e-intent.md`；`research/paper-notes-20261009.md`「有限性」段；A 正文 Relationship to Paper E
- 短报：[`paper-a-heartbeat-report-20261010-1629.md`](paper-a-heartbeat-report-20261010-1629.md)
- 父节点：**E → A**

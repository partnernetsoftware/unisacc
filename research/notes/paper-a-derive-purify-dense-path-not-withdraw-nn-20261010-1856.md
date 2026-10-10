# Paper A 衍生净化：DENSE 产品路径 ≠ 收回 NN 命名／≠ 运行时架构定理

- **拍点：** 2026-10-10 ~18:56 Asia/Shanghai（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `cabb96010ce9a1d1e614247899328ec2374693be`
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1）→ 部署形态／产品路径旁支（DENSE 为构造精确网络的一种实现形态）→ 最终仍回 **A**
- **状态：** **note only**（升格登记：把既有 seal 升为正式 `derive-purify-*` 系列；**不起稿正文**；**不改** A 根主张、键数身份、§4.4／摘要 pin 句、product/kernel/weights/facts；**不重写** [`../seal-dense-path-not-withdraw-nn-20261009.md`](../seal-dense-path-not-withdraw-nn-20261009.md)）
- **Softguess：** tip 四文件 **NONE×4**（本拍机扫：`research/unisacc-paper.md` / `unisacc-paper.en.md` / `arxiv-paper-a/main.tex` / `arxiv-paper-a/abstract.txt` 均 0 hit）
- **测量身份：** 8509/8769 **仍开** — 引用既有钉 [`paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)；本拍**不重钉、不叠**；**不叠** ~17:08 决策卡 Ο1/Ο2/Ο3
- **相邻已封／已切（本拍不重做）：**
  - [`../seal-dense-path-not-withdraw-nn-20261009.md`](../seal-dense-path-not-withdraw-nn-20261009.md)（**父证据／正文 pin**：DENSE 不收回命名、不削弱 T1）
  - [`paper-a-derivative-purify-dense-deploy-not-live-infer-20261010-1140.md`](paper-a-derivative-purify-dense-deploy-not-live-infer-20261010-1140.md)（**前驱草稿**：`derivative-purify` 命名；聚焦「部署形态 ≠ 直播推理」；**未**进正式 `derive-purify-*` 封口列表／理论扩段——本拍补正式升格）
  - [`paper-a-derive-purify-table5-timing-20261010-1457.md`](paper-a-derive-purify-table5-timing-20261010-1457.md)（表 5 DENSE 耗时；**正交**：测量标签 vs 命名／架构定理）
  - [`paper-a-derive-purify-product-maturity-not-theory-20261010-1801.md`](paper-a-derive-purify-product-maturity-not-theory-20261010-1801.md)（RQ1/门禁；**正交**：成熟度 vs DENSE 分类学）
  - [`paper-a-derive-purify-verif-discipline-beyond-nn-20261010-1845.md`](paper-a-derive-purify-verif-discipline-beyond-nn-20261010-1845.md)（beyond-NN 纪律；**正交**：方法推广 vs 部署形态命名）

---

## 主张一句（本页唯一）

**把「已出货 DENSE 查表产品路径」或「DENSE 算不算神经网络」的分类学争论，升成必须收回 Paper A「基于神经网络的编译器（构造网络与权重）」命名的强制条件，或升成「根主张要求／禁止某一种运行时矩阵形态」的架构定理——不得搭乘 A。A 只需保留：TSV 表构造 + T1 + 诚实披露产品路径；命名仍锚定构造与 T1；DENSE 是同一构造决策的一种实现路径，不是第二套编译器理论。**

---

## 切什么（从 A 的主张负担里切出）

切出对象是 **两类搭乘**（均已有 seal／§4.4 pin 防误读，但尚未以正式 derive-purify 登记为可独立旁支）：

| 稿内位点（tip `cabb96010ce9`） | 摘句要点 | 分类 |
| --- | --- | --- |
| §4.4 / *What is not the model*（CN L210 / EN L208） | 「…命名锚定在构造与 T1…DENSE 查表是该构造精确网络…的部署形态，既不收回前述命名主张，也不削弱 T1」 | **keep 最小命名锚 + 诚实产品路径披露**；**「DENSE 出货 ⇒ 必须改名／收回 NN」migrate** |
| 摘要决策网络实例句 | 「命名同样锚定在构造与 T1，而不要求每次产品询问都现场算权重」 | **keep 最短 T1 锚**；**「直播 gemv 才配叫 NN」分类学定理 migrate** |
| §3.1 附近（CN L93） | 「出货构造…与运行时查 DENSE 表是不同部署形态，但 T1 判据始终是…」 | **keep 部署谱系事实一句**；**「根主张强制／禁止某矩阵运行时形态」架构定理 migrate** |
| 表 5／§7.2 DENSE 标注 | 实证测量标签（历史） | **不切**：仍属 A 实证载体说明（已另切 timing） |

**不切：** 根主张（构造非训练、T1、神经编译器命名本身）、§4.4 已粘 pin 句（本拍不改正文）、表 1 键数、网络编译器七阶段实例、Softguess、8509/8769、目标参数化（已切）。

---

## 为何不挡 A 投稿

1. **A 已有 seal 与正文 pin。** [`seal-dense-path-not-withdraw-nn-20261009.md`](../seal-dense-path-not-withdraw-nn-20261009.md) 已钉「命名锚定构造+T1；DENSE 不收回命名、不削弱 T1」。本拍只做 derive-purify **升格登记**，把「分类学强制改名／运行时架构定理」从 A 理论负担卸下。
2. **根主张不依赖特定产品 `ask` 形态。** 核心是构造权重实现表函数（T1）；DENSE 查表、直接整数网络推理、七阶段逐步执行，都是同一构造决策的实现谱系，不是第二根。
3. **分类学争论不是投稿闸门。** 「DENSE 算不算 neural」是术语边界讨论；A 已用「基于神经网络（构造网络与权重）」大白话锚定；升成强制改名条件会把 A 绑死在争议定义上。
4. **本拍零正文 diff。** 不改正文 CN/EN/TeX/abstract、不改键数；只登记（与 ~18:45 verif-discipline、~18:26 tape/ledger 同模式）。

---

## 父节点如何回 A

- **理论句回 A：** 构造权重实现表函数（T1）；「基于神经网络的编译器」命名锚定构造权与 T1，不是锚定每一次产品 `ask` 的矩阵运算形态。
- **旁支：** DENSE／直播推理／部署核的谱系与「neural 分类学」讨论可独立成短文；**禁止**写成与 A 并列的「查表编译器」根。
- **DAG：** A（construction + T1；honest path disclosure）→ DeployForm／dense-naming（deployment taxonomy；optional second upstream C only if generalizing beyond compilers）；与 table5-timing（测量）、product-maturity（门禁）正交。

---

## 切后 A 哪一句更硬

切出后，A 可把命名／产品路径收成一句硬边界（**本拍不改正文；仅登记主张**）：

> **「基于神经网络的编译器」锚定在 TSV 表构造网络与权重以及 T1；决策网络实例出货 DENSE 查表是同一构造精确网络的部署形态，既不强制收回该命名，也不构成「根主张要求／禁止某一种运行时矩阵形态」的架构定理。未完成部署谱系短文或术语分类学辩论，不自动否定构造法或 T1。**

攻击面从「你们产品在查表，不配叫神经网络编译器／必须改架构证明」缩回 **「A = 构造 + T1 + 诚实路径披露；DENSE 是实现，不是第二理论」**。

---

## 衍生课题提案（不起稿）

| 项 | 内容 |
| --- | --- |
| **暂定题** | DENSE Deployment vs Neural Naming: Product Paths Do Not Retract Constructed-Weight Claims (hangs on A) |
| **一行主张** | Elevating a shipped DENSE lookup path or a taxonomy debate over whether DENSE “counts as neural” into a forced retraction of Paper A’s neural-network-based (constructed weights) naming, or into a theorem that the root claim requires/forbids a particular runtime matrix form, is an independent deployment-taxonomy proposition, not required to submit Paper A; Paper A only needs construction + T1 + honest product-path disclosure. |
| **上游** | Paper A（§4.4 / 摘要；[`seal-dense-path-not-withdraw-nn-20261009.md`](../seal-dense-path-not-withdraw-nn-20261009.md)）；前驱草稿 [`paper-a-derivative-purify-dense-deploy-not-live-infer-20261010-1140.md`](paper-a-derivative-purify-dense-deploy-not-live-infer-20261010-1140.md) |
| **协议指针** | 不起稿不得改 A 键数或 T1；不得删 §4.4 诚实披露冒充「已证必须直播推理」；不得与 8509-8769 身份钉或 Ο1/Ο2 混读；不得写成第二套编译器理论 |
| **状态** | **note only** — 无独立 TeX/中英草稿；不改正文；**不重写** seal |

---

## 与既有衍生／硬缺口的正交

| 主题 | 关系 |
| --- | --- |
| 测量身份 8509/8769 | **仍开硬缺口**；键数身份 ≠ DENSE 命名／架构 |
| 同身份平台矩阵（~16:45 C） | **仍开硬缺口**；平台执行矩阵 ≠ 部署分类学 |
| 重测身份决策卡 Ο1/Ο2/Ο3（~17:08 B） | **正交**；本拍**不叠、不代裁** |
| table5-timing（~14:57 E） | **正交**：DENSE 耗时测量 vs 命名／架构定理 |
| 11:40 `derivative-purify` 草稿 | **本拍升格承接**：补正式 `derive-purify-*` 登记 + 显式切出「运行时架构定理」搭乘；不重做部署≠直播推理正文 |
| verif-discipline／tape／maint／product-maturity／Algo1／trapdoor／width／partial-fn／finiteness／table4／bootstrap／combo-merge／locality／target-param | 正交（已切或不重切） |
| Softguess | **NONE×4**；非硬缺口 |

---

## 明确不做

- 不改正文 CN/EN/TeX/abstract（含**不删** §4.4 pin、**不重写** 摘要命名句为强制直播推理）
- 不改键数身份 8509/8769/9174；不改 product/kernel/weights/facts
- 不起稿；不叠决策卡 Ο1/Ο2/Ο3；不重钉 8509/8769
- **不重写** seal-dense；不把 11:40 草稿删掉或改名（仅交叉引用）
- 不与 table5-timing／product-maturity／verif-discipline 合并；不叠 Softguess；不发明 Softguess 证据

---

## 证据指针（本拍）

- tip before：`cabb96010ce9a1d1e614247899328ec2374693be`
- 四 blob（本拍零正文 diff）：CN `74893e3df1f9663b5d11f5bad6dc3cf0ca59e884` · EN `95657738e0b447ee4e40ea9ca3dd961329aebffc` · TeX `3830ede13556e8c9673f994fd6d843937476ecf5` · abs `43d7ea7e7297ed8be9a96e3950e014e359467973`
- Softguess 机扫：NONE×4
- 父 seal：[`../seal-dense-path-not-withdraw-nn-20261009.md`](../seal-dense-path-not-withdraw-nn-20261009.md)（仅引用）
- 前驱：[`paper-a-derivative-purify-dense-deploy-not-live-infer-20261010-1140.md`](paper-a-derivative-purify-dense-deploy-not-live-infer-20261010-1140.md)（仅引用）
- 短报：[`paper-a-heartbeat-report-20261010-1856.md`](paper-a-heartbeat-report-20261010-1856.md)

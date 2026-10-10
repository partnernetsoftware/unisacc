# Paper A 衍生净化：宽度作为一种复杂度 / W* vs training-width race

- **拍点：** 2026-10-10 ~16:15 Asia/Shanghai（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `ec855d036b531214d411ba91a01b5e2f1ddf756c`
- **父节点：** **A2 RQ1**（宽度带／可达性；三条推翻条件；归档预注册 [`../../archive/research/a2-preregistration.md`](../../archive/research/a2-preregistration.md)）→ 最终回 **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；实证载体 unisacc POSIX C99 跨架构）
- **状态：** **note only**（登记未来独立「宽度作为一种复杂度／W* vs 训练宽度竞赛」命题；**不起稿正文**；**不改** A 根主张、键数身份、§7.3 Adam 表数字、product/kernel/weights/facts；**不重做** [`../seal-adam-width-not-theory-20261009.md`](../seal-adam-width-not-theory-20261009.md)）
- **Softguess：** tip 四文件 **NONE×4**（本拍机扫：`research/unisacc-paper.md` / `unisacc-paper.en.md` / `arxiv-paper-a/main.tex` / `abstract.txt` 均 0 hit）
- **测量身份：** 8509/8769 **仍开** — 引用既有钉 [`paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)；本拍**不重钉、不叠**路线Ι
- **已封口（本拍不重做）：**
  - [`../seal-adam-width-not-theory-20261009.md`](../seal-adam-width-not-theory-20261009.md)（§7.3 Adam 跌落／「须加宽」= 历史示意，≠ 宽度或可到达性优势定理；W* 竞赛属 A2；三条推翻成立则 A 优势缩为确定性／可证明／成本）

---

## 主张一句（本页唯一）

**把构造隐藏宽度 W* 与训练宽度的竞赛，以及把 §7.3 Adam 跌落／「须加宽」读成 Paper A 的宽度或可到达性优势定理，不得搭乘 A 投稿；属 A2（尤 RQ1 宽度带 + 三条推翻条件）。A 只保留「存在一组构造出来的权重，在声明域上全域精确（T1）」。若 A2 三条推翻条件均成立，A 的优势语言缩为确定性／可证明／成本——文案封口已有，本拍升格为 derive-purify 登记。**

---

## 切什么（从 A 的主张负担里切出）

切出对象是 **把「W* vs 训练宽度竞赛」或「Adam 未达 1.000 ⇒ 构造在宽度／可达性上优于训练」升成 A 必须携带的宽度／可达性优势定理**，以及任何把 §7.3 Adam 表误读成 A 已证命题的投稿级负荷；登记为挂回 **A2 RQ1** 的未来独立课题：

| 稿内位点（tip `ec855d03`） | 摘句要点 | 分类 |
| --- | --- | --- |
| §7.3 Adam 表（CN/EN/TeX） | type 0.9514／须加宽；merge 0.8143；many-target 0.4419；历史示意 | **keep 历史示意读法 in A**（已由 seal-adam-width 澄清）；**「宽度／可达性优势定理」migrate** |
| 摘要 Adam 半句 | 「未对齐…非宽度或可到达性优势定理」 | **keep**（seal 已软化）；不升格为 A 须证宽度定理 |
| A2 预注册 RQ1 | 宽度带；W* 为基准；三条推翻条件 | **正式实验宿主**；本衍生不改预注册正文 |
| `paper-notes`「宽度作为一种复杂度」段 | 已写「A 只声称存在构造精确权重；W* 竞赛属 A2；三条推翻→优势改写确定性／可证明／成本」+ seal-adam 交叉引用 | **本拍升格登记** → 正式 derive-purify 笔记 |
| 先验封口 | seal-adam-width-not-theory | **evidence parent**；本衍生不重写 seal |

**不切：** 根主张（构造非训练、T1、神经编译器命名）、表 1 键数、§7.3 Adam **数字单元格**、表 4/表 5 历史耗时（已登记）、自举不动点（已登记）、偏函数定义边界（已登记）、局部性／编辑定理（已登记）、阶段组合／表合并定理（已登记）、目标参数化（已登记）、DeployForm/DENSE 命名边界（已封）、BootstrapHost 字节非语义（已封）。

---

## 为何不挡 A 投稿

1. **A 根主张不依赖宽度竞赛定理。** 核心是「存在一组构造出来的权重，在声明域上全域精确（T1）」；W* 与训练宽度谁更省、训练在何种宽度带可对齐，是 A2 RQ1 材料，不是 A 的存在性定理。
2. **Adam≠宽度优势定理已在 seal 对齐。** seal-adam-width 已把 §7.3／摘要钉死为历史示意；审稿人仍可能把「须加宽／未达 1.000」读成 A 的须证宽度定理——故升格登记衍生以卸负担。
3. **A2 RQ1 已冻结宽度带与三条推翻条件。** 系统性构造—训练宽度对照的实验宿主是 A2；若三条件均成立，A 优势语言缩为确定性／可证明／成本（已封，本拍只登记）。
4. **本拍零正文 diff。** 不改正文、不改键数、不重做 seal-adam-width；只登记。

---

## 父节点如何回 A

- **实证／实验回 A2→A：** W* 基准宽度带、训练加宽／多目标／合并可达性属 A2 RQ1；结果可收窄 A 是否保留任何「Adam 跌落＝构造宽度优势」软读法，但不能长成第二套编译器理论。
- **理论句回 A：** 「存在一组构造出来的权重，在声明域上全域精确（T1）」仍是根主张；衍生只搬走「须证宽度／可达性优势定理」升格读法。
- **DAG：** A → A2（RQ1 宽度带 + 三条推翻）→（未来）width-complexity / W*-vs-training-width race；**不是**第二根，也不是阶段合并衍生、局部性衍生或表 4/5 测量衍生。

---

## 切后 A 哪一句更硬

切出后，A 可把宽度叙事收成一句硬边界：

> **存在一组由决策表构造出来的权重，在该表声明的有限域上与表逐键一致（T1）。§7.3 Adam 跌落与「须加宽」等行仅为该节协议下的历史示意，本文不主张、亦不证明构造在隐藏宽度或可到达性上优于训练；$W^*$ 与训练宽度的竞赛及系统性构造—训练对照留给 A2 RQ1；若 A2 三条推翻条件均成立，本文优势语言只保留确定性、可证明性与成本。**

攻击面从「是不是在证明构造更窄／训练须加宽？」缩回 **「存在构造精确权重」+ T1**，与根主张正交；与既有 seal-adam-width 文案封口同向。

---

## 衍生课题提案（不起稿）

| 项 | 内容 |
| --- | --- |
| **暂定题** | Width as Complexity: Contests of Construction Hidden Width $W^*$ versus Training Width, and Falsification via A2 RQ1 Width Band |
| **一行主张** | Whether construction's hidden width $W^*$ systematically beats training width (or whether Adam drops / 「须加宽」 prove reachability superiority) is an independent proposition owned by A2 RQ1 and its three falsifiers, not a Paper A theorem; Paper A only keeps existence of constructed exact weights on the declared domain (T1). |
| **上游** | Paper A（T1 存在性；[`seal-adam-width-not-theory-20261009.md`](../seal-adam-width-not-theory-20261009.md)）；**直接父节点** A2 RQ1（[`../../archive/research/a2-preregistration.md`](../../archive/research/a2-preregistration.md)） |
| **协议指针** | Adam≠宽度优势定理已封于 seal-adam-width；本课题若起稿须沿 A2 预注册 RQ1 宽度带与三条推翻判据另开，不得改 A 键数或「存在构造精确权重」表述；三条推翻均成立时 A 优势缩为确定性／可证明／成本（已封口语言，不本拍重写） |
| **状态** | **note only** — 无独立 TeX/中英草稿；不改 A2 预注册；不重做 seal-adam-width |

---

## 与既有衍生／硬缺口的正交

| 主题 | 关系 |
| --- | --- |
| 测量身份 8509/8769 | **仍开硬缺口**；本拍只引用 12:39 钉，不重论 |
| 表 4 速度区间（~14:25 E） | 正交 |
| 自举不动点（~14:40 E） | 正交 |
| 表 5 历史耗时（~14:57 E） | 正交 |
| 偏函数／partial-fn（~15:12 E） | 正交 |
| 局部性／edit-locality（~15:43 E） | 正交 |
| 阶段组合／combo-merge（~15:52 E） | **相邻但正交**：combo-merge 切「阶段可合并／宽度和＝定理」；本拍切「W* vs 训练宽度竞赛／Adam＝宽度优势定理」。共用 A2 RQ1 宿主，命题不同 |
| 目标参数化 | 正交 |
| DeployForm/DENSE；BootstrapHost bytes-not-semantic | 已封／已登记；正交 |
| seal-adam-width-not-theory（2026-10-09） | **父证据**：Adam≠宽度优势定理已封；本拍是 derive-purify 升格登记，不重做 seal |
| A2 RQ1 宽度带 + 三条推翻 | **直接父节点／实验宿主** |
| Softguess | **NONE×4**；非硬缺口 |

---

## 明确不做

- 不改正文 CN/EN/TeX/abstract（含不改 §7.3 Adam 句、不重写 seal-adam-width）
- 不改键数身份 8509/8769/9174；不改 product/kernel/weights/facts
- 不起稿；不叠决策卡；不重钉 8509/8769
- 不改写 A2 预注册；不发明新定理句塞回 A
- 不重做已封口的 Adam≠宽度／可达性优势定理
- 不与 ~15:52 阶段合并衍生合并（相邻宿主、不同命题）

---

## 证据

- tip（before）：`ec855d036b531214d411ba91a01b5e2f1ddf756c`
- Paper A blobs：CN `f40070830bc44538ac15c3734dcc8257a8e3d7ee` · EN `7878d8072a35438db9653cb21c257b362c7a0665` · TeX `eacae1a1dafa8b4d81eaaa4283d73e1792636a48` · abs `43d7ea7e7297ed8be9a96e3950e014e359467973`（相对 ~15:52 研究 tip／PR #42 后产品 tip **SAME** 正文）
- Softguess：**NONE×4**（四正文文件无 Softguess 字面）
- 先验：`research/seal-adam-width-not-theory-20261009.md`；`archive/research/a2-preregistration.md` RQ1；`research/paper-notes-20261009.md`「宽度作为一种复杂度」段
- 短报：[`paper-a-heartbeat-report-20261010-1615.md`](paper-a-heartbeat-report-20261010-1615.md)
- 父节点：**A2 RQ1 → A**

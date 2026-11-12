# Paper A 衍生净化：活板门 / trapdoor ≠ 不可达性定理

- **拍点：** 2026-10-10 ~19:15 Asia/Singapore（SGT / UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `89bc23ed9094f767837792833db3d750a25621f7`
- **父节点：** **意向 A2**（构造 vs 训练预注册／梯度竞赛；归档 [`../../archive/research/a2-preregistration.md`](../../archive/research/a2-preregistration.md)；RQ1/RQ3 可达性／梯度竞赛）→ 最终回 **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；实证载体 unisacc POSIX C99 跨架构）
- **状态：** **note only**（登记未来独立「活板门／trapdoor 形式不可达性／梯度精确性竞赛」命题；**不起稿正文**；**不改** A 根主张、键数身份、§7.3 机理段、product/kernel/weights/facts；**不重写** [`../seal-trapdoor-not-unreachability-20261009.md`](../seal-trapdoor-not-unreachability-20261009.md) 与 §7.3）
- **Softguess：** tip 四文件 **NONE×4**（本拍机扫：`research/unisacc-paper.md` / `unisacc-paper.en.md` / `arxiv-paper-a/main.tex` / `arxiv-paper-a/abstract.txt` 均 0 hit）
- **测量身份：** 8509/8769 **仍开** — 引用既有钉 [`paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)；本拍**不重钉、不叠**；**不叠** Ο1/Ο2 决策卡
- **已封口（本拍不重做）：**
  - [`../seal-trapdoor-not-unreachability-20261009.md`](../seal-trapdoor-not-unreachability-20261009.md)（§7.3「活板门」= 立方体表示下非形式机理示意，≠ Paper A 不可达性定理，≠ Adam/SGD 在编译器规模合并与扩表上无法达到精确的证明；系统性失败留给 A2）
  - 相邻：[`../seal-adam-width-not-theory-20261009.md`](../seal-adam-width-not-theory-20261009.md) + [`paper-a-derive-purify-width-complexity-wstar-20261010-1615.md`](paper-a-derive-purify-width-complexity-wstar-20261010-1615.md)（Adam 行 ≠ 宽度／可到达性优势定理；宽度竞赛已登记；本拍切不可达性**标签**，命题不同）
- **tip 核对说明：** 当前 main tip `89bc23ed`（csih）的 `research/notes/` **无** `paper-a-derive-purify-trapdoor-*`；API 列目录仅见至 ~16:45 钉。搜索索引仍可见孤儿 tip `10e7450a` 上曾合入的 ~17:27 同题笔记，但**不在本 tip**，故本拍按「未在 tip 登记」正式升格登记（新 slug `-1915`；不改正文）。

---

## 主张一句（本页唯一）

**把「活板门／trapdoor」标签升成 Paper A 的不可达性定理，或读成「Adam/SGD 在编译器规模合并与扩表上无法达到精确」的证明，不得搭乘 A 投稿；正式梯度不可达／构造—训练竞赛属意向 A2（RQ1/RQ3）。A 只保留 §7.3 立方体表示下的非形式机理示意，以及 T1／构造法主张；seal-trapdoor 文案已钉死，本拍升格为 derive-purify 登记。**

---

## 切什么（从 A 的主张负担里切出）

切出对象是 **把 §7.3 命名的「活板门现象」升成 A 必须携带的不可达性／梯度不可能性定理**，以及任何把「小 $W_1$ 梯度步无法协调 $(W_1,b_1)$ 联合离散跳」读成 A 已证「训练不可达精确」的投稿级负荷；登记为挂回 **A2（梯度竞赛／构造 vs 训练）** 的未来独立课题：

| 稿内／笔记位点（tip `89bc23ed`） | 摘句要点 | 分类 |
| --- | --- | --- |
| CN §7.3 机理段 | 「机理示意…（活板门现象）。此处「活板门」仅为本节立方体表示下的非形式机理示意，不是 Paper A 的不可达性定理，也不证明 Adam/SGD 在编译器规模合并与扩表上无法达到精确。…系统性失败，留给预注册研究 A2。」 | **keep 机理示意 in A**（seal 已澄清）；**「不可达性／SGD 不可能性定理」migrate** |
| EN/TeX §7.3 | Mechanism illustration；trapdoor phenomenon；同边界 seal 句；systematic failure → A2 | **keep / migrate** 同上 |
| `seal-trapdoor-not-unreachability-20261009.md` | 审稿风险：命名现象可被拔高为产品级梯度不可能；系统失败 → A2 | **evidence parent**；本衍生不重写 seal／§7.3 |
| A2 预注册 | 构造 vs 训练；梯度／可达性竞赛宿主（RQ1/RQ3） | **正式实验宿主**；本衍生不改预注册正文 |
| `paper-notes` 封口列表 | 已有 seal-trapdoor 交叉引用；「理论还能往哪扩」尚无独立「活板门」衍生段（本 tip） | **本拍升格登记** → 正式 derive-purify 笔记 + 理论扩段 |
| tip 已登记衍生 | partial-fn／locality／combo-merge／width-W*／finiteness／target-param／table4／table5／bootstrap／平台矩阵钉 | **正交**；不 redo（尤 width-W* 切宽度竞赛，本拍切不可达性标签） |

**不切：** 根主张（构造非训练、T1、神经编译器命名）、表 1 键数、§7.3 机理示意句本身（仅引用、不重写）、Adam 宽度优势读法（已登记 width-W*）、表 4/5 历史测量、自举不动点、偏函数定义边界、局部性、阶段合并、目标参数化、有限性／造表、重测身份决策卡 Ο1/Ο2。

---

## 为何不挡 A 投稿

1. **A 根主张不依赖不可达性定理。** 核心是构造法 + T1（已造表 → 精确权重）；梯度是否在编译器规模合并／扩表上「不可达精确」，是 A2 对照实验材料，不是 A 的存在性定理。
2. **seal 已明确禁止 trapdoor-as-theorem。** 2026-10-09 已把「活板门」钉为非形式机理示意；审稿人仍可能把命名现象拔高为 A 的须证不可达性——故升格登记衍生以卸负担。
3. **A2 已是系统性构造—训练／梯度竞赛宿主。** 正式不可达／精确性竞赛若起稿，沿 A2 预注册（RQ1/RQ3）另开；结果可收窄 A 是否保留任何「训练不可达」软读法，不能长成第二套理论。
4. **本拍零正文 diff。** 不改正文 CN/EN/TeX/abstract、不改键数、不重写 seal 或 §7.3；只登记。

---

## 父节点如何回 A

- **实验回 A2→A：** 梯度不可达／合并与扩表上的精确性竞赛、Adam/SGD 对照属意向 A2；结果可收窄 A 是否保留任何「活板门＝训练不可达精确」软读法，但不能长成第二套编译器理论。
- **理论句回 A：** §7.3 只保留立方体表示下的非正式机理标签 + T1／构造主张；衍生只搬走「须证不可达性定理」升格读法。
- **DAG：** A → A2（construct-vs-train／gradient contests，RQ1/RQ3）→（未来）trapdoor / formal unreachability；**不是**第二根；与 width-W*（同宿主 A2 RQ1，不同命题：宽度竞赛 vs 不可达性标签）正交。

---

## 切后 A 哪一句更硬

切出后，A 可把活板门叙事收成一句硬边界（**文案已由 seal 落地；本拍不重写正文**）：

> **§7.3「活板门／trapdoor」仅为该节立方体表示下的非形式机理示意：小步梯度难以协调 $(W_1,b_1)$ 一类联合离散跳的直观说明。本文不主张、亦不证明梯度方法在此构造上不可达，也不证明 Adam/SGD 在编译器规模合并与扩表上无法达到精确；该类形式竞赛留给 A2。A 的硬主张仍是构造法与 T1。**

攻击面从「是不是在证明训练不可达／SGD 不可能精确？」缩回 **「机理示意 + T1／构造」**，与根主张正交；与既有 seal-trapdoor 文案封口同向。

---

## 衍生课题提案（不起稿）

| 项 | 内容 |
| --- | --- |
| **暂定题** | Trapdoor as Informal Mechanism, Not Unreachability: Formal Gradient Contests Belong to A2 (Construct-vs-Train) |
| **一行主张** | Whether the §7.3 trapdoor label upgrades to a Paper A unreachability theorem (or a proof that Adam/SGD cannot reach exactness on compiler-scale merges/extensions) is an independent proposition owned by intentional A2 gradient contests (RQ1/RQ3), not a Paper A theorem; Paper A only keeps the informal mechanism sketch under the cube representation plus T1/construction claims. |
| **上游** | Paper A（T1；构造；[`seal-trapdoor-not-unreachability-20261009.md`](../seal-trapdoor-not-unreachability-20261009.md)）；**直接父节点** 意向 A2（[`../../archive/research/a2-preregistration.md`](../../archive/research/a2-preregistration.md)） |
| **协议指针** | trapdoor≠不可达性定理已封于 seal-trapdoor；本课题若起稿须沿 A2 预注册另开形式梯度竞赛，不得改 A 键数或「机理示意／T1」表述；不得重写 §7.3 seal 句当成本拍新正文 |
| **状态** | **note only** — 无独立 TeX/中英草稿；不改 A2 预注册；不重写 seal-trapdoor／§7.3 |

---

## 与既有衍生／硬缺口的正交

| 主题 | 关系 |
| --- | --- |
| 测量身份 8509/8769 | **仍开硬缺口**；本拍只引用 12:39 钉，不重论 |
| 重测身份决策卡 Ο1/Ο2 | **正交**；本拍**不叠、不代裁** |
| 表 4 速度区间（~14:25 E） | 正交 |
| 自举不动点（~14:40 E） | 正交 |
| 表 5 历史耗时（~14:57 E） | 正交 |
| 偏函数／partial-fn（~15:12 E） | 正交 |
| 局部性／edit-locality（~15:43 E） | 正交 |
| 阶段组合／combo-merge（~15:52 E） | 正交（合并定理 vs 不可达性标签） |
| 宽度／W*（~16:15 E） | **相邻但正交**：width 切「W* vs 训练宽度／Adam＝宽度优势」；本拍切「trapdoor＝不可达性／SGD 不可能性定理」。共用 §7.3／A2 语汇与部分宿主，命题不同 |
| 有限性／控制表（~16:29 E） | 正交（宿主意向 E） |
| 平台矩阵钉（~16:45 C） | 正交 |
| 目标参数化 | 正交 |
| seal-trapdoor-not-unreachability（2026-10-09） | **父证据**：文案已封；本拍是 derive-purify 升格登记，**不重写 seal／§7.3** |
| A2 预注册（构造 vs 训练／梯度） | **直接父节点／实验宿主** |
| Softguess | **NONE×4**；非硬缺口 |

---

## 明确不做

- 不改正文 CN/EN/TeX/abstract（含**不重写** §7.3 活板门 seal 句）
- 不改键数身份 8509/8769/9174；不改 product/kernel/weights/facts
- 不起稿；不叠决策卡 Ο1/Ο2；不重钉 8509/8769
- 不改写 A2 预注册；不发明新定理句塞回 A
- **不重做**已封口的 seal-trapdoor-not-unreachability
- 不与 ~16:15 width-W* 合并（相邻 §7.3／A2 宿主、不同命题）

---

## 证据

- tip（before）：`89bc23ed9094f767837792833db3d750a25621f7`
- Paper A blobs：CN `f40070830bc44538ac15c3734dcc8257a8e3d7ee` · EN `7878d8072a35438db9653cb21c257b362c7a0665` · TeX `eacae1a1dafa8b4d81eaaa4283d73e1792636a48` · abs `43d7ea7e7297ed8be9a96e3950e014e359467973`（本 tip；本拍零正文 diff → **SAME**）
- Softguess：**NONE×4**（四正文文件无 Softguess 字面）
- 先验：`research/seal-trapdoor-not-unreachability-20261009.md`；`archive/research/a2-preregistration.md`；`research/paper-notes-20261009.md` 封口活板门行；CN §7.3 机理示意段（活板门）
- 短报：[`paper-a-heartbeat-report-20261010-1915.md`](paper-a-heartbeat-report-20261010-1915.md)
- 父节点：**A2 → A**

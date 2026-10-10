# Paper A 衍生净化：算法 1 的 35→20 / 796k→71k ≠ 最优性定理

- **拍点：** 2026-10-10 ~17:43 Asia/Shanghai（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `7bf5a7deb71fdff134c240adeb9e9b4faa80137c`
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；算法 1 是 A 的构造程序）→ 可选旁支 **A2 / 意向 E**（宽度／复杂度竞赛、贪心差距实验）→ 最终仍回 **A**
- **状态：** **note only**（登记未来独立「算法 1 最优性／贪心—精确差距／构造宽度最小性」命题；**不起稿正文**；**不改** A 根主张、键数身份、§3.2 构造段、product/kernel/weights/facts；**不重写** [`../seal-algo1-35-20-not-optimality-20261009.md`](../seal-algo1-35-20-not-optimality-20261009.md) 与 §3.2）
- **Softguess：** tip 四文件 **NONE×4**（本拍机扫：`research/unisacc-paper.md` / `unisacc-paper.en.md` / `arxiv-paper-a/main.tex` / `arxiv-paper-a/abstract.txt` 均 0 hit）
- **测量身份：** 8509/8769 **仍开** — 引用既有钉 [`paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)；本拍**不重钉、不叠**路线Ι；**不叠** ~17:08 决策卡 Ο1/Ο2/Ο3
- **已封口（本拍不重做）：**
  - [`../seal-algo1-35-20-not-optimality-20261009.md`](../seal-algo1-35-20-not-optimality-20261009.md)（§3.2：35→20 精确枚举对照 ≠ 算法 1 贪心最优证书 ≠ 796,490→71,828 缩参证书；Paper A 不升格为最优性定理）
  - 相邻：[`../seal-adam-width-not-theory-20261009.md`](../seal-adam-width-not-theory-20261009.md) + [`paper-a-derive-purify-width-complexity-wstar-20261010-1615.md`](paper-a-derive-purify-width-complexity-wstar-20261010-1615.md)（W* vs 训练宽度竞赛；命题不同：宽度竞赛 vs 算法 1／贪心缩参最优性）

---

## 主张一句（本页唯一）

**把算法 1 的 35→20 精确枚举与 796k→71k 贪心缩参升成「最优性定理」（或主张构造宽度已证最小），不得搭乘 A 投稿；A 只需「存在一组构造出来的精确解／T1 全表重建」。最优性／最小宽度／贪心差距是独立定理或实验，不是 A 的投稿必要条件。seal-algo1 文案已钉死，本拍升格为 derive-purify 登记。**

---

## 切什么（从 A 的主张负担里切出）

切出对象是 **把 §3.2 两段相邻测量（五小表立方体内精确枚举合计 35→20；全套 18 阶段贪心覆盖 796,490→71,828）读成算法 1（或其贪心路径）已近最优／已证最优，或读成构造宽度已证为任意网络最小**，从而给 A 加上须防守「这宽度最优」的投稿级负荷：

| 稿内／笔记位点（tip `7bf5a7de`） | 摘句要点 | 分类 |
| --- | --- | --- |
| §3.2（CN/EN/TeX） | 贪心 796,490→71,828；五小表立方体内 35→20；已有 seal 句：非贪心最优证书、非缩参证书、不升格最优性定理 | **keep 构造程序 + 两段测量 in A**（seal 已澄清）；**「最优性／最小宽度定理」migrate** |
| `seal-algo1-35-20-not-optimality-20261009.md` | 混读风险：精确枚举 vs 贪心缩参 vs 任意网络最小宽度 | **evidence parent**；本衍生不重写 seal／§3.2 |
| `paper-a-notes.md` 算法 1 复杂度／最优性差距 | 逐表贪心—精确对比与贪心运行时间仍开放 | **正式实验宿主候选**；本拍不改笔记逐表义务 |
| 先验 width-W*（~16:15 E） | W* vs 训练宽度／Adam＝宽度优势 | **相邻但正交**（宽度竞赛 ≠ 算法 1 贪心／枚举最优性） |
| `paper-notes` 封口列表 | 已有 seal-algo1 交叉引用；理论扩段尚无独立「算法 1≠最优性」衍生段 | **本拍升格登记** |

**不切：** 根主张（构造非训练、T1、神经编译器命名）、表 1 键数、§3.2 算法 1 作为构造程序的表述本身（仅引用、不重写）、两段测量数字、width-W* 登记、表 4/5、trapdoor、偏函数、局部性、阶段合并、有限性、目标参数化、重测身份决策卡 Ο1/Ο2/Ο3。

---

## 为何不挡 A 投稿

1. **A 根主张不依赖最优性定理。** 核心是构造法 + T1（已造表 → 精确权重）；算法 1 证明的是「找到了一组可工作的构造宽度／参数规模」，不是「该宽度或该贪心缩参已最优」。
2. **seal 已明确禁止 Algo1-as-optimality。** 2026-10-09 已把 35→20 与 796k→71k 钉为不可混读、不升格定理；审稿人仍可能把两段测量拔高为 A 须证的最小宽度——故升格登记衍生以卸负担。
3. **最优性／差距可另开。** 贪心运行时间、逐表贪心—精确对比、相对任意网络的最小宽度、与训练宽度竞赛，可挂 A2 RQ1 或意向 E／独立复杂度笔记；结果可收窄 A 是否保留任何「近最优」软读法，不能长成第二套理论。
4. **本拍零正文 diff。** 不改正文 CN/EN/TeX/abstract、不改键数、不重写 seal 或 §3.2；只登记（与 trapdoor E ~17:27 同模式）。

---

## 父节点如何回 A

- **理论句回 A：** 算法 1 仍是 A 的构造程序（有限决策表 → 价值网络权重）；衍生只搬走「须证最优／最小宽度」升格读法。
- **实验旁支：** 可选挂 A2（宽度／构造—训练竞赛）或意向 E（系统造表／复杂度分类）；结果可回来收窄措辞，**不是**第二根。
- **DAG：** A（Algo1 = construction procedure）→（未来）Algo1 optimality / greedy-gap study；与 width-W*（同可挂 A2 RQ1，不同命题：训练宽度竞赛 vs 贪心／枚举最优性）正交。

---

## 切后 A 哪一句更硬

切出后，A 可把算法 1 叙事收成一句硬边界（**文案已由 seal 落地；本拍不重写正文**）：

> **算法 1 是找到一组可工作构造宽度的程序：全套 18 阶段贪心覆盖给出 796,490→71,828 的参数缩减，五小表立方体内精确枚举给出合计 35→20 的对照。本文不主张、亦不证明该贪心覆盖最优，不证明 35→20 为任意网络最小宽度，也不把任一数升格为最优性定理。A 的硬主张仍是构造法与 T1（存在构造精确解）。**

攻击面从「是不是在证明这宽度／这缩参最优？」缩回 **「构造程序找到可工作宽度 + T1」**；挑战者用更宽网重训或改枚举边界时，A 不必防守「已证最小」。

---

## 衍生课题提案（不起稿）

| 项 | 内容 |
| --- | --- |
| **暂定题** | Algorithm 1 Exact Enumeration and Greedy Shrink Are Not an Optimality Theorem: Width/Complexity Gaps Belong Off Paper A |
| **一行主张** | Claiming Algo1's 35→20 cube-class exact enumeration and/or 796k→71k greedy parameter shrink as an optimality theorem (or that the constructed width is proven minimal over arbitrary networks) is an independent proposition, not required to submit Paper A; Paper A only needs existence of a constructed exact solution / T1 rebuild, with Algo1 kept as the construction procedure that found a working width. |
| **上游** | Paper A（构造程序 Algo1；T1；[`seal-algo1-35-20-not-optimality-20261009.md`](../seal-algo1-35-20-not-optimality-20261009.md)）；可选旁支 A2 RQ1／意向 E |
| **协议指针** | Algo1≠最优性已封于 seal-algo1；本课题若起稿须另开贪心差距／最小宽度实验，不得改 A 键数或「构造程序 + T1」表述；不得重写 §3.2 seal 句当成本拍新正文 |
| **状态** | **note only** — 无独立 TeX/中英草稿；不重写 seal-algo1／§3.2 |

---

## 与既有衍生／硬缺口的正交

| 主题 | 关系 |
| --- | --- |
| 测量身份 8509/8769 | **仍开硬缺口**；本拍只引用 12:39 钉，不重论 |
| 重测身份决策卡 Ο1/Ο2/Ο3（~17:08 B） | **正交**；本拍**不叠、不代裁** |
| 宽度／W*（~16:15 E） | **相邻但正交**：width 切「W* vs 训练宽度／Adam＝宽度优势」；本拍切「Algo1 枚举／贪心缩参＝最优性定理」。共用 §3.2／复杂度语汇，命题不同 |
| 活板门／trapdoor（~17:27 E） | 正交（不可达性标签 vs 最优性标签） |
| 局部性／combo-merge／finiteness／partial-fn／table4／table5／bootstrap／target-param | 正交 |
| seal-algo1-35-20-not-optimality（2026-10-09） | **父证据**：文案已封；本拍是 derive-purify 升格登记，**不重写 seal／§3.2** |
| Softguess | **NONE×4**；非硬缺口 |

---

## 明确不做

- 不改正文 CN/EN/TeX/abstract（含**不重写** §3.2 Algo1 seal 句）
- 不改键数身份 8509/8769/9174；不改 product/kernel/weights/facts
- 不起稿；不叠决策卡 Ο1/Ο2/Ο3；不重钉 8509/8769
- 不发明新定理句塞回 A；不叠 Softguess／weight≡logic 卡
- **不重做**已封口的 seal-algo1-35-20-not-optimality
- 不与 ~16:15 width-W* 合并（相邻复杂度语汇、不同命题）

---

## 证据

- tip（before）：`7bf5a7deb71fdff134c240adeb9e9b4faa80137c`
- Paper A blobs：CN `74893e3df1f9663b5d11f5bad6dc3cf0ca59e884` · EN `95657738e0b447ee4e40ea9ca3dd961329aebffc` · TeX `3830ede13556e8c9673f994fd6d843937476ecf5` · abs `43d7ea7e7297ed8be9a96e3950e014e359467973`（相对 PR #46–#48 后 tip **SAME** 正文；本拍零正文 diff；§3.2 seal 句已在位，软读已封）
- Softguess：**NONE×4**（四正文文件无 Softguess 字面）
- 先验：`research/seal-algo1-35-20-not-optimality-20261009.md`；`research/paper-a-notes.md`（算法 1 复杂度／最优性差距仍开放）；`research/paper-notes-20261009.md` 封口 Algo1 行；width-W* 登记 `paper-a-derive-purify-width-complexity-wstar-20261010-1615.md`
- 短报：[`paper-a-heartbeat-report-20261010-1743.md`](paper-a-heartbeat-report-20261010-1743.md)
- 父节点：**A**（Algo1 = construction procedure）；可选旁支 A2/E
# Paper A 衍生净化：算法 1／Algo1 35→20 ≠ 最优性定理

- **拍点：** 2026-10-10 ~19:30 Asia/Singapore（SGT / UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `ab44d66bfe05fee5ec551de35f8a27ebd24bcedc`
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；实证载体 unisacc POSIX C99 跨架构）。复杂度／最优性差距分析若起稿，仍挂回 A 的诚实成本披露义务，**不是**第二根。
- **状态：** **note only**（登记未来独立「算法 1 贪心覆盖 vs 精确枚举／最优性差距」命题；**不起稿正文**；**不改** A 根主张、键数身份、§3.2 文案、product/kernel/weights/facts；**不重写** [`../seal-algo1-35-20-not-optimality-20261009.md`](../seal-algo1-35-20-not-optimality-20261009.md) 与 §3.2）
- **Softguess：** tip 四文件 **NONE×4**（本拍沿用既有机扫惯例：`research/unisacc-paper.md` / `unisacc-paper.en.md` / `arxiv-paper-a/main.tex` / `arxiv-paper-a/abstract.txt`；Softguess 字面仅见于 notes，正文 0 hit）
- **测量身份：** 8509/8769 **仍开** — 引用既有钉 [`paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)；本拍**不重钉、不叠**；**不叠** Ο1/Ο2 决策卡
- **同身份矩阵：** **仍开** — 引用 [`paper-a-gap-nail-platform-matrix-same-identity-20261010-1645.md`](paper-a-gap-nail-platform-matrix-same-identity-20261010-1645.md)；本拍不触
- **已封口（本拍不重做）：**
  - [`../seal-algo1-35-20-not-optimality-20261009.md`](../seal-algo1-35-20-not-optimality-20261009.md)（§3.2：35→20 = 立方体表示下五小表精确枚举合计；796,490→71,828 = 全套 18 阶段贪心覆盖缩参；二者不可混读；Paper A 不把任一数升格为算法 1／贪心最优性定理；逐表对比与贪心运行时间仍开放）
- **tip 核对说明：** 当前 main tip `ab44d66b`（PR #55 trapdoor 重登记后）的 `research/notes/` **无** `paper-a-derive-purify-algo1-*`；API 列目录与本机浅克隆均未见 ~17:43 一带曾合入的 Algo1／maint／bdiff／DENSE／verification 衍生笔记（csih `89bc23ed` 后 tip 上相关 afternoon E 登记缺失）。本拍按「未在 tip 登记」**优先重登记 Algo1**（新 slug `-1930`；不改正文）。其余丢失集（maint-surface／bdiff／DENSE／verification-discipline 等）后续拍续切。

---

## 主张一句（本页唯一）

**把 §3.2 的 35→20 精确枚举合计，或 796,490→71,828 贪心缩参，升成「算法 1／贪心覆盖已证最优」或「35→20 为 796k→71k 作证书」的最优性定理，不得搭乘 A 投稿。A 只保留构造法 + T1 + 诚实成本／缩参披露；最优性差距与贪心复杂度分析若独立成题，挂回 A，不起第二根。seal-algo1 文案已钉死，本拍升格为 derive-purify 登记（csih 后 tip 重登记）。**

---

## 切什么（从 A 的主张负担里切出）

切出对象是 **把算法 1（或其余贪心路径）读成 Paper A 必须携带的最优性定理**，以及把「五小表精确枚举 35→20」与「全套 18 阶段贪心 796k→71k」混成互证的投稿级负荷：

| 稿内／笔记位点（tip `ab44d66b`） | 摘句要点 | 分类 |
| --- | --- | --- |
| CN/EN/TeX §3.2 | 贪心覆盖 796,490→71,828（18 阶段）；五小表精确枚举合计 35→20；seal 句：二者并列、不可互证、不升格最优性定理 | **keep 诚实披露 in A**；**「最优性定理／互证读法」migrate** |
| `seal-algo1-35-20-not-optimality-20261009.md` | 混读风险表；CN/EN/TeX 已钉同一边界句 | **evidence parent**；本衍生不重写 seal／§3.2 |
| `paper-a-notes.md` | 「算法 1 的复杂度与最优性差距」仍为投稿前应当补强；逐表对比与运行时间未完成 | **开放实验宿主**（仍属 A 披露义务，非新根） |
| tip 已登记衍生 | trapdoor／partial-fn／locality／combo／width／finiteness／table4／table5／bootstrap／平台矩阵钉 | **正交**；不 redo |

**不切：** 根主张（构造非训练、T1、神经编译器命名）、表 1 键数、§3.2 披露数字本身与 seal 句（仅引用、不重写）、表 4/5、自举、偏函数、局部性、阶段合并、宽度竞赛、活板门、目标参数化、有限性、重测身份决策卡 Ο1/Ο2。

---

## 为何不挡 A 投稿

1. **A 根主张不依赖最优性定理。** 核心是构造法 + T1；缩参数字是诚实成本披露，不是「算法 1 已证最优」的存在性条件。
2. **seal 已明确禁止 Algo1-as-optimality-theorem。** 2026-10-09 已把 35→20 与 796k→71k 钉为不可混读；审稿人仍可能把相邻句拔高为最优性——故升格登记衍生以卸负担。
3. **复杂度／最优性差距本就是 A 的开放补强项，不是新理论根。** 若日后起「贪心 vs 精确」短文，父节点仍回 A；结果可收窄披露措辞，不能长成第二套编译器理论。
4. **本拍零正文 diff。** 不改正文 CN/EN/TeX/abstract、不改键数、不重写 seal 或 §3.2；只登记。

---

## 父节点如何回 A

- **披露回 A：** §3.2 只保留构造路径上的诚实缩参／枚举对照 + T1；衍生只搬走「须证最优性定理」升格读法。
- **开放实验回 A：** 逐表贪心-vs-精确与贪心运行时间仍列在 `paper-a-notes.md` 补强清单；完成时服务 A 披露，不起第二根。
- **DAG：** A →（未来）Algo1／greedy-vs-exact optimality-gap note；**不是**第二根；与 width-W*（宽度竞赛／A2 RQ1）正交。

---

## 切后 A 哪一句更硬

切出后，A 可把算法 1 叙事收成一句硬边界（**文案已由 seal 落地；本拍不重写正文**）：

> **§3.2 的 35→20 仅是立方体表示下五小表精确枚举合计；796,490→71,828 是全套 18 阶段贪心覆盖缩参。二者对象不同，不可互证。本文不主张、亦不证明算法 1 或其贪心路径为最优；最优性差距与贪心复杂度分析仍开放。A 的硬主张仍是构造法与 T1，外加诚实成本披露。**

攻击面从「是不是在证明算法最优／缩参已证最小？」缩回 **「构造 + T1 + 诚实披露」**，与根主张正交；与既有 seal-algo1 文案封口同向。

---

## 衍生课题提案（不起稿）

| 项 | 内容 |
| --- | --- |
| **暂定题** | Algorithm 1 Greedy Cover vs Exact Enumeration: Cost Disclosure, Not an Optimality Theorem |
| **一行主张** | Neither the 35→20 exact-enumeration aggregate on five small tables nor the 796,490→71,828 greedy parameter reduction on the full 18-stage suite is a Paper A optimality theorem; Paper A only keeps construction + T1 + honest cost disclosure. Formal greedy-vs-exact / runtime-gap study, if drafted, hangs back to A. |
| **上游** | Paper A（T1；构造；[`seal-algo1-35-20-not-optimality-20261009.md`](../seal-algo1-35-20-not-optimality-20261009.md)；`paper-a-notes.md` 复杂度／最优性差距开放项） |
| **协议指针** | Algo1≠最优性定理已封于 seal-algo1；本课题若起稿不得改 A 键数或 §3.2 seal 句；不得把 35→20 写成 796k→71k 的证书 |
| **状态** | **note only** — 无独立 TeX/中英草稿；不重写 seal-algo1／§3.2 |

---

## 与既有衍生／硬缺口的正交

| 主题 | 关系 |
| --- | --- |
| 测量身份 8509/8769 | **仍开硬缺口**；本拍只引用 12:39 钉，不重论 |
| 同身份平台矩阵 | **仍开**；引用 16:45 钉，不触 |
| 重测身份决策卡 Ο1/Ο2 | **正交**；本拍**不叠、不代裁** |
| trapdoor（~19:15 E／PR #55） | 正交（不可达性标签 vs 最优性标签） |
| width-W*／combo／locality／partial-fn／finiteness／table4／table5／bootstrap | 正交 |
| seal-algo1-35-20-not-optimality（2026-10-09） | **父证据**：文案已封；本拍是 derive-purify 升格／tip 重登记，**不重写 seal／§3.2** |
| Softguess | **NONE×4**；非硬缺口 |
| 丢失集其余项（maint／bdiff／DENSE／verification…） | **后续拍续切**；本拍只重登记 Algo1 |

---

## 明确不做

- 不改正文 CN/EN/TeX/abstract（含**不重写** §3.2 seal 句）
- 不改键数身份 8509/8769/9174；不改 product/kernel/weights/facts
- 不起稿；不叠决策卡 Ο1/Ο2；不重钉 8509/8769
- 不发明新定理句塞回 A；不把 35→20 与 796k→71k 写成互证
- **不重做**已封口的 seal-algo1-35-20-not-optimality
- 本拍不顺手重登记 maint／bdiff／DENSE／verification（按优先序只做 Algo1）

---

## 证据

- tip（before）：`ab44d66bfe05fee5ec551de35f8a27ebd24bcedc`
- Paper A blobs：CN `f40070830bc44538ac15c3734dcc8257a8e3d7ee` · EN `7878d8072a35438db9653cb21c257b362c7a0665` · TeX `eacae1a1dafa8b4d81eaaa4283d73e1792636a48` · abs `43d7ea7e7297ed8be9a96e3950e014e359467973`（本 tip；本拍零正文 diff → **SAME**）
- Softguess：**NONE×4**
- 先验：`research/seal-algo1-35-20-not-optimality-20261009.md`；`research/paper-a-notes.md`（算法 1 复杂度／最优性差距开放项）；`research/paper-notes-20261009.md` 封口 Algo1 行；CN/EN/TeX §3.2
- 短报：[`paper-a-heartbeat-report-20261010-1930.md`](paper-a-heartbeat-report-20261010-1930.md)
- 父节点：**A**

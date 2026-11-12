# Paper A 衍生净化：发布身份台账 ≠ tape 层清空定理／镜像闭合定理

- **拍点：** 2026-10-10 ~18:26 Asia/Shanghai（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `436ec713238c08f0ffef38909f64a4ff564c58c8`
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；§5.5 / §5.7 / §8.1 具名差异两层披露）→ 可选旁支 **发布身份逐字节台账闭合协议／六目标镜像核对笔记** → 最终仍回 **A**
- **状态：** **note only**（登记未来独立「发布身份台账闭合／tape≠ledger／六目标镜像确认公理」命题；**不起稿正文**；**不改** A 根主张、键数身份、§5.5/§5.7 单元格与具名债名单、product/kernel/weights/facts；**不重写** [`../seal-bdiff-ledger-vs-tape-20261009.md`](../seal-bdiff-ledger-vs-tape-20261009.md)）
- **Softguess：** tip 四文件 **NONE×4**（本拍机扫：`research/unisacc-paper.md` / `unisacc-paper.en.md` / `arxiv-paper-a/main.tex` / `arxiv-paper-a/abstract.txt` 均 0 hit）
- **测量身份：** 8509/8769 **仍开** — 引用既有钉 [`paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)；本拍**不重钉、不叠**；**不叠** ~17:08 决策卡 Ο1/Ο2/Ο3
- **相邻已封／已切（本拍不重做）：**
  - [`../seal-bdiff-ledger-vs-tape-20261009.md`](../seal-bdiff-ledger-vs-tape-20261009.md)（**父证据**：v0.0.14 发布身份台账开 vs §5.7 v0.0.20 开发候选 tape 层清空；两层不可混读）
  - [`paper-a-derive-purify-product-maturity-not-theory-20261010-1801.md`](paper-a-derive-purify-product-maturity-not-theory-20261010-1801.md)（产品成熟度／门禁；**正交**：语料成熟度 vs 台账闭合协议）
  - [`paper-a-derive-purify-maint-surface-not-trend-20261010-1812.md`](paper-a-derive-purify-maint-surface-not-trend-20261010-1812.md)（维护面快照≠趋势；**正交**：行数台账 vs 字节差异台账）
  - [`paper-a-seal-section81-latest-drift-20261010-1659.md`](paper-a-seal-section81-latest-drift-20261010-1659.md)（冻结身份 vs Latest 漂移；**正交**：重测身份选择 ≠ 本拍「tape＝ledger 闭合」）

---

## 主张一句（本页唯一）

**把「§5.7 开发候选上 tape 层与参考逐字节相同」升成「v0.0.14（或任一发布身份）台账行已闭合／六目标镜像已证明一致」的定理——或反过来用「台账仍开」否定构造法／T1——不得搭乘 A 投稿。A 只需两层诚实披露（发布身份义务开着；候选 tape 进展另记）；台账闭合公理与六目标镜像确认协议留给独立测量／工程旁支。**

---

## 切什么（从 A 的主张负担里切出）

切出对象是 **把 §5.5 / §5.7 / §8.1 第 4 条混读成「已证发布身份字节等价」或「tape 清空＝台账闭合定理」**，从而在「台账仍开两项债」或「镜像尚未候选核对」时被挑战者用来否定构造法或逐字节主张诚实：

| 稿内／笔记位点（tip `436ec713`） | 摘句要点 | 分类 |
| --- | --- | --- |
| §5.5 | v0.0.14 发布身份下 `b_compound`/`b_pp2` 仍开；只主张清单外已测输入 | **keep 发布身份义务 in A**；**「台账闭合公理」migrate** |
| §5.7 | v0.0.20 开发候选 tape 层清空；六目标镜像仍待候选核对 | **keep 候选进展披露 in A**；**「tape＝ledger／镜像已证」migrate** |
| §8 / Limitations | 两层脚注：发布身份台账仍开；候选 tape 不闭合台账行 | **keep 两层诚实**；**正式闭合协议 migrate** |
| §8.1 第 4 条 | tape 清空 ≠ 台账闭合；镜像确认前方可闭合 | **keep 开放义务句**；**闭合判据形式化 OUT** |
| `seal-bdiff-ledger-vs-tape` | 已 pin 两层语义 | **keep 文案封口**；本拍 **升格登记**，不重写 seal |

**不切：** 根主张（构造非训练、T1）、`b_compound`/`b_pp2` 具名债事实本身、§5.7 实测数字（264/264 等）、键数 8509/8769/9174、product maturity／maint-surface／Algo1／trapdoor／width／partial-fn／表4/5／自举／DENSE／目标参数化、重测身份决策卡 Ο1/Ο2/Ο3。

---

## 为何不挡 A 投稿

1. **A 根主张不依赖台账闭合定理。** 核心是构造法 + T1；逐字节主张本就可以按「已列清单之外」口径诚实表述，不必先证六目标镜像闭合。
2. **既有 seal 已把两层解耦。** 本拍是 derive-purify **升格登记**，把「闭合公理／镜像确认协议」整段从 A 理论负担卸下，不重测候选、不重写 seal。
3. **闭合协议可另开测量／工程旁支。** 何种条件算台账行闭合、镜像核对清单、与 Latest 漂移如何对齐，可挂旁支；结果可回来收窄 §5.5/§8.1 措辞强度，不能长成第二套理论。
4. **本拍零正文 diff。** 不改正文 CN/EN/TeX/abstract、不改键数；只登记（与 maint-surface E ~18:12、product-maturity E ~18:01 同模式）。

---

## 父节点如何回 A

- **理论句回 A：** 构造 + T1 + 具名差异两层披露仍是 A；台账是义务台账，不是第二根。
- **测量／工程旁支：** 可选挂「release-identity ledger closure protocol（tape ≠ ledger；six-target image confirm）」独立笔记；父边仍回 A（§5.5/§5.7/§8.1）。
- **DAG：** A（construction + T1；two-tier byte-diff disclosure）→（未来）bdiff ledger-closure study；与 product-maturity、maint-surface、§8.1 Latest-drift **正交**。

---

## 切后 A 哪一句更硬

切出后，A 可把 §5.5/§5.7/§8.1 叙事收成一句硬边界（**本拍不改正文；仅登记主张**）：

> **§5.5 登记的是发布身份上仍开的具名逐字节债；§5.7 登记的是开发候选上 tape 层与参考对齐的进展。后者不构成前者台账行已闭合，也不构成六目标镜像已证明一致。未完成镜像核对与台账闭合，不自动否定构造法或 T1；把 tape 清空升成发布身份等价定理，不得搭乘 A。**

攻击面从「台账还开着／你们不是说 tape 已经清了吗，主张是不是自相矛盾？」缩回 **「两层披露诚实；闭合协议另立」**；挑战者用未闭合的工程债攻击根主张时，A 不必把台账闭合定理当投稿前提来防守。

---

## 衍生课题提案（不起稿）

| 项 | 内容 |
| --- | --- |
| **暂定题** | Tape-Level Clearance Is Not Ledger Closure: Release-Identity Byte-Parity Protocols Belong Off Paper A’s Theory Claim |
| **一行主张** | Elevating development-candidate tape-level equality with the reference into a proved published-identity ledger closure (or six-target image identity) theorem is an independent measurement/process proposition, not required to submit Paper A; Paper A only needs construction + T1 + honest two-tier disclosure of open release-identity obligations vs candidate tape progress. |
| **上游** | Paper A（§5.5/§5.7/§8.1；[`seal-bdiff-ledger-vs-tape-20261009.md`](../seal-bdiff-ledger-vs-tape-20261009.md)） |
| **协议指针** | 不起稿不得改 A 键数或 T1；不得把 §5.7 数字写成 v0.0.14 台账闭合；不得与 8509-8769 身份钉或 Ο1/Ο2 混读 |
| **状态** | **note only** — 无独立 TeX/中英草稿；不改正文；**不重跑** exec-chain／镜像核对 |

---

## 与既有衍生／硬缺口的正交

| 主题 | 关系 |
| --- | --- |
| 测量身份 8509/8769 | **仍开硬缺口**；键数身份 ≠ 本拍字节差异台账 |
| 同身份平台矩阵（~16:45 C） | **仍开硬缺口**；平台执行矩阵 ≠ 具名前端债台账闭合 |
| 重测身份决策卡 Ο1/Ο2/Ο3（~17:08 B） | **正交**；本拍**不叠、不代裁** |
| §8.1 Latest 漂移（~16:59） | **正交**：冻结 vs Latest 披露 ≠ tape＝ledger |
| product-maturity（~18:01 E） | **正交**：RQ1/Lua/门禁 vs 台账闭合协议 |
| maint-surface（~18:12 E） | **正交**：维护面行数快照 vs 字节差异台账 |
| Algo1／trapdoor／width／partial-fn／finiteness／table4/5／bootstrap／DENSE／target-param／combo | 正交 |
| Softguess | **NONE×4**；非硬缺口 |

---

## 明确不做

- 不改正文 CN/EN/TeX/abstract（含**不删** §5.5/§5.7 两层句、**不重写** seal）
- 不改键数身份 8509/8769/9174；不改 product/kernel/weights/facts
- 不起稿；不叠决策卡 Ο1/Ο2/Ο3；不重钉 8509/8769
- **不重跑** exec-chain／六目标镜像；**不重写** seal-bdiff-ledger-vs-tape
- 不发明「台账已闭合」或「必须先闭合才能投」新句塞回 A
- 不与 ~18:12 maint-surface／~18:01 product-maturity 合并；不叠 Softguess／weight≡logic 卡

---

## 证据指针（本拍）

- tip before：`436ec713238c08f0ffef38909f64a4ff564c58c8`
- 四 blob（本拍零正文 diff）：CN `74893e3df1f9663b5d11f5bad6dc3cf0ca59e884` · EN `95657738e0b447ee4e40ea9ca3dd961329aebffc` · TeX `3830ede13556e8c9673f994fd6d843937476ecf5` · abs `43d7ea7e7297ed8be9a96e3950e014e359467973`
- Softguess 机扫：NONE×4
- 父 seal：[`../seal-bdiff-ledger-vs-tape-20261009.md`](../seal-bdiff-ledger-vs-tape-20261009.md)（仅引用）
- 短报：[`paper-a-heartbeat-report-20261010-1826.md`](paper-a-heartbeat-report-20261010-1826.md)

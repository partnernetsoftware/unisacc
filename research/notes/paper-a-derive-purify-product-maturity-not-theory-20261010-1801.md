# Paper A 衍生净化：产品成熟度／RQ1·Lua·门禁 ≠ 构造法／T1 理论定理

- **拍点：** 2026-10-10 ~18:01 Asia/Shanghai（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `e31eaddaf99513eb2b7f707f45b1168068098268`
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；unisacc 为实证载体）→ 可选旁支 **工程／产品实证笔记**（语料棘轮、门禁、真实程序成熟度）→ 最终仍回 **A**
- **状态：** **note only**（登记未来独立「产品成熟度／TDD·语料棘轮 ≠ 构造定理」命题；**不起稿正文**；**不改** A 根主张、键数身份、§5/§7.1/§7.4 实证段、product/kernel/weights/facts；**不重写** [`../seal-combo-fail-not-t1-20261009.md`](../seal-combo-fail-not-t1-20261009.md)）
- **Softguess：** tip 四文件 **NONE×4**（本拍机扫：`research/unisacc-paper.md` / `unisacc-paper.en.md` / `arxiv-paper-a/main.tex` / `arxiv-paper-a/abstract.txt` 均 0 hit）
- **测量身份：** 8509/8769 **仍开** — 引用既有钉 [`paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)；本拍**不重钉、不叠**；**不叠** ~17:08 决策卡 Ο1/Ο2/Ο3
- **相邻已封／已切（本拍不重做）：**
  - [`../seal-combo-fail-not-t1-20261009.md`](../seal-combo-fail-not-t1-20261009.md) + §7.4：构造组合产品失败不否定 T1
  - [`paper-a-derive-purify-combo-merge-theorem-20261010-1552.md`](paper-a-derive-purify-combo-merge-theorem-20261010-1552.md)（阶段合并定理 OUT）
  - [`../seal-maint-surface-not-trend-20261009.md`](../seal-maint-surface-not-trend-20261009.md)（维护面快照 ≠ 缩小定理；命题相邻但正交：维护面趋势 vs 产品成熟度／RQ1）

---

## 主张一句（本页唯一）

**把「方法支撑真实编译器」（RQ1）、Lua 5.4 onelua 语料棘轮、或 N/N 本地门禁通过，升成构造法／T1 的理论定理——或主张产品未达 SQLite／Lua 级成熟则 A 根主张不成立——不得搭乘 A 投稿。A 只需构造法 + T1（已造表 → 精确权重）；真实程序与门禁是 unisacc 实证载体的工程证据，不是第二套理论。**

---

## 切什么（从 A 的主张负担里切出）

切出对象是 **把产品成熟度读成 A 必须证明的理论义务**，从而在「某真实程序仍失败／门禁未齐／尚未替代常见编译器」时被挑战者用来否定构造法或 T1：

| 稿内／笔记位点（tip `e31eadda`） | 摘句要点 | 分类 |
| --- | --- | --- |
| §7 开篇 RQ1（CN/EN） | 「方法能否支撑一个真实的编译器？」 | **keep RQ1 作为实证问题 in A**；**「RQ1 通过 = 构造定理／T1 成立条件」migrate** |
| §5.x / §7.1 门禁数字 | v0.0.9 193/193、v0.0.19 415、v0.0.20 418 等本地门禁 | **keep 有限测试证据 in A**；**「门禁全绿 = 理论完备／C99 定理」migrate** |
| §5.8 Lua onelua | 真实程序继续找缺陷；语料棘轮多过一个程序 | **keep 工程发现叙述 in A**；**「通过 Lua = 理论证明可替代常见编译器」migrate** |
| 贡献点 4 / 结论 | 验证纪律与经验；构造+枚举+差分可迁移 | **keep §7.4 经验叙述**；与「产品成熟度＝理论」正交（验证纪律另可升方法枝，本拍不切 RQ4 全文） |
| `seal-combo-fail-not-t1` | 组合失败不否定 T1 | **evidence parent**；本衍生把「成熟度门槛」从理论负担卸下 |

**不切：** 根主张（构造非训练、T1、神经编译器命名）、表 1 键数、§5/§7 里具体门禁与 Lua 事实句（仅引用、不删实证）、Algo1／trapdoor／width／partial-fn／表4/5／自举／目标参数化、重测身份决策卡 Ο1/Ο2/Ο3、维护面 seal（正交）。

---

## 为何不挡 A 投稿

1. **A 根主张不依赖产品成熟度定理。** 核心是构造法 + T1；unisacc 是实证载体。某版门禁未齐或某真实程序仍失败，说明载体工程未完成，不自动推翻「已造表上网络=表」。
2. **§7.4 / seal-combo-fail-not-t1 已把组合失败与 T1 解耦。** 成熟度门槛若仍搭乘「理论是否成立」，会把已解耦的工程债重新绑回根主张。
3. **成熟度可另开工程／产品笔记。** 语料棘轮协议、与 gcc/cc 对拍矩阵、SQLite 级 workload 路线，可挂产品实证或未来应用枝；结果可回来收窄 A 的「支撑真实编译器」措辞强度，不能长成第二套编译器理论。
4. **本拍零正文 diff。** 不改正文 CN/EN/TeX/abstract、不改键数；只登记（与 Algo1 E ~17:43、trapdoor E ~17:27 同模式）。

---

## 父节点如何回 A

- **理论句回 A：** 构造 + T1 仍是 A 的硬主张；RQ1/门禁/Lua 仍是载体实证，不是第二根。
- **工程旁支：** 可选挂「产品成熟度／TDD 语料棘轮」独立笔记或产品路线文档；父边仍回 A（unisacc 实证）。
- **DAG：** A（construction + T1；unisacc carrier）→（未来）product-maturity / corpus-ratchet study；与 maint-surface（维护面趋势）正交。

---

## 切后 A 哪一句更硬

切出后，A 可把 RQ1／真实程序叙事收成一句硬边界（**本拍不改正文；仅登记主张**）：

> **RQ1、本地门禁与 Lua 等真实程序是 unisacc 实证载体的工程证据：它们支持「方法在某一发布身份上走到了多远」的叙述，不构成「构造法／T1 成立当且仅当产品达某成熟门槛」的定理。产品未过某语料或门禁，不自动否定已发布网络在声明域上等于其表。**

攻击面从「还没过 SQLite／Lua／全部门禁，理论是不是空的？」缩回 **「构造 + T1；实证另计」**；挑战者用未完成的产品债攻击根主张时，A 不必把成熟度当理论前提来防守。

---

## 衍生课题提案（不起稿）

| 项 | 内容 |
| --- | --- |
| **暂定题** | Product Maturity and Corpus Ratchets Are Not Theorems of Constructed Compilers: RQ1/Lua/Gates Belong Off Paper A’s Theory Claim |
| **一行主张** | Elevating RQ1 (“supports a real compiler”), Lua/onelua corpus ratchets, or N/N local gates into a theory theorem of construction/T1—or claiming A’s root fails until SQLite-class product maturity—is an independent engineering proposition, not required to submit Paper A; Paper A only needs construction + T1, with real-program evidence kept as carrier empirics. |
| **上游** | Paper A（T1；unisacc 实证；[`seal-combo-fail-not-t1-20261009.md`](../seal-combo-fail-not-t1-20261009.md)） |
| **协议指针** | 不起稿不得改 A 键数或 T1；不得把「未过某语料」写成 T1 反例；不得与 maint-surface 缩小定理混读 |
| **状态** | **note only** — 无独立 TeX/中英草稿；不改正文 |

---

## 与既有衍生／硬缺口的正交

| 主题 | 关系 |
| --- | --- |
| 测量身份 8509/8769 | **仍开硬缺口**；本拍只引用 12:39 钉，不重论 |
| 同身份平台矩阵（~16:45 C） | **仍开硬缺口**；实证测量义务 ≠ 本拍「成熟度＝理论」 |
| 重测身份决策卡 Ο1/Ο2/Ο3（~17:08 B） | **正交**；本拍**不叠、不代裁** |
| combo-fail-not-t1 / combo-merge | **相邻**：组合失败不否定 T1；本拍把「成熟度门槛」整段卸下 |
| maint-surface-not-trend | **正交**：维护面缩小趋势 vs 产品语料／门禁成熟度 |
| Algo1／trapdoor／width／partial-fn／finiteness／table4/5／bootstrap／target-param | 正交 |
| Softguess | **NONE×4**；非硬缺口 |

---

## 明确不做

- 不改正文 CN/EN/TeX/abstract（含**不删** §5 Lua／门禁事实句、**不重写** RQ1 标题）
- 不改键数身份 8509/8769/9174；不改 product/kernel/weights/facts
- 不起稿；不叠决策卡 Ο1/Ο2/Ο3；不重钉 8509/8769
- 不发明「已达成熟」或「必须先过 SQLite」新句塞回 A
- **不重做** seal-combo-fail-not-t1；不与 ~16:29 finiteness 合并
- 不叠 Softguess／weight≡logic 卡；不切 RQ4 验证纪律全文为本拍范围

---

## 证据指针（本拍）

- tip before：`e31eaddaf99513eb2b7f707f45b1168068098268`
- 四 blob（本拍零正文 diff，应与 PR #47–#49 后 SAME）：CN `74893e3d` · EN `95657738` · TeX `3830ede1` · abs `43d7ea7e`
- Softguess 机扫：NONE×4

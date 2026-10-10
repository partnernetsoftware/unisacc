# Paper A 衍生净化：算法 1 五小表贪心-vs-精确逐表对比 + 贪心运行时间完稿不得搭乘 A

- **拍点：** 2026-10-11 ~04:12 Asia/Shanghai（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `0d47a07de25a5ed824733ac6c3c39fef43cb4699`
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表；unisacc 为 POSIX C99 子集跨架构实证载体）→ 未来 **Algo1 成本披露旁支**（五小表贪心-vs-精确逐表对比 + 贪心运行时间测量完稿）→ 仍回 **A**
- **状态：** **note only**（登记「须先完成算法 1 五小表贪心-vs-精确逐表对比 + 贪心运行时间测量完稿，才许投 A／构造+T1 才成立／缺逐表对比则根主张或成本披露不完整」升格 ≠ 理论／投稿闸门；**不起稿**独立逐表／runtime 正文；**不改** A 根主张、键数身份、CN/EN/TeX/abstract、§3.2 文案、seal-algo1 文案、product/kernel/weights/facts；**不发明**逐表数字；**不跑**实验；**不叠** Ο1/Ο2；**不重做** algo1-not-optimality-1930、seal-algo1-35-20-not-optimality、width-W*、ablation、fig2、format/venue、external-referee、maint、c99-seed、clause-ledger、five-vs-six、platform-matrix program、cx-lab、A2、product-maturity、TDD 等已切项）
- **Softguess：** tip 四文件 **NONE×4 产品路径**（本拍机扫：CN `74893e3d…` · EN `95657738…` · TeX `3830ede1…` · abs `43d7ea7e…`；四文件 Softguess 字面 0 hit，无 Softguess 产品路径升格）
- **测量身份：** 8509/8769 **仍开** — 仅引用既有钉；本拍**不重钉、不叠**；**不叠** Ο1/Ο2
- **Paper A 正文漂移：** tip 四 blob **SAME**（相对 ~03:58 封口机扫基线；本拍零正文 diff）
- **搭乘风险源（本拍钉）：** [`paper-a-notes.md`](../paper-a-notes.md)「**投稿前应当补强**」→「**算法 1 的复杂度与最优性差距**…给出贪心覆盖的运行时间，以及 5 个小表上贪心与精确最小值的逐表对比（现在只有合计 35 → 20）…逐表对比与运行时间**仍未完成**」——开放补强实验 ≠ 「须先完稿才许投 A／缺之则根主张或成本披露不完整」闸门。
- **相邻已封／已切（本拍不重做；正交声明）：**
  - [`paper-a-derive-purify-algo1-not-optimality-20261010-1930.md`](paper-a-derive-purify-algo1-not-optimality-20261010-1930.md)（切出「35→20／796k→71k＝最优性定理」读法；**正交**：定理读法 ≠ 「须先完成逐表对比+runtime 完稿＝理论／投稿闸门」）
  - [`../seal-algo1-35-20-not-optimality-20261009.md`](../seal-algo1-35-20-not-optimality-20261009.md)（§3.2 已钉：35→20＝五小表精确枚举合计；796k→71k＝全套 18 阶段贪心缩参；不升格最优性定理；逐表与 runtime **仍开放实验**——开放 ≠ 闸门）
  - [`paper-a-derive-purify-width-complexity-wstar-20261010-1615.md`](paper-a-derive-purify-width-complexity-wstar-20261010-1615.md)（宽度竞赛／W*；**正交**）
  - [`paper-a-derive-purify-ablation-not-a-gate-20261011-0316.md`](paper-a-derive-purify-ablation-not-a-gate-20261011-0316.md)／[`paper-a-derive-purify-fig2-pipeline-diagram-not-a-gate-20261011-0338.md`](paper-a-derive-purify-fig2-pipeline-diagram-not-a-gate-20261011-0338.md)／[`paper-a-derive-purify-format-venue-bib-anon-not-a-gate-20261011-0358.md`](paper-a-derive-purify-format-venue-bib-anon-not-a-gate-20261011-0358.md)／[`paper-a-derive-purify-external-referee-coverage-20261011-0052.md`](paper-a-derive-purify-external-referee-coverage-20261011-0052.md)（论证／格式补强其它项；**不重做**）
  - 平台矩阵 §8.1／8509-8769／Ο1/Ο2 — **仍开硬缺口**；本拍**不混读**为「缺逐表对比」理论否决

---

## 主张一句（本页唯一）

**把「必须先完成算法 1 五小表贪心-vs-精确逐表对比 + 贪心运行时间测量完稿，才许投 A／构造+T1 才成立／缺逐表对比则根主张或成本披露不完整」——不得搭乘 A。A 只保留构造 + T1 + 诚实 §3.2 成本／缩参披露（35→20 精确合计；796k→71k 贪心；seal 已禁最优性定理）；逐表对比与贪心 runtime 完稿是挂回 A 的成本披露旁支，不是理论／投稿闸门，也不是第二套编译器理论。本拍与 algo1-not-optimality-1930（定理读法）正交。**

---

## 切什么（从 A 的主张负担里切出）

切出对象是 **把 `paper-a-notes` 开放项「逐表对比与运行时间仍未完成」升成 A 理论完备或投稿闸门**，从而在「尚未交出五小表逐表贪心-vs-精确表、尚未测量贪心运行时间」时被挑战者用来否定构造法／T1／可投稿性／成本披露完整性：

| 位点 | 摘句／现状要点 | 分类 |
| --- | --- | --- |
| `paper-a-notes.md` 应当补强 · Algo1 | 「给出贪心覆盖的**运行时间**，以及 5 个小表上贪心与精确最小值的**逐表对比**（现在只有合计 35 → 20）…逐表对比与运行时间**仍未完成**」 | **keep 开放实验愿望 in notes**；切出「须先逐表+runtime 完稿＝理论／投稿闸门」 |
| §3.2 / seal-algo1 | 35→20 精确合计；796k→71k 贪心；已禁最优性定理 | **keep 诚实披露 in A**（本拍不改正文／不重写 seal） |
| algo1-not-optimality-1930 | 切出「数字＝最优性定理」 | **正交已切**；本拍不重做 |
| A 根主张／T1 | 构造网络；声明域精确；unisacc 实证 | **keep in A** |
| 仍开封口债 | §8.1 矩阵；8509/8769；Ο1/Ο2 | **keep seal-blocking**；**≠** 「缺逐表对比」理论否决 |

**不切：** 根主张、键数 8509/8769/9174、CN/EN/TeX/abstract、§3.2 披露数字与 seal 句（仅引用）、逐表／runtime 开放实验本身（可做，非闸门）、§8.1 同身份矩阵硬缺口、Ο1/Ο2、algo1-not-optimality／seal-algo1／width-W*／ablation／fig2／format／裁判／maint／种子 TCB／clause-ledger／五 vs 六／平台程序／cx-lab／A2／product-maturity／TDD 已切项、product/kernel/weights/facts。

---

## 为何不挡 A 投稿（切出项本身）

1. **A 根主张是构造＋T1，不是「已交出逐表贪心-vs-精确表／已测完贪心 runtime」。** 缺逐表细化不否定网络≡表、也不否定 §3.2 已披露的合计缩参。
2. **开放补强清单 ≠ 理论闸门。** `paper-a-notes` 把逐表与 runtime 列在「投稿前应当补强」——是成本披露细化愿望；seal 与 1930 已明确合计披露足够支撑「非最优性定理」边界，不能反向读成「必须先做完逐表才许谈根主张／成本披露才完整」。
3. **与已切最优性定理读法正交。** 1930 切的是「把 35→20／796k→71k 读成最优性定理」；本拍切的是「把『仍未完成』升成 seal-before／理论完备闸门」。二者不得混读、不互替。
4. **与仍开硬缺口正交。** §8.1 矩阵／8509-8769／Ο1/Ο2 仍是 A 封口债；逐表／runtime **不能替代**也不能**冒充**这些测量债已闭。
5. **本拍零 Paper A 正文 diff。** 只 notes + registry；四 blob 不改；**不**发明逐表数字；**不**跑实验；**不**重写 seal／§3.2。

---

## 父节点如何回 A

- **披露回 A：** 任何未来逐表贪心-vs-精确表或贪心 runtime 测量，仍服务 A 已有 §3.2 诚实成本／缩参披露——细化服务主张，不颠倒为「细化先于主张／先于可投」。
- **旁支回 A：** 父边 **A（construction + T1 + §3.2 诚实合计披露 + seal 禁最优性定理）→ Algo1 逐表／runtime 成本披露旁支**；结果可回来收紧 notes「应当补强」措辞与披露表，不能长成与 A 并列的第二套编译器理论，也不能把「缺逐表完稿」回写成理论投稿前提。
- **DAG：** A 在主张与测量债收口、政委点头后可独立投／公开；逐表／runtime 可并行；**切出的是完稿升格闸门，不是删掉 notes 开放实验提醒，也不是降级仍开测量债，也不是代裁公开**。

---

## 切后 A 哪一句更硬

切出后，A 可把 Algo1 成本披露边界再收一句（**本拍不改正文；仅登记主张**）：

> **Paper A 只主张基于神经网络（构造网络与权重）的编译器与 T1，并以 unisacc 为实证；§3.2 已诚实披露 35→20 精确合计与 796k→71k 贪心缩参，且 seal 禁止最优性定理。「必须先完成五小表贪心-vs-精确逐表对比 + 贪心运行时间测量完稿才可投／缺之则根主张或成本披露不完整」不是理论／投稿闸门——该旁支若独立成题，挂回 A，不起稿挡粘；不替代 §8.1／8509-8769／Ο1/Ο2，也不重做 1930 最优性读法切出。**

攻击面从「你们还没交逐表对比／还没测贪心 runtime，成本披露空／不能谈封口？」缩回 **「A = 构造 + T1 + 已有合计诚实披露；逐表／runtime 细化另挂」**。

---

## 衍生课题提案（不起稿）

| 项 | 内容 |
| --- | --- |
| 题名意向 | Algo1 per-table greedy-vs-exact + greedy runtime: cost-disclosure side branch (hangs on A) |
| 父节点 | A（经 construction + T1 + §3.2 诚实合计披露 + seal-algo1 禁最优性定理） |
| 与 A 关系 | 可回来收紧成本披露表与 notes 补强项；不能另起第二根；不能把「缺逐表／runtime 完稿」写成理论闸门；**不**替代 §8.1 矩阵／8509-8769／Ο1/Ο2；**不**重做 1930；**不**发明数字／改正文 |
| 本拍动作 | **不起稿**；不改正文；不发明逐表数字；不跑实验；不重写 seal／§3.2；不叠 Ο1/Ο2 |

---

## 明确非目标

- 不解决、不重钉、不降级 §8.1／8509-8769／Ο1/Ο2；**不**代裁公开／arXiv
- 不改正文键数、表 1/4/5、摘要主张方向、§3.2 散文；不改 product/kernel/weights/facts
- 不发明逐表数字；不跑贪心／精确实验；不重写 seal-algo1
- 不重做 algo1-not-optimality-1930／width-W*／ablation／fig2／format-venue／external-referee／maint／verif→C／A2／平台程序／五 vs 六／种子 TCB／clause-ledger／cx-lab／T3／bootstrap-FP／B／Softguess／wasm／C／D／product-maturity／TDD
- 不删 `paper-a-notes`「逐表对比与运行时间仍未完成」开放提醒（仅登记 GATE 升格禁令 + 链本笔记）
- 不把 Softguess／近似推断升成产品路径；本拍确认 tip **NONE×4**

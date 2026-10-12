# Paper A 成篇定稿增量：贡献#1「方法」对齐摘要／§1／§10「精确可执行形式／非学得近似」

- **拍点：** 2026-10-12 ~22:56 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `bc917a195ad789e1899c0493f17d15828c2c50ff`（短 `bc917a19`；= #326 after Cut A 2217 tip-after；相对任务书 tip **无漂**）
- **tip after：** `19672d05e10ee74e548df5390f79f2ddded83979`（短 `19672d05`；= squash Merge #327 本 A）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 $K_s$；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only **贡献列表第 1 条（方法）** CN+EN+TeX 同义收紧，对齐已硬的摘要／§1／§10「精确可执行形式／非学得近似」；**abstract.txt SAME**（无贡献列表；摘要刚在 2217 收紧，本拍不重改）；**不改**键数身份（8509/8769/9174）、product/kernel/weights/facts、投稿方向；**不叠** Ο1（仍荐）；**不重钉** Softguess／§8.1#3／8509·8769；**不**粘 Softguess 口号字面
- **Softguess：** tip 四文件 **NONE×4**（BEFORE+AFTER 机扫 `Softguess`/`softguess`/`SoftGuess`/`权重就是`/`weights are the logic`/`逻辑本身`/`权重即逻辑`/`权重≡逻辑` 均 0 hit；**未**粘 Softguess 口号）
- **Latest：** **v0.0.41**；冻结重测身份仍 **v0.0.19**；tip version **0.0.42**（本拍不 bump）；Draft 仅 **v0.0.31**

---

## 主张一句（本页唯一）

**贡献点第 1 条「方法」把「权重由表构造而非训练（不是学得的统计替代）」收紧为「权重由表构造而非训练——在声明域上给出表的精确可执行形式（与表逐键同一），不是学得的近似／统计替代」——与已硬的摘要／§1／§10 同框；保留 TSV→构造→枚举框架与 T1／模型检验发布判据；堵住贡献列表仍可被读成「仅否定统计替代、未声明可执行形式非近似」的攻击面；同一实质改动同步进 CN/EN/TeX；abs 有意 SAME；不引入 Softguess 口号字面。**

---

## 路由（为何不是假 A / 非 E / 非 C / 非 B）

- **非假 A（Latest 披露）：** tip §8.1 已写公开 Latest=**v0.0.41**；本拍不重复披露。
- **非 C：** 相对 2016／1928 无新 gold／pin；再 tip-refresh 8509/8769＝巡检式 FAIL。
- **非 B：** 相对 1914 Ο1 **无新信息**；叠卡禁。
- **非 E：** 上拍 2132 已切 c42 acceptance≠seal；本拍有可核对 **贡献#1** phrasing 增量 → 优先 **A**。
- **DISTINCT vs 2217 A：** 2217 收紧 **摘要核心主张句**；本拍只动 **贡献#1 方法**，摘要不动（abs SAME）。
- **DISTINCT vs 2054 A：** 2054 收紧 **§10 开句**；本拍不动 §10。
- **DISTINCT vs 0956 A：** 0956 收紧 **§1**；本拍不动 §1。
- **DISTINCT vs 1011/1026：** 那些是 §7.4／结论验证纪律句；本拍只动贡献#1。

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md` 贡献#1

- **Before：** `1. **方法。** 一种“TSV 表 DSL → 网络构造 → 枚举”的方法：决策被表示为有限定义域上的全函数；权重由表构造而非训练（不是学得的统计替代）；发布判据是声明域上穷举判定的逐键相等（T1／模型检验），不是相对样本的拟合精度或训练精度（§2、§3）。`
- **After：** `1. **方法。** 一种“TSV 表 DSL → 网络构造 → 枚举”的方法：决策被表示为有限定义域上的全函数；权重由表构造而非训练——在声明域上给出表的精确可执行形式（与表逐键同一），不是学得的近似／统计替代；发布判据是声明域上穷举判定的逐键相等（T1／模型检验），不是相对样本的拟合精度或训练精度（§2、§3）。`

### EN `research/unisacc-paper.en.md` Contributions #1

- **Before：** `1. **Method.** A "TSV table DSL → network construction → enumeration" method: decisions are total functions on finite domains; weights are constructed from tables rather than trained (not a learned statistical surrogate); the release criterion is key-by-key equality decided by exhaustion on the declared domain (T1 / model checking), not sample fit or training accuracy (§2, §3).`
- **After：** `1. **Method.** A "TSV table DSL → network construction → enumeration" method: decisions are total functions on finite domains; weights are constructed from tables rather than trained—on the declared domain they yield the table's exact executable form (identical key-by-key), not a learned approximation / statistical surrogate; the release criterion is key-by-key equality decided by exhaustion on the declared domain (T1 / model checking), not sample fit or training accuracy (§2, §3).`

### TeX `research/arxiv-paper-a/main.tex` Contributions item 1

- **Before：** `\textbf{Method.} A ``TSV table DSL → network construction → enumeration'' method: decisions are total functions on finite domains; weights are constructed from tables rather than trained (not a learned statistical surrogate); the release criterion is key-by-key equality decided by exhaustion on the declared domain (T1 / model checking), not sample fit or training accuracy (§2, §3).`
- **After：** `\textbf{Method.} A ``TSV table DSL → network construction → enumeration'' method: decisions are total functions on finite domains; weights are constructed from tables rather than trained---on the declared domain they yield the table's exact executable form (identical key-by-key), not a learned approximation / statistical surrogate; the release criterion is key-by-key equality decided by exhaustion on the declared domain (T1 / model checking), not sample fit or training accuracy (§2, §3).`

### abs `research/arxiv-paper-a/abstract.txt`

- **SAME**（无贡献列表；本拍有意不改摘要；2217 已收紧核心主张句）。

---

## 四 blob 状态（本拍后）

| 文件 | 相对 tip `bc917a19` |
| --- | --- |
| CN `research/unisacc-paper.md` | **DIFF**（仅贡献#1 方法） |
| EN `research/unisacc-paper.en.md` | **DIFF**（仅 Contributions #1 Method） |
| TeX `research/arxiv-paper-a/main.tex` | **DIFF**（仅 Contributions item Method） |
| abs `research/arxiv-paper-a/abstract.txt` | **SAME**（有意） |

（新 blob SHA 以 merge 后 `git`/`gh api` 为准，本页不臆造。）

Before blobs @ tip `bc917a19`：CN `b9d122995d9668809d12abb8a1f1028bf1345b31`；EN `a48d113c23ecf71ac94ff7fc35fbf765fc179720`；TeX `00f581d02e9e3881d00086f75acac9d0d653f4b4`；abs `67fdf36f98c34d7ed6048c28ea1d52250a97794f`。

---

## 挡粘仍开（本拍不闭合）

1. **§8.1#3** 平台矩阵（须先选定单一密封测量身份）
2. **8509/8769** 测量身份（活 gold op-87/8769/569 vs EN/TeX/abs op-74/8509/517）
3. **Ο1|Ο2** 投稿/封口重测身份（**仍荐 Ο1**；本拍**不叠**新卡；引用 1914）

---

## 登记

- paper-notes 追加本拍 A 行（见 `research/paper-notes-20261009.md` 末）
- 心跳短报：`research/notes/paper-a-heartbeat-report-20261012-2256.md`

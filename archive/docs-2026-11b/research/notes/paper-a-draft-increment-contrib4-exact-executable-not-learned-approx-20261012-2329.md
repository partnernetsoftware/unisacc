# Paper A 成篇定稿增量：贡献#4「验证纪律」对齐摘要／§1／贡献#1／§10「精确可执行形式／非学得近似」

- **拍点：** 2026-10-12 ~23:29 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `ee90239631ba04f3632df816518b7efc411e2174`（短 `ee902396`；= #328 after Cut A 2256 tip-after；相对任务书 tip **无漂**）
- **tip after：** `0f8e9c49bce572a8eb2fd08bd37b7d316ffc0658`（短 `0f8e9c49`；= squash Merge #329 本 A）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 $K_s$；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only **贡献列表第 4 条（验证纪律与经验）** CN+EN+TeX 同义收紧，对齐已硬的摘要／§1／贡献#1／§10「精确可执行形式／非学得近似」；保留「不是训练的替代品或逼近／distinct verification discipline」原框；**abstract.txt SAME**（无贡献列表；摘要核心主张句已在 2217 收紧，本拍不重改）；**不改**键数身份（8509/8769/9174）、product/kernel/weights/facts、投稿方向；**不叠** Ο1（仍荐）；**不重钉** Softguess／§8.1#3／8509·8769；**不**粘 Softguess 口号字面
- **Softguess：** tip 四文件 **NONE×4**（BEFORE+AFTER 机扫 `Softguess`/`softguess`/`SoftGuess`/`权重就是`/`weights are the logic`/`逻辑本身`/`权重即逻辑`/`权重≡逻辑` 均 0 hit；**未**粘 Softguess 口号）
- **Latest：** **v0.0.41**；冻结重测身份仍 **v0.0.19**；tip version **0.0.42**（本拍不 bump）；Draft 仅 **v0.0.31**

---

## 主张一句（本页唯一）

**贡献点第 4 条「验证纪律与经验」把括注「权重由表构造而非训练」收紧为「权重由表构造而非训练——在声明域上给出表的精确可执行形式（与表逐键同一），不是学得的近似」——与已硬的摘要／§1／贡献#1／§10 同框；保留「不是训练的替代品或逼近／distinct verification discipline」与分层验证／手写缺陷发现；堵住贡献列表仍可被读成「仅否定训练替代、未声明可执行形式非近似」的攻击面；同一实质改动同步进 CN/EN/TeX；abs 有意 SAME；不引入 Softguess 口号字面。**

---

## 路由（为何不是假 A / 非 E / 非 C / 非 B）

- **非假 A（Latest 披露）：** tip §8.1 已写公开 Latest=**v0.0.41**；本拍不重复披露。
- **非 C：** 相对 2016／1928 无新 gold／pin；再 tip-refresh 8509/8769＝巡检式 FAIL。
- **非 B：** 相对 1914 Ο1 **无新信息**；叠卡禁。
- **非 E：** 上拍 2132 已切 c42 acceptance≠seal；本拍有可核对 **贡献#4** phrasing 增量 → 优先 **A**。
- **DISTINCT vs 2256 A：** 2256 收紧 **贡献#1 方法**；本拍只动 **贡献#4 验证纪律**，贡献#1／摘要不动（abs SAME）。
- **DISTINCT vs 2217 A：** 2217 收紧 **摘要核心主张句**；本拍不动摘要。
- **DISTINCT vs 2054 A：** 2054 收紧 **§10 开句**；本拍不动 §10。
- **DISTINCT vs 0956 A：** 0956 收紧 **§1**；本拍不动 §1。
- **DISTINCT vs 1011/1026：** 那些是 §7.4／结论验证纪律句正文位；本拍只动贡献列表 #4。

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md` 贡献#4

- **Before：** `4. **验证纪律与经验。** 构造—枚举不是训练的替代品或逼近，而是一种与训练不同类的验证纪律（权重由表构造而非训练）；穷举枚举、逐字节差分与外部裁判的分层验证，以及把枚举用于手写纯函数时发现的、抽样无法发现的缺陷（§6、§7.4）。`
- **After：** `4. **验证纪律与经验。** 构造—枚举不是训练的替代品或逼近，而是一种与训练不同类的验证纪律（权重由表构造而非训练——在声明域上给出表的精确可执行形式（与表逐键同一），不是学得的近似）；穷举枚举、逐字节差分与外部裁判的分层验证，以及把枚举用于手写纯函数时发现的、抽样无法发现的缺陷（§6、§7.4）。`

### EN `research/unisacc-paper.en.md` Contributions #4

- **Before：** `4. **A verification discipline and its findings.** Construction plus enumeration is not a substitute for—or approximation of—training, but a distinct verification discipline (weights constructed from tables, not trained); layered verification by exhaustive enumeration, byte-level differential testing and external referees, including defects in hand-written code that enumeration found and sampling could not (§6, §7.4).`
- **After：** `4. **A verification discipline and its findings.** Construction plus enumeration is not a substitute for—or approximation of—training, but a distinct verification discipline (weights constructed from tables, not trained—on the declared domain they yield the table's exact executable form (identical key-by-key), not a learned approximation); layered verification by exhaustive enumeration, byte-level differential testing and external referees, including defects in hand-written code that enumeration found and sampling could not (§6, §7.4).`

### TeX `research/arxiv-paper-a/main.tex` Contributions item 4

- **Before：** `\textbf{A verification discipline and its findings.} Construction plus enumeration is not a substitute for---or approximation of---training, but a distinct verification discipline (weights constructed from tables, not trained); layered verification by exhaustive enumeration, byte-level differential testing and external referees, including defects in hand-written code that enumeration found and sampling could not (§6, §7.4).`
- **After：** `\textbf{A verification discipline and its findings.} Construction plus enumeration is not a substitute for---or approximation of---training, but a distinct verification discipline (weights constructed from tables, not trained---on the declared domain they yield the table's exact executable form (identical key-by-key), not a learned approximation); layered verification by exhaustive enumeration, byte-level differential testing and external referees, including defects in hand-written code that enumeration found and sampling could not (§6, §7.4).`

### abs `research/arxiv-paper-a/abstract.txt`

- **SAME**（无贡献列表；本拍有意不改摘要；2217 已收紧核心主张句）。

---

## 四 blob 状态（本拍后）

| 文件 | 相对 tip `ee902396` |
| --- | --- |
| CN `research/unisacc-paper.md` | **DIFF**（仅贡献#4 验证纪律） |
| EN `research/unisacc-paper.en.md` | **DIFF**（仅 Contributions #4） |
| TeX `research/arxiv-paper-a/main.tex` | **DIFF**（仅 Contributions item 验证纪律） |
| abs `research/arxiv-paper-a/abstract.txt` | **SAME**（有意） |

Before blobs @ tip `ee902396`：CN `28bc7badd730cab6ca291decca6d22ca530a9241`；EN `b0c8f4237ea68e391dc1eee12748006bcbd43666`；TeX `e4a3c3b9cc0cc47b2109d4dda55f4495407cd6ba`；abs `67fdf36f98c34d7ed6048c28ea1d52250a97794f`。

---

## 挡粘仍开（本拍不闭合）

1. **§8.1#3** 平台矩阵（须先选定单一密封测量身份）
2. **8509/8769** 测量身份（活 gold op-87/8769/569 vs EN/TeX/abs op-74/8509/517）
3. **Ο1|Ο2** 投稿/封口重测身份（**仍荐 Ο1**；本拍**不叠**新卡；引用 1914）

---

## 登记

- paper-notes 追加本拍 A 行（见 `research/paper-notes-20261009.md` 末）
- 心跳短报：`research/notes/paper-a-heartbeat-report-20261012-2329.md`

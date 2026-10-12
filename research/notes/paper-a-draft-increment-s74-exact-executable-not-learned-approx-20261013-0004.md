# Paper A 成篇定稿增量：§7.4「两条可迁移的经验」括注对齐摘要／§1／贡献#1／贡献#4／§10「精确可执行形式／非学得近似」

- **拍点：** 2026-10-13 ~00:04 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `b7caac334a342c046a7a120a7f5772615e1defbc`（短 `b7caac33`；= #330 after Cut A contrib#4 tip-after；相对任务书 tip **无漂**）
- **tip after：** （合入本 A 后填；见 heartbeat-report / tip-after PR）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 $K_s$；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only **§7.4 收束「两条可迁移的经验／Two lessons transfer」** CN+EN+TeX 同义收紧，对齐已硬的摘要／§1／贡献#1／贡献#4／§10「精确可执行形式／非学得近似」；保留「不是训练的替代品或逼近／distinct verification discipline that can run through an entire implementation」与「逐字节相同」后半句原框；**abstract.txt SAME**（无 §7.4；摘要核心主张句已硬，本拍 body-only）；**不改**键数身份（8509/8769/9174）、product/kernel/weights/facts、投稿方向；**不叠** Ο1（仍荐）；**不重钉** Softguess／§8.1#3／8509·8769；**不**粘 Softguess 口号字面
- **Softguess：** tip 四文件 **NONE×4**（BEFORE+AFTER 机扫 `Softguess`/`softguess`/`SoftGuess`/`权重就是`/`weights are the logic`/`逻辑本身`/`权重即逻辑`/`权重≡逻辑` 均 0 hit；**未**粘 Softguess 口号；部署路径「不含 softmax」技术句保留，不计入 Softguess 漂移）
- **Latest：** **v0.0.41**；冻结重测身份仍 **v0.0.19**；tip version **0.0.42**（本拍不 bump）；Draft 仅 **v0.0.31**

---

## 主张一句（本页唯一）

**§7.4 收束「两条可迁移的经验」把括注「权重由表构造而非训练」收紧为「权重由表构造而非训练——在声明域上给出表的精确可执行形式（与表逐键同一），不是学得的近似」——与已硬的摘要／§1／贡献#1／贡献#4／§10 同框；堵住 RQ4 经验句仍可被读成比贡献列表更软的攻击面；同一实质改动同步进 CN/EN/TeX；abs 有意 SAME；不引入 Softguess 口号字面。**

---

## 路由（为何不是假 A / 非 E / 非 C / 非 B）

- **非假 A（Latest 披露）：** tip §8.1 已写公开 Latest=**v0.0.41**；本拍不重复披露。
- **非 C：** 相对既有 8509/8769 登记无新 gold／pin；再 tip-refresh＝巡检式 FAIL。
- **非 B：** 相对 1914 Ο1 **无新信息**；叠卡禁。
- **非 E：** 本拍有可核对 **§7.4 经验括注** phrasing 增量 → 优先 **A**。
- **DISTINCT vs 2329 A：** 2329 收紧 **贡献#4**；本拍只动 **§7.4 正文收束句**，贡献列表不动。
- **DISTINCT vs 2256 A：** 2256 收紧 **贡献#1 方法**；本拍不动贡献#1。
- **DISTINCT vs 2217／2054／0956：** 那些动摘要／§10／§1；本拍 body-only §7.4。
- **DISTINCT vs 1011/1026：** 那些是更早的 §7.4／结论验证纪律句；本拍专钉「两条可迁移的经验」括注与已硬 contrib#4 同框。

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md` §7.4

- **Before：** `两条可迁移的经验：构造—枚举不是训练的替代品或逼近，而是一种与训练不同类、可以贯穿整个实现的验证纪律（权重由表构造而非训练）；“逐字节相同”远强于“输出相同”，它不给实现留任何自由度，而差分对照让它在没有形式规范时依然可检。`
- **After：** `两条可迁移的经验：构造—枚举不是训练的替代品或逼近，而是一种与训练不同类、可以贯穿整个实现的验证纪律（权重由表构造而非训练——在声明域上给出表的精确可执行形式（与表逐键同一），不是学得的近似）；“逐字节相同”远强于“输出相同”，它不给实现留任何自由度，而差分对照让它在没有形式规范时依然可检。`

### EN `research/unisacc-paper.en.md` §7.4

- **Before：** `Two lessons transfer: construction plus enumeration is not a substitute for—or approximation of—training, but a distinct verification discipline that can run through an entire implementation (weights are constructed from tables, not trained); and "byte-identical" is far stronger than "same output" — it leaves the implementation no freedom, and differential comparison keeps it checkable without a formal specification.`
- **After：** `Two lessons transfer: construction plus enumeration is not a substitute for—or approximation of—training, but a distinct verification discipline that can run through an entire implementation (weights are constructed from tables, not trained—on the declared domain they yield the table's exact executable form (identical key-by-key), not a learned approximation); and "byte-identical" is far stronger than "same output" — it leaves the implementation no freedom, and differential comparison keeps it checkable without a formal specification.`

### TeX `research/arxiv-paper-a/main.tex` §7.4

- **Before：** `Two lessons transfer: construction plus enumeration is not a substitute for---or approximation of---training, but a distinct verification discipline that can run through an entire implementation (weights are constructed from tables, not trained); and ``byte-identical'' is far stronger than ``same output'' --- it leaves the implementation no freedom, and differential comparison keeps it checkable without a formal specification.`
- **After：** `Two lessons transfer: construction plus enumeration is not a substitute for---or approximation of---training, but a distinct verification discipline that can run through an entire implementation (weights are constructed from tables, not trained---on the declared domain they yield the table's exact executable form (identical key-by-key), not a learned approximation); and ``byte-identical'' is far stronger than ``same output'' --- it leaves the implementation no freedom, and differential comparison keeps it checkable without a formal specification.`

### abs `research/arxiv-paper-a/abstract.txt`

- **SAME**（无 §7.4；本拍有意不改摘要；body-only）。

---

## 四 blob 状态（本拍后）

| 文件 | 相对 tip `b7caac33` |
| --- | --- |
| CN `research/unisacc-paper.md` | **DIFF**（仅 §7.4 两条可迁移的经验括注） |
| EN `research/unisacc-paper.en.md` | **DIFF**（仅 §7.4 Two lessons transfer 括注） |
| TeX `research/arxiv-paper-a/main.tex` | **DIFF**（仅 §7.4 Two lessons transfer 括注） |
| abs `research/arxiv-paper-a/abstract.txt` | **SAME**（有意） |

Before blobs @ tip `b7caac33`：CN `29c8c14fb2ff55aee3d263c184b69252cb9bf913`；EN `a619ea85bbf6c94725e53e9213f1a54d231b401f`；TeX `1fa6c28e3d96497a8a532e4c8f1839fa916ac4b5`；abs `67fdf36f98c34d7ed6048c28ea1d52250a97794f`。

After blobs（本分支）：CN `df18288d36f426900917b0487b66b0dbd15be731`；EN `d54cdc0b828324353112d560a4fdd2d3f2cf609c`；TeX `b67c3ea9828598eece428595435f550d14e10233`；abs `67fdf36f98c34d7ed6048c28ea1d52250a97794f`（SAME）。

---

## 挡粘仍开（本拍不闭合）

1. **§8.1#3** 平台矩阵（须先选定单一密封测量身份）
2. **8509/8769** 测量身份（活 gold op-87/8769/569 vs EN/TeX/abs op-74/8509/517）
3. **Ο1|Ο2** 投稿/封口重测身份（**仍荐 Ο1**；本拍**不叠**新卡；引用 1914）

---

## 登记

- 心跳短报：`research/notes/paper-a-heartbeat-report-20261013-0004.md`
- Co-author note：论文撰写员（research-only self-review+merge）

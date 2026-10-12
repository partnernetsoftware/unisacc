# Paper A 成篇定稿增量：§10 结论开句「构造权重＝声明域精确可执行形式，非学得近似」三联对齐

- **拍点：** 2026-10-12 ~20:54 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `55b3c02bbd9f2cbf96a103a532ce3a9169649201`（短 `55b3c02b`；= #319 after Cut C 2016 tip-refresh；相对任务书 tip **无漂**）
- **tip after：** （短 ；= squash Merge #320 本 A）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only CN+EN+TeX §10 结论**开句**同义收紧，对齐已硬的 §1「精确可执行形式／非学得统计替代」；**不改** abstract（摘要无该结论开句）；**不改**键数身份（8509/8769/9174）、product/kernel/weights/facts、投稿方向；**不叠** Ο1（仍荐）；**不重钉** Softguess／§8.1#3／8509·8769；**不**粘 Softguess 口号字面（`逻辑本身`／`权重即逻辑`／`weights are the logic` 等仍 NONE）
- **Softguess：** tip 四文件 **NONE×4**（BEFORE+AFTER 机扫 `Softguess`/`softguess`/`SoftGuess`/`权重就是`/`weights are the logic`/`逻辑本身`/`权重即逻辑`/`权重≡逻辑` 均 0 hit；bib「A neural compiler」为他文题名不计；**未**粘 Softguess 口号）
- **Latest：** **v0.0.41**；冻结重测身份仍 **v0.0.19**；tip version **0.0.42**（本拍不 bump）；Draft 仅 **v0.0.31**

---

## 主张一句（本页唯一）

**§10 结论开句把「权重也可以由表直接构造而不必训练 / the weights can be constructed … instead of trained」收紧为「构造权重在声明域上给出表的精确可执行形式（与表逐键同一），不是学得的近似 / constructed weights yield the table's exact executable form (identical key-by-key), not a learned approximation」——与已硬的 §1 同质；堵住结论开句仍可被读成「仅换了一种得权方式、权重仍可能是近似」的攻击面；同一实质改动同步进 CN/EN/TeX；abstract 无该开句故 SAME；不引入 Softguess 口号字面。**

---

## 路由（为何不是假 A / 非 E / 非 C / 非 B）

- **非假 A（Latest 披露）：** tip §8.1 已写公开 Latest=**v0.0.41**（#317 路由已关）；本拍不重复披露。
- **非 C：** tip 仍 `55b3c02b`，#319 后无新产品提交可改 gold；再 tip-refresh 8509/8769＝巡检式 FAIL。
- **非 B：** 相对 1914 Ο1 **无新信息**；叠卡禁。
- **非 E：** 本拍有可核对正文 phrasing 增量 → 优先 **A**。
- **DISTINCT vs 1026 A：** 1026 收紧的是 §10 **收束句**「不是训练的替代品或逼近…验证纪律」；本拍收紧的是 §10 **开句**「构造→精确可执行形式／非学得近似」。
- **DISTINCT vs 0956 A：** 0956 收紧的是 **§1**；本拍把同一硬度对齐进 **§10 开句**。

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md` §10

- **Before：** `对编译器中查表形状的决策，“精确”是一个可以被穷举判定的性质，权重也可以由表直接构造而不必训练。`
- **After：** `对编译器中查表形状的决策，“精确”是一个可以被穷举判定的性质；权重由表直接构造而不必训练——在声明域上，构造权重给出的是表的精确可执行形式（与表逐键同一），不是学得的近似。`

### EN `research/unisacc-paper.en.md` §10

- **Before：** `For table-shaped decisions in a compiler, exactness is a property that can be decided by exhaustion, and the weights can be constructed from the tables instead of trained.`
- **After：** `For table-shaped decisions in a compiler, exactness is a property that can be decided by exhaustion; the weights are constructed from the tables rather than trained—and on the declared domain those constructed weights yield the table's exact executable form (identical key-by-key), not a learned approximation.`

### TeX `research/arxiv-paper-a/main.tex` §10 Conclusion

- **Before：** `For table-shaped decisions in a compiler, exactness is a property that can be decided by exhaustion, and the weights can be constructed from the tables instead of trained.`
- **After：** `For table-shaped decisions in a compiler, exactness is a property that can be decided by exhaustion; the weights are constructed from the tables rather than trained---and on the declared domain those constructed weights yield the table's exact executable form (identical key-by-key), not a learned approximation.`

### abstract `research/arxiv-paper-a/abstract.txt`

- **本拍不改**（摘要无该 §10 开句；核心主张句已含 constructed weights / key-by-key / not learned from data；构造—枚举 ≠ training substitute/approx 已另硬）。

---

## 四 blob 状态（本拍后）

| 文件 | 相对 tip `55b3c02b` |
| --- | --- |
| CN `research/unisacc-paper.md` | **DIFF**（仅 §10 开句） |
| EN `research/unisacc-paper.en.md` | **DIFF**（仅 §10 开句） |
| TeX `research/arxiv-paper-a/main.tex` | **DIFF**（仅 §10 开句） |
| abs `research/arxiv-paper-a/abstract.txt` | **SAME** `92a6472c92a13cb4e8149bbd798f24e6d6a5f648` |

（CN/EN/TeX 新 blob SHA 以 merge 后 `git`/`gh api` 为准，本页不臆造。）

---

## 挡粘仍开（本拍不闭合）

1. **§8.1#3** 平台矩阵（须先选定单一密封测量身份）
2. **8509/8769** 测量身份（活 gold op-87/8769/569 vs EN/TeX/abs op-74/8509/517）
3. **Ο1|Ο2** 投稿/封口重测身份（**仍荐 Ο1**；本拍**不叠**新卡；引用 1914）

---

## 登记

- paper-notes 追加本拍 A 行（见 `research/paper-notes-20261009.md` 末）
- 心跳短报：`research/notes/paper-a-heartbeat-report-20261012-2054.md`

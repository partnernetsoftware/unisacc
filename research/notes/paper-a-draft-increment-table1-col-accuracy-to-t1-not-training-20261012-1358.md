# Paper A 成篇定稿增量：表 1 列头「精度／Accuracy」→ T1（非训练精度）

- **拍点：** 2026-10-12 13:58 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `e3bf35f28af56f8a9a4d3f439eb590657aaf4f17`（短 `e3bf35f2`；= #270 after A #269 abs all-exact pin）
- **tip after：** `bb811af14f6b56a42dd1a4b5650a8dd774ea8e49`（短 `bb811af1`；= squash Merge #271 本 A）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only CN+EN+TeX 表 1 列头裸「精度」／「Accuracy」改名为 **T1**；表注已钉 T1／定义 2，列头单独仍可被误读为训练精度；**不改** abstract.txt；**不改**键数身份、单元格 1.000、product/kernel/weights/facts、投稿方向、定义 2／3 本文、表注已钉句；**不叠** Ο1；**不重钉** Softguess／§8.1#3／8509·8769
- **Softguess：** tip 四文件口号 **NONE×4**（机扫 Softguess/softguess/soft-guess/Softmax-as-product／「权重就是」／“weights are the logic”／「逻辑本身」／「权重即逻辑」／「权重≡逻辑」均 0 hit）；软读 **HIT 表头「精度／Accuracy」**（本拍已钉）；残余候选：T1 状态格「全域枚举」、§7.3 Adam「accuracy hit 1.000」（训练对照故意用语，勿钉成 T1）
- **Latest：** **v0.0.40**；冻结重测身份仍 **v0.0.19**；tip version **0.0.41**（本拍不 bump）

---

## 主张一句（本页唯一）

**表 1 列头裸「精度」／「Accuracy」收紧为列名 T1——表身直接命名定义 2／声明域穷举逐键相等，堵住单看列头仍可读成训练／样本精度的攻击面；CN/EN/TeX 三联；abstract.txt SAME；不引入 Softguess 口号字面。**

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md`（`<!-- stages-zh:begin -->`…`end`）

- **Before：** `| 阶段 | 键字段 × 词表大小 | 输出 | 键数 | 单元 | 精度 |`
- **After：** `| 阶段 | 键字段 × 词表大小 | 输出 | 键数 | 单元 | T1 |`
- 表注已含「精度为全域枚举：声明域穷举逐键相等，即 T1／模型检验／定义 2，不是相对样本的拟合精度或训练精度」— **本拍不改表注**；键数／1.000 **不变**。

### EN `research/unisacc-paper.en.md`

- **Before：** `| Stage | What it decides | Key fields × vocabulary size | Output | Keys | Units | Accuracy |`
- **After：** `| Stage | What it decides | Key fields × vocabulary size | Output | Keys | Units | T1 |`
- 表注已钉 accuracy by full-domain enumeration = T1… — **本拍不改表注**。

### TeX `research/arxiv-paper-a/main.tex`（longtable firsthead + head）

- **Before：** minipage 列头单元格裸词 `Accuracy`（两处：`\endfirsthead` 前与 `\endhead` 前）
- **After：** 同位置裸词 `T1`
- Caption 已钉 accuracy by full-domain enumeration = T1… — **本拍不改 caption**；combo 行散文「Accuracy 1.000」保留（指单元格读法，非列头）。

### abs `research/arxiv-paper-a/abstract.txt`

- **本拍 SAME**（不触）。

---

## 为何更难挑战

| 攻击读法 | Before 软点 | After 挡法 |
| --- | --- | --- |
| 单看表头「精度／Accuracy」像训练 accuracy | 表注已钉但列头仍裸 | 列名即 T1＝定义 2／穷举逐键相等 |
| Softguess／破平近似合法出货 | 不涉及 | **未**粘 Softguess 口号；口号 NONE×4 |

---

## tip / blob 表

| 项 | before (`e3bf35f2`) | after (`bb811af1`) |
| --- | --- | --- |
| tip | `e3bf35f28af56f8a9a4d3f439eb590657aaf4f17` | `bb811af14f6b56a42dd1a4b5650a8dd774ea8e49`（短 `bb811af1`；#271） |
| CN `research/unisacc-paper.md` | `5711fd4991c347abdc161acaf894488e755bcacb` | **CHANGED**（列头 精度→T1） |
| EN `research/unisacc-paper.en.md` | `55f103209cadb9aedf6cf2f3456c9728a9b337a7` | **CHANGED**（列头 Accuracy→T1） |
| TeX `research/arxiv-paper-a/main.tex` | `f1e93825d0465a578c286c4361dd02a5daf74036` | **CHANGED**（两处列头 Accuracy→T1） |
| abs `research/arxiv-paper-a/abstract.txt` | （未改） | **SAME** |
| Softguess 口号 | **NONE×4** | **NONE×4** |
| Softguess 软读 | HIT 表头「精度／Accuracy」 | **HIT 表头「精度／Accuracy」**（本拍钉） |

---

## Softguess 机扫 @ 2026-10-12 13:58 Asia/Singapore（工作区 tip 四文件）

| 文件 | Softguess | softguess | soft-guess | Softmax-as-product | 权重就是 | weights are the logic | 逻辑本身 | 权重即逻辑 | 权重≡逻辑 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| EN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| TeX | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| abstract | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

- **Softguess_NONE_x4 = true**（口号字面）
- **软读 HIT = 表头「精度／Accuracy」**（本拍钉；非口号字面）

---

## 同族链

0956 → … → 1156 表 1 表注精度＝T1 → 1209 表 3 阶段精度判据＝T1 → … → 1345 abs「all exact」＝T1 → **1358 表 1 列头精度／Accuracy→T1**

---

## 风险自审（可自合）

- research-only；仅表 1 列头三文件 + notes；不改键数／1.000／abs／product／投稿方向
- 低风险：可自 review 后 merge；第二 PR 回填 tip-after + PR 号

# Paper A 成篇定稿增量：表 3「阶段精度」判据钉为 T1／非训练精度三联对齐

- **拍点：** 2026-10-12 ~12:09 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `9947f6167fb6617c7c23bb2ca2d539c0fba98847`（短 `9947f61`；= #255 tip-after fill for #254 Table1-caption；相对任务书 tip `9947f61` **无漂**）
- **tip after：** `6ca1e3c874e5b7a509edeb377d1e80c76d8235bd`（短 `6ca1e3c`；= squash Merge #257 本 A）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only CN+EN+TeX 表 3「阶段精度」行判据「全域枚举」／「Full-domain enumeration」同义收紧为 T1／定义 2；**不改** abstract.txt（无表 3 行）；**不改**键数身份、结果列「1.000」、项目列「阶段精度」／Stage accuracy、表 1 已钉表注、product/kernel/weights/facts、投稿方向、定义 2／3 本文、摘要已钉句；**不叠** Ο1；**不重钉** Softguess／§8.1#3／8509·8769
- **Softguess：** tip 四文件 **NONE×4**（机扫 `Softguess`/`softguess`/`soft-guess`/`Softmax-as-product`／「权重就是」／“weights are the logic”／「逻辑本身」／「权重即逻辑」／「权重≡逻辑」均 0 hit；§3「不含 softmax」／「no softmax」否认句不计；**未**粘 Softguess 口号）
- **Latest：** **v0.0.40**；冻结重测身份仍 **v0.0.19**；tip version **0.0.41**（本拍不 bump）
- **同族链（本拍续）：** 0956 → 1011 §7.4 → 1026 §10 → 1046 abstract → 1056 contrib4 → 1109 contrib1 Method → 1130 Def2 exactness → 1141 精度 1.000＝T1 → 1156 表 1 表注精度＝T1 → **本拍表 3 阶段精度判据＝T1 非训练精度**

---

## 主张一句（本页唯一）

**表 1 表注「精度为全域枚举」已钉为 T1／定义 2（#254）之后，§7.1 表 3「阶段精度」行仍写判据「全域枚举」／「Full-domain enumeration」与结果「1.000」，可软读为样本拟合／训练准确率验收标签；本拍在保留「全域枚举」与「1.000」事实的同时，在判据列显式括注其为声明域穷举逐键相等（T1／模型检验／定义 2），并排除相对样本的拟合精度或训练精度；CN/EN/TeX 三联；abs SAME；不改结果数值与项目列名；不引入 Softguess 口号字面；不改键数。**

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md` 表 3 行

- **Before：** `| 阶段精度 | 全域枚举 | 18 个阶段全部 1.000 |`
- **After：** `| 阶段精度 | 全域枚举（声明域穷举逐键相等，即 T1／模型检验／定义 2，不是相对样本的拟合精度或训练精度） | 18 个阶段全部 1.000 |`

### EN `research/unisacc-paper.en.md` Table 3 row

- **Before：** `| Stage accuracy | Full-domain enumeration | all 18 stages 1.000 |`
- **After：** `| Stage accuracy | Full-domain enumeration (= T1 / exhaustive key-by-key equality on the declared domain / Definition 2, not sample fit or training accuracy) | all 18 stages 1.000 |`

### TeX `research/arxiv-paper-a/main.tex` Table 3 longtable row

- **Before：** `Stage accuracy & Full-domain enumeration & all 18 stages 1.000 \\`
- **After：** `Stage accuracy & Full-domain enumeration (= T1 / exhaustive key-by-key equality on the declared domain / Definition~2, not sample fit or training accuracy) & all 18 stages 1.000 \\`

### abstract `research/arxiv-paper-a/abstract.txt`

- **本拍不改**（无表 3「阶段精度」行；摘要「all exact」已于 #252 钉 T1）。

### 邻近句（可选 follow-through；本拍不改）

- 表 1 列头「精度」／`Accuracy`、表 1 数值列 `1.000`、附录命令注释「全域精度」／full-domain accuracy、§7.3 Adam「accuracy hit 1.000」（训练对照语境）——不扩改。

---

## 为何更难挑战

| 攻击读法 | Before 软点 | After 挡法 |
| --- | --- | --- |
| 表 3「阶段精度」＝训练准确率验收 | 判据仅写「全域枚举」／Full-domain enumeration | 括注「声明域穷举逐键相等（T1／模型检验／定义 2）」／「= T1 / … Definition 2」 |
| 结果 1.000＝样本拟合度量 | 判据未排除训练读法 | 显式「不是相对样本的拟合精度或训练精度」／「not sample fit or training accuracy」 |
| Softguess／破平近似合法出货 | 不涉及 | **未**粘 Softguess 口号；Softguess NONE×4 保持 |

---

## tip / blob 表

| 项 | before (`9947f61`) | after（本拍；merge 后填） |
| --- | --- | --- |
| tip | `9947f6167fb6617c7c23bb2ca2d539c0fba98847` | `6ca1e3c874e5b7a509edeb377d1e80c76d8235bd`（短 `6ca1e3c`；#257） |
| CN `research/unisacc-paper.md` | `ec4fc12edc5d47baf598960200b015bf4a7e8830` | **CHANGED** → `b1568103f5ab1e71aecae0027c3ec03ef1cf3b4e` |
| EN `research/unisacc-paper.en.md` | `30e524175292b6d4d95aa46d10e51a5c1522a06f` | **CHANGED** → `98a7e0a13493674afa78b7241b43e844cd4fed4a` |
| TeX `research/arxiv-paper-a/main.tex` | `56edb8235d7879e7a60dcc63cc2a5a833e20dae2` | **CHANGED** → `03c69b5f35885d18e084895957c0de9a6ea2e07c` |
| abs `research/arxiv-paper-a/abstract.txt` | `980f670df7734b49a5d90bb3c305cd2b565d0b8b` | **SAME**（未改；`980f670df7734b49a5d90bb3c305cd2b565d0b8b`） |
| Softguess | **NONE×4** | **NONE×4** |

---

## Softguess 机扫 @ 2026-10-12 12:09 Asia/Singapore（工作区，merge 前 tip 四文件 + 改后）

| 文件 | Softguess | softguess | soft-guess | Softmax-as-product | 权重就是 | weights are the logic | 逻辑本身 | 权重即逻辑 | 权重≡逻辑 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| EN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| TeX | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| abstract | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

- **Softguess_NONE_x4 = true**

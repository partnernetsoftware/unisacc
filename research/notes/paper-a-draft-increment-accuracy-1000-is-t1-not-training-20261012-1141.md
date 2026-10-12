# Paper A 成篇定稿增量：摘要「精度均为 1.000」钉为 T1／非训练精度三联对齐

- **拍点：** 2026-10-12 ~11:41 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `de462ea68a6e7d0b3ec8defae72e9c8f80f35f75`（短 `de462ea6`；= #251 tip-after fill for #250 Def2；相对任务书 tip `de462ea6` **无漂**）
- **tip after：** `daca18ed0cd174f86957bedf336eb98e1bb18d53`（短 `daca18e`；= squash Merge #252 本 A）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only CN+EN+TeX 摘要实证句「精度／accuracy 1.000／all exact」同义收紧为 T1／定义 2；**不改** abstract.txt（无「accuracy 1.000」软读字面，已用 all exact）；**不改**键数身份、product/kernel/weights/facts、投稿方向、定义 2／3 本文、T1 表行、贡献列表；**不叠** Ο1；**不重钉** Softguess／§8.1#3／8509·8769
- **Softguess：** tip 四文件 **NONE×4**（机扫 `Softguess`/`softguess`/`soft-guess`/`Softmax-as-product`／「权重就是」／“weights are the logic”／「逻辑本身」／「权重即逻辑」／「权重≡逻辑」均 0 hit；§3「不含 softmax」／「no softmax」否认句不计；**未**粘 Softguess 口号）
- **Latest：** **v0.0.40**；冻结重测身份仍 **v0.0.19**；tip version **0.0.41**（本拍不 bump）
- **同族链（本拍续）：** 0956 → 1011 §7.4 → 1026 §10 → 1046 abstract → 1056 contrib4 → 1109 contrib1 Method → 1130 Def2 exactness → **本拍 1.000＝T1 非训练精度**

---

## 主张一句（本页唯一）

**定义 2 已钉「精确＝声明域穷举逐键相等（T1／模型检验）」之后，摘要实证句仍写「精度均为 1.000」／「accuracy 1.000」／「all exact」，可软读为样本拟合或训练精度；本拍在保留 1.000／exact 事实的同时，显式括注其为 T1／模型检验／定义 2，并排除相对样本的拟合精度或训练精度；CN/EN/TeX 三联；abs SAME；不引入 Softguess 口号字面；不改键数、不改定义 2／3／T1 表行。**

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md` 摘要 · 决策网络实例

- **Before：** `在表 1 合计的 8,769 个键（op-87 词表身份）上精度均为 1.000，`
- **After：** `在表 1 合计的 8,769 个键（op-87 词表身份）上精度均为 1.000（此为声明域穷举逐键相等，即 T1／模型检验／定义 2，不是相对样本的拟合精度或训练精度），`

### EN `research/unisacc-paper.en.md` Abstract · decision-network instance

- **Before：** `with accuracy 1.000 on the 8,509 keys in Table 1 (op-74 vocabulary identity;`
- **After：** `with accuracy 1.000 on the 8,509 keys in Table 1 (exhaustive key-by-key equality on the declared domain: T1 / model checking / Definition 2, not sample fit or training accuracy; op-74 vocabulary identity;`

### TeX `research/arxiv-paper-a/main.tex` abstract · 18 constructed networks

- **Before：** `(8,509 keys, op-74 vocabulary identity, all exact;`
- **After：** `(8,509 keys, op-74 vocabulary identity, all exact under T1 / exhaustive key-by-key equality on the declared domain (Definition~2), not sample fit or training accuracy;`

### abstract `research/arxiv-paper-a/abstract.txt`

- **本拍不改**（无「accuracy 1.000」／「精度均为 1.000」软读字面；已用 `all exact`；与 TeX 摘要本拍括注可有一字面差，记作 intentional SAME）。

### 邻近句（可选 follow-through）

- **本拍不改**：表 1 数值列 `1.000`、表注「精度为全域枚举」／「accuracy by full-domain enumeration」、combo 行 T1 读法、§7.3 Adam「首次达到 1.000」、定义 2／3 本文、贡献列表——已含或已钉 T1 读法，不扩改表单元格。

---

## 为何更难挑战

| 攻击读法 | Before 软点 | After 挡法 |
| --- | --- | --- |
| 摘要「精度 1.000」＝训练准确率 | 未点名 T1／定义 2 | 括注「声明域穷举逐键相等（T1／模型检验／定义 2）」／「T1 / model checking / Definition 2」 |
| 1.000＝样本拟合度量 | 仅写数值 | 显式「不是相对样本的拟合精度或训练精度」／「not sample fit or training accuracy」 |
| Softguess／破平近似合法出货 | 不涉及 | **未**粘 Softguess 口号；Softguess NONE×4 保持 |

---

## tip / blob 表

| 项 | before (`de462ea6`) | after（本拍；#252 `daca18e`） |
| --- | --- | --- |
| tip | `de462ea68a6e7d0b3ec8defae72e9c8f80f35f75` | `daca18ed0cd174f86957bedf336eb98e1bb18d53`（短 `daca18e`；#252） |
| CN `research/unisacc-paper.md` | `482661ec2954ae995a9841145c2b44db2cac95f3` | **CHANGED** → `4d33c55ea81e100dabd426382cdda559eda2f5d7` |
| EN `research/unisacc-paper.en.md` | `a7761815fffff16ffa31e1837d68ec46bcf5b396` | **CHANGED** → `844b97f1fbefa85e33b2aca3d8fae6c57dd42744` |
| TeX `research/arxiv-paper-a/main.tex` | `9ee7cd263691f961538737885fe0cd9332e6e5c1` | **CHANGED** → `569b029ff333a82bd3f1ac58ddc2f698cf36142b` |
| abs `research/arxiv-paper-a/abstract.txt` | `980f670df7734b49a5d90bb3c305cd2b565d0b8b` | **SAME**（未改；`980f670df7734b49a5d90bb3c305cd2b565d0b8b`） |
| Softguess | **NONE×4** | **NONE×4** |

---

## Softguess 机扫 @ 2026-10-12 11:41 Asia/Singapore（工作区，merge 前 tip 四文件 + 改后）

| 文件 | Softguess | softguess | soft-guess | Softmax-as-product | 权重就是 | weights are the logic | 逻辑本身 | 权重即逻辑 | 权重≡逻辑 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| EN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| TeX | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| abstract | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

- **Softguess_NONE_x4 = true**

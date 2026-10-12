# Paper A 成篇定稿增量：表 1 表注「精度为全域枚举」钉为 T1／非训练精度三联对齐

- **拍点：** 2026-10-12 ~11:56 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `a2253091bad7426b5d9579d150a67c06b57771dc`（短 `a2253091`；= #253 tip-after fill for #252 accuracy-1000；相对任务书 tip `a2253091` **无漂**）
- **tip after：** （merge 后填；本 PR squash）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only CN+EN+TeX 表 1 表注「精度为全域枚举」／「accuracy by full-domain enumeration」同义收紧为 T1／定义 2；**不改** abstract.txt（无表 1 表注）；**不改**键数身份、表单元格 1.000、列头「精度」／Accuracy、「阶段精度」行、product/kernel/weights/facts、投稿方向、定义 2／3 本文、摘要已钉句；**不叠** Ο1；**不重钉** Softguess／§8.1#3／8509·8769
- **Softguess：** tip 四文件 **NONE×4**（机扫 `Softguess`/`softguess`/`soft-guess`/`Softmax-as-product`／「权重就是」／“weights are the logic”／「逻辑本身」／「权重即逻辑」／「权重≡逻辑」均 0 hit；§3「不含 softmax」／「no softmax」否认句不计；**未**粘 Softguess 口号）
- **Latest：** **v0.0.40**；冻结重测身份仍 **v0.0.19**；tip version **0.0.41**（本拍不 bump）
- **同族链（本拍续）：** 0956 → 1011 §7.4 → 1026 §10 → 1046 abstract → 1056 contrib4 → 1109 contrib1 Method → 1130 Def2 exactness → 1141 精度 1.000＝T1 → **本拍表 1 表注精度＝T1 非训练精度**

---

## 主张一句（本页唯一）

**摘要「精度均为 1.000」已钉为 T1／定义 2（#252）之后，表 1 表注仍写「精度为全域枚举」／「accuracy by full-domain enumeration」，可软读为样本拟合度量标签；本拍在保留「全域枚举」事实的同时，显式括注其为声明域穷举逐键相等（T1／模型检验／定义 2），并排除相对样本的拟合精度或训练精度；CN/EN/TeX 三联；abs SAME；不改表数值与列头；不引入 Softguess 口号字面；不改键数。**

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md` 表 1 表注

- **Before：** `**表 1　决策网络实例：各阶段的键空间与构造网络规模**（精度为全域枚举）`
- **After：** `**表 1　决策网络实例：各阶段的键空间与构造网络规模**（精度为全域枚举：声明域穷举逐键相等，即 T1／模型检验／定义 2，不是相对样本的拟合精度或训练精度）`

### EN `research/unisacc-paper.en.md` Table 1 caption

- **Before：** `(accuracy by full-domain enumeration; **op-74 vocabulary identity**:`
- **After：** `(accuracy by full-domain enumeration = T1 / exhaustive key-by-key equality on the declared domain (Definition 2), not sample fit or training accuracy; **op-74 vocabulary identity**:`

### TeX `research/arxiv-paper-a/main.tex` Table 1 `\caption`

- **Before：** `(accuracy by full-domain enumeration; op-74 vocabulary identity:`
- **After：** `(accuracy by full-domain enumeration = T1 / exhaustive key-by-key equality on the declared domain (Definition~2), not sample fit or training accuracy; op-74 vocabulary identity:`

### abstract `research/arxiv-paper-a/abstract.txt`

- **本拍不改**（无表 1 表注；摘要「all exact」已于 #252 钉 T1）。

### 邻近句（可选 follow-through；本拍不改）

- 表 1 数值列 `1.000`、列头「精度」／`Accuracy`、§x「阶段精度」／`Stage accuracy` 行、combo 行 T1 读法、摘要已钉句、定义 2／3——不扩改。

---

## 为何更难挑战

| 攻击读法 | Before 软点 | After 挡法 |
| --- | --- | --- |
| 表注「精度」＝训练准确率标签 | 仅写「全域枚举」／full-domain enumeration | 括注「声明域穷举逐键相等（T1／模型检验／定义 2）」／「= T1 / … (Definition 2)」 |
| 1.000 列＝样本拟合度量 | 表注未排除训练读法 | 显式「不是相对样本的拟合精度或训练精度」／「not sample fit or training accuracy」 |
| Softguess／破平近似合法出货 | 不涉及 | **未**粘 Softguess 口号；Softguess NONE×4 保持 |

---

## tip / blob 表

| 项 | before (`a2253091`) | after（本拍；merge 后填） |
| --- | --- | --- |
| tip | `a2253091bad7426b5d9579d150a67c06b57771dc` | （merge 后填） |
| CN `research/unisacc-paper.md` | `4d33c55ea81e100dabd426382cdda559eda2f5d7` | **CHANGED** → `ec4fc12edc5d47baf598960200b015bf4a7e8830` |
| EN `research/unisacc-paper.en.md` | `844b97f1fbefa85e33b2aca3d8fae6c57dd42744` | **CHANGED** → `30e524175292b6d4d95aa46d10e51a5c1522a06f` |
| TeX `research/arxiv-paper-a/main.tex` | `569b029ff333a82bd3f1ac58ddc2f698cf36142b` | **CHANGED** → `56edb8235d7879e7a60dcc63cc2a5a833e20dae2` |
| abs `research/arxiv-paper-a/abstract.txt` | `980f670df7734b49a5d90bb3c305cd2b565d0b8b` | **SAME**（未改；`980f670df7734b49a5d90bb3c305cd2b565d0b8b`） |
| Softguess | **NONE×4** | **NONE×4** |

---

## Softguess 机扫 @ 2026-10-12 11:56 Asia/Singapore（工作区，merge 前 tip 四文件 + 改后）

| 文件 | Softguess | softguess | soft-guess | Softmax-as-product | 权重就是 | weights are the logic | 逻辑本身 | 权重即逻辑 | 权重≡逻辑 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| EN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| TeX | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| abstract | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

- **Softguess_NONE_x4 = true**

# Paper A 成篇定稿增量：摘要裸「精度均为 1.000／accuracy 1.000」→ T1 均为 1.000／T1 1.000（非训练精度）

- **拍点：** 2026-10-12 ~14:52 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `42b281b702e014412ddd956107436fc825376df3`（短 `42b281b7`；= #276 after combo T1 #275）
- **tip after：** `TIP_AFTER_PLACEHOLDER`（短 `TIP_AFTER_SHORT`；= squash Merge #PR_A 本 A）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only CN+EN markdown 摘要决策网络实例句裸「精度均为 1.000」／「accuracy 1.000」改名为 **T1 均为 1.000**／**T1 1.000**；与表 1 列头 T1／#275 combo「T1 为 1.000／T1 1.000」对齐；括注 T1／定义 2／非训练精度 **保留**；**TeX `\begin{abstract}` 与 `abstract.txt` SAME**（已是「all exact under T1 …」，无裸 accuracy 1.000 领头词）；**不改**键数身份、表单元格 1.000、product/kernel/weights/facts、投稿方向、定义 2／3 本文、§7.3 Adam「accuracy hit 1.000」（训练对照故意用语，勿钉成 T1）；**不叠** Ο1；**不重钉** Softguess／§8.1#3／8509·8769
- **Softguess：** tip 四文件口号 **NONE×4**；软读 **HIT 摘要「精度均为／accuracy 1.000」**（本拍已钉）；残余候选：T1 状态格「全域枚举」、§7.3 Adam「accuracy hit 1.000」（勿钉成 T1）
- **Latest：** **v0.0.40**；冻结重测身份仍 **v0.0.19**；tip version 不 bump 论文键数／身份

---

## 主张一句（本页唯一）

**摘要决策网络实例句裸「精度均为 1.000」／「accuracy 1.000」收紧为「T1 均为 1.000」／「T1 1.000」——与表 1 列头 T1 及 combo #275 对齐，堵住单看摘要领头词仍可读成训练／样本 accuracy 的攻击面；CN/EN 改、TeX/abs SAME（已 T1）；不引入 Softguess 口号字面。**

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md`

- **Before：** `在表 1 合计的 8,769 个键（op-87 词表身份）上精度均为 1.000（此为声明域穷举逐键相等，即 T1／模型检验／定义 2，不是相对样本的拟合精度或训练精度）`
- **After：** `在表 1 合计的 8,769 个键（op-87 词表身份）上 T1 均为 1.000（此为声明域穷举逐键相等，即 T1／模型检验／定义 2，不是相对样本的拟合精度或训练精度）`
- 键数身份 **不变**；括注 **保留**。

### EN `research/unisacc-paper.en.md`

- **Before：** `with accuracy 1.000 on the 8,509 keys in Table 1 (exhaustive key-by-key equality on the declared domain: T1 / model checking / Definition 2, not sample fit or training accuracy;`
- **After：** `with T1 1.000 on the 8,509 keys in Table 1 (exhaustive key-by-key equality on the declared domain: T1 / model checking / Definition 2, not sample fit or training accuracy;`

### TeX `research/arxiv-paper-a/main.tex`

- **本拍 SAME**（`\begin{abstract}` 已是「all exact under T1 / exhaustive key-by-key equality …」，无裸「accuracy 1.000」领头词）。

### abs `research/arxiv-paper-a/abstract.txt`

- **本拍 SAME**（已与 TeX abs 同质「all exact under T1 …」；#269 已钉）。

### 故意不钉

- §7.3 Adam「accuracy hit 1.000」／「首次达到 1.000」——训练基线度量，保持与 T1 区分。

---

## 为何更难挑战

| 攻击读法 | Before 软点 | After 挡法 |
| --- | --- | --- |
| 单看摘要「精度／accuracy 1.000」像训练 accuracy | 括注已有 T1 但领头词仍裸 | 领头词即 T1＝定义 2／声明域穷举逐键相等 |
| CN/EN 软、TeX/abs 已硬，双通道摘要漂移 | markdown 摘要滞后于 TeX「all exact under T1」 | CN/EN 领头词对齐 T1；TeX/abs 无需再改 |
| Softguess／破平近似合法出货 | 不涉及 | **未**粘 Softguess 口号；口号 NONE×4 |

---

## tip / blob 表

| 项 | before (`42b281b7`) | after (`TIP_AFTER_SHORT`) |
| --- | --- | --- |
| tip | `42b281b702e014412ddd956107436fc825376df3` | `TIP_AFTER_PLACEHOLDER`（短 `TIP_AFTER_SHORT`；#PR_A） |
| CN `research/unisacc-paper.md` | `5aecb9e0d037fa546a32e84e68b33ec36eca795e` | **CHANGED** → `CN_BLOB_AFTER` |
| EN `research/unisacc-paper.en.md` | `ea74eda6a3a2c2d10e92b1ce05ba3c09f82db386` | **CHANGED** → `EN_BLOB_AFTER` |
| TeX `research/arxiv-paper-a/main.tex` | `7ca29cc5c9c24a6ffb3a7107b9f731915a9a5097` | **SAME** |
| abs `research/arxiv-paper-a/abstract.txt` | `92a6472c92a13cb4e8149bbd798f24e6d6a5f648` | **SAME** |
| Softguess 口号 | **NONE×4** | **NONE×4** |
| Softguess 软读 | HIT 摘要「精度均为／accuracy 1.000」 | **HIT 摘要**（本拍钉） |

---

## Softguess 机扫 @ 2026-10-12 ~14:52 Asia/Singapore（tip 四文件）

| 文件 | Softguess | softguess | soft-guess | Softmax-as-product | 权重就是 | weights are the logic | 逻辑本身 | 权重即逻辑 | 权重≡逻辑 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| EN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| TeX | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| abs | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

- **Softguess_NONE_x4 = true**（口号字面）
- **软读 HIT = 摘要「精度均为 1.000／accuracy 1.000」**（本拍钉；非口号字面）

---

## 仍挡 / 残余下一拍候选

- **仍挡：** §8.1#3；8509/8769 键数身份差（本拍不碰）；Ο1（荐不叠卡）
- **残余候选：** T1 状态格「全域枚举」澄清；§7.3 Adam「accuracy hit 1.000」保持训练用语（勿钉成 T1）；combo+摘要 accuracy 误读面已闭合后可做 C 钉页
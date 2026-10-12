# Paper A 成篇定稿增量：表 3 行标「阶段精度／Stage accuracy」→ 阶段 T1／Stage T1（非训练精度）

- **拍点：** 2026-10-12 14:18 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `9647ed68ef5caf4bf8163ab67ce13eacea822595`（短 `9647ed68`）
- **tip after：** （合并后填）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only CN+EN+TeX 表 3 验收表行标裸「阶段精度」／「Stage accuracy」改名为 **阶段 T1**／**Stage T1**；判据格已钉 T1／定义 2，行标单独仍可被误读为训练／样本阶段精度；**不改** abstract.txt；**不改**键数身份、单元格 1.000、product/kernel/weights/facts、投稿方向、定义 2／3 本文、判据格已钉句；**不叠** Ο1；**不重钉** Softguess／§8.1#3／8509·8769
- **Softguess：** tip 四文件口号 **NONE×4**；软读 **HIT 表 3 行标「阶段精度／Stage accuracy」**（本拍已钉）；残余候选：摘要／§1「精度均为 1.000」散文、combo「Accuracy 1.000」、§7.3 Adam「accuracy hit 1.000」（训练对照故意用语，勿钉成 T1）
- **Latest：** **v0.0.40**；冻结重测身份仍 **v0.0.19**；tip version **0.0.41**（本拍不 bump）

---

## 主张一句（本页唯一）

**表 3 行标裸「阶段精度」／「Stage accuracy」收紧为「阶段 T1」／「Stage T1」——与表 1 列头 T1 对齐，表身直接命名定义 2／声明域穷举逐键相等，堵住单看行标仍可读成训练／样本阶段精度的攻击面；CN/EN/TeX 三联；abstract.txt SAME；不引入 Softguess 口号字面。**

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md`

- **Before：** `| 阶段精度 | 全域枚举（声明域穷举逐键相等，即 T1／模型检验／定义 2，不是相对样本的拟合精度或训练精度） | 18 个阶段全部 1.000 |`
- **After：** `| 阶段 T1 | 全域枚举（声明域穷举逐键相等，即 T1／模型检验／定义 2，不是相对样本的拟合精度或训练精度） | 18 个阶段全部 1.000 |`
- 判据格已钉 T1 — **本拍不改判据格**；单元格 1.000 **不变**。

### EN `research/unisacc-paper.en.md`

- **Before：** `| Stage accuracy | Full-domain enumeration (= T1 / exhaustive key-by-key equality on the declared domain / Definition 2, not sample fit or training accuracy) | all 18 stages 1.000 |`
- **After：** `| Stage T1 | Full-domain enumeration (= T1 / exhaustive key-by-key equality on the declared domain / Definition 2, not sample fit or training accuracy) | all 18 stages 1.000 |`

### TeX `research/arxiv-paper-a/main.tex`

- **Before：** `Stage accuracy & Full-domain enumeration (= T1 / … Definition~2, not sample fit or training accuracy) & all 18 stages 1.000 \\`
- **After：** `Stage T1 & Full-domain enumeration (= T1 / … Definition~2, not sample fit or training accuracy) & all 18 stages 1.000 \\`

### abs `research/arxiv-paper-a/abstract.txt`

- **本拍 SAME**（不触）。

---

## 为何更难挑战

| 攻击读法 | Before 软点 | After 挡法 |
| --- | --- | --- |
| 单看表 3 行标「阶段精度／Stage accuracy」像训练 stage accuracy | 判据格已钉但行标仍裸 | 行名即阶段 T1＝定义 2／穷举逐键相等 |
| Softguess／破平近似合法出货 | 不涉及 | **未**粘 Softguess 口号；口号 NONE×4 |

---

## tip / blob 表

| 项 | before (`9647ed68`) | after（合并后填） |
| --- | --- | --- |
| tip | `9647ed68ef5caf4bf8163ab67ce13eacea822595` | （填） |
| CN `research/unisacc-paper.md` | `1a59b52ae25dcd11a0a70099688d6ff454be4a9e` | **CHANGED**（行标 阶段精度→阶段 T1） |
| EN `research/unisacc-paper.en.md` | `7370053fa9abba8cfc5947a6e1dfb27e821179f2` | **CHANGED**（行标 Stage accuracy→Stage T1） |
| TeX `research/arxiv-paper-a/main.tex` | `5b835dabd408bb66eaa85bc6a2eb9172cf259099` | **CHANGED**（行标 Stage accuracy→Stage T1） |
| abs `research/arxiv-paper-a/abstract.txt` | `92a6472c92a13cb4e8149bbd798f24e6d6a5f648` | **SAME** |
| Softguess 口号 | **NONE×4** | **NONE×4** |
| Softguess 软读 | HIT 表 3 行标「阶段精度／Stage accuracy」 | **HIT 表 3 行标**（本拍钉） |

---

## Softguess 机扫 @ 2026-10-12 14:18 Asia/Singapore（工作区 tip 四文件）

| 文件 | Softguess | softguess | soft-guess | Softmax-as-product | 权重就是 | weights are the logic | 逻辑本身 | 权重即逻辑 | 权重≡逻辑 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| EN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| TeX | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| abs | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

- **Softguess_NONE_x4 = true**（口号字面）
- **软读 HIT = 表 3 行标「阶段精度／Stage accuracy」**（本拍钉；非口号字面）
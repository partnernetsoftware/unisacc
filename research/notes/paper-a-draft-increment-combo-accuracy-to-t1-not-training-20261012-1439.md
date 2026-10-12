# Paper A 成篇定稿增量：combo 行「精度／Accuracy 1.000」→ T1 1.000（非训练精度）

- **拍点：** 2026-10-12 ~14:39 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `93b46173892212d1f49505883a592947acb1ab40`（短 `93b46173`）
- **tip after：** `bc88b8059e8a4216b39b6d333f293b53a5c355eb`（短 `bc88b805`；= squash Merge #275 本 A）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only CN+EN+TeX 表 1 combo 控制实验句裸「该行精度 1.000」／「Accuracy 1.000 on that row」改名为 **该行 T1 为 1.000**／**T1 1.000 on that row**；与表 1 列头 T1／表 3 行标 Stage T1 对齐；句内「相同的 T1 读法」保留；**不改** abstract.txt；**不改**键数身份、表单元格 1.000、product/kernel/weights/facts、投稿方向、定义 2／3 本文、§7.3 Adam「accuracy hit 1.000」（训练对照故意用语）；**不叠** Ο1；**不重钉** Softguess／§8.1#3／8509·8769
- **Softguess：** tip 四文件口号 **NONE×4**；软读 **HIT combo「精度／Accuracy 1.000」**（本拍已钉）；残余候选：摘要／§1「精度均为 1.000」散文（已有 T1 括注）、T1 状态格「全域枚举」、§7.3 Adam「accuracy hit 1.000」（勿钉成 T1）
- **Latest：** **v0.0.40**；冻结重测身份仍 **v0.0.19**；tip version **0.0.42**（本拍不 bump 论文键数／身份）

---

## 主张一句（本页唯一）

**表 1 combo 控制实验句裸「精度 1.000」／「Accuracy 1.000」收紧为「T1 为 1.000」／「T1 1.000」——与表 1 列头 T1 对齐，堵住单看 combo 句仍可读成训练／样本 accuracy 的攻击面；CN/EN/TeX 三联；abstract.txt SAME；不引入 Softguess 口号字面。**

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md`

- **Before：** `该行精度 1.000 只表明：对**该合并表自身声明的有限域**，构造网络与表逐键一致（即与其它行相同的 T1 读法）；`
- **After：** `该行 T1 为 1.000 只表明：对**该合并表自身声明的有限域**，构造网络与表逐键一致（即与其它行相同的 T1 读法）；`
- 单元格 1.000 **不变**；键数身份 **不变**。

### EN `research/unisacc-paper.en.md`

- **Before：** `Accuracy 1.000 on that row means only that the constructed network agrees with **that merged table's own declared finite domain** (the same T1 reading as every other row);`
- **After：** `T1 1.000 on that row means only that the constructed network agrees with **that merged table's own declared finite domain** (the same T1 reading as every other row);`

### TeX `research/arxiv-paper-a/main.tex`

- **Before：** `Accuracy 1.000 on that row means only that the constructed network agrees with \textbf{that merged table's own declared finite domain} (the same T1 reading as every other row);`
- **After：** `T1 1.000 on that row means only that the constructed network agrees with \textbf{that merged table's own declared finite domain} (the same T1 reading as every other row);`

### abs `research/arxiv-paper-a/abstract.txt`

- **本拍 SAME**（不触）。

---

## 为何更难挑战

| 攻击读法 | Before 软点 | After 挡法 |
| --- | --- | --- |
| 单看 combo「精度／Accuracy 1.000」像训练 accuracy | 句内已有 T1 读法但领头词仍裸 Accuracy | 领头词即 T1＝定义 2／声明域穷举逐键相等 |
| Softguess／破平近似合法出货 | 不涉及 | **未**粘 Softguess 口号；口号 NONE×4 |

---

## tip / blob 表

| 项 | before (`93b46173`) | after (`bc88b805`) |
| --- | --- | --- |
| tip | `93b46173892212d1f49505883a592947acb1ab40` | `bc88b8059e8a4216b39b6d333f293b53a5c355eb`（短 `bc88b805`；#275） |
| CN `research/unisacc-paper.md` | `19abf29013f019f1ea87171a512d637f3ae7809f` | **CHANGED** → `5aecb9e0d037fa546a32e84e68b33ec36eca795e` |
| EN `research/unisacc-paper.en.md` | `f7ab3c8551cac933c9ca684e4b81fc85b2e1270a` | **CHANGED** → `ea74eda6a3a2c2d10e92b1ce05ba3c09f82db386` |
| TeX `research/arxiv-paper-a/main.tex` | `99b0e09fcce3caa33b4ebc1baf8f2e799f2e0851` | **CHANGED** → `7ca29cc5c9c24a6ffb3a7107b9f731915a9a5097` |
| abs `research/arxiv-paper-a/abstract.txt` | `92a6472c92a13cb4e8149bbd798f24e6d6a5f648` | **SAME** |
| Softguess 口号 | **NONE×4** | **NONE×4** |
| Softguess 软读 | HIT combo「精度／Accuracy 1.000」 | **HIT combo**（本拍钉） |

---

## Softguess 机扫 @ 2026-10-12 ~14:39 Asia/Singapore（工作区 tip 四文件）

| 文件 | Softguess | softguess | soft-guess | Softmax-as-product | 权重就是 | weights are the logic | 逻辑本身 | 权重即逻辑 | 权重≡逻辑 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| EN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| TeX | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| abs | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

- **Softguess_NONE_x4 = true**（口号字面）
- **软读 HIT = combo「精度／Accuracy 1.000」**（本拍钉；非口号字面）

---

## 同族链

0956 → … → 1358 表 1 列头 Accuracy→T1 → 1418 表 3 行标 阶段精度→阶段 T1 → **1439 combo Accuracy→T1**

# Paper A 成篇定稿增量：附录 CLI「全域精度」／full-domain accuracy 钉为 T1／非训练精度三联对齐

- **拍点：** 2026-10-12 ~12:43 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `f78a575bc70efb9d91a161adf3943c5974a0ce70`（短 `f78a575b`；= #260 tip-after fill for #259 §6.1-enumeration；相对任务书 tip `f78a575b` **无漂**）
- **tip after：** _(merge 后填；见 follow-up notes PR 若本 commit 时尚未知)_
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only CN+EN+TeX 附录命令注释「全域精度」／「full-domain accuracy」同义收紧为 T1／定义 2；**不改** abstract.txt（无本短语）；**不改**键数身份、表 1／表 3／§6.1／Def2／摘要已钉句、product/kernel/weights/facts、投稿方向；**不叠** Ο1；**不重钉** Softguess／§8.1#3／8509·8769
- **Softguess：** tip 四文件 **NONE×4**（机扫 `Softguess`/`softguess`/`soft-guess`/`Softmax-as-product`／「权重就是」／“weights are the logic”／「逻辑本身」／「权重即逻辑」／「权重≡逻辑」均 0 hit；§3「不含 softmax」／「no softmax」否认句不计；**未**粘 Softguess 口号）
- **Latest：** **v0.0.40**；冻结重测身份仍 **v0.0.19**；tip version **0.0.41**（本拍不 bump）
- **同族链（本拍续）：** 0956 → 1011 §7.4 → 1026 §10 → 1046 abstract → 1056 contrib4 → 1109 contrib1 Method → 1130 Def2 exactness → 1141 精度 1.000＝T1 → 1156 表 1 表注精度＝T1 → 1209 表 3 阶段精度判据＝T1 → 1225 §6.1 全域枚举开篇＝T1 → **本拍附录 CLI「全域精度」／full-domain accuracy＝T1 非训练精度**

---

## 主张一句（本页唯一）

**表 1 表注、表 3「阶段精度」判据与 §6.1「全域枚举」开篇已钉为 T1／定义 2（#254／#257／#259）之后，附录可复现命令注释仍只写「全域精度」／「full-domain accuracy」，可软读为样本拟合或训练准确率；本拍在保留 `python3 -m unisa acc` 命令与「表 1／Table 1」指称的同时，括注其为声明域穷举逐键相等（T1／模型检验／定义 2），并排除相对样本的拟合精度或训练精度；CN/EN/TeX 三联；abs SAME；不改键数、不引入 Softguess 口号字面。**

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md` 附录 CLI

- **Before：** `python3 -m unisa acc                            # 表 1：全域精度`
- **After：** `python3 -m unisa acc                            # 表 1：全域精度（声明域穷举逐键相等，即 T1／模型检验／定义 2，不是相对样本的拟合精度或训练精度）`

### EN `research/unisacc-paper.en.md` appendix CLI

- **Before：** `python3 -m unisa acc                            # Table 1: full-domain accuracy`
- **After：** `python3 -m unisa acc                            # Table 1: full-domain accuracy (T1 / exhaustive key-by-key equality on the declared domain / Definition 2, not sample fit or training accuracy)`

### TeX `research/arxiv-paper-a/main.tex` appendix CLI

- **Before：** `python3 -m unisa acc                            # Table 1: full-domain accuracy`
- **After：** `python3 -m unisa acc                            # Table 1: full-domain accuracy (T1 / exhaustive key-by-key equality on the declared domain / Definition~2, not sample fit or training accuracy)`

### abstract `research/arxiv-paper-a/abstract.txt`

- **本拍不改**（无「全域精度」／「full-domain accuracy」短语；摘要「all exact」已于 #252 钉 T1）。

### 邻近句（可选 follow-through；本拍不改）

- 定理档 T1 状态列「全域枚举」／full-domain enumeration、P-3「做全域枚举」、§7.3 Adam「accuracy hit 1.000」（训练对照语境）——不扩改。

---

## 为何更难挑战

| 攻击读法 | Before 软点 | After 挡法 |
| --- | --- | --- |
| 附录「全域精度」＝样本准确率／训练 accuracy | 命令注释未钉 T1≠训练精度 | 括注「声明域穷举逐键相等（T1／模型检验／定义 2）」／「T1 / … Definition 2」 |
| 可复现命令块「精度」族可软读为 ML metric | 表 1／表 3／§6.1 已钉，附录 CLI 未钉 | 在附录 `unisa acc` 注释入口显式排除「相对样本的拟合精度或训练精度」 |
| Softguess／破平近似合法出货 | 不涉及 | **未**粘 Softguess 口号；Softguess NONE×4 保持 |

---

## tip / blob 表

| 项 | before (`f78a575b`) | after（本拍；merge 后填 tip） |
| --- | --- | --- |
| tip | `f78a575bc70efb9d91a161adf3943c5974a0ce70` | _(merge 后填)_ |
| CN `research/unisacc-paper.md` | `8067f705b685169bd32b83bc0e57c3063dc70289` | **CHANGED** → `4f5da1c5af011500fd27542155901b81c5778380` |
| EN `research/unisacc-paper.en.md` | `0fda19450c03cc8a1c8165dd721222b7c6420328` | **CHANGED** → `eb01072afc097b3dcbd54120c72cb284c13937ad` |
| TeX `research/arxiv-paper-a/main.tex` | `f519dc81c9b139e565f8c4bcbefd8af31f56cf5e` | **CHANGED** → `6df0c6a4e23e4b1e80551b3c047377c02e7190c7` |
| abs `research/arxiv-paper-a/abstract.txt` | `980f670df7734b49a5d90bb3c305cd2b565d0b8b` | **SAME**（未改；`980f670df7734b49a5d90bb3c305cd2b565d0b8b`） |
| Softguess | **NONE×4** | **NONE×4** |

---

## Softguess 机扫 @ 2026-10-12 12:43 Asia/Singapore（工作区，merge 前 tip 四文件 + 改后）

| 文件 | Softguess | softguess | soft-guess | Softmax-as-product | 权重就是 | weights are the logic | 逻辑本身 | 权重即逻辑 | 权重≡逻辑 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| EN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| TeX | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| abstract | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

- **Softguess_NONE_x4 = true**

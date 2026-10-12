# Paper A 成篇定稿增量：abstract.txt「all exact」钉为 T1／非训练精度（闭合 1329 残余）

- **拍点：** 2026-10-12 ~13:45 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `75fd4ea0023c281bb5bffed30d319b9b5c54a1fd`（短 `75fd4ea0`；= #268 after A #267 §9 full-domain enum）
- **tip after：** `0af0e41d3d5aa8c874b01e27fd47afa3e2f407fb`（短 `0af0e41d`；= squash Merge #269 本 A）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only 闭合上拍 #267 笔记明示的残余候选：`research/arxiv-paper-a/abstract.txt` 中「all exact」相对 TeX `\begin{abstract}` 已钉句漂移；**不改** CN/EN/TeX 正文（摘要开篇「精度均为 1.000／accuracy 1.000」与 TeX abs 已钉 T1）；**不改**键数身份、表 1／表 3／§6.1／Def2／附录 CLI／P-3／§7.4／§9／product/kernel/weights/facts、投稿方向；**不叠** Ο1；**不重钉** Softguess／§8.1#3／8509·8769
- **Softguess：** 口号机扫 **NONE×4**；软读扫描 **HIT abs「all exact」**（本拍已钉）；残余候选：表头「精度／Accuracy」、T1 状态格「全域枚举」、§7.3 Adam「accuracy hit 1.000」（训练对照故意用语，勿钉成 T1）
- **Latest：** **v0.0.40**；冻结重测身份仍 **v0.0.19**；tip version **0.0.41**（本拍不 bump）

---

## 主张一句（本页唯一）

**`abstract.txt` 决策网络实例句内裸「all exact」收紧为「all exact under T1 / exhaustive key-by-key equality on the declared domain (Definition 2), not sample fit or training accuracy」——与已硬的 TeX `\begin{abstract}` 同质；堵住 arXiv 纯文本摘要仍可被读成训练／样本精度的攻击面；CN/EN markdown 摘要与 TeX abs 本拍 SAME（已钉）；不引入 Softguess 口号字面。**

---

## Before / after 句段（精确）

### abs `research/arxiv-paper-a/abstract.txt`

- **Before：** `…enumerated from 18 constructed networks (8,509 keys, op-74 vocabulary identity, all exact; Chinese internal draft Table 1 currently uses distinct op-87 identity 8,769/569 — do not collapse until one sealed measurement)…`
- **After：** `…enumerated from 18 constructed networks (8,509 keys, op-74 vocabulary identity, all exact under T1 / exhaustive key-by-key equality on the declared domain (Definition 2), not sample fit or training accuracy; Chinese internal draft Table 1 currently uses distinct op-87 identity 8,769/569 — do not collapse until one sealed measurement)…`
- **本拍改**（闭合 1329 「残余候选 abs「all exact」等未叠本拍」）。

### CN / EN / TeX 正文

- **本拍不改**（CN 摘要「精度均为 1.000（此为声明域穷举…）」、EN「accuracy 1.000 … (exhaustive key-by-key … T1 …)」、TeX abs「all exact under T1 …」均已钉；本拍只对齐滞后的 `abstract.txt`）。

---

## 为何更难挑战

| 攻击读法 | Before 软点 | After 挡法 |
| --- | --- | --- |
| arXiv 纯文本摘要「all exact」像训练精度／样本准确率 | 裸「all exact」无 T1／定义 2／非拟合括注 | 显式 T1 + exhaustive key-by-key + not sample fit or training accuracy |
| TeX abs 已硬、`abstract.txt` 仍软，双通道摘要漂移 | 1329 故意 abs SAME，记残余 | 闭合与 TeX `\begin{abstract}` 同质钉句 |
| Softguess／破平近似合法出货 | 不涉及 | **未**粘 Softguess 口号；口号 NONE×4 |

---

## tip / blob 表

| 项 | before (`75fd4ea0`) | after (`0af0e41d`) |
| --- | --- | --- |
| tip | `75fd4ea0023c281bb5bffed30d319b9b5c54a1fd` | `0af0e41d3d5aa8c874b01e27fd47afa3e2f407fb`（短 `0af0e41d`；#269） |
| CN `research/unisacc-paper.md` | `5711fd4991c347abdc161acaf894488e755bcacb` | **SAME** |
| EN `research/unisacc-paper.en.md` | `55f103209cadb9aedf6cf2f3456c9728a9b337a7` | **SAME** |
| TeX `research/arxiv-paper-a/main.tex` | `f1e93825d0465a578c286c4361dd02a5daf74036` | **SAME** |
| abs `research/arxiv-paper-a/abstract.txt` | `980f670df7734b49a5d90bb3c305cd2b565d0b8b` | **CHANGED** → `92a6472c92a13cb4e8149bbd798f24e6d6a5f648` |
| Softguess 口号 | **NONE×4** | **NONE×4** |
| Softguess 软读 | 残余 abs「all exact」 | **HIT abs「all exact」**（本拍钉） |

---

## Softguess 机扫 @ 2026-10-12 13:45 Asia/Singapore（工作区，merge 前 tip 四文件 + 改后 abs）

| 文件 | Softguess | softguess | SoftGuess | 权重就是 | weights are the logic | 逻辑本身 | 权重即逻辑 | 权重≡逻辑 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| EN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| TeX | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| abstract | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

- **Softguess_NONE_x4 = true**（口号字面）
- **软读 HIT = abs「all exact」**（本拍钉；非口号字面）

---

## 同族链

0956 → 1011 §7.4 → 1026 §10 → 1046 abstract → 1056 贡献 #4 → 1109 贡献 #1 Method → 1130 定义 2 exactness → 1141 精度 1.000＝T1 → 1156 表 1 表注精度＝T1 → 1209 表 3 阶段精度判据＝T1 → 1225 §6.1 全域枚举开篇＝T1 → 1243 附录 CLI 全域精度＝T1 → 1301 P-3 全域枚举＝T1 → 1308 §7.4 枚举 1.000＝T1 → 1329 §9 全定义域枚举＝T1 → **1345 abs「all exact」＝T1**

---

## 风险自审（可自合）

- research-only；仅 `abstract.txt` + notes
- 不改键数、product、方向主张
- 不叠 Ο1
- 低风险：自 review + merge

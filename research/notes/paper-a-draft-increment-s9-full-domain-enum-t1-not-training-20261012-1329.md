# Paper A 成篇定稿增量：§9「全定义域枚举」／full declared domain 钉为 T1／非训练精度三联对齐

- **拍点：** 2026-10-12 ~13:29 Asia/Shanghai（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `b636295141a54d374989b776dc12b932566c1a1f`（短 `b636295`；= #266 tip-after fill on main；paper 父 `#265` tip `96cc83fa` 之后）
- **tip after：** _(merge 后填；本文件随 A PR 提交时占位)_
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only CN+EN+TeX §9 相关工作「有限自动机与程序的网络编码」／「Network encodings…」句内「全定义域枚举」／「full declared domain」／TeX 另钉「full-domain enumeration as the release criterion」同义收紧为 T1／定义 2；**不改** abstract.txt（本拍优先 abs SAME；摘要「all exact」已在 TeX/EN 开篇钉过 T1，留作下一拍候选）；**不改**键数身份、表 1／表 3／§6.1／Def2／附录 CLI／P-3／§7.4／摘要已钉句、product/kernel/weights/facts、投稿方向；**不叠** Ο1；**不重钉** Softguess／§8.1#3／8509·8769
- **Softguess：** tip 四文件口号机扫 **NONE×4**（`Softguess`/`softguess`/`soft-guess`/`Softmax-as-product`／「权重就是」／“weights are the logic”／「逻辑本身」／「权重即逻辑」／「权重≡逻辑」均 0 hit；§3「不含 softmax」／「no softmax」否认句不计；**未**粘 Softguess 口号）；**软读扫描 HIT §9**（本拍钉点）
- **Latest：** **v0.0.40**；冻结重测身份仍 **v0.0.19**；tip version **0.0.41**（本拍不 bump）
- **同族链（本拍续）：** 0956 → 1011 §7.4 → 1026 §10 → 1046 abstract → 1056 contrib4 → 1109 contrib1 Method → 1130 Def2 exactness → 1141 精度 1.000＝T1 → 1156 表 1 表注精度＝T1 → 1209 表 3 阶段精度判据＝T1 → 1225 §6.1 全域枚举开篇＝T1 → 1243 附录 CLI 全域精度＝T1 → 1301 P-3 判据「全域枚举」＝T1 → 1308 §7.4「枚举 1.000」＝T1 → **本拍 §9「全定义域枚举」／full declared domain＝T1 非训练精度**

---

## 主张一句（本页唯一）

**表 1／表 3／§6.1／附录 CLI／P-3／§7.4 已钉「精度／枚举」为 T1／定义 2 之后，§9 相关工作对照句仍只写「全定义域枚举」／「full declared domain」（TeX 另有「full-domain enumeration as the release criterion」），可软读为样本拟合或训练准确率式发布判据；本拍括注其为声明域穷举逐键相等（T1／模型检验／定义 2），并排除相对样本的拟合精度或训练精度；CN/EN/TeX 三联；abs SAME；不改键数、不引入 Softguess 口号字面。**

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md` §9

- **Before：** `本文另以全定义域枚举核对每张发布网络与其表。`
- **After：** `本文另以全定义域枚举（声明域穷举逐键相等，即 T1／模型检验／定义 2，不是相对样本的拟合精度或训练精度）核对每张发布网络与其表。`

### EN `research/unisacc-paper.en.md` §9

- **Before：** `here each released network is additionally checked against its table over the full declared domain.`
- **After：** `here each released network is additionally checked against its table over the full declared domain (= T1 / exhaustive key-by-key equality / Definition 2, not sample fit or training accuracy).`

### TeX `research/arxiv-paper-a/main.tex` §9

- **Before₁：** `here each released network is additionally checked against its table over the full declared domain.`
- **After₁：** `here each released network is additionally checked against its table over the full declared domain (= T1 / exhaustive key-by-key equality / Definition~2, not sample fit or training accuracy).`
- **Before₂：** `and use full-domain enumeration as the release criterion.`
- **After₂：** `and use full-domain enumeration (= T1 / exhaustive key-by-key equality on the declared domain / Definition~2, not sample fit or training accuracy) as the release criterion.`

### abstract `research/arxiv-paper-a/abstract.txt`

- **本拍不改**（优先 abs SAME；「all exact」与 TeX 开篇已钉句的漂移记为下一拍候选，不叠本拍）。

---

## 为何更难挑战

| 攻击读法 | Before 软点 | After 挡法 |
| --- | --- | --- |
| §9「全定义域枚举」＝样本准确率／训练 accuracy | 相关工作对照句未钉 T1≠训练精度 | 括注「声明域穷举逐键相等（T1／模型检验／定义 2）」／「= T1 / … Definition 2」 |
| TeX「full-domain enumeration as the release criterion」可软读为 ML metric 发布门 | EN 无该尾句；TeX 未钉 | 在 release criterion 处显式排除 sample fit / training accuracy |
| Softguess／破平近似合法出货 | 不涉及 | **未**粘 Softguess 口号；口号 Softguess NONE×4 保持 |

---

## tip / blob 表

| 项 | before (`b636295`) | after（#A tip 待填） |
| --- | --- | --- |
| tip | `b636295141a54d374989b776dc12b932566c1a1f` | _(merge 后填)_ |
| CN `research/unisacc-paper.md` | `7b3b8735aaed1207a9501e33bd25b3bf3a725384` | **CHANGED** → `5711fd4991c347abdc161acaf894488e755bcacb` |
| EN `research/unisacc-paper.en.md` | `0fd1612e8d9e1ce41b25877c467f16cd6aab7f90` | **CHANGED** → `55f103209cadb9aedf6cf2f3456c9728a9b337a7` |
| TeX `research/arxiv-paper-a/main.tex` | `ad31486ecf8f2abdad85bf10eb09a247c6fc8778` | **CHANGED** → `f1e93825d0465a578c286c4361dd02a5daf74036` |
| abs `research/arxiv-paper-a/abstract.txt` | `980f670df7734b49a5d90bb3c305cd2b565d0b8b` | **SAME**（未改；`980f670df7734b49a5d90bb3c305cd2b565d0b8b`） |
| Softguess 口号 | **NONE×4** | **NONE×4** |
| 软读扫描 | **HIT §9**（本拍钉） | §9 已钉；残余候选：abs「all exact」、表头「精度／Accuracy」、T1 状态格「全域枚举」、§7.3 Adam「accuracy hit 1.000」（训练对照故意用语） |

---

## Softguess 口号机扫 @ 2026-10-12 13:29 Asia/Shanghai（工作区，merge 前 tip 四文件 + 改后）

| 文件 | Softguess | softguess | soft-guess | Softmax-as-product | 权重就是 | weights are the logic | 逻辑本身 | 权重即逻辑 | 权重≡逻辑 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| EN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| TeX | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| abstract | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

- **Softguess_NONE_x4 = true**（口号族）
- **软读 HIT = §9「全定义域枚举」／full declared domain／TeX release-criterion 句**（本拍已钉）

---

## 挡粘点（仍开，本拍未动）

- §8.1#3 同身份平台矩阵
- 8509/8769 双身份
- Ο1（荐冻结 v0.0.19 vs Latest）— **不叠新决策卡**；仍荐 Ο1

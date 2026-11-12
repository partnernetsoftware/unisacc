# Paper A 成篇定稿增量：§7.4「枚举 1.000」／enumeration 1.000 钉为 T1／非训练精度三联对齐

- **拍点：** 2026-10-12 ~13:08 Asia/Shanghai（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `620fcc138a105b47b06449697b4c76e3bebd8c49`（短 `620fcc13`；= #264 tip-after fill on main；paper 父 `#263` tip `140ecf8e` 之后）
- **tip after：** `96cc83fa1859f6beb12b13a418219058fcba7574`（短 `96cc83fa`；= squash Merge #265 本 A）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only CN+EN+TeX §7.4「真值表本身会错」／「Truth tables can be wrong」句内「枚举 1.000」／「enumeration 1.000」同义收紧为 T1／定义 2；**不改** abstract.txt（无该短语）；**不改**键数身份、表 1／表 3／§6.1／Def2／附录 CLI／P-3／摘要已钉句、product/kernel/weights/facts、投稿方向；**不叠** Ο1；**不重钉** Softguess／§8.1#3／8509·8769
- **Softguess：** tip 四文件 **NONE×4**（机扫 `Softguess`/`softguess`/`soft-guess`/`Softmax-as-product`／「权重就是」／“weights are the logic”／「逻辑本身」／「权重即逻辑」／「权重≡逻辑」均 0 hit；§3「不含 softmax」／「no softmax」否认句不计；**未**粘 Softguess 口号）
- **Latest：** **v0.0.40**；冻结重测身份仍 **v0.0.19**；tip version **0.0.41**（本拍不 bump）
- **同族链（本拍续）：** 0956 → 1011 §7.4 → 1026 §10 → 1046 abstract → 1056 contrib4 → 1109 contrib1 Method → 1130 Def2 exactness → 1141 精度 1.000＝T1 → 1156 表 1 表注精度＝T1 → 1209 表 3 阶段精度判据＝T1 → 1225 §6.1 全域枚举开篇＝T1 → 1243 附录 CLI 全域精度＝T1 → 1301 P-3 判据「全域枚举」＝T1 → **本拍 §7.4「枚举 1.000」／enumeration 1.000＝T1 非训练精度**

---

## 主张一句（本页唯一）

**表 1 表注、表 3、§6.1、附录 CLI、P-3 已钉「全域枚举／精度」为 T1／定义 2（#254／#257／#259／#261／#263）之后，§7.4「真值表本身会错」仍只写「枚举 1.000」／「enumeration 1.000」，可软读为样本拟合或训练准确率；本拍在保留「全部套件通过／every suite green」并列结构的同时，括注其为声明域穷举逐键相等（T1／模型检验／定义 2），并排除相对样本的拟合精度或训练精度；CN/EN/TeX 三联；abs SAME；不改键数、不引入 Softguess 口号字面。**

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md` §7.4

- **Before：** `它在整个项目期间都存在——枚举 1.000、全部套件通过——`
- **After：** `它在整个项目期间都存在——枚举 1.000（声明域穷举逐键相等，即 T1／模型检验／定义 2，不是相对样本的拟合精度或训练精度）、全部套件通过——`

### EN `research/unisacc-paper.en.md` §7.4

- **Before：** `It survived the whole project — enumeration 1.000, every suite green —`
- **After：** `It survived the whole project — enumeration 1.000 (= T1 / exhaustive key-by-key equality on the declared domain / Definition 2, not sample fit or training accuracy), every suite green —`

### TeX `research/arxiv-paper-a/main.tex` §7.4

- **Before：** `It survived the whole project --- enumeration 1.000, every suite green ---`
- **After：** `It survived the whole project --- enumeration 1.000 (= T1 / exhaustive key-by-key equality on the declared domain / Definition~2, not sample fit or training accuracy), every suite green ---`

### abstract `research/arxiv-paper-a/abstract.txt`

- **本拍不改**（无 §7.4「enumeration 1.000」短语；摘要「all exact」已于先前节拍钉 T1／且本拍不叠摘要）。

---

## 为何更难挑战

| 攻击读法 | Before 软点 | After 挡法 |
| --- | --- | --- |
| §7.4「枚举 1.000」＝样本准确率／训练 accuracy | 叙述句未钉 T1≠训练精度 | 括注「声明域穷举逐键相等（T1／模型检验／定义 2）」／「= T1 / … Definition 2」 |
| 「套件全绿 + 1.000」族可软读为 ML metric | 表 1／表 3／§6.1／附录／P-3 已钉，§7.4 未钉 | 在缺陷叙事入口显式排除「相对样本的拟合精度或训练精度」 |
| Softguess／破平近似合法出货 | 不涉及 | **未**粘 Softguess 口号；Softguess NONE×4 保持 |

---

## tip / blob 表

| 项 | before (`620fcc13`) | after（#265 tip `96cc83fa`） |
| --- | --- | --- |
| tip | `620fcc138a105b47b06449697b4c76e3bebd8c49` | `96cc83fa1859f6beb12b13a418219058fcba7574`（短 `96cc83fa`；#265） |
| CN `research/unisacc-paper.md` | `eda0b09521c35220014d7333357929779d041673` | **CHANGED** → `7b3b8735aaed1207a9501e33bd25b3bf3a725384` |
| EN `research/unisacc-paper.en.md` | `61e7999a213837b0e3a6424b39ccbd728c637101` | **CHANGED** → `0fd1612e8d9e1ce41b25877c467f16cd6aab7f90` |
| TeX `research/arxiv-paper-a/main.tex` | `7d315a722d493b018cb6764386f2e9e277f6e91a` | **CHANGED** → `ad31486ecf8f2abdad85bf10eb09a247c6fc8778` |
| abs `research/arxiv-paper-a/abstract.txt` | `980f670df7734b49a5d90bb3c305cd2b565d0b8b` | **SAME**（未改；`980f670df7734b49a5d90bb3c305cd2b565d0b8b`） |
| Softguess | **NONE×4** | **NONE×4** |

---

## Softguess 机扫 @ 2026-10-12 13:08 Asia/Shanghai（工作区，merge 前 tip 四文件 + 改后）

| 文件 | Softguess | softguess | soft-guess | Softmax-as-product | 权重就是 | weights are the logic | 逻辑本身 | 权重即逻辑 | 权重≡逻辑 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| EN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| TeX | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| abstract | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

- **Softguess_NONE_x4 = true**

---

## 挡粘点（仍开，本拍未动）

- §8.1#3 同身份平台矩阵
- 8509/8769 双身份
- Ο1（荐冻结 v0.0.19 vs Latest）— **不叠新决策卡**；仍荐 Ο1

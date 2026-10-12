# Paper A 成篇定稿增量：§6.1「全域枚举」开篇钉为 T1／非训练精度三联对齐

- **拍点：** 2026-10-12 ~12:25 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `76820857d7d21c680c8daca884f9291e2d199ba9`（短 `76820857`；= #258 tip-after fill for #257 Table3-stage-accuracy；相对任务书 tip `7682085` **无漂**）
- **tip after：** `TIP_AFTER_PENDING`（短 `TIP_AFTER_SHORT`；= squash Merge #PRNUM 本 A）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only CN+EN+TeX §6.1「全域枚举（T1）」／「Full-domain enumeration (T1)」开篇句同义收紧为 T1／定义 2；**不改** abstract.txt（无 §6.1 开篇）；**不改**键数身份、表 1／表 3 已钉句、product/kernel/weights/facts、投稿方向、定义 2／3 本文、摘要已钉句；**不叠** Ο1；**不重钉** Softguess／§8.1#3／8509·8769
- **Softguess：** tip 四文件 **NONE×4**（机扫 `Softguess`/`softguess`/`soft-guess`/`Softmax-as-product`／「权重就是」／“weights are the logic”／「逻辑本身」／「权重即逻辑」／「权重≡逻辑」均 0 hit；§3「不含 softmax」／「no softmax」否认句不计；**未**粘 Softguess 口号）
- **Latest：** **v0.0.40**；冻结重测身份仍 **v0.0.19**；tip version **0.0.41**（本拍不 bump）
- **同族链（本拍续）：** 0956 → 1011 §7.4 → 1026 §10 → 1046 abstract → 1056 contrib4 → 1109 contrib1 Method → 1130 Def2 exactness → 1141 精度 1.000＝T1 → 1156 表 1 表注精度＝T1 → 1209 表 3 阶段精度判据＝T1 → **本拍 §6.1 全域枚举开篇＝T1 非训练精度**

---

## 主张一句（本页唯一）

**表 1 表注与表 3「阶段精度」判据已钉为 T1／定义 2（#254／#257）之后，§6「验证方法」§6.1「全域枚举（T1）」开篇仍只写「每一个键上按部署算术运行」／「on every key of the original domain」，未显式排除样本拟合／训练准确率读法；本拍在保留「每一个键／every key」事实与拒绝发布纪律的同时，括注其为声明域穷举逐键相等（T1／模型检验／定义 2），并排除相对样本的拟合精度或训练精度；CN/EN/TeX 三联；abs SAME；不改键数、不引入 Softguess 口号字面。**

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md` §6.1 开篇

- **Before：** `每个立方体网络在原始定义域的**每一个**键上按部署算术运行；只要有一个键答错或出现并列，构造即失败，拒绝发布。`
- **After：** `每个立方体网络在原始定义域的**每一个**键上按部署算术运行（此为声明域穷举逐键相等，即 T1／模型检验／定义 2，不是相对样本的拟合精度或训练精度）；只要有一个键答错或出现并列，构造即失败，拒绝发布。`

### EN `research/unisacc-paper.en.md` §6.1 opening

- **Before：** `Every cube network is run with deployment arithmetic on **every** key of the original domain; any wrong key or tie fails construction and blocks release.`
- **After：** `Every cube network is run with deployment arithmetic on **every** key of the original domain (this is T1 / exhaustive key-by-key equality on the declared domain / Definition 2, not sample fit or training accuracy); any wrong key or tie fails construction and blocks release.`

### TeX `research/arxiv-paper-a/main.tex` §6.1 opening

- **Before：** `Every cube network is run with deployment arithmetic on \textbf{every} key of the original domain; any wrong key or tie fails construction and blocks release.`
- **After：** `Every cube network is run with deployment arithmetic on \textbf{every} key of the original domain (this is T1 / exhaustive key-by-key equality on the declared domain / Definition~2, not sample fit or training accuracy); any wrong key or tie fails construction and blocks release.`

### abstract `research/arxiv-paper-a/abstract.txt`

- **本拍不改**（无 §6.1 开篇；摘要「all exact」已于 #252 钉 T1）。

### 邻近句（可选 follow-through；本拍不改）

- 定理档 T1 状态列「全域枚举」／full-domain enumeration、P-3「做全域枚举」、附录命令注释「全域精度」／full-domain accuracy、§7.3 Adam「accuracy hit 1.000」（训练对照语境）——不扩改。

---

## 为何更难挑战

| 攻击读法 | Before 软点 | After 挡法 |
| --- | --- | --- |
| §6.1「每一个键」＝样本覆盖率／训练准确率验收 | 开篇未钉 T1≠训练精度 | 括注「此为声明域穷举逐键相等（T1／模型检验／定义 2）」／「this is T1 / … Definition 2」 |
| 验证章「精度」族可软读为 ML metric | 表 1／表 3 已钉，验证方法开篇未钉 | 在 §6.1 验证方法入口显式排除「相对样本的拟合精度或训练精度」 |
| Softguess／破平近似合法出货 | 不涉及 | **未**粘 Softguess 口号；Softguess NONE×4 保持 |

---

## tip / blob 表

| 项 | before (`76820857`) | after（本拍；merge 后填） |
| --- | --- | --- |
| tip | `76820857d7d21c680c8daca884f9291e2d199ba9` | `TIP_AFTER_PENDING`（短 `TIP_AFTER_SHORT`；#PRNUM） |
| CN `research/unisacc-paper.md` | `b1568103f5ab1e71aecae0027c3ec03ef1cf3b4e` | **CHANGED** → `CN_BLOB_PENDING` |
| EN `research/unisacc-paper.en.md` | `98a7e0a13493674afa78b7241b43e844cd4fed4a` | **CHANGED** → `EN_BLOB_PENDING` |
| TeX `research/arxiv-paper-a/main.tex` | `03c69b5f35885d18e084895957c0de9a6ea2e07c` | **CHANGED** → `TEX_BLOB_PENDING` |
| abs `research/arxiv-paper-a/abstract.txt` | `980f670df7734b49a5d90bb3c305cd2b565d0b8b` | **SAME**（未改；`980f670df7734b49a5d90bb3c305cd2b565d0b8b`） |
| Softguess | **NONE×4** | **NONE×4** |

---

## Softguess 机扫 @ 2026-10-12 12:25 Asia/Singapore（工作区，merge 前 tip 四文件 + 改后）

| 文件 | Softguess | softguess | soft-guess | Softmax-as-product | 权重就是 | weights are the logic | 逻辑本身 | 权重即逻辑 | 权重≡逻辑 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| EN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| TeX | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| abstract | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

- **Softguess_NONE_x4 = true**

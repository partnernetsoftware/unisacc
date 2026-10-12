# Paper A 成篇定稿增量：摘要对齐 §7.4/§10「不是训练的替代或逼近」三联+abs

- **拍点：** 2026-10-12 ~10:46 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `3205de9b8e305495981c0525d04dbe5b0c598eae`（短 `3205de9b`；= #243 after A #242 §10 conclusion-not-training-substitute；相对任务书 tip `3205de9b` **无漂**）
- **tip after：** `TIP_AFTER_PLACEHOLDER`（短 `TIP_AFTER_SHORT`；= squash Merge #N 本 A；merge 后回填）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only CN+EN+TeX+abs 摘要第三段同义收紧，对齐已硬的 §7.4/§10；**不改**键数身份、product/kernel/weights/facts、投稿方向；**不叠** Ο1（仍荐）；**不重钉** Softguess／§8.1#3／8509·8769
- **Softguess：** tip 四文件 **NONE×4**（机扫 `Softguess`/`softguess`/`SoftGuess`/`权重就是`/`weights are the logic`/`逻辑本身`/`权重即逻辑`/`权重≡逻辑` 均 0 hit；§3「不含 softmax」／「no softmax」否认句不计；**未**粘 Softguess 口号）
- **Latest：** **v0.0.40**；冻结重测身份仍 **v0.0.19**；tip version **0.0.41**（本拍不 bump）

---

## 主张一句（本页唯一）

**摘要第三段在「产品与主张路径仅用构造／rely on construction, not training」之后补一句「构造—枚举不是训练的替代品或逼近，而是与训练不同类的验证纪律（权重由表构造而非训练）／construction plus enumeration is not a substitute for—or approximation of—training, but a distinct verification discipline (weights constructed from tables, not trained)」——与已硬的 §7.4/§10 同质；堵住摘要仍可被读成「只是另一种训练替代」的攻击面；同一实质改动同步进 CN/EN/TeX/abs；不引入 Softguess 口号字面。**

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md` 摘要第三段

- **Before：** `**产品与主张路径仅用构造**，不依赖训练；§7.3 仅记录一次历史 Adam 示意对照，预注册研究 A2 负责系统性训练对照。`
- **After：** `**产品与主张路径仅用构造**，不依赖训练；构造—枚举不是训练的替代品或逼近，而是一种与训练不同类的验证纪律（权重由表构造而非训练）。§7.3 仅记录一次历史 Adam 示意对照，预注册研究 A2 负责系统性训练对照。`

### EN `research/unisacc-paper.en.md` Abstract ¶3

- **Before：** `**The product and our claims rely on construction, not training**; §7.3 records one illustrative historical Adam comparison only, and preregistered study A2 owns systematic training comparisons.`
- **After：** `**The product and our claims rely on construction, not training**; construction plus enumeration is not a substitute for—or approximation of—training, but a distinct verification discipline (weights constructed from tables, not trained). §7.3 records one illustrative historical Adam comparison only, and preregistered study A2 owns systematic training comparisons.`

### TeX `research/arxiv-paper-a/main.tex` `\begin{abstract}`

- **Before：** `\textbf{The product and our claims rely on construction, not training}; §7.3 records one illustrative historical Adam comparison only, and preregistered study A2 owns systematic training comparisons.`
- **After：** `\textbf{The product and our claims rely on construction, not training}; construction plus enumeration is not a substitute for---or approximation of---training, but a distinct verification discipline (weights constructed from tables, not trained). §7.3 records one illustrative historical Adam comparison only, and preregistered study A2 owns systematic training comparisons.`

### abstract `research/arxiv-paper-a/abstract.txt`

- **Before：** `The product and our claims rely on construction, not training; section 7.3 records one illustrative historical Adam comparison only, and preregistered study A2 owns systematic training comparisons.`
- **After：** `The product and our claims rely on construction, not training; construction plus enumeration is not a substitute for—or approximation of—training, but a distinct verification discipline (weights constructed from tables, not trained). section 7.3 records one illustrative historical Adam comparison only, and preregistered study A2 owns systematic training comparisons.`
- **本拍改**（Paper A 摘要表面；闭合上拍 #242 abs SAME 缺口）。

---

## 为何更难挑战

| 攻击读法 | Before 软点 | After 挡法 |
| --- | --- | --- |
| 摘要只说「用构造不用训练」未排除训练替代范畴 | 「仅用构造／rely on construction, not training」可被读成训练工作流的替代品陈述 | 显式「不是…替代品或逼近」／「not as a substitute for—or approximation of—training」 |
| §7.4/§10 已硬、摘要仍软，审稿人可挑摘要 | 上拍 #242/#243 报 abs SAME | 摘要与 §7.4/§10 同质措辞 |
| 构造≈另一种训练／逼近训练 | 摘要无「不同类验证纪律」句 | 「与训练不同类」+ 括注锚定 construction≠training |
| Softguess／破平近似合法出货 | 不涉及；义务仍在 Def2/3·Alg1.8·§3 | **未**粘 Softguess 口号；Softguess NONE×4 保持 |

---

## tip / blob 表

| 项 | before (`3205de9b`) | after（本拍；merge 后回填 tip SHA） |
| --- | --- | --- |
| tip | `3205de9b8e305495981c0525d04dbe5b0c598eae` | `TIP_AFTER_PLACEHOLDER` |
| CN `research/unisacc-paper.md` | `d00fd2507fde66253a12efd2f3b3048282b7acb7` | **CHANGED** → `dd22dc1c4dde5e0abd8da7f3b72316da4eed255d` |
| EN `research/unisacc-paper.en.md` | `5ed764f4ffdd9d23f2ea2aaf39de4098b4e489ff` | **CHANGED** → `b10c1fc8e8e74bc0c2f36000b1cf5b1351982845` |
| TeX `research/arxiv-paper-a/main.tex` | `bf62f9e90f67957cf2242153a910fc1420a9aaf0` | **CHANGED** → `33b376437ea541c8a99f3b5e6787d8638cdb0156` |
| abs `research/arxiv-paper-a/abstract.txt` | `d2eb48f7814b7b578b5644c5e19ff9a76ffe3d3b` | **CHANGED** → `43f0a7fd18ed6769338fd66021f7ff524287876d` |
| Softguess | **NONE×4** | **NONE×4** |

相对 #168 基线：本拍起 Paper A 摘要相对 tip `3205de9b` 四文件均变；闭合上拍 abs SAME。

---

## Softguess 机扫 @ 2026-10-12 10:46 Asia/Singapore（工作区，merge 前 tip 四文件 + 改后）

| 文件 | Softguess | softguess | SoftGuess | 权重就是 | weights are the logic | 逻辑本身 | 权重即逻辑 | 权重≡逻辑 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| EN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| TeX | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| abstract | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

- **Softguess_NONE_x4 = true**
- **weights_slogan_NONE_x4 = true**

---

## 挡粘点（本拍不处理）

1. §8.1#3 同一测量身份六目标原生矩阵仍开
2. 8509/8769 测量身份并存仍 seal-blocking
3. Ο1 已荐、本拍**不叠**新决策卡

---

## 待批板

- **Ο1：** 仍荐；本拍无新信息 → **禁止叠卡**

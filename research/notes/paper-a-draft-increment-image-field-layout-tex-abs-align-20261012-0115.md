# Paper A 成篇定稿增量：TeX/abstract +image field layout（对齐 CN/EN）

- **拍点：** 2026-10-12 ~01:15 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `9106bba1f1db1a46df8f94673fdb089fdf8d615d`（= #167 E derive-purify K6-15 `unisa.image.pe` ≠ A gate）
- **tip after：** `ae7f7424585f60fc0184e86e3dbd3b32686125bf`（#168 merge）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only 三联对齐补漏；**不改** CN/EN 正文、键数身份、product/kernel/weights/facts、投稿方向；**不叠** Ο1/Ο2（仍荐 Ο1）；**不重钉** Softguess／§8.1#3／8509·8769
- **Softguess：** tip 四文件 **NONE×4**（机扫 `Softguess`/`softguess`/`权重就是`/`weights are the logic`/`逻辑本身`/`权重即逻辑`/`权重≡逻辑` 均 0 hit；§3「不含 softmax」／「no softmax」否认句不计）
- **Latest：** **v0.0.40**；冻结重测身份仍 **v0.0.19**（本拍不改）

---

## 主张一句（本页唯一）

**TeX `main.tex` 与 `abstract.txt` 摘要须在有限决策枚举中列出 image field layout，与 CN「镜像字段布局」／EN「image field layout」同义对齐；这是构造表布局（T1 声明域上的有限决策），不是产品 K6-15 PE C↔py write hygiene／Windows-load 闭合（已由 #167 切出、不得升 seal-before）。**

---

## Before / after 句段（精确）

### TeX `research/arxiv-paper-a/main.tex` abstract

- **Before：** `Compiler decisions such as character classes, production choice, type promotion, instruction and calling-convention selection are finite functions.`
- **After：** `Compiler decisions such as character classes, production choice, type promotion, instruction and calling-convention selection, and image field layout are finite functions.`

### `research/arxiv-paper-a/abstract.txt`

- **Before：** `Many compiler decisions are finite functions: character classes, production choice, type promotion, instruction and calling-convention selection.`
- **After：** `Many compiler decisions are finite functions: character classes, production choice, type promotion, instruction and calling-convention selection, and image field layout.`

### CN / EN（本拍不改；已含）

- **CN：** `字符分类、产生式选择、类型提升、指令与调用约定选择、镜像字段布局等决策是有限函数。`
- **EN：** `Compiler decisions such as character classes, production choice, type promotion, instruction and calling-convention selection, and image field layout are finite functions.`

---

## tip / blob 表

| 项 | before (`9106bba1…` / #154 正文基线) | after（本拍） |
| --- | --- | --- |
| tip | `9106bba1f1db1a46df8f94673fdb089fdf8d615d` | `ae7f7424585f60fc0184e86e3dbd3b32686125bf` |
| CN `research/unisacc-paper.md` | `5231dd3bf3e60804c678d48015f1e337edd9dc56` **SAME** vs #154 | **SAME**（未改） |
| EN `research/unisacc-paper.en.md` | `0a707d704079bfef3b2b5193bfa8dc2b5ec90303` **SAME** | **SAME**（未改） |
| TeX `research/arxiv-paper-a/main.tex` | `0ef6027ccc73b20b1d7d4371f89c47b069885bb7` **SAME** vs #154 | **CHANGED** → `aab7bd9b8e3c72d08aa2672a559513bb70e02049` |
| abs `research/arxiv-paper-a/abstract.txt` | `43d7ea7e7297ed8be9a96e3950e014e359467973` **SAME** | **CHANGED** → `d2eb48f7814b7b578b5644c5e19ff9a76ffe3d3b` |
| Softguess | **NONE×4** | **NONE×4** |

Paper A 末次正文基线仍记 #154 `bdec4bcbf823d0b527a15c40afe61f69942063fc`（T1→\(K_s\)）；本拍仅 TeX+abs 摘要枚举对齐，CN/EN 自 #154 起已含 image field layout。

---

## DISTINCT vs #167（K6-15 PE ≠ A gate）

| | 本拍 A | #167 E |
| --- | --- | --- |
| 对象 | 摘要有限决策枚举漏列 **image field layout**（构造表布局／T1 域） | 产品 `unisa.image.pe` C↔py static write hygiene |
| 动作 | TeX+abs **补进 A**（与 CN/EN 三联一致） | 切出「须先 pe 绿才许封 A」升格 |
| 结果 | A 更紧：三联摘要同列镜像字段布局 | A 更纯：pe write ≠ seal-before |

**Explicit：** 构造表里的 image field layout **留在 A**；产品 PE write hygiene **不进 seal-before**（#167 已切）。二者同词「image/PE」但层级不同，不得互替。

---

## 切后 A 更紧（一句）

切后 A 三联（CN/EN/TeX+abs）摘要对有限决策枚举一致：镜像字段布局／image field layout 与字符分类、产生式、类型提升、指令与调用约定同列——不再出现「中英已写、arXiv 摘要漏写」的阅读卫生缺口。

---

## Blockers unchanged

- §8.1#3 same-identity six-target **native** matrix — **仍开**
- 8509/8769 dual measurement identity — **仍开**
- Ο1/Ο2 pending — **仍开**（**荐Ο1**；本拍不叠决策卡）

---

## OUT（本拍明确不做）

- 改 CN/EN 正文；改键数 8509/8769/517/569 或身份措辞
- 粘 Softguess／「权重就是逻辑」；叠 Ο1/Ο2；代裁投稿身份
- 改 product/kernel/weights/facts；重做 #167；把 K6-15 升 seal-before
- 发明第二套理论；把 pe write hygiene 写回摘要作闸门

---

## 证据索引

- 本笔记：`research/notes/paper-a-draft-increment-image-field-layout-tex-abs-align-20261012-0115.md`
- 心跳短报：`research/notes/paper-a-heartbeat-report-20261012-0115.md`
- DISTINCT 先验：[`paper-a-e-derive-purify-k6-15-pe-ne-a-gate-20261012-0101.md`](paper-a-e-derive-purify-k6-15-pe-ne-a-gate-20261012-0101.md)（#167）
- 成篇增量先验：[`paper-a-seal-t1-declared-domain-unify-20261011-2127.md`](paper-a-seal-t1-declared-domain-unify-20261011-2127.md)（#154 前 T1 措辞）

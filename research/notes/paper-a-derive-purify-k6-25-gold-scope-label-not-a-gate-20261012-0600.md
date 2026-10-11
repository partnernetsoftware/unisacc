# Paper A 衍生净化：K6-25 unisa.gold scope_label 不得搭乘 A

- **拍点：** 2026-10-12 ~06:00 Asia/Singapore（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `7ebcbf48588bcacb6eec0070c1980c6d7744300f`（短 `7ebcbf48`）— 0544 C tip-after fill #204；相对任务书 tip **无漂**；父链含 **`89d012e993cdd6d899b717df30ba2d5008c11ed0`（短 `89d012e9`）K6-25 scope_label 产品叶**（与 tip 比较：tip ahead_by=2、behind=0，K6-25 **仍在 tip 祖先**）；**本拍首次 E 净化 K6-25**。父链：#201/#202 K6-24 peep E ← **`89d012e9` K6-25 scope_label 产品叶** ← #203/#204 C 8509 tip-refresh ← …
- **锚产品叶（已入仓、本拍不重做）：** `89d012e993cdd6d899b717df30ba2d5008c11ed0`（短 `89d012e9`）— K6-25：**`unisa.gold` `scope_label`**；C 参考 `seed/unisa_gold_scope_label.c` ↔ python 经 **ast 抽取** 4 定义（`CTX`/`KIND`/`TYPEKW`/`scope_label`；**不 import gold.py、不调用 `build()`**）；**有限域 `CTX`(6)×`KIND`(5) = 30 行全表** + **17 命名/边界点** byte-equal（SAME 18 of 18：全表 1 + 命名 17）；neg **2/2**（missing-kind／unknown-mode）；**不称 gold Stage 表／其它标签函数／`build()`／权重训练／gold 全闭包**。这是 **作用域标签有限域（scope_label）阶段点采样 hygiene**，不是 A 理论。
- **产品证据页（只引）：** [`research/c41-k6-25-gold-scope-label.md`](../c41-k6-25-gold-scope-label.md)（blob `071f30f8b9a6fb9d60aa1ac9bf80cd5ecb57f6dd`）；直跑日志 `research/c41-k6-25-gold-scope-label-run.log`；夹具 `tests/unisagoldscopelabelcheck.sh`；seed `seed/unisa_gold_scope_label.c`；gate 行 `unisa-gold-scope-label`。
- **Latest：** **v0.0.40**（assets=10）；Paper A 冻结重测身份仍 **v0.0.19**（本拍不改；禁 bump/Latest）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证载体／字节流管道）→ 产品／unisa **gold `scope_label`／CTX×KIND 点采样 hygiene**（明确 **不是** T1／测量身份／§8.1#3／Softguess）旁支 → 仍回 **A**
- **状态：** **note only**（登记「须先 K6-25 gold scope_label／30 全表绿／gold 全闭包／Stage 表／全部标签函数／`build()` 完稿，才许投 A／构造+T1 才成立／gate 绿＝测量身份闭合／缺之则实证封口不完整／须升 Softguess／第四 seal-before／＝§8.1#3 闭合」升格 ≠ 理论闸门；**不起稿**；**不改** A 根主张、键数身份、CN/EN/TeX/abstract、product/kernel/weights/facts、投稿方向；**不代裁** Ο1/Ο2（仍荐 Ο1）；**不重钉** Softguess／8509·8769／§8.1#3；**不重做** #201/#202 K6-24 peep_label E、#199/#200 K6-23 opinfo_label E、#195/#196 K6-22 binsel_label E、#191/#192 K6-21 type_label E、#189/#190 K6-20 lex_label E、#185/#186 K6-18 prec_label E、#183/#184 K6-19 emit_arm E、#181 §7.4 cost-meas E、#120 type-table host-cc keydomain E、#203/#204 8509 C tip-refresh、#197/#198 §8.1#3 tip-refresh；**本拍对象仅 K6-24 E 之后 NEW 未 E 净化的 K6-25**；**不授** 产品下一叶）
- **Softguess：** tip 四文件 **NONE×4**（机扫 `Softguess`/`softguess`/`SoftGuess`/`权重就是`/`weights are the logic`/`逻辑本身`/`权重即逻辑`/`权重≡逻辑` 均 0 hit；§3「不含 softmax」／「no softmax」否认句不计——CN/EN/TeX 各 1、abs 0；**NONE≠seal-before 债**；字面粘贴仍 P0 UNAPPLIED，本拍不粘）
- **测量身份：** 8509/8769 **仍开** — 仅引用既有 tip-refresh 钉（0544/#203/#204）；本拍**不重钉、不叠**；**不叠** Ο1/Ο2（仍荐 Ο1）
- **Paper A 正文漂移：** tip 四 blob 相对 #168 `ae7f7424` **SAME×4**（本拍零正文 diff）；相对 #154 `bdec4bcb`：CN/EN **SAME**，TeX/abs **CHANGED**（#168 image field layout 已入 TeX/abs）
- **为何本拍选 E 而非 C／B：** tip 已 `7ebcbf48`（含 K6-25 产品叶 + C 0544 身份钉仍挡）；上拍 E 0526 停在 K6-24，**未**净化 K6-25；政委禁 inspection-only 重钉 SAME 的 C；Ο1 无新决策信息，禁叠 B；本拍切 K6-25。

---

## 主张一句（本页唯一）

**把产品叶 K6-25——`unisa.gold` 的 `scope_label`（有限域 CTX×KIND = 30 行全表 + 17 命名/边界点；C 参考 vs gold.py 源码经 ast 抽取 4 定义，不调用 `build()`；byte-equal，neg 2/2；明确不称 gold Stage／其它标签／`build()`／全闭包）——升成 Paper A seal-before／T1 替代／8509·8769 测量身份闭合／Softguess 证据／第四闸门／§8.1#3 same-identity six-target native matrix 闭合——不得搭乘 A。** 这是产品／gold 作用域标签有限域点采样 hygiene，不是 A 理论。A seal-before 仍只是 §8.1#3、8509·8769、Ο1|Ο2（荐Ο1）。**NEW vs 既有 K6 E／#120：** 对象是 **gold `scope_label`／CTX×KIND 有限域 C↔gold.py**，**≠ K6-24 `peep_label`**（#201）、**≠ K6-23 `opinfo_label`**（#199）、**≠ K6-22 `binsel_label`**（#195）、**≠ K6-21 `type_label`**（#191）、**≠ K6-20 `lex_label`**（#189）、**≠ K6-18 `prec_label`**（#185）、**≠ K6-19 `emit_arm`**（#183）、**≠ #120 type-table host-cc keydomain**（宿主 cc 键域完备，另一条）、≠ image pe/elf/macho/fat/tape、≠ §7.4 cost-meas（#181）、≠ Softguess 产品路径、≠ 旧 peephole 产品 opts 完备性 E（1245）。

---

## 切什么

| 战役 | 对象 | 分类 |
| --- | --- | --- |
| K6-25 gold scope_label | C↔ast-extracted gold.py：`CTX×KIND`=30；命名点 17；ast 抽 4 定义；neg 2/2（missing-kind／unknown-mode）；不 import／不 `build()` | **keep 产品／gold 作用域标签 hygiene**；切出「须先绿／完稿 gold 全闭包·Stage·全部标签·`build()` 才许投 A／＝T1／身份闭合／Softguess／第四 seal／＝§8.1#3 闭合」 |
| K6-24 gold peep_label（已切） | #201/#202 | **keep 已切旁支**；本拍不重做；**DISTINCT**（peep ≠ scope） |
| K6-23 gold opinfo_label（已切） | #199/#200 | **keep 已切旁支**；本拍不重做；**DISTINCT**（opinfo ≠ scope） |
| K6-22 gold binsel_label（已切） | #195/#196 | **keep 已切旁支**；本拍不重做；**DISTINCT**（binsel ≠ scope） |
| K6-21 gold type_label（已切） | #191/#192 | **keep 已切旁支**；本拍不重做；**DISTINCT**（type ≠ scope） |
| K6-20 gold lex_label（已切） | #189/#190 | **keep 已切旁支**；本拍不重做；**DISTINCT**（lex ≠ scope） |
| K6-18 gold prec_label（已切） | #185/#186 | **keep 已切旁支**；本拍不重做；**DISTINCT**（prec ≠ scope） |
| K6-19 emit_arm（已切） | #183/#184 | **keep 已切旁支**；本拍不重做 |
| #120 type-table host-cc keydomain（已切） | #120（~12:53） | **keep 已切旁支**；**DISTINCT**：宿主 cc 键域完备 ≠ gold scope_label 有限域 C↔gold.py |
| §7.4 cost-meas（已切） | #181/#182 | **keep 已切旁支**；本拍不重做 |
| §8.1#3／8509·8769／Ο1\|Ο2 | 仍开硬缺口与决策卡（荐Ο1） | **keep seal-blocking**；本拍不重钉、不叠、不代裁 |
| A 根主张／T1 on \(K_s\) | 构造网络；声明域逐键全等 | **keep in A** |

**不切：** 根主张、键数数字、§8.1#3／Ο1/Ο2、product/kernel/weights/facts、CN/EN/TeX/abstract、投稿方向、K6-25 工程本身与任何未来 gold Stage／标签闭包／`build()` 工程债（只切升格读法）、Softguess 字面粘贴（仍 UNAPPLIED）。

**DISTINCT：** **K6-25 gold scope_label ≠ K6-24 peep_label**（#201）**≠ K6-23 opinfo_label**（#199）**≠ K6-22 binsel_label**（#195）**≠ K6-21 type_label**（#191）**≠ K6-20 lex_label**（#189）**≠ K6-18 prec_label**（#185）**≠ K6-19 emit_arm**（#183）**≠ #120 host-cc keydomain** ≠ K6-17 vm／divmod／deferred ≠ ape（#171）≠ pe（#167）≠ elf/macho（#163）≠ fat（#160）≠ tapebin／tape ≠ control／catalog／fp ≠ hosted-CI（#164）≠ Softguess 产品路径（2355）≠ §7.4 cost-meas（#181）≠ 旧 peephole opts suite completeness（1245）。本拍对象是 **gold 作用域标签有限域点采样（C↔gold.py）**，不是窥孔标签、不是算符信息标签、不是二进制选择标签、不是类型标签、不是词法标签、不是优先级标签、不是 ARM64 发射器、不是宿主 cc 键域完备、不是镜像写出、也不是代价测量方法学。

**Explicit negatives：** **K6-25 scope_label 点采样 ≠ T1**；**≠ 8509/8769 measurement identity**；**≠ Softguess**；**≠ fourth gate**；**≠ §8.1#3 closure**；**≠ gold 全闭包／`build()`／Stage 表／全部标签函数**；**≠ #120 host-cc keydomain**。

---

## 为何不挡 A 投稿

1. **A 根主张是构造＋T1（声明域 \(K_s\)），不是「gold scope_label／30 全表已绿／gold 全闭包」。** 作用域标签有限域点采样是产品旁路 hygiene，不是构造 falsifier，也不是第二套理论。
2. **gate／点采样绿 ≠ Softguess／第四 seal-before／8509·8769 闭合／§8.1#3 六目标原生矩阵。** 产品页自己是窄切（30 全表 + 17 命名点；ast 抽 4 定义；不 `build()`；不称 gold 全闭包）——更不能反向读成跨架构实证封口、测量身份闭合、或 A 投稿前提。
3. **与仍开硬缺口及已切旁支正交。** 本拍不把 scope_label 加为第四挡粘，不叠决策卡；对象 DISTINCT 如上（尤其 ≠ K6-24 peep、≠ K6-23 opinfo、≠ K6-21 type、≠ #120 host-cc keydomain）。
4. **切后 A 更纯：** A 不再被诱惑把「须先绿 gold scope_label／gold 全闭包」当作 seal-before；攻击面从「须先作用域表／gold hygiene 全绿才许封 A」缩回「A = 构造表→整数阈值网络 + T1 声明域逐键全等 + 仍开测量债（8509/8769、§8.1#3）；gold scope_label 点采样挂产品枝」。
5. **本拍零 Paper A 正文 diff。**

---

## 父节点如何回 A

- **旁支回 A：** A（字节流／构造 TSV→weights 编译器 + T1 on \(K_s\) + unisacc 实证）→ unisa.gold（scope_label／CTX×KIND）产品 hygiene 旁支 → 仍回 A。
- 结果可回来收紧「产品旁路／gold 表卫生如何披露」措辞，不能长成与 A 并列的第二套理论，也不能把「scope_label 未绿／gold 未闭」回写成投稿前提、T1 替代、Softguess 触发器或 §8.1#3 闭合证明。
- **DAG：** 切出的是升格闸门，不是删掉 gold scope 工程债或 A 硬缺口。gold 作用域标签点采样是实证旁路，不是编译器根主张。K6-25 = empirical product side-path under unisa gold stack，**not** a theory gate for the constructed neural compiler paper。

---

## 切后 A 哪一句更硬

切出后，A 可把 gold 旁支边界收成一句更硬的主张（**本拍不改正文；仅登记**）：

> **Paper A 只主张基于神经网络（构造网络与权重）的编译器与 T1（网络≡表于声明域 \(K_s\)），并以 unisacc 为 POSIX C99 子集跨架构实证；`unisa.gold` 的 `scope_label`／CTX×KIND 有限域点采样（及日后 Stage 表／全部标签／`build()` 闭包）都属于产品载体／gold hygiene，不是 A 的 seal-before、不是 T1、不是测量身份闭合、也不是 Softguess 触发。**

**切后 A 更纯在哪（一句）：** A 更纯在：scope 有限域金标对照是产品／旁支实证，不进根主张闸门；攻击面缩回构造+T1+仍开测量债，该 scope_label hygiene 挂产品枝。

---

## tip / blob 表（本拍核对）

| 项 | 值 |
| --- | --- |
| tip before | `7ebcbf48588bcacb6eec0070c1980c6d7744300f` |
| tip after（本 note 合入后填） | _(fill after merge)_ |
| K6-25 产品叶 | `89d012e993cdd6d899b717df30ba2d5008c11ed0`（仍在 tip 祖先；tip ahead_by=2 vs 该叶） |
| 上拍 E 锚 tip | `f2806d67…`（#202 tip-after；K6-24 E 已合）；其后 NEW 产品叶 = K6-25，未 E 净化直至本拍 |
| 上拍 C 锚 tip | `7ebcbf48…`（#204 tip-after fill；8509/8769 仍挡） |
| Paper A 末次正文（TeX/abs） | #168 `ae7f7424585f60fc0184e86e3dbd3b32686125bf`（image field layout） |
| Paper A 末次正文（CN/EN 相对 #154） | #154 `bdec4bcbf823d0b527a15c40afe61f69942063fc`（T1→\(K_s\)；CN/EN 自此未改） |
| CN `research/unisacc-paper.md` | `5231dd3bf3e60804c678d48015f1e337edd9dc56` **SAME** vs #168 / vs #154 |
| EN `research/unisacc-paper.en.md` | `0a707d704079bfef3b2b5193bfa8dc2b5ec90303` **SAME** vs #168 / vs #154 |
| TeX `research/arxiv-paper-a/main.tex` | `aab7bd9b8e3c72d08aa2672a559513bb70e02049` **SAME** vs #168；**CHANGED** vs #154 |
| abs `research/arxiv-paper-a/abstract.txt` | `d2eb48f7814b7b578b5644c5e19ff9a76ffe3d3b` **SAME** vs #168；**CHANGED** vs #154 |
| Softguess | **NONE×4**（预期态；不升 seal-before；字面粘贴仍 P0 UNAPPLIED） |

---

## Explicit：不得升格

**本 hygiene 不得升成：** A seal-before／T1／8509·8769／Softguess／第四闸门／§8.1#3。

---

## Blockers unchanged（本拍不重钉）

| 挡粘 | 状态 |
| --- | --- |
| §8.1#3 same-identity six-target native matrix | **仍开** |
| 8509/8769 measurement identity | **仍开**（活 gold op-87／8769／569 vs EN/TeX/abs op-74／8509／517；见 0544 C） |
| Ο1\|Ο2（投稿/封口重测身份） | **仍开**；**仍荐 Ο1**；本拍不叠、不代裁 |
| Softguess 字面粘贴 | NONE×4 预期态；字面仍 P0 UNAPPLIED；**不升** seal-before |

---

## OUT（本拍明确不做）

- 改正文 CN/EN/TeX/abstract；粘 Softguess／近似推断
- 代裁／叠卡 Ο1/Ο2；重钉 Softguess tip-refresh 或 8509·8769 tip-refresh 或 §8.1#3 tip-refresh
- 改 product/kernel/weights/facts／键数／投稿方向；bump Latest／冻结身份 v0.0.19
- 重做 #201 K6-24 peep／#199 K6-23 opinfo／#195 K6-22 binsel／#191 K6-21 type／#189 K6-20 lex／#185 K6-18 prec／#183 K6-19 emit_arm／#181 cost-meas／#120 host-cc keydomain／更早旁支 E／#203 8509 C／#197 §8.1#3 C
- 把 K6-25 scope_label 加为第四 seal-before／Softguess 触发／8509·8769 闭合证明
- 与 #120 host-cc keydomain 混读；与 1245 peephole opts suite completeness 混读

---

## 证据索引

- 本笔记：`research/notes/paper-a-derive-purify-k6-25-gold-scope-label-not-a-gate-20261012-0600.md`
- 同模式先验：[`paper-a-derive-purify-k6-24-gold-peep-label-not-a-gate-20261012-0526.md`](paper-a-derive-purify-k6-24-gold-peep-label-not-a-gate-20261012-0526.md)（#201/#202）；[`paper-a-derive-purify-k6-23-gold-opinfo-label-not-a-gate-20261012-0512.md`](paper-a-derive-purify-k6-23-gold-opinfo-label-not-a-gate-20261012-0512.md)（#199/#200）；[`paper-a-derive-purify-k6-22-gold-binsel-label-not-a-gate-20261012-0440.md`](paper-a-derive-purify-k6-22-gold-binsel-label-not-a-gate-20261012-0440.md)（#195/#196）；[`paper-a-derive-purify-k6-21-gold-type-label-not-a-gate-20261012-0415.md`](paper-a-derive-purify-k6-21-gold-type-label-not-a-gate-20261012-0415.md)（#191/#192）；[`paper-a-derive-purify-k6-20-gold-lex-label-not-a-gate-20261012-0401.md`](paper-a-derive-purify-k6-20-gold-lex-label-not-a-gate-20261012-0401.md)（#189/#190）；[`paper-a-derive-purify-k6-18-gold-prec-label-not-a-gate-20261012-0322.md`](paper-a-derive-purify-k6-18-gold-prec-label-not-a-gate-20261012-0322.md)（#185/#186）
- **DISTINCT #120：** type-table host-cc keydomain completeness ≠ A gate（~12:53）；本拍是 gold scope_label 有限域 C↔gold.py，勿混
- **DISTINCT 1245：** peephole product opts suite completeness ≠ A gate；本拍是 gold `scope_label` 有限域点采样，勿混
- 产品证据：K6-25 `89d012e9…`；`research/c41-k6-25-gold-scope-label.md`；`seed/unisa_gold_scope_label.c`；`tests/unisagoldscopelabelcheck.sh`；gate `unisa-gold-scope-label`
- 上拍 E（K6-24；明示其后 NEW 叶未 E）：[`paper-a-derive-purify-k6-24-gold-peep-label-not-a-gate-20261012-0526.md`](paper-a-derive-purify-k6-24-gold-peep-label-not-a-gate-20261012-0526.md)（#201/#202）
- 上拍 C（8509 仍挡；明示 K6-25 ≠ 身份闭合）：[`paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-after-k6-15-24-20261012-0544.md`](paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-after-k6-15-24-20261012-0544.md)（#203/#204）
- 本拍短报：[`paper-a-heartbeat-report-20261012-0600.md`](paper-a-heartbeat-report-20261012-0600.md)

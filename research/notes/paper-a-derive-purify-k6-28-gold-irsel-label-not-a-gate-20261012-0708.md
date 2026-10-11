# Paper A 衍生净化：K6-28 unisa.gold irsel_label 不得搭乘 A

- **拍点：** 2026-10-12 ~07:08 Asia/Singapore（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `dde789eaab0fecef25667eafb49bf5db3db682c2`（短 `dde789ea`）— 任务书 tip；= K6-28 irsel_label 产品叶刚入 main；相对任务书 **无漂**；**本拍首次 E 净化 K6-28**。父链：#211/#212 K6-27 reloc_label E → `dde789ea` K6-28 irsel_label 产品叶 ← tip `dde789ea`。
- **锚产品叶（已入仓、本拍不重做）：** `dde789eaab0fecef25667eafb49bf5db3db682c2`（短 `dde789ea`）— K6-28：**`unisa.gold` `irsel_label`**；C 参考 `seed/unisa_gold_irsel_label.c` ↔ python 经 **ast 抽取** 2 定义（`IRSEL_MAP` + `irsel_label` from gold.py；**不 import gold.py、不调用 `build()`**）；**映射全表 `IRSEL_MAP` 共 70 条**（alu 24、mem 6、ctrl 3、call 5、lit 4、fpu 28；**非笛卡尔积**；按 gold.py 插入序）+ **19 命名／边界点** byte-equal（SAME 20 of 20：全表 1 + 命名 19）；neg **3/3**（unknown-family／missing-flavour／unknown-mode）；首红（缺 `<stdlib.h>`）已记、同窗单行修正后绿；**不称 gold Stage 表／其它标签函数／`build()`／权重训练／gold 全闭包／parse／reloc**。这是 **IR 选择标签映射表（irsel_label／IRSEL_MAP）阶段点采样 hygiene**，不是 A 理论。
- **产品证据页（只引）：** [`research/c41-k6-28-gold-irsel-label.md`](../c41-k6-28-gold-irsel-label.md)（blob `5034569ba7a0aedeb56c8b2ca19aae3d904c07e3`）；直跑日志 `research/c41-k6-28-gold-irsel-label-run.log`；夹具 `tests/unisagoldirsellabelcheck.sh`；seed `seed/unisa_gold_irsel_label.c`；gate 行 `unisa-gold-irsel-label`。
- **Latest：** **v0.0.40**（assets=10）；Paper A 冻结重测身份仍 **v0.0.19**（本拍不改；禁 bump/Latest）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证载体／字节流管道）→ 产品／unisa **gold `irsel_label`／IRSEL_MAP 点采样 hygiene**（明确 **不是** T1／测量身份／§8.1#3／Softguess）旁支 → 仍回 **A**
- **状态：** **note only**（登记「须先 K6-28 gold irsel_label／70 全表绿／gold 全闭包／Stage 表／全部标签函数／`build()`／parse 完稿，才许投 A／构造+T1 才成立／gate 绿＝测量身份闭合／缺之则实证封口不完整／须升 Softguess／第四 seal-before／＝§8.1#3 闭合」升格 ≠ 理论闸门；**不起稿**；**不改** A 根主张、键数身份、CN/EN/TeX/abstract、product/kernel/weights/facts、投稿方向；**不代裁** Ο1/Ο2（仍荐 Ο1）；**不重钉** Softguess／8509·8769／§8.1#3；**不重做** #211/#212 K6-27 reloc_label E、#207/#208 K6-26 pp_label E、#205/#206 K6-25 scope_label E、#201/#202 K6-24 peep_label E、#199/#200 K6-23 opinfo_label E、#195/#196 K6-22 binsel_label E、#191/#192 K6-21 type_label E、#189/#190 K6-20 lex_label E、#185/#186 K6-18 prec_label E、#183/#184 K6-19 emit_arm E、#181 §7.4 cost-meas E、#120 type-table host-cc keydomain E、#209/#210 8509 C tip-refresh、#197/#198 §8.1#3 tip-refresh；**本拍对象仅 K6-27 E 之后 NEW 的 K6-28**；**不授** 产品下一叶）
- **Softguess：** tip 四文件 **NONE×4**（机扫 `Softguess`/`softguess`/`SoftGuess`/`权重就是`/`weights are the logic`/`逻辑本身`/`权重即逻辑`/`权重≡逻辑` 均 0 hit；§3「不含 softmax」／「no softmax」否认句不计——CN/EN/TeX 各 1、abs 0；**NONE≠seal-before 债**；字面粘贴仍 P0 UNAPPLIED，本拍不粘）
- **测量身份：** 8509/8769 **仍开** — 仅引用既有 tip-refresh 钉（0639/#209/#210）；本拍**不重钉、不叠**；**不叠** Ο1/Ο2（仍荐 Ο1）
- **Paper A 正文漂移：** tip 四 blob 相对 #168 `ae7f7424` **SAME×4**（本拍零正文 diff）；相对 #154 `bdec4bcb`：CN/EN **SAME**，TeX/abs **CHANGED**（#168 image field layout 已入 TeX/abs）
- **为何本拍选 E 而非 C／B：** tip 已 `dde789ea`（= K6-28 产品叶；上拍 E 0652 停在 K6-27）；政委禁 inspection-only 重钉 SAME 的 C；Ο1 无新决策信息，禁叠 B；本拍切 K6-28。

---

## 主张一句（本页唯一）

**把产品叶 K6-28——`unisa.gold` 的 `irsel_label`（映射全表 IRSEL_MAP = 70 条 + 19 命名／边界点；C 参考 vs gold.py 源码经 ast 抽取，不调用 `build()`；byte-equal，neg 3/3；明确不称 gold Stage／其它标签／`build()`／全闭包／parse／reloc）——升成 Paper A seal-before／T1 替代／8509·8769 测量身份闭合／Softguess 证据／第四闸门／§8.1#3 same-identity six-target native matrix 闭合——不得搭乘 A。** 这是产品／gold IR 选择标签映射表点采样 hygiene，不是 A 理论。A seal-before 仍只是 §8.1#3、8509·8769、Ο1|Ο2（荐Ο1）。**NEW vs 既有 K6 E／#120：** 对象是 **gold `irsel_label`／IRSEL_MAP 有限域 C↔gold.py**，**≠ K6-27 `reloc_label`**（#211）、**≠ K6-26 `pp_label`**（#207）、**≠ K6-25 `scope_label`**（#205）、**≠ K6-24 `peep_label`**（#201）、**≠ K6-23 `opinfo_label`**（#199）、**≠ K6-22 `binsel_label`**（#195）、**≠ K6-21 `type_label`**（#191）、**≠ K6-20 `lex_label`**（#189）、**≠ K6-18 `prec_label`**（#185）、**≠ K6-19 `emit_arm`**（#183）、**≠ #120 type-table host-cc keydomain**（宿主 cc 键域完备，另一条）、≠ image pe/elf/macho/fat/tape、≠ §7.4 cost-meas（#181）、≠ Softguess 产品路径、≠ 旧 peephole 产品 opts 完备性 E（1245）。

---

## 切什么

| 战役 | 对象 | 分类 |
| --- | --- | --- |
| K6-28 gold irsel_label | C↔ast-extracted gold.py：`IRSEL_MAP`=70；命名／边界点 19；ast 抽 2 定义；neg 3/3（unknown-family／missing-flavour／unknown-mode）；不 import／不 `build()`；非笛卡尔积 | **keep 产品／gold IR 选择标签 hygiene**；切出「须先绿／完稿 gold 全闭包·Stage·全部标签·`build()`·parse 才许投 A／＝T1／身份闭合／Softguess／第四 seal／＝§8.1#3 闭合」 |
| K6-27 gold reloc_label（已切） | #211/#212 | **keep 已切旁支**；本拍不重做；**DISTINCT**（reloc ≠ irsel） |
| K6-26 gold pp_label（已切） | #207/#208 | **keep 已切旁支**；本拍不重做；**DISTINCT**（pp ≠ irsel） |
| K6-25 gold scope_label（已切） | #205/#206 | **keep 已切旁支**；本拍不重做；**DISTINCT**（scope ≠ irsel） |
| K6-24 gold peep_label（已切） | #201/#202 | **keep 已切旁支**；本拍不重做；**DISTINCT**（peep ≠ irsel） |
| K6-23 gold opinfo_label（已切） | #199/#200 | **keep 已切旁支**；本拍不重做；**DISTINCT**（opinfo ≠ irsel） |
| K6-22 gold binsel_label（已切） | #195/#196 | **keep 已切旁支**；本拍不重做；**DISTINCT**（binsel ≠ irsel） |
| K6-21 gold type_label（已切） | #191/#192 | **keep 已切旁支**；本拍不重做；**DISTINCT**（type ≠ irsel） |
| K6-20 gold lex_label（已切） | #189/#190 | **keep 已切旁支**；本拍不重做；**DISTINCT**（lex ≠ irsel） |
| K6-18 gold prec_label（已切） | #185/#186 | **keep 已切旁支**；本拍不重做；**DISTINCT**（prec ≠ irsel） |
| K6-19 emit_arm（已切） | #183/#184 | **keep 已切旁支**；本拍不重做 |
| #120 type-table host-cc keydomain（已切） | #120（~12:53） | **keep 已切旁支**；**DISTINCT**：宿主 cc 键域完备 ≠ gold irsel_label 有限域 C↔gold.py |
| §7.4 cost-meas（已切） | #181/#182 | **keep 已切旁支**；本拍不重做 |
| §8.1#3／8509·8769／Ο1\|Ο2 | 仍开硬缺口与决策卡（荐Ο1） | **keep seal-blocking**；本拍不重钉、不叠、不代裁 |
| A 根主张／T1 on \(K_s\) | 构造网络；声明域逐键全等 | **keep in A** |

**不切：** 根主张、键数数字、§8.1#3／Ο1/Ο2、product/kernel/weights/facts、CN/EN/TeX/abstract、投稿方向、K6-28 工程本身与任何未来 gold Stage／标签闭包／`build()`／parse 工程债（只切升格读法）、Softguess 字面粘贴（仍 UNAPPLIED）。

**DISTINCT：** **K6-28 gold irsel_label ≠ K6-27 reloc_label**（#211）**≠ K6-26 pp_label**（#207）**≠ K6-25 scope_label**（#205）**≠ K6-24 peep_label**（#201）**≠ K6-23 opinfo_label**（#199）**≠ K6-22 binsel_label**（#195）**≠ K6-21 type_label**（#191）**≠ K6-20 lex_label**（#189）**≠ K6-18 prec_label**（#185）**≠ K6-19 emit_arm**（#183）**≠ #120 host-cc keydomain** ≠ K6-17 vm／divmod／deferred ≠ ape（#171）≠ pe（#167）≠ elf/macho（#163）≠ fat（#160）≠ tapebin／tape ≠ control／catalog／fp ≠ hosted-CI（#164）≠ Softguess 产品路径（2355）≠ §7.4 cost-meas（#181）≠ 旧 peephole opts suite completeness（1245）。本拍对象是 **gold IR 选择标签映射表点采样（C↔gold.py IRSEL_MAP）**，不是重定位标签、不是预处理标签、不是作用域标签、不是窥孔标签、不是算符信息标签、不是二进制选择标签、不是类型标签、不是词法标签、不是优先级标签、不是 ARM64 发射器、不是宿主 cc 键域完备、不是镜像写出、也不是代价测量方法学；亦**不并刀 parse**。

**Explicit negatives：** **K6-28 irsel_label 点采样 ≠ T1**；**≠ 8509/8769 measurement identity**；**≠ Softguess**；**≠ fourth gate**；**≠ §8.1#3 closure**；**≠ gold 全闭包／`build()`／Stage 表／全部标签函数／parse／reloc**；**≠ #120 host-cc keydomain**。

---

## 为何不挡 A 投稿

1. **A 根主张是构造＋T1（声明域 \(K_s\)），不是「gold irsel_label／70 全表已绿／gold 全闭包」。** IR 选择标签映射表点采样是产品旁路 hygiene，不是构造 falsifier，也不是第二套理论。
2. **gate／点采样绿 ≠ Softguess／第四 seal-before／8509·8769 闭合／§8.1#3 六目标原生矩阵。** 产品页自己是窄切（70 映射行 + 19 命名／边界点；ast 抽 2 定义；不 `build()`；不称 gold 全闭包／parse）——更不能反向读成跨架构实证封口、测量身份闭合、或 A 投稿前提。
3. **与仍开硬缺口及已切旁支正交。** 本拍不把 irsel_label 加为第四挡粘，不叠决策卡；对象 DISTINCT 如上（尤其 ≠ K6-27 reloc、≠ K6-26 pp、≠ K6-21 type、≠ #120 host-cc keydomain）。
4. **切后 A 更纯：** A 不再被诱惑把「须先绿 gold irsel_label／gold 全闭包／parse」当作 seal-before；攻击面从「须先 IRSEL 表／gold hygiene 全绿才许封 A」缩回「A = 构造表→整数阈值网络 + T1 声明域逐键全等 + 仍开测量债（8509/8769、§8.1#3）；gold irsel_label 点采样挂产品枝」。
5. **本拍零 Paper A 正文 diff。**

---

## 父节点如何回 A

- **旁支回 A：** A（字节流／构造 TSV→weights 编译器 + T1 on \(K_s\) + unisacc 实证）→ unisa.gold（irsel_label／IRSEL_MAP）产品 hygiene 旁支 → 仍回 A。
- 结果可回来收紧「产品旁路／gold 表卫生如何披露」措辞，不能长成与 A 并列的第二套理论，也不能把「irsel_label 未绿／gold 未闭／parse 未做」回写成投稿前提、T1 替代、Softguess 触发器或 §8.1#3 闭合证明。
- **DAG：** 切出的是升格闸门，不是删掉 gold irsel 工程债或 A 硬缺口。gold IR 选择标签点采样是实证旁路，不是编译器根主张。K6-28 = empirical product side-path under unisa gold stack，**not** a theory gate for the constructed neural compiler paper。

---

## 切后 A 哪一句更硬

切出后，A 可把 gold 旁支边界收成一句更硬的主张（**本拍不改正文；仅登记**）：

> **Paper A 只主张基于神经网络（构造网络与权重）的编译器与 T1（网络≡表于声明域 \(K_s\)），并以 unisacc 为 POSIX C99 子集跨架构实证；`unisa.gold` 的 `irsel_label`／IRSEL_MAP 映射表点采样（及日后 Stage 表／全部标签／`build()`／parse 闭包）都属于产品载体／gold hygiene，不是 A 的 seal-before、不是 T1、不是测量身份闭合、也不是 Softguess 触发。**

**切后 A 更纯在哪（一句）：** A 更纯在：irsel／IRSEL_MAP 金标对照是产品／旁支实证，不进根主张闸门；攻击面缩回构造+T1+仍开测量债，该 irsel_label hygiene 挂产品枝。

---

## tip / blob 表（本拍核对）

| 项 | 值 |
| --- | --- |
| tip before | `dde789eaab0fecef25667eafb49bf5db3db682c2` |
| tip after（本 note 合入后填） | `56968e171a3c794c6dddb03592e6fb8134bb6fb0`（短 `56968e17`；= squash Merge #213 本 E） |
| K6-28 产品叶 | `dde789eaab0fecef25667eafb49bf5db3db682c2`（= tip before；本拍首次 E） |
| 上拍 E 锚 tip | `8e1b8482…`（#212 tip-after；K6-27 E 已合）；其后 NEW 产品叶 = K6-28 `dde789ea`，未 E 净化直至本拍 |
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
| 8509/8769 measurement identity | **仍开**（活 gold op-87／8769／569 vs EN/TeX/abs op-74／8509／517；见 0639 C #209/#210） |
| Ο1\|Ο2（投稿/封口重测身份） | **仍开**；**仍荐 Ο1**；本拍不叠、不代裁 |
| Softguess 字面粘贴 | NONE×4 预期态；字面仍 P0 UNAPPLIED；**不升** seal-before |

---

## OUT（本拍明确不做）

- 改正文 CN/EN/TeX/abstract；粘 Softguess／近似推断
- 代裁／叠卡 Ο1/Ο2；重钉 Softguess tip-refresh 或 8509·8769 tip-refresh 或 §8.1#3 tip-refresh
- 改 product/kernel/weights/facts／键数／投稿方向；bump Latest／冻结身份 v0.0.19
- 重做 #211 K6-27 reloc／#207 K6-26 pp／#205 K6-25 scope／#201 K6-24 peep／#199 K6-23 opinfo／#195 K6-22 binsel／#191 K6-21 type／#189 K6-20 lex／#185 K6-18 prec／#183 K6-19 emit_arm／#181 cost-meas／#120 host-cc keydomain／更早旁支 E／#209 8509 C／#197 §8.1#3 C
- 把 K6-28 irsel_label 加为第四 seal-before／Softguess 触发／8509·8769 闭合证明
- 与 #120 host-cc keydomain 混读；与 1245 peephole opts suite completeness 混读；与 #211 K6-27 reloc_label 混读；并刀 parse

---

## 证据索引

- 本笔记：`research/notes/paper-a-derive-purify-k6-28-gold-irsel-label-not-a-gate-20261012-0708.md`
- 同模式先验：[`paper-a-derive-purify-k6-27-gold-reloc-label-not-a-gate-20261012-0652.md`](paper-a-derive-purify-k6-27-gold-reloc-label-not-a-gate-20261012-0652.md)（#211/#212）；[`paper-a-derive-purify-k6-26-gold-pp-label-not-a-gate-20261012-0627.md`](paper-a-derive-purify-k6-26-gold-pp-label-not-a-gate-20261012-0627.md)（#207/#208）；[`paper-a-derive-purify-k6-25-gold-scope-label-not-a-gate-20261012-0600.md`](paper-a-derive-purify-k6-25-gold-scope-label-not-a-gate-20261012-0600.md)（#205/#206）；[`paper-a-derive-purify-k6-24-gold-peep-label-not-a-gate-20261012-0526.md`](paper-a-derive-purify-k6-24-gold-peep-label-not-a-gate-20261012-0526.md)（#201/#202）；[`paper-a-derive-purify-k6-23-gold-opinfo-label-not-a-gate-20261012-0512.md`](paper-a-derive-purify-k6-23-gold-opinfo-label-not-a-gate-20261012-0512.md)（#199/#200）；[`paper-a-derive-purify-k6-22-gold-binsel-label-not-a-gate-20261012-0440.md`](paper-a-derive-purify-k6-22-gold-binsel-label-not-a-gate-20261012-0440.md)（#195/#196）；[`paper-a-derive-purify-k6-21-gold-type-label-not-a-gate-20261012-0415.md`](paper-a-derive-purify-k6-21-gold-type-label-not-a-gate-20261012-0415.md)（#191/#192）；[`paper-a-derive-purify-k6-20-gold-lex-label-not-a-gate-20261012-0401.md`](paper-a-derive-purify-k6-20-gold-lex-label-not-a-gate-20261012-0401.md)（#189/#190）；[`paper-a-derive-purify-k6-18-gold-prec-label-not-a-gate-20261012-0322.md`](paper-a-derive-purify-k6-18-gold-prec-label-not-a-gate-20261012-0322.md)（#185/#186）
- **DISTINCT #120：** type-table host-cc keydomain completeness ≠ A gate（~12:53）；本拍是 gold irsel_label 有限域 C↔gold.py，勿混
- **DISTINCT 1245：** peephole product opts suite completeness ≠ A gate；本拍是 gold `irsel_label` 映射表点采样，勿混
- **DISTINCT #211：** K6-27 reloc_label／JMPKIND×ARCH ≠ 本拍 irsel_label／IRSEL_MAP
- 产品证据：K6-28 `dde789ea…`；`research/c41-k6-28-gold-irsel-label.md`（blob `5034569b…`）；`seed/unisa_gold_irsel_label.c`；`tests/unisagoldirsellabelcheck.sh`；gate `unisa-gold-irsel-label`
- 上拍 E（K6-27；明示其后 NEW 叶未 E）：[`paper-a-derive-purify-k6-27-gold-reloc-label-not-a-gate-20261012-0652.md`](paper-a-derive-purify-k6-27-gold-reloc-label-not-a-gate-20261012-0652.md)（#211/#212）
- 8509 引用（不重钉）：[`paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-after-k6-25-26-20261012-0639.md`](paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-after-k6-25-26-20261012-0639.md)（#209/#210）
- 本拍短报：[`paper-a-heartbeat-report-20261012-0708.md`](paper-a-heartbeat-report-20261012-0708.md)

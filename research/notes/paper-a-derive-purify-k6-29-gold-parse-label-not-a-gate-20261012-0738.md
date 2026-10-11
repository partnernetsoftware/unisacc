# Paper A 衍生净化：K6-29 unisa.gold parse_label 不得搭乘 A

- **拍点：** 2026-10-12 ~07:38 Asia/Singapore（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `a3d7f2cf8e5ff773d93773c0e7825f515310ed82`（短 `a3d7f2c`）— 任务书 tip；= merge #216 bookkeeping after C #215（8509/8769 tip-refresh）；相对任务书 **无漂**；**本拍首次 E 净化 K6-29**。父链：#213/#214 K6-28 irsel_label E → K6-29 `e36b596` 产品叶（入 tip 于 #214→#215 之间）→ #215/#216 8509 C（明示 ≠ 身份闭合）← tip `a3d7f2c`。
- **锚产品叶（已入仓、本拍不重做）：** `e36b59628b145babd1c4e537c0229ce3dacf4636`（短 `e36b596`）— K6-29：**`unisa.gold` `parse_label`**；C 参考 `seed/unisa_gold_parse_label.c` ↔ python 经 **ast 抽取** 6 定义（`NT`、`TOKS`、`PRODS`、`SAME_NAME`、`PARSE_DEFAULT`、`parse_label` from gold.py；**不 import gold.py、不调用 `build()`**）；**阶段域 NT×TOKS（5×68=340 行）** byte-equal + **22 命名／边界点**（SAME 23 of 23：全表 1 + 命名 22）；标签 ⊂ `PRODS`（36）；neg **3/3**（unknown-nonterminal／missing-token／unknown-mode）；首红 **无**；**不称前端 parse 产品路径／gold 全闭包／其它标签函数／`build()`／权重训练／irsel／reloc**。这是 **parse 阶段域标签（parse_label／NT×TOKS）点采样 hygiene**，不是 A 理论；**≠ 前端 parse 产品路径**；**≠ gold 全闭包**；**≠ T1**。
- **产品证据页（只引）：** [`research/c41-k6-29-gold-parse-label.md`](../c41-k6-29-gold-parse-label.md)（blob `fe77ead4dabc1f879b9003c675ff9e2eee57becb`）；直跑日志 `research/c41-k6-29-gold-parse-label-run.log`；夹具 `tests/unisagoldparselabelcheck.sh`；seed `seed/unisa_gold_parse_label.c`；gate 行 `unisa-gold-parse-label`。
- **Latest：** **v0.0.40**（assets=10）；Paper A 冻结重测身份仍 **v0.0.19**（本拍不改；禁 bump/Latest）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证载体／字节流管道）→ 产品／unisa **gold `parse_label`／NT×TOKS 阶段域点采样 hygiene**（明确 **不是** T1／测量身份／§8.1#3／Softguess／前端 parse 产品路径／gold 全闭包）旁支 → 仍回 **A**
- **状态：** **note only**（登记「须先 K6-29 gold parse_label／340 阶段域绿／前端 parse 产品路径完稿／gold 全闭包／全部标签函数／`build()`／irsel／reloc 完稿，才许投 A／构造+T1 才成立／gate 绿＝测量身份闭合／缺之则实证封口不完整／须升 Softguess／第四 seal-before／＝§8.1#3 闭合」升格 ≠ 理论闸门；**不起稿**；**不改** A 根主张、键数身份、CN/EN/TeX/abstract、product/kernel/weights/facts、投稿方向；**不代裁** Ο1/Ο2（仍荐 Ο1）；**不重钉** Softguess／8509·8769／§8.1#3；**不重做** #213/#214 K6-28 irsel_label E、#211/#212 K6-27 reloc_label E、#207/#208 K6-26 pp_label E、#205/#206 K6-25 scope_label E、#201/#202 K6-24 peep_label E、#199/#200 K6-23 opinfo_label E、#195/#196 K6-22 binsel_label E、#191/#192 K6-21 type_label E、#189/#190 K6-20 lex_label E、#185/#186 K6-18 prec_label E、#183/#184 K6-19 emit_arm E、#181 §7.4 cost-meas E、#120 type-table host-cc keydomain E、#215/#216 8509 C tip-refresh、#197/#198 §8.1#3 tip-refresh；**本拍对象仅 K6-28 E 与 C #215 之后 NEW 的 K6-29**（产品叶已入 tip、C 已明示 ≠ 身份闭合、**尚未** E 登记）；**不授** 产品下一叶）
- **Softguess：** tip 四文件 **NONE×4**（机扫 `Softguess`/`softguess`/`SoftGuess`/`权重就是`/`weights are the logic`/`逻辑本身`/`权重即逻辑`/`权重≡逻辑` 均 0 hit；§3「不含 softmax」／「no softmax」否认句不计——CN/EN/TeX 各 1、abs 0；**NONE≠seal-before 债**；字面粘贴仍 P0 UNAPPLIED，本拍不粘）
- **测量身份：** 8509/8769 **仍开** — 仅引用既有 tip-refresh 钉（0722/#215/#216）；本拍**不重钉、不叠**；**不叠** Ο1/Ο2（仍荐 Ο1）
- **Paper A 正文漂移：** tip 四 blob 相对 #168 `ae7f7424` **SAME×4**（本拍零正文 diff）；相对 #154 `bdec4bcb`：CN/EN **SAME**，TeX/abs **CHANGED**（#168 image field layout 已入 TeX/abs）
- **为何本拍选 E 而非 C／B：** tip 已 `a3d7f2c`（= #216 after C #215；上拍 C 0722 已 tip-refresh 8509 并明示 K6-29 ≠ 身份闭合）；政委禁 inspection-only 重钉 SAME 的 C；Ο1 无新决策信息，禁叠 B；K6-29 产品叶已入仓且 **尚未** E 登记 → 本拍切 K6-29。

---

## 主张一句（本页唯一）

**把产品叶 K6-29——`unisa.gold` 的 `parse_label`（阶段域 NT×TOKS = 5×68=340 行 + 22 命名／边界点；C 参考 vs gold.py 源码经 ast 抽取 6 定义，不调用 `build()`；byte-equal，neg 3/3；标签 ⊂ PRODS；明确不称前端 parse 产品路径／gold Stage／其它标签／`build()`／全闭包／irsel／reloc）——升成 Paper A seal-before／T1 替代／8509·8769 测量身份闭合／Softguess 证据／第四闸门／§8.1#3 same-identity six-target native matrix 闭合——不得搭乘 A。** 这是产品／gold parse 阶段域标签点采样 hygiene，不是 A 理论；**parse_label 阶段域点采样 ≠ 前端 parse 产品路径 ≠ gold 全闭包 ≠ T1**。A seal-before 仍只是 §8.1#3、8509·8769、Ο1|Ο2（荐Ο1）。**NEW vs 既有 K6 E／#120／C #215：** 对象是 **gold `parse_label`／NT×TOKS 阶段域 C↔gold.py**，**≠ K6-28 `irsel_label`**（#213）、**≠ K6-27 `reloc_label`**（#211）、**≠ K6-26 `pp_label`**（#207）、**≠ K6-25 `scope_label`**（#205）、**≠ K6-24 `peep_label`**（#201）、**≠ K6-23 `opinfo_label`**（#199）、**≠ K6-22 `binsel_label`**（#195）、**≠ K6-21 `type_label`**（#191）、**≠ K6-20 `lex_label`**（#189）、**≠ K6-18 `prec_label`**（#185）、**≠ K6-19 `emit_arm`**（#183）、**≠ #120 type-table host-cc keydomain**（宿主 cc 键域完备，另一条）、**≠ C #215 8509 tip-refresh**（测量硬缺口钉，不是 E）、≠ image pe/elf/macho/fat/tape、≠ §7.4 cost-meas（#181）、≠ Softguess 产品路径、≠ 旧 peephole 产品 opts 完备性 E（1245）。

---

## 切什么

| 战役 | 对象 | 分类 |
| --- | --- | --- |
| K6-29 gold parse_label | C↔ast-extracted gold.py：NT×TOKS=340；命名／边界点 22；ast 抽 6 定义；labels ⊂ PRODS；neg 3/3（unknown-nonterminal／missing-token／unknown-mode）；不 import／不 `build()`；首红无 | **keep 产品／gold parse 阶段域标签 hygiene**；切出「须先绿／完稿 gold 全闭包·前端 parse 产品路径·全部标签·`build()`·irsel·reloc 才许投 A／＝T1／身份闭合／Softguess／第四 seal／＝§8.1#3 闭合」 |
| K6-28 gold irsel_label（已切） | #213/#214 | **keep 已切旁支**；本拍不重做；**DISTINCT**（irsel ≠ parse） |
| K6-27 gold reloc_label（已切） | #211/#212 | **keep 已切旁支**；本拍不重做；**DISTINCT**（reloc ≠ parse） |
| K6-26 gold pp_label（已切） | #207/#208 | **keep 已切旁支**；本拍不重做；**DISTINCT**（pp ≠ parse） |
| K6-25 gold scope_label（已切） | #205/#206 | **keep 已切旁支**；本拍不重做；**DISTINCT**（scope ≠ parse） |
| K6-24 gold peep_label（已切） | #201/#202 | **keep 已切旁支**；本拍不重做；**DISTINCT**（peep ≠ parse） |
| K6-23 gold opinfo_label（已切） | #199/#200 | **keep 已切旁支**；本拍不重做；**DISTINCT**（opinfo ≠ parse） |
| K6-22 gold binsel_label（已切） | #195/#196 | **keep 已切旁支**；本拍不重做；**DISTINCT**（binsel ≠ parse） |
| K6-21 gold type_label（已切） | #191/#192 | **keep 已切旁支**；本拍不重做；**DISTINCT**（type ≠ parse） |
| K6-20 gold lex_label（已切） | #189/#190 | **keep 已切旁支**；本拍不重做；**DISTINCT**（lex ≠ parse） |
| K6-18 gold prec_label（已切） | #185/#186 | **keep 已切旁支**；本拍不重做；**DISTINCT**（prec ≠ parse） |
| K6-19 emit_arm（已切） | #183/#184 | **keep 已切旁支**；本拍不重做 |
| #120 type-table host-cc keydomain（已切） | #120（~12:53） | **keep 已切旁支**；**DISTINCT**：宿主 cc 键域完备 ≠ gold parse_label 阶段域 C↔gold.py |
| §7.4 cost-meas（已切） | #181/#182 | **keep 已切旁支**；本拍不重做 |
| C #215 8509 tip-refresh（已钉） | #215/#216 | **keep 已钉硬缺口**；本拍不重做；**DISTINCT**（C 测量钉 ≠ E 升格切） |
| §8.1#3／8509·8769／Ο1\|Ο2 | 仍开硬缺口与决策卡（荐Ο1） | **keep seal-blocking**；本拍不重钉、不叠、不代裁 |
| A 根主张／T1 on \(K_s\) | 构造网络；声明域逐键全等 | **keep in A** |

**不切：** 根主张、键数数字、§8.1#3／Ο1/Ο2、product/kernel/weights/facts、CN/EN/TeX/abstract、投稿方向、K6-29 工程本身与任何未来 gold Stage／标签闭包／`build()`／前端 parse 产品路径工程债（只切升格读法）、Softguess 字面粘贴（仍 UNAPPLIED）。

**DISTINCT：** **K6-29 gold parse_label ≠ K6-28 irsel_label**（#213）**≠ K6-27 reloc_label**（#211）**≠ K6-26 pp_label**（#207）**≠ K6-25 scope_label**（#205）**≠ K6-24 peep_label**（#201）**≠ K6-23 opinfo_label**（#199）**≠ K6-22 binsel_label**（#195）**≠ K6-21 type_label**（#191）**≠ K6-20 lex_label**（#189）**≠ K6-18 prec_label**（#185）**≠ K6-19 emit_arm**（#183）**≠ #120 host-cc keydomain** ≠ K6-17 vm／divmod／deferred ≠ ape（#171）≠ pe（#167）≠ elf/macho（#163）≠ fat（#160）≠ tapebin／tape ≠ control／catalog／fp ≠ hosted-CI（#164）≠ Softguess 产品路径（2355）≠ §7.4 cost-meas（#181）≠ 旧 peephole opts suite completeness（1245）**≠ C #215 8509 tip-refresh**。本拍对象是 **gold parse 阶段域标签点采样（C↔gold.py NT×TOKS）**，不是 IR 选择标签、不是重定位标签、不是预处理标签、不是作用域标签、不是窥孔标签、不是算符信息标签、不是二进制选择标签、不是类型标签、不是词法标签、不是优先级标签、不是 ARM64 发射器、不是宿主 cc 键域完备、不是镜像写出、也不是代价测量方法学、也不是 8509 测量硬缺口钉；亦**不并刀前端 parse 产品路径／gold 全闭包／irsel／reloc**。

**Explicit negatives：** **K6-29 parse_label 阶段域点采样 ≠ T1**；**≠ 8509/8769 measurement identity**；**≠ Softguess**；**≠ fourth gate**；**≠ §8.1#3 closure**；**≠ 前端 parse 产品路径**；**≠ gold 全闭包／`build()`／Stage 表／全部标签函数／irsel／reloc**；**≠ #120 host-cc keydomain**；**≠ C #215**。

---

## 为何不挡 A 投稿

1. **A 根主张是构造＋T1（声明域 \(K_s\)），不是「gold parse_label／340 阶段域已绿／前端 parse 产品路径完稿／gold 全闭包」。** parse 阶段域标签点采样是产品旁路 hygiene，不是构造 falsifier，也不是第二套理论。
2. **gate／点采样绿 ≠ Softguess／第四 seal-before／8509·8769 闭合／§8.1#3 六目标原生矩阵。** 产品页自己是窄切（340 阶段域行 + 22 命名／边界点；ast 抽 6 定义；不 `build()`；明文不称前端 parse 产品路径／gold 全闭包）——更不能反向读成跨架构实证封口、测量身份闭合、或 A 投稿前提。C #215 已明示 K6-29 ≠ 身份闭合；本拍只登记升格禁读。
3. **与仍开硬缺口及已切旁支正交。** 本拍不把 parse_label 加为第四挡粘，不叠决策卡；对象 DISTINCT 如上（尤其 ≠ K6-28 irsel、≠ K6-27 reloc、≠ K6-20 lex、≠ #120 host-cc keydomain、≠ C #215）。
4. **切后 A 更纯：** A 不再被诱惑把「须先绿 gold parse_label／前端 parse 产品路径／gold 全闭包」当作 seal-before；攻击面从「须先 parse 阶段域／gold hygiene 全绿才许封 A」缩回「A = 构造表→整数阈值网络 + T1 声明域逐键全等 + 仍开测量债（8509/8769、§8.1#3）；gold parse_label 阶段域点采样挂产品枝」。
5. **本拍零 Paper A 正文 diff。**

---

## 父节点如何回 A

- **旁支回 A：** A（字节流／构造 TSV→weights 编译器 + T1 on \(K_s\) + unisacc 实证）→ unisa.gold（parse_label／NT×TOKS）产品 hygiene 旁支 → 仍回 A。
- 结果可回来收紧「产品旁路／gold 表卫生如何披露」措辞，不能长成与 A 并列的第二套理论，也不能把「parse_label 未绿／前端 parse 未做／gold 未闭」回写成投稿前提、T1 替代、Softguess 触发器或 §8.1#3 闭合证明。
- **DAG：** 切出的是升格闸门，不是删掉 gold parse 工程债或 A 硬缺口。gold parse 阶段域标签点采样是实证旁路，不是编译器根主张。K6-29 = empirical product side-path under unisa gold stack，**not** a theory gate for the constructed neural compiler paper；**not** front-end parse product path；**not** gold full closure；**not** T1.

---

## 切后 A 哪一句更硬

切出后，A 可把 gold 旁支边界收成一句更硬的主张（**本拍不改正文；仅登记**）：

> **Paper A 只主张基于神经网络（构造网络与权重）的编译器与 T1（网络≡表于声明域 \(K_s\)），并以 unisacc 为 POSIX C99 子集跨架构实证；`unisa.gold` 的 `parse_label`／NT×TOKS 阶段域点采样（及日后前端 parse 产品路径／Stage 表／全部标签／`build()`／全闭包）都属于产品载体／gold hygiene，不是 A 的 seal-before、不是 T1、不是测量身份闭合、也不是 Softguess 触发。**

**切后 A 更纯在哪（一句）：** A 更纯在：parse_label／NT×TOKS 金标对照是产品／旁支实证（阶段域点采样，≠ 前端 parse 路径、≠ gold 全闭包），不进根主张闸门；攻击面缩回构造+T1+仍开测量债，该 parse_label hygiene 挂产品枝。

---

## tip / blob 表（本拍核对）

| 项 | 值 |
| --- | --- |
| tip before | `a3d7f2cf8e5ff773d93773c0e7825f515310ed82` |
| tip after（本 note 合入后填） | **TBD**（squash Merge 本 E 后由 bookkeeping PR 填） |
| K6-29 产品叶 | `e36b59628b145babd1c4e537c0229ce3dacf4636`（入 tip 于 #214→#215 之间；≠ tip before；本拍首次 E） |
| 上拍锚 tip | `a3d7f2c`（#216 tip-after；C #215 8509 已合）；其间／其上 NEW 产品叶 = K6-29 `e36b596`，未 E 净化直至本拍 |
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
| 8509/8769 measurement identity | **仍开**（活 gold op-87／8769／569 vs EN/TeX/abs op-74／8509／517；见 0722 C #215/#216） |
| Ο1\|Ο2（投稿/封口重测身份） | **仍开**；**仍荐 Ο1**；本拍不叠、不代裁 |
| Softguess 字面粘贴 | NONE×4 预期态；字面仍 P0 UNAPPLIED；**不升** seal-before |

---

## OUT（本拍明确不做）

- 改正文 CN/EN/TeX/abstract；粘 Softguess／近似推断
- 代裁／叠卡 Ο1/Ο2；重钉 Softguess tip-refresh 或 8509·8769 tip-refresh 或 §8.1#3 tip-refresh
- 改 product/kernel/weights/facts／键数／投稿方向；bump Latest／冻结身份 v0.0.19
- 重做 #213 K6-28 irsel／#211 K6-27 reloc／#207 K6-26 pp／#205 K6-25 scope／#201 K6-24 peep／#199 K6-23 opinfo／#195 K6-22 binsel／#191 K6-21 type／#189 K6-20 lex／#185 K6-18 prec／#183 K6-19 emit_arm／#181 cost-meas／#120 host-cc keydomain／更早旁支 E／#215 8509 C／#197 §8.1#3 C
- 把 K6-29 parse_label 加为第四 seal-before／Softguess 触发／8509·8769 闭合证明
- 与 #120 host-cc keydomain 混读；与 1245 peephole opts suite completeness 混读；与 #213 K6-28 irsel_label 混读；与 #211 K6-27 reloc_label 混读；并刀前端 parse 产品路径／gold 全闭包／irsel／reloc；与 C #215 混读为身份闭合

---

## 证据索引

- 本笔记：`research/notes/paper-a-derive-purify-k6-29-gold-parse-label-not-a-gate-20261012-0738.md`
- 同模式先验：[`paper-a-derive-purify-k6-28-gold-irsel-label-not-a-gate-20261012-0708.md`](paper-a-derive-purify-k6-28-gold-irsel-label-not-a-gate-20261012-0708.md)（#213/#214）；[`paper-a-derive-purify-k6-27-gold-reloc-label-not-a-gate-20261012-0652.md`](paper-a-derive-purify-k6-27-gold-reloc-label-not-a-gate-20261012-0652.md)（#211/#212）；[`paper-a-derive-purify-k6-26-gold-pp-label-not-a-gate-20261012-0627.md`](paper-a-derive-purify-k6-26-gold-pp-label-not-a-gate-20261012-0627.md)（#207/#208）；[`paper-a-derive-purify-k6-25-gold-scope-label-not-a-gate-20261012-0600.md`](paper-a-derive-purify-k6-25-gold-scope-label-not-a-gate-20261012-0600.md)（#205/#206）；[`paper-a-derive-purify-k6-24-gold-peep-label-not-a-gate-20261012-0526.md`](paper-a-derive-purify-k6-24-gold-peep-label-not-a-gate-20261012-0526.md)（#201/#202）；[`paper-a-derive-purify-k6-23-gold-opinfo-label-not-a-gate-20261012-0512.md`](paper-a-derive-purify-k6-23-gold-opinfo-label-not-a-gate-20261012-0512.md)（#199/#200）；[`paper-a-derive-purify-k6-22-gold-binsel-label-not-a-gate-20261012-0440.md`](paper-a-derive-purify-k6-22-gold-binsel-label-not-a-gate-20261012-0440.md)（#195/#196）；[`paper-a-derive-purify-k6-21-gold-type-label-not-a-gate-20261012-0415.md`](paper-a-derive-purify-k6-21-gold-type-label-not-a-gate-20261012-0415.md)（#191/#192）；[`paper-a-derive-purify-k6-20-gold-lex-label-not-a-gate-20261012-0401.md`](paper-a-derive-purify-k6-20-gold-lex-label-not-a-gate-20261012-0401.md)（#189/#190）；[`paper-a-derive-purify-k6-18-gold-prec-label-not-a-gate-20261012-0322.md`](paper-a-derive-purify-k6-18-gold-prec-label-not-a-gate-20261012-0322.md)（#185/#186）
- **DISTINCT #120：** type-table host-cc keydomain completeness ≠ A gate（~12:53）；本拍是 gold parse_label 阶段域 C↔gold.py，勿混
- **DISTINCT 1245：** peephole product opts suite completeness ≠ A gate；本拍是 gold `parse_label` 阶段域点采样，勿混
- **DISTINCT #213：** K6-28 irsel_label／IRSEL_MAP ≠ 本拍 parse_label／NT×TOKS
- **DISTINCT #211：** K6-27 reloc_label／JMPKIND×ARCH ≠ 本拍 parse_label／NT×TOKS
- **DISTINCT #215：** 8509/8769 C tip-refresh ≠ 本拍 E 升格切（C 已明示 K6-29 ≠ 身份闭合）
- 产品证据：K6-29 `e36b596…`；`research/c41-k6-29-gold-parse-label.md`（blob `fe77ead4…`）；`seed/unisa_gold_parse_label.c`；`tests/unisagoldparselabelcheck.sh`；gate `unisa-gold-parse-label`
- 上拍 C（明示 K6-29 ≠ 身份闭合、未 E）：[`paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-after-k6-27-28-20261012-0722.md`](paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-after-k6-27-28-20261012-0722.md)（#215/#216）
- 上拍 E（K6-28；明示其后 NEW 叶未 E）：[`paper-a-derive-purify-k6-28-gold-irsel-label-not-a-gate-20261012-0708.md`](paper-a-derive-purify-k6-28-gold-irsel-label-not-a-gate-20261012-0708.md)（#213/#214）
- 8509 引用（不重钉）：[`paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-after-k6-27-28-20261012-0722.md`](paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-after-k6-27-28-20261012-0722.md)（#215/#216）
- 本拍短报：[`paper-a-heartbeat-report-20261012-0738.md`](paper-a-heartbeat-report-20261012-0738.md)

# Paper A 衍生净化：K6-18 unisa.gold prec_label 不得搭乘 A

- **拍点：** 2026-10-12 ~03:22 Asia/Singapore（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `ce4e942030ca83c6b45d3f01a6d8e0a88d17c83c`（短 `ce4e9420`）— fill tip-after for K6-19 E #184 after #183（K6-19 emit_arm E 已入）。父链含 #183/#184 K6-19 emit_arm E ← #181/#182 §7.4 cost-meas E ← #179/#180 8509/8769 C tip-refresh ← **`5e59250a` K6-18 prec_label** ← #178+#177 K6-17 divmod E ← …
- **锚产品叶（已入仓、本拍不重做）：** `5e59250afe85b08d8f95129cd2b2d47678b44569`（短 `5e59250a`）— K6-18：**`unisa.gold` `prec_label` / `PREC_OPS`**；C 参考 `seed/unisa_gold_prec.c` ↔ python 经 **ast 抽取** `PREC_OPS`/`PREC_LEV`/`_PREC`/`prec_label`（**不 import gold.py、不调用 `build()`**）；**PREC_OPS 全表 19** + 命名算符点 + 边界字符串；15 点 C↔python stdout 逐字节相等；neg **2/2**；**不称 gold 全闭包**。这是 **优先级标签表（precedence label）点采样 hygiene**，不是 A 理论。
- **产品证据页（只引）：** [`research/c41-k6-18-gold-prec.md`](../c41-k6-18-gold-prec.md)（blob `dd5e295d2e0390e19c5165ef29f1f736e885fecf`）；直跑日志 `research/c41-k6-18-gold-prec-run.log`；夹具 `tests/unisagoldpreccheck.sh`；seed `seed/unisa_gold_prec.c`；gate 行 `unisa-gold-prec`。
- **Latest：** **v0.0.40**（assets=10）；Paper A 冻结重测身份仍 **v0.0.19**（本拍不改；禁 bump/Latest）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证载体／字节流管道）→ 产品／unisa **gold `prec_label`／`PREC_OPS` 点采样 hygiene**（明确 **不是** T1／测量身份／§8.1#3／Softguess）旁支 → 仍回 **A**
- **状态：** **note only**（登记「须先 K6-18 gold prec_label／PREC_OPS 全表绿／gold 全闭包／Stage 表／全部标签函数／`build()` 完稿，才许投 A／构造+T1 才成立／gate 绿＝测量身份闭合／缺之则实证封口不完整／须升 Softguess／第四 seal-before／＝§8.1#3 闭合」升格 ≠ 理论闸门；**不起稿**；**不改** A 根主张、键数身份、CN/EN/TeX/abstract、product/kernel/weights/facts、投稿方向；**不代裁** Ο1/Ο2（仍荐 Ο1）；**不重钉** Softguess／8509·8769／§8.1#3；**不重做** #183 K6-19 emit_arm E、#181 §7.4 cost-meas E、#179 8509 tip-refresh、#177/#175/#173 K6-17 三刀 E、#171 K6-16 ape、#167 K6-15 pe、#163 K6-13/14、#160 K6-12 fat、#164 hosted-CI、#169 Softguess；**本拍对象仅此前 defer 的 K6-18**（上拍 #183 注明「本拍不叠 K6-18；留待下拍」）；**不授** 产品下一叶）
- **Softguess：** tip 四文件 **NONE×4**（机扫 `Softguess`/`softguess`/`SoftGuess`/`权重就是`/`weights are the logic`/`逻辑本身`/`权重即逻辑`/`权重≡逻辑` 均 0 hit；§3「不含 softmax」／「no softmax」否认句不计——CN/EN/TeX 各 1、abs 0；**NONE≠seal-before 债**；字面粘贴仍 P0 UNAPPLIED，本拍不粘）
- **测量身份：** 8509/8769 **仍开** — 仅引用 [`paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-after-k6-17-e-20261012-0242.md`](paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-after-k6-17-e-20261012-0242.md)；本拍**不重钉、不叠**；**不叠** Ο1/Ο2（仍荐 Ο1）
- **Paper A 正文漂移：** tip 四 blob 相对 #168 `ae7f7424` **SAME×4**（本拍零正文 diff）；相对 #154 `bdec4bcb`：CN/EN **SAME**，TeX/abs **CHANGED**（#168 image field layout 已入 TeX/abs）
- **为何本拍选 E 而非 C／B：** tip 自 `888b24e3`（上拍 C 锚）→ cost E → K6-19 产品叶 → **`ce4e9420`（#184 tip-after）**；K6-19 E 已做（#183/#184）；**K6-18 上拍明确 defer 本拍**；政委禁 inspection-only 重钉 SAME 的 C；Ο1 无新决策信息，禁叠 B。

---

## 主张一句（本页唯一）

**把产品叶 K6-18——`unisa.gold` 的 `prec_label`／`PREC_OPS`（全表 19 ops + 边界字符串；C 参考 vs gold.py 源码经 ast 抽取，不调用 `build()`；15 点 byte-equal，neg 2/2；明确不称 gold 全闭包）——升成 Paper A seal-before／T1 替代／8509·8769 测量身份闭合／Softguess 证据／第四闸门／§8.1#3 same-identity six-target native matrix 闭合——不得搭乘 A。** 这是产品／gold 优先级标签表点采样 hygiene，不是 A 理论。A seal-before 仍只是 §8.1#3、8509·8769、Ο1|Ο2（荐Ο1）。**NEW vs 既有 K6 E：** 对象是 **gold `prec_label`／`PREC_OPS` 有限域标签表**，不是 K6-19 `emit_arm` N/w/movimm/mem（#183）、不是 K6-17 vm／divmod／deferred、不是 ape／pe／elf／macho／fat／tape 镜像写出族，也不是 #181 §7.4 代价测量升格，也不是 Softguess 产品路径。

---

## 切什么

| 战役 | 对象 | 分类 |
| --- | --- | --- |
| K6-18 gold prec_label | C↔ast-extracted gold.py：`PREC_OPS` 19；命名算符点；边界字符串 5；15/15；neg 2/2（missing-operator／unknown-mode）；不 import／不 `build()` | **keep 产品／gold 优先级标签 hygiene**；切出「须先绿／完稿 gold 全闭包·Stage·全部标签·`build()` 才许投 A／＝T1／身份闭合／Softguess／第四 seal／＝§8.1#3 闭合」 |
| K6-19 emit_arm（已切） | #183/#184 | **keep 已切旁支**；本拍不重做 |
| §7.4 cost-meas（已切） | #181/#182 | **keep 已切旁支**；本拍不重做 |
| K6-17 三刀（已切） | vm／deferred／divmod；#173/#175/#177 | **keep 已切旁支**；本拍不重做 |
| §8.1#3／8509·8769／Ο1\|Ο2 | 仍开硬缺口与决策卡（荐Ο1） | **keep seal-blocking**；本拍不重钉、不叠、不代裁 |
| A 根主张／T1 on \(K_s\) | 构造网络；声明域逐键全等 | **keep in A** |

**不切：** 根主张、键数数字、§8.1#3／Ο1/Ο2、product/kernel/weights/facts、CN/EN/TeX/abstract、投稿方向、K6-18 工程本身与任何未来 gold Stage／标签闭包／`build()` 工程债（只切升格读法）、Softguess 字面粘贴（仍 UNAPPLIED）。

**DISTINCT：** **K6-18 gold prec_label ≠ K6-19 emit_arm**（#183）≠ K6-17 vm／divmod／deferred ≠ ape（#171）≠ pe（#167）≠ elf/macho（#163）≠ fat（#160）≠ tapebin／tape ≠ control／catalog／fp ≠ hosted-CI（#164）≠ Softguess 产品路径（2355）≠ §7.4 cost-meas（#181）。本拍对象是 **gold 优先级标签有限域点采样**，不是 ARM64 发射器、不是 VM 算术、不是镜像写出、也不是代价测量方法学。

**Explicit negatives：** **K6-18 prec_label 点采样 ≠ T1**；**≠ 8509/8769 measurement identity**；**≠ Softguess**；**≠ fourth gate**；**≠ §8.1#3 closure**；**≠ gold 全闭包／`build()`／Stage 表／全部标签函数**。

---

## 为何不挡 A 投稿

1. **A 根主张是构造＋T1（声明域 \(K_s\)），不是「gold prec_label／PREC_OPS 已绿／gold 全闭包」。** 运算符优先级标签有限域点采样是产品旁路 hygiene，不是构造 falsifier，也不是第二套理论。
2. **gate／点采样绿 ≠ Softguess／第四 seal-before／8509·8769 闭合／§8.1#3 六目标原生矩阵。** 0239 裁定自己是窄切（PREC_OPS 全表 + 边界；ast 抽取；不 `build()`；不称 gold 全闭包）——更不能反向读成跨架构实证封口、测量身份闭合、或 A 投稿前提。
3. **与仍开硬缺口及已切旁支正交。** 本拍不把 prec_label 加为第四挡粘，不叠决策卡；对象 DISTINCT 如上。
4. **切后 A 更纯：** A 不再被诱惑把「须先绿 gold prec_label／gold 全闭包」当作 seal-before；攻击面从「须先优先级标签表／gold hygiene 全绿才许封 A」缩回「A = 构造表→整数阈值网络 + T1 声明域逐键全等 + 仍开测量债（8509/8769、§8.1#3）；gold prec_label 点采样挂产品枝」。
5. **本拍零 Paper A 正文 diff。**

---

## 父节点如何回 A

- **旁支回 A：** A（字节流／构造 TSV→weights 编译器 + T1 on \(K_s\) + unisacc 实证）→ unisa.gold（prec_label／PREC_OPS）产品 hygiene 旁支 → 仍回 A。
- 结果可回来收紧「产品旁路／gold 表卫生如何披露」措辞，不能长成与 A 并列的第二套理论，也不能把「prec_label 未绿／gold 未闭」回写成投稿前提、T1 替代、Softguess 触发器或 §8.1#3 闭合证明。
- **DAG：** 切出的是升格闸门，不是删掉 gold prec 工程债或 A 硬缺口。gold 优先级标签点采样是实证旁路，不是编译器根主张。K6-18 = empirical product side-path under unisa gold stack，**not** a theory gate for the constructed neural compiler paper.

---

## 切后 A 哪一句更硬

切出后，A 可把 gold 旁支边界收成一句更硬的主张（**本拍不改正文；仅登记**）：

> **Paper A 只主张基于神经网络（构造网络与权重）的编译器与 T1（网络≡表于声明域 \(K_s\)），并以 unisacc 为 POSIX C99 子集跨架构实证；`unisa.gold` 的 `prec_label`／`PREC_OPS` 有限域点采样（及日后 Stage 表／全部标签／`build()` 闭包）都属于产品载体／gold hygiene，不是 A 的 seal-before、不是 T1、不是测量身份闭合、也不是 Softguess 触发。**

**切后 A 更纯在哪（一句）：** A 不再被诱惑把「须先绿 gold prec_label／gold 全闭包」当作封口前条件；攻击面缩回构造+T1+仍开测量债，该优先级标签 hygiene 挂产品枝。

---

## tip / blob 表（本拍核对）

| 项 | 值 |
| --- | --- |
| tip before | `ce4e942030ca83c6b45d3f01a6d8e0a88d17c83c` |
| tip after（本 note 合入后填） | `7e87921f2b2894272bc10d8e341f7672d46b3416`（短 `7e87921f`；= squash Merge #185 本 E） |
| K6-18 产品叶 | `5e59250afe85b08d8f95129cd2b2d47678b44569` |
| 上拍 E 锚 tip | `ce4e942030ca83c6b45d3f01a6d8e0a88d17c83c`（#184 tip-after；K6-19 E 已入；本拍切此前 defer 的 K6-18） |
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

# Paper A 衍生净化：tip c41 / K6-15 `unisa.image.pe` 不得搭乘 A

- **拍点：** 2026-10-12 ~01:01 Asia/Singapore（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `eec99ccc1b1115d551bf917b18ad94a8e6fad591`（= K6-15 `unisa.image.pe` static write；父链含 #166 C 8509 tip-refresh `482f9c15…` ← #165 C §8.1#3 ← #164 hosted-CI E ← #163 K6-13/14 elf+macho E ← #154 T1→\(K_s\)）
- **父节点 tip 链（相关）：** K6-15 `eec99ccc…`（research(c41)+seed+tests：`unisa.image.pe` static write，stub empty，dynamic False；C reference vs `pe.py`；7 specs byte-equal；import/load-config/reloc content rechecked；neg 3/3；**not** an APE/Windows-load closure；0048 ruling）；祖先含 #166 C 8509 tip-refresh、#164 hosted-CI E、#163 K6-13/14、#160 K6-12 fat E、#154 T1→\(K_s\)
- **Latest：** **v0.0.40**；Paper A 冻结重测身份仍 **v0.0.19**（本拍不改；禁 bump/Latest）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证载体／字节流管道）→ 产品／unisa **image.pe**（C↔py write 参考对 hygiene／PE 镜像点采样验收；明确非 APE／Windows-load 闭合）旁支 → 仍回 **A**
- **状态：** **note only**（登记「须先 K6-15 pe 绿／完稿才许投 A／构造+T1 才成立／gate 绿＝测量身份闭合／缺之则实证封口不完整／须升 Softguess／第四 seal-before／＝§8.1#3 闭合」升格 ≠ 理论闸门；**不起稿**；**不改** A 根主张、键数身份、CN/EN/TeX/abstract、product/kernel/weights/facts、投稿方向；**不代裁** Ο1/Ο2（仍荐 Ο1）；**不重钉** Softguess／8509·8769／§8.1#3；**不重做** #163 K6-13/14、#160 K6-12 fat、#164 hosted-CI、#166 C tip-refresh）
- **Softguess：** tip 四文件 **NONE×4**（机扫 `Softguess`/`softguess`/`权重就是`/`weights are the logic`/`逻辑本身`/`权重即逻辑`/`权重≡逻辑` 均 0 hit；§3「不含 softmax」／「no softmax」否认句不计；**NONE≠seal-before 债**）
- **测量身份：** 8509/8769 **仍开** — 本拍不重钉、不叠；**不叠** Ο1/Ο2（仍荐 Ο1）
- **Paper A 正文漂移：** tip 四 blob 相对 #154 `bdec4bcb` **SAME**（本拍零正文 diff）

---

## 主张一句（本页唯一）

**把 tip 产品战役 K6-15（`unisa.image.pe` C↔`pe.py` static write；stub empty；dynamic False；7 specs byte-equal；import/load-config/reloc rechecked；neg 3/3；明确非 APE/Windows-load 闭合）升成 Paper A seal-before／T1 替代／8509·8769 测量身份闭合／Softguess 证据／第四闸门／§8.1#3 闭合——不得搭乘 A。** 这是产品／镜像／PE 载体 hygiene 点采样，不是 A 理论。A seal-before 仍只是 §8.1#3、8509·8769、Ο1|Ο2（荐Ο1）。同模式见 #163（K6-13 elf + K6-14 macho ≠ A gate）、#160（K6-12 fat ≠ A gate）、#164（hosted-CI green ≠ T1/§8.1#3 native）。

---

## 切什么

| 战役 | 对象 | 分类 |
| --- | --- | --- |
| K6-15 | `unisa.image.pe` C↔`unisa/image/pe.py` static write；stub empty；dynamic False；7 specs byte-equal；import/load-config/reloc content rechecked；neg 3/3；**not** an APE/Windows-load closure；0048 ruling | **keep 产品／镜像／PE 参考对 hygiene**；切出「须先绿才许投 A／＝T1／身份闭合／Softguess／第四 seal／＝§8.1#3 闭合」 |
| §8.1#3／8509·8769／Ο1\|Ο2 | 仍开硬缺口与决策卡（荐Ο1） | **keep seal-blocking**；本拍不重钉、不叠、不代裁 |
| A 根主张／T1 on \(K_s\) | 构造网络；声明域逐键全等 | **keep in A** |

**不切：** 根主张、键数数字、§8.1#3／Ο1/Ο2、product/kernel/weights/facts、CN/EN/TeX/abstract、投稿方向、K6-15 工程本身（只切升格读法）。

**DISTINCT：** pe ≠ elf/macho（#163）≠ fat（#160）≠ tapebin ≠ tape ≠ control／catalog／fp ≠ hosted-CI（#164）。本拍对象是 PE 静态写盘参考对点采样，不是 ELF/Mach-O、不是 FAT、不是 tape 族、不是托管 CI 绿。

**Explicit negatives：** **K6-15 ≠ T1**；**≠ 8509/8769 measurement identity**；**≠ Softguess**；**≠ fourth gate**；**≠ §8.1#3 closure**.

---

## 为何不挡 A 投稿

1. **A 根主张是构造＋T1（声明域 \(K_s\)），不是「unisa.image.pe 点采样已绿」。** PE C↔py write 是应用／镜像层写出旁路 hygiene，不是构造 falsifier，也不是第二套理论。
2. **gate／点采样绿 ≠ Softguess／第四 seal-before／8509·8769 闭合／§8.1#3 六目标原生矩阵。** K6-15 自己写明 **not an APE/Windows-load closure**——更不能反向读成跨架构实证封口、Windows 加载闭合、或测量身份闭合。
3. **与仍开硬缺口及已切旁支正交。** 本拍不把 pe 加为第四挡粘，不叠决策卡；对象 DISTINCT 如上。
4. **切后 A 更纯：** A 不再被诱惑把 PE image write 点采样 parity 当作 seal-before；攻击面从「须先 K6-15 完稿才许封 A」缩回「硬缺口仍只开 §8.1#3、8509·8769、Ο1|Ο2」。
5. **本拍零 Paper A 正文 diff。**

---

## 父节点如何回 A

- **旁支回 A：** A（字节流／构造 TSV→weights 编译器 + T1 on \(K_s\) + unisacc 实证）→ unisa.image.pe 产品／镜像参考对 hygiene 旁支 → 仍回 A。
- 结果可回来收紧「载体／镜像写出如何披露」措辞，不能长成与 A 并列的第二套理论，也不能把「K6-15 未绿」回写成投稿前提、T1 替代、Softguess 触发器或 §8.1#3 闭合证明。
- **DAG：** 切出的是升格闸门，不是删掉 pe 工程债或 A 硬缺口。字节流／镜像写出是实证旁路，不是编译器根主张。

---

## 切后 A 哪一句更硬

切出后，A 可把镜像旁支边界收成一句更硬的主张（**本拍不改正文；仅登记**）：

> **Paper A 只主张基于神经网络（构造网络与权重）的编译器与 T1（网络≡表于声明域 \(K_s\)），并以 unisacc 为 POSIX C99 子集跨架构实证；PE／ELF／Mach-O／FAT 等 image write 点采样属于产品载体／镜像写出 hygiene，不是 A 的 seal-before、不是 T1、不是测量身份闭合、也不是 Softguess 触发。**

**切后 A 更纯在哪（一句）：** A 不再被诱惑把 PE image write C↔py byte-equal 点采样当作封口前条件。

---

## tip / blob 表（本拍核对）

| 项 | 值 |
| --- | --- |
| tip before | `eec99ccc1b1115d551bf917b18ad94a8e6fad591` |
| tip after（本 note 合入后填） | _(merge 后由 PR 填)_ |
| Paper A 末次正文 | #154 `bdec4bcbf823d0b527a15c40afe61f69942063fc`（T1→\(K_s\)） |
| K6-15 | `eec99ccc1b1115d551bf917b18ad94a8e6fad591` |
| CN `research/unisacc-paper.md` | `5231dd3bf3e60804c678d48015f1e337edd9dc56` **SAME** vs #154 |
| EN `research/unisacc-paper.en.md` | `0a707d704079bfef3b2b5193bfa8dc2b5ec90303` **SAME** |
| TeX `research/arxiv-paper-a/main.tex` | `0ef6027ccc73b20b1d7d4371f89c47b069885bb7` **SAME** |
| abs `research/arxiv-paper-a/abstract.txt` | `43d7ea7e7297ed8be9a96e3950e014e359467973` **SAME** |
| Softguess | **NONE×4**（预期态；不升 seal-before） |

---

## Blockers unchanged

- §8.1#3 same-identity six-target **native** matrix — **仍开**
- 8509/8769 dual measurement identity — **仍开**
- Ο1/Ο2 pending — **仍开**（**荐Ο1**；本拍不叠决策卡）

---

## OUT（本拍明确不做）

- 改正文 CN/EN/TeX/abstract；粘 Softguess／近似推断
- 代裁／叠卡 Ο1/Ο2；重钉 Softguess tip-refresh 或 8509·8769／§8.1#3 tip-refresh
- 改 product/kernel/weights/facts／键数／投稿方向；bump Latest
- 重做 #163 K6-13/14、#160 K6-12 fat、#164 hosted-CI、#166 C tip-refresh
- 把 K6-15 pe 加为第四 seal-before／Softguess 触发／8509·8769 闭合证明／T1 替代／§8.1#3 闭合

---

## 证据索引

- 本笔记：`research/notes/paper-a-e-derive-purify-k6-15-pe-ne-a-gate-20261012-0101.md`
- 同模式先验：[`paper-a-derive-purify-k6-13-14-image-elf-macho-not-a-gate-20261011-2356.md`](paper-a-derive-purify-k6-13-14-image-elf-macho-not-a-gate-20261011-2356.md)（#163）；[`paper-a-derive-purify-k6-12-image-fat-not-a-gate-20261011-2316.md`](paper-a-derive-purify-k6-12-image-fat-not-a-gate-20261011-2316.md)（#160）；[`paper-a-derive-purify-hosted-runner-ci-green-not-t1-or-s81-native-20261012-0014.md`](paper-a-derive-purify-hosted-runner-ci-green-not-t1-or-s81-native-20261012-0014.md)（#164）
- 最近 C tip-refresh（勿重做）：[`paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-after-hosted-ci-matrix-c-20261012-0039.md`](paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-after-hosted-ci-matrix-c-20261012-0039.md)（#166）
- 产品证据：K6-15 `eec99ccc…`
- T1 声明域统一：#154；Softguess tip-refresh：#153

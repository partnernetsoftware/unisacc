# Paper A 衍生净化：tip c41 / K6-13 elf + K6-14 macho 不得搭乘 A

- **拍点：** 2026-10-11 ~23:56 Asia/Singapore（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `c3c6a6d0169ed985f542cd46c74e6cf2481bd74b`（= K6-14 unisa.image.macho；父 = K6-13 `45de2a759e0f4c4e8bb17f3b69e79e413b85dd92` ← #162 Adam E `62779188…` ← #161 C tip-refresh）
- **父节点 tip 链（相关）：** K6-14 `c3c6a6d0…`（research(c41)+seed+tests：unisa.image.macho C reference vs `macho.py` `write()`；5 specs byte-equal；load commands／DYLD_INFO binds／ad-hoc SuperBlob per-page SHA-256；neg 3/3；**not** a loadable/codesign closure；2350 ruling）；K6-13 `45de2a75…`（unisa.image.elf static write C vs `elf.py`；7 specs byte-equal；layout property；signed entry；neg 3/3；2345 ruling）；祖先含 #162 Adam E、#161 C §8.1#3、#160 K6-12 fat E、#154 T1→\(K_s\)
- **Latest：** **v0.0.40**；Paper A 冻结重测身份仍 **v0.0.19**（本拍不改；禁 bump/Latest）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证载体／字节流管道）→ 产品／unisa **image.elf** + **image.macho**（C↔py write 参考对 hygiene／ELF·Mach-O 镜像点采样验收）旁支 → 仍回 **A**
- **状态：** **note only**（登记「须先 K6-13 elf／K6-14 macho 绿／完稿才许投 A／构造+T1 才成立／gate 绿＝测量身份闭合／缺之则实证封口不完整／须升 Softguess／第四 seal-before」升格 ≠ 理论闸门；**不起稿**；**不改** A 根主张、键数身份、CN/EN/TeX/abstract、product/kernel/weights/facts、投稿方向；**不代裁** Ο1/Ο2（仍荐 Ο1）；**不重钉** Softguess／8509·8769／§8.1#3；**不重做** #160 K6-12 fat E、#162 Adam E、#161 C、#158/#157/#155 旁支 E）
- **Softguess：** tip 四文件 **NONE×4**（机扫 `Softguess`/`softguess`/`权重就是`/`weights are the logic`/`逻辑本身`/`权重即逻辑`/`权重≡逻辑` 均 0 hit；§3「不含 softmax」／「no softmax」否认句不计；**NONE≠seal-before 债**）
- **测量身份：** 8509/8769 **仍开** — 本拍不重钉、不叠；**不叠** Ο1/Ο2
- **Paper A 正文漂移：** tip 四 blob 相对 #154 `bdec4bcb` **SAME**（本拍零正文 diff）

---

## 主张一句（本页唯一）

**把 tip 产品战役 K6-13（unisa.image.elf C↔elf.py write；7 specs byte-equal；layout／signed entry；neg 3/3）与 K6-14（unisa.image.macho C↔macho.py write；5 specs byte-equal；load commands／DYLD_INFO／ad-hoc SuperBlob；neg 3/3；明确非 loadable/codesign 闭合）升成 Paper A seal-before／T1／测量身份／Softguess／第四闸门——不得搭乘 A。** 这是产品／镜像／ELF·Mach-O 载体 hygiene 点采样，不是 A 理论。A seal-before 仍只是 §8.1#3、8509·8769、Ο1|Ο2（荐Ο1）。同模式见 #160（K6-12 fat ≠ A gate）、#158（K6-11 tapebin）、#157（K6-10 tape）、#155（K6-7/8/9）。

---

## 切什么

| 战役 | 对象 | 分类 |
| --- | --- | --- |
| K6-13 | `unisa.image.elf` C↔`unisa/image/elf.py` `write()`；7 specs byte-equal；layout property；signed entry；neg 3/3；2345 ruling | **keep 产品／镜像／ELF 参考对 hygiene**；切出「须先绿才许投 A／＝T1／身份闭合／Softguess／第四 seal」 |
| K6-14 | `unisa.image.macho` C↔`macho.py` `write()`；5 specs byte-equal；load commands／DYLD_INFO binds／ad-hoc SuperBlob per-page SHA-256；neg 3/3；**not** loadable/codesign closure；2350 ruling | **keep 产品／镜像／Mach-O 参考对 hygiene**；切出同上升格读法（且明确「非 codesign 闭合」更不能冒充跨平台实证封口） |
| §8.1#3／8509·8769／Ο1|Ο2 | 仍开硬缺口与决策卡（荐Ο1） | **keep seal-blocking**；本拍不重钉、不叠、不代裁 |
| A 根主张／T1 on \(K_s\) | 构造网络；声明域逐键全等 | **keep in A** |

**不切：** 根主张、键数数字、§8.1#3、Ο1/Ο2、product/kernel/weights/facts、CN/EN/TeX/abstract、投稿方向、K6-13/14 工程本身（只切升格读法）。

**DISTINCT：** elf／macho ≠ fat（#160）≠ tapebin ≠ tape ≠ control／catalog／fp ≠ CXIF ≠ Adam 协议细节（#162）。本拍对象是 ELF 静态写盘与 Mach-O 写盘参考对，不是 FAT、不是 tape 族、不是 Adam 实验协议。

---

## 为何不挡 A 投稿

1. **A 根主张是构造＋T1（声明域 \(K_s\)），不是「unisa.image.elf／macho 点采样已绿」。** ELF/Mach-O C↔py write 是载体／镜像周边 hygiene，不是构造 falsifier，也不是第二套理论。
2. **gate／点采样绿 ≠ Softguess／第四 seal-before／8509·8769 闭合／§8.1#3 六目标矩阵。** K6-14 自己写明 **not a loadable/codesign closure**——更不能反向读成跨架构实证封口或测量身份闭合。
3. **与仍开硬缺口及已切旁支正交。** 本拍不把 elf/macho 加为第四挡粘，不叠决策卡；对象 DISTINCT 如上。
4. **切后 A 更纯：** A 保持「构造 + T1 on declared domain \(K_s\)」；ELF/Mach-O 点采样不进摘要／seal 判据。攻击面从「须先 K6-13/14 完稿才许封 A」缩回「硬缺口仍只开 §8.1#3、8509·8769、Ο1|Ο2」。
5. **本拍零 Paper A 正文 diff。**

---

## 父节点如何回 A

- **旁支回 A：** A（字节流／构造 TSV→weights 编译器 + T1 on \(K_s\) + unisacc 实证）→ unisa.image.elf + unisa.image.macho 产品／镜像参考对 hygiene 旁支 → 仍回 A。
- 结果可回来收紧「载体／点采样如何披露」措辞，不能长成与 A 并列的第二套理论，也不能把「K6-13/14 未绿」回写成投稿前提或 Softguess 触发器。
- **DAG：** 切出的是升格闸门，不是删掉 elf/macho 工程债或 A 硬缺口。

---

## 切后 A 哪一句更硬

切出后，A 可把镜像旁支边界收成一句更硬的主张（**本拍不改正文；仅登记**）：

> **Paper A 只主张基于神经网络（构造网络与权重）的编译器与 T1（网络≡表于声明域 \(K_s\)），并以 unisacc 为 POSIX C99 子集跨架构实证；ELF／Mach-O／FAT 等 image write 点采样属于产品载体 hygiene，不是 A 的 seal-before、不是 T1、不是测量身份闭合、也不是 Softguess 触发。**

攻击面从「须先 K6-13/14 完稿才许投／封 A」缩回 **「A = 构造 + T1 + C99 实证（含仍开测量债 §8.1#3／8509·8769／Ο1|Ο2）；镜像点采样另挂产品旁支」**。

---

## tip / blob 表（本拍核对）

| 项 | 值 |
| --- | --- |
| tip before | `c3c6a6d0169ed985f542cd46c74e6cf2481bd74b` |
| tip after（本 note 合入后填） | _(merge 后由 PR 填)_ |
| Paper A 末次正文 | #154 `bdec4bcbf823d0b527a15c40afe61f69942063fc`（T1→\(K_s\)） |
| K6-13 | `45de2a759e0f4c4e8bb17f3b69e79e413b85dd92` |
| K6-14 | `c3c6a6d0169ed985f542cd46c74e6cf2481bd74b` |
| CN `research/unisacc-paper.md` | `5231dd3bf3e60804c678d48015f1e337edd9dc56` **SAME** vs #154 |
| EN `research/unisacc-paper.en.md` | `0a707d704079bfef3b2b5193bfa8dc2b5ec90303` **SAME** |
| TeX `research/arxiv-paper-a/main.tex` | `0ef6027ccc73b20b1d7d4371f89c47b069885bb7` **SAME** |
| abs `research/arxiv-paper-a/abstract.txt` | `43d7ea7e7297ed8be9a96e3950e014e359467973` **SAME** |
| Softguess | **NONE×4**（预期态；不升 seal-before） |

---

## OUT（本拍明确不做）

- 改正文 CN/EN/TeX/abstract；粘 Softguess／近似推断
- 代裁／叠卡 Ο1/Ο2；重钉 Softguess tip-refresh 或 8509·8769／§8.1#3 tip-refresh
- 改 product/kernel/weights/facts／键数／投稿方向；bump Latest
- 重做 #160 K6-12 fat、#162 Adam、#161 C、#158/#157/#155 旁支 E
- 把 K6-13 elf／K6-14 macho 加为第四 seal-before／Softguess 触发／8509·8769 闭合证明

---

## 证据索引

- 本笔记：`research/notes/paper-a-derive-purify-k6-13-14-image-elf-macho-not-a-gate-20261011-2356.md`
- 同模式先验：[`paper-a-derive-purify-k6-12-image-fat-not-a-gate-20261011-2316.md`](paper-a-derive-purify-k6-12-image-fat-not-a-gate-20261011-2316.md)（#160）；[`paper-a-derive-purify-adam-protocol-detail-not-a-gate-20261011-2337.md`](paper-a-derive-purify-adam-protocol-detail-not-a-gate-20261011-2337.md)（#162）；[`paper-a-derive-purify-k6-11-tapebin-not-a-gate-20261011-2245.md`](paper-a-derive-purify-k6-11-tapebin-not-a-gate-20261011-2245.md)（#158）
- 最近 C tip-refresh（勿重做）：[`paper-a-gap-nail-platform-matrix-tip-refresh-t1-k612-20261011-2323.md`](paper-a-gap-nail-platform-matrix-tip-refresh-t1-k612-20261011-2323.md)（#161）
- 产品证据：K6-13 `45de2a75…`；K6-14 `c3c6a6d0…`
- T1 声明域统一：#154；Softguess tip-refresh：#153
- 短报：[`paper-a-heartbeat-report-20261011-2356.md`](paper-a-heartbeat-report-20261011-2356.md)

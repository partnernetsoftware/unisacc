# Paper A 成篇定稿增量：摘要核心主张句「精确可执行形式／非学得近似」四联对齐

- **拍点：** 2026-10-12 ~22:17 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `525a5cd230320a619804e635160209b7eaa5f6d1`（短 `525a5cd2`；= #323 after Cut E 2132 tip-after；相对任务书 tip **无漂**）
- **tip after：** `2d58f27e82cf2cc59fe154b0e6ecc54132627b60`（短 `2d58f27e`；= squash Merge #324 本 A）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 $K_s$；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only **摘要核心主张句** CN+EN+TeX+abs 四联同义收紧，对齐已硬的 §1／§10「精确可执行形式／非学得近似」；**不改** §1／§10 正文（已硬）；**不改**键数身份（8509/8769/9174）、product/kernel/weights/facts、投稿方向；**不叠** Ο1（仍荐）；**不重钉** Softguess／§8.1#3／8509·8769；**不**粘 Softguess 口号字面
- **Softguess：** tip 四文件 **NONE×4**（BEFORE+AFTER 机扫 `Softguess`/`softguess`/`SoftGuess`/`权重就是`/`weights are the logic`/`逻辑本身`/`权重即逻辑`/`权重≡逻辑` 均 0 hit；**未**粘 Softguess 口号）
- **Latest：** **v0.0.41**；冻结重测身份仍 **v0.0.19**；tip version **0.0.42**（本拍不 bump）；Draft 仅 **v0.0.31**

---

## 主张一句（本页唯一）

**摘要核心主张句把「与表逐键一致 / agrees with the table key-by-key」收紧为「在声明域上给出表的精确可执行形式（与表逐键同一），不是学得的近似 / yields the table's exact executable form (identical key-by-key), not a learned approximation」——与已硬的 §1／§10 同质；堵住摘要仍可被读成「仅声称键一致、未声明可执行形式非近似」的攻击面；同一实质改动同步进 CN/EN/TeX/abs；不引入 Softguess 口号字面。**

---

## 路由（为何不是假 A / 非 E / 非 C / 非 B）

- **非假 A（Latest 披露）：** tip §8.1 已写公开 Latest=**v0.0.41**；本拍不重复披露。
- **非 C：** 相对 2016／1928 无新 gold／pin；再 tip-refresh 8509/8769＝巡检式 FAIL。
- **非 B：** 相对 1914 Ο1 **无新信息**；叠卡禁。
- **非 E：** 上拍 2132 已切 c42 acceptance≠seal；本拍有可核对 **abstract** phrasing 增量 → 优先 **A**。
- **DISTINCT vs 2054 A：** 2054 收紧 **§10 开句**且显式 **abstract SAME**；本拍兑现该遗留：只改 **摘要核心主张句**（CN/EN/TeX/abs），§10／§1 不动。
- **DISTINCT vs 0956 A：** 0956 收紧 **§1**；本拍把同一硬度对齐进 **摘要**。

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md` 摘要

- **Before：** `**核心主张**是：存在由表导出的构造权重，使整数阈值网络在表声明的定义域上与表逐键一致；层、阈值与宽度是选定的网络结构，不是从数据学出的编译行为。`
- **After：** `**核心主张**是：存在由表导出的构造权重，使整数阈值网络在表声明的定义域上给出表的精确可执行形式（与表逐键同一），不是学得的近似；层、阈值与宽度是选定的网络结构，不是从数据学出的编译行为。`

### EN `research/unisacc-paper.en.md` Abstract

- **Before：** `**Our core claim** is that there exist constructed weights, derived from the table, such that the integer threshold network agrees with the table key-by-key on the table's declared domain; layers, thresholds and width are chosen network structure, not compiler behaviour learned from data.`
- **After：** `**Our core claim** is that there exist constructed weights, derived from the table, such that on the table's declared domain the integer threshold network yields the table's exact executable form (identical key-by-key), not a learned approximation; layers, thresholds and width are chosen network structure, not compiler behaviour learned from data.`

### TeX `research/arxiv-paper-a/main.tex` abstract

- **Before：** `\textbf{Our core claim} is that there exist constructed weights, derived from the table, such that the integer threshold network agrees with the table key-by-key on the table's declared domain; layers, thresholds and width are chosen network structure, not compiler behaviour learned from data.`
- **After：** `\textbf{Our core claim} is that there exist constructed weights, derived from the table, such that on the table's declared domain the integer threshold network yields the table's exact executable form (identical key-by-key), not a learned approximation; layers, thresholds and width are chosen network structure, not compiler behaviour learned from data.`

### abs `research/arxiv-paper-a/abstract.txt`

- **Before：** `Our core claim is that there exist constructed weights, derived from the table, such that the integer threshold network agrees with the table key-by-key on the table's declared domain; layers, thresholds and width are chosen network structure, not compiler behaviour learned from data.`
- **After：** `Our core claim is that there exist constructed weights, derived from the table, such that on the table's declared domain the integer threshold network yields the table's exact executable form (identical key-by-key), not a learned approximation; layers, thresholds and width are chosen network structure, not compiler behaviour learned from data.`

---

## 四 blob 状态（本拍后）

| 文件 | 相对 tip `525a5cd2` |
| --- | --- |
| CN `research/unisacc-paper.md` | **DIFF**（仅摘要核心主张句） |
| EN `research/unisacc-paper.en.md` | **DIFF**（仅 Abstract 核心主张句） |
| TeX `research/arxiv-paper-a/main.tex` | **DIFF**（仅 abstract 核心主张句） |
| abs `research/arxiv-paper-a/abstract.txt` | **DIFF**（仅 core claim 句） |

（新 blob SHA 以 merge 后 `git`/`gh api` 为准，本页不臆造。）

Before blobs @ tip `525a5cd2`：CN `ae1efe0262f6cbbcd913029ccee89a55f7f1c9a8`；EN `99492b1726e41b57713ccea1f50e1058fe1a53e4`；TeX `40efe6d47db4c86c9a47ff7610a71f66a3880408`；abs `92a6472c92a13cb4e8149bbd798f24e6d6a5f648`。

---

## 挡粘仍开（本拍不闭合）

1. **§8.1#3** 平台矩阵（须先选定单一密封测量身份）
2. **8509/8769** 测量身份（活 gold op-87/8769/569 vs EN/TeX/abs op-74/8509/517）
3. **Ο1|Ο2** 投稿/封口重测身份（**仍荐 Ο1**；本拍**不叠**新卡；引用 1914）

---

## 登记

- paper-notes 追加本拍 A 行（见 `research/paper-notes-20261009.md` 末）
- 心跳短报：`research/notes/paper-a-heartbeat-report-20261012-2217.md`

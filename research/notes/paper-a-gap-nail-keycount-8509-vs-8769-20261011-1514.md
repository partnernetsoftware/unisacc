# Paper A · 真缺口钉：键数测量身份 8509 vs 8769（tip `7ebafcb5` 可引用一页）

- **拍点：** 2026-10-11 ~15:14 Asia/Singapore（UTC+8）
- **本拍切口：** **C. 真缺口钉**
- **仓库 tip（before / 本拍核对基）：** `7ebafcb5db83d50bd496923b5bc6d0e21e30cd58`（Merge PR #129 Darwin host-target E；Latest **v0.0.39**）
- **四 blob（git）：** CN `e2c67f1a` · EN `ca2bf002` · TeX `bd62b23a` · abstract `43d7ea7e` — 相对 ~14:22 tip-refresh / ~15:01 E 记忆锚 **SAME**（本拍**零正文 diff**）
- **Softguess：** **NONE×4**（机扫 `Softguess`／`softguess`／`权重就是`／`weights are the logic`／`逻辑本身`／`权重即逻辑` 于 CN/EN/TeX 均 0 hit；本钉不粘 Softguess）
- **角色：** 可引用一页钉（citation nail）；**supersede tip 证据 only**；**不是**新决策卡。先验：[`paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)、[`paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-20261011-1422.md`](paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-20261011-1422.md)（tip `25140540`）。Ο1/Ο2：[`paper-a-decision-card-remeasure-identity-20261011-1407.md`](paper-a-decision-card-remeasure-identity-20261011-1407.md) **不叠**（仍荐 Ο1，无新信息）。
- **父节点：** **A**

---

## 主张一句（本页唯一）

**摘要／正文里的 8,509 与表 1（中文）合计 8,769 不是同一身份的笔误或草稿漂移：它们是两套已标注并存的词表测量身份（EN/arXiv＝op-74／8509／517；CN＝op-87／8769／569）。tip `7ebafcb5` 活 gold `#field` 域积与 `built.json` H 仍只复现 8769／569；8509／517 仍无 tip frozen gold 对称 pin。单一冻结投稿测量身份未闭合 → seal-blocking 硬缺口。本钉不改正文键数、不代裁 Ο1/Ο2、不改 Softguess／§8.1。**

---

## 证据表：数字 → 源工件 → 计数定义 → Paper A 引用位点

| 数字 | 源工件（tip `7ebafcb5`） | 计数定义 | Paper A 引用（abstract vs Table 1） |
| --- | --- | --- | --- |
| **8,769** | 活表：`weights/gold/{pp…combo}.tsv` 十八阶段 `#field` 词表长度笛卡尔积之和；回执 [`../seal-remeasure-20261009.md`](../seal-remeasure-20261009.md) / [`../seal-remeasure-tip.json`](../seal-remeasure-tip.json)；本拍重算 **8769** | Table 1 **键**合计（op-87：enc/isel/abi/combo 的 `op` 长度=**87**） | **CN** 摘要 L13；表 1 合计 L141；表 1 下说明 L144；§5.1 规模行 L220。**非** EN/TeX/abstract 主合计 |
| **569** | `weights/built.json` 十八阶段 `H` 之和（本拍读出 **569**）；`built.json` SHA-256 `5e45bf5e…d665a`（44,094 B）同历史 pin | Table 1 **单元**合计（op-87 构造宽度） | **CN** 表 1 合计 L141；说明 L144；§5.1 L220 |
| **8,509** | **稿内标注**的 EN/arXiv Table 1 行加总（op-74：同四阶段 `op`=**74** → enc 444、isel 148、abi 444、combo 444）；**不是** tip gold 活求和 | 历史／并存 **op-74** 键身份 | **EN** 摘要 L13；Table 1 题注 L120；Total L142；§5.1 L218。**TeX** abstract L76；caption L262；Total L319；Size L462。**abstract.txt** L1。CN 仅在免责里对照提及 |
| **517** | EN/arXiv Table 1 Units 列行加总（稿内 op-74）；**不是** tip `built.json` live H 和 | op-74 **单元**身份 | 同 8509 位点（EN/TeX/abstract）；CN 免责对照 |
| **Δ键 260** | 机械：8769−8509；分解仅 enc/isel/abi/combo：78+26+78+78（522−444、174−148、522−444、522−444） | 两身份键差 | 正文免责暗示；详 seal-remeasure；**非**第三套键数 |
| **Δ单元 52** | 569−517；仅 isel/abi/combo：14+17+21；**enc 两侧单元皆 5** | 两身份单元差 | 同上 |
| **9,124 B** | 十八阶段权重合计字节（CN L152 / EN L150 / TeX L334） | **权重字节**，不是键数 | 与 8509/8769 **正交**；勿与 9174 混读 |
| **9,174** | 维护面／历史回忆；`wc -l weights/gold/*.tsv` 类巧合曾被提及 | **无** tip frozen gold 键域积＝9174 | **不得**升为 Table 1 键数；A2／维护面旁支 |

### 本拍 tip 活表复核（方法同 seal-remeasure）

| Check | Value @ `7ebafcb5` |
| --- | --- |
| 18-stage `#field` 域积合计 | **8769** |
| Matches CN Table 1 (op-87) | **yes** |
| Matches EN Table 1 labeled 8509 | **no**（差仅 enc/isel/abi/combo） |
| SHA-256 pin（`===<stage>\n` + bytes，Table 1 顺序） | `b8244bd8d9422dd32bb21af7053c38ef009d9d5a027c0924b1cac167fa454e70` |
| Pin vs 2026-10-09 / 1239 / 1422 | **MATCH** |
| 20-gold `===<stem>\n` sorted pin | `1ef9608742cded03d0a7c1acc74572379efeae228a8f34e15be047bf6ff581b3` **MATCH** |
| enc/isel/abi/combo `#field op` 长度 | **87 / 87 / 87 / 87**（域积 522 / 174 / 522 / 522） |
| `built.json` H 十八段和 | **569** |

→ tip 活产物仍锁 **op-87／8769／569**。EN/arXiv **8509／517** 仍为稿内 **labeled** op-74 身份。

### 正文免责位点（披露已盖；单一身份未盖）

| 文件 | 行 | 纪律要点 |
| --- | --- | --- |
| `research/unisacc-paper.md` | 13, 144, 220 | op-87 主数字 +「禁止互相替换」 |
| `research/unisacc-paper.en.md` | 13, 120, 218 | op-74 主数字 +「do not collapse until one sealed measurement」 |
| `research/arxiv-paper-a/main.tex` | 76, 262, 462 | 同 EN |
| `research/arxiv-paper-a/abstract.txt` | 1 | 同 EN |

**因此：摘要 8509 vs 中文表 1 合计 8769 的「差」= 双身份披露，不是未标注 bug；仍挡封口的是「单一冻结测量身份 + 投稿侧 pin」未闭合。**

---

## 缺口是什么／不是什么

| 是 | 不是 |
| --- | --- |
| 两套**合法标注**的测量身份并存（CN tip-gold 对齐 vs EN/arXiv 历史 op-74 表） | 同一身份下的加减笔误 |
| tip gold pin 稳定复现 8769（本拍仍 MATCH） | 正文数字在 PR #129 后漂移（四 blob SAME） |
| seal-blocking：未选投稿身份前无法「一个摘要数字＝一个 Table 1 合计」 | Softguess 类命名卫生问题（仍 NONE×4） |
| EN op-74 **缺** tip frozen gold 对称 pin | 9174 可作为第三 Table 1 键身份（无 tip gold） |

---

## 封口可选项（仅选项；**不改正文**、须政委批）

1. **Ο1（仍荐，不叠卡）：** 以 tip gold **op-87／8769／569** 为唯一投稿测量身份；政委批后三联改写 EN/TeX/abstract（及必要 CN 免责收敛），并钉 `b8244bd8…` + `built.json` `5e45bf5e…` 为投稿 pin。
2. **Ο2：** 永久双标：两侧各冻 pin（op-87 tip pin 已有；op-74 须另给具名历史 tip/artifact 可复现 pin），摘要与表题注永久对照，禁止静默互换。
3. **另开具名 tip 全量重测**后再选 1 或 2（键：`#field` 域积；单元：`unisa acc`／H；可选 `unisa docs --check`）。

本拍**禁止**预写正文键数；**禁止**把 8509↔8769 单侧改掉。

---

## 明确不改（本钉边界）

- Softguess / Softguess 字面：**不改**（仍 NONE×4）
- Ο1/Ο2：**不叠**第二决策卡；仍荐 Ο1，无新信息
- §8.1 同身份平台矩阵：**不降级、不代裁**（正交硬缺口）
- Paper A CN/EN/TeX/abstract **任何键数／单元单元格**
- kernel / weights / facts / 产品代码
- 9174 升格为 Table 1 键数

---

## Softguess 行

**Softguess：NONE×4**（本拍机证；与测量身份硬缺口不同类）

---

## 证据指针

- tip：`7ebafcb5db83d50bd496923b5bc6d0e21e30cd58`
- 方法：[`../seal-remeasure-20261009.md`](../seal-remeasure-20261009.md)、[`../seal-remeasure-tip.json`](../seal-remeasure-tip.json)
- 先验钉：[`paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)
- tip-refresh（`25140540`）：[`paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-20261011-1422.md`](paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-20261011-1422.md)
- Ο1/Ο2（不叠）：[`paper-a-decision-card-remeasure-identity-20261011-1407.md`](paper-a-decision-card-remeasure-identity-20261011-1407.md)
- Latest：**v0.0.39**

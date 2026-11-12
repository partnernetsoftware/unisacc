# Paper A · 真缺口钉 tip-refresh：测量身份 8509 vs 8769（Draft v0.0.40 tip 活表复核）

- **拍点：** 2026-10-11 ~16:41 Asia/Shanghai（UTC+8；论文心跳）
- **本拍切口：** **C. 真缺口钉**
- **主张一句：** tip `b12901c7`（Draft v0.0.40 已开窗；Latest 仍 v0.0.39）活 gold `#field` 域积与 `built.json` H **仍只锁 op-87／8769／569**；EN/arXiv 8509／517 仍为稿内 labeled op-74；**单一冻结投稿测量身份未闭合 → seal-blocking**。本拍只 supersede tip 证据；**不代裁 Ο1/Ο2（仍荐 Ο1）、不改正文键数、不叠决策卡**。
- **仓库 tip（本拍核对）：** `b12901c749475fa53df3e5cbdffd4dc53bf432fc`（短 `b12901c7`；相对上一拍 E 合入后 tip `2696229d` 前进到 c40 Draft open；**Paper A 四 blob 未漂**）
- **四 blob（git）：** CN `e2c67f1a0df02a1d5ecd548aa957fa555aaf955b` · EN `ca2bf002cc6d10ae2695c58b78fefb5ced7ab848` · TeX `bd62b23a058b7572a75e8e20264402d84512993d` · abstract `43d7ea7e7297ed8be9a96e3950e014e359467973` — 相对 1514／1422／1628 **SAME**
- **Softguess / 权重就是 / weights are the logic / 逻辑本身 / 权重即逻辑：** 仍 **NONE×4**（本钉不粘 Softguess）
- **角色：** 证据刷新（supersede tip 证据 only）；**不是**新决策卡。先验：[`paper-a-gap-nail-keycount-8509-vs-8769-20261011-1514.md`](paper-a-gap-nail-keycount-8509-vs-8769-20261011-1514.md)、[`paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-20261011-1422.md`](paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-20261011-1422.md)、[`paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)。Ο1/Ο2：[`paper-a-decision-card-remeasure-identity-20261011-1407.md`](paper-a-decision-card-remeasure-identity-20261011-1407.md) **不叠**（无新信息：Draft 开窗未改 gold／built／正文键数）。
- **父节点：** **A**
- **为何本拍选 C（非再叠 E）：** Draft／发布验收证据包（1558）、Darwin flaky（1611）、管道 efficiency／折叠（1628）已切出。tip 新事件是 **Draft v0.0.40 开窗**；须核对产品 tip 前进是否漂了测量身份硬缺口。本拍重算证明：**未漂、仍挡粘**。

---

## 本拍重测（方法同 [`../seal-remeasure-20261009.md`](../seal-remeasure-20261009.md)）

对 tip `b12901c7` 的 `weights/gold/{pp…combo}.tsv`（Table 1 十八阶段顺序：pp, lex, parse, type, scope, irsel, enc, reloc, regmap, tyinfo, pfconv, peep, opinfo, prec, binsel, isel, abi, combo）：

| Check | Value @ `b12901c7` |
| --- | --- |
| 18-stage `#field` 域积合计 | **8769** |
| Matches CN Table 1 (op-87) | **yes** |
| Matches EN Table 1 labeled 8509 | **no**（差仅 enc/isel/abi/combo） |
| SHA-256 pin（`===<stage>\n` + bytes，Table 1 顺序） | `b8244bd8d9422dd32bb21af7053c38ef009d9d5a027c0924b1cac167fa454e70` |
| Pin vs 2026-10-09 / 1239 / 1422 / 1514 | **MATCH** |
| 20-gold `===<stem>\n` sorted pin | `1ef9608742cded03d0a7c1acc74572379efeae228a8f34e15be047bf6ff581b3` **MATCH** |
| enc/isel/abi/combo `#field op` 长度 | **87 / 87 / 87 / 87**（域积 522 / 174 / 522 / 522） |
| `weights/built.json` SHA-256 | `5e45bf5e018c9e1f0417fdbeb17cbf3b87463c761542f676686a7c5fc03d665a`（44,094 B）**MATCH** |
| 18-stage `H` 行加总 | **569**（enc=5, isel=88, abi=69, combo=121 …）对齐 CN；**不是** EN 517 |

→ Draft v0.0.40 产品 tip **未改** 活 gold／built 测量身份。活产物仍锁 **op-87／8769／569**。EN/arXiv **8509／517** 仍为稿内 **labeled** op-74。

### 机械差（与先验钉一致；本拍未改）

- 键差 **8769−8509 = 260**，仅 **enc / isel / abi / combo**（`op` 87 vs 74；分解 78+26+78+78）
- 单元差 **569−517 = 52**，仅 **isel / abi / combo**（`enc` 两侧单元同为 5）

---

## 仍挡封口（still-blocking）

1. **单一冻结投稿测量身份尚未选定**（Ο1/Ο2 待政委批；默认仍荐 Ο1＝以 tip gold op-87／8769／569 为唯一投稿身份并三联改写；本拍不代裁、不叠卡）。
2. **EN op-74 无 tip gold pin**：`b8244bd8…` 钉的是 tip gold 的 **op-87 活表**；op-74 没有对称 frozen gold 复现。
3. **CN/EN/TeX/abstract 三联数字在选定身份前不得互换或单侧改写**（正文免责已在；本拍零正文 diff）。
4. 正交仍开：§8.1#3 同身份平台执行矩阵（另钉 1525）；本钉只钉键数双身份。

**Draft v0.0.40 开窗本身 ≠ 测量身份闭合**（正交 E 1558：Draft／发布验收证据包不得升成 A 理论闸门；本 C 钉证明身份债在 Draft 开窗后仍开）。

---

## OUT OF SCOPE（本钉明确不做）

- 改正文 CN/EN/TeX/abstract 任何键数/单元；互换 8509↔8769
- 发明 op-74 tip gold / 把 9174 升格为 Table 1 键数
- Softguess 字面粘贴；叠第二张 Ο1/Ο2 决策卡
- 改 kernel / weights / facts / 产品代码；代裁 Draft→Latest／seal/.com
- 重写 1239／1422／1514 钉正文（本页只 supersede tip 证据）
- 重做 1558／1611／1628 衍生净化

---

## 证据与机核

- tip：`b12901c749475fa53df3e5cbdffd4dc53bf432fc`（Draft v0.0.40 open；Latest **v0.0.39**）
- Paper A blobs：`e2c67f1a…` / `ca2bf002…` / `bd62b23a…` / `43d7ea7e…` **SAME**
- Softguess：**NONE×4**
- 18-stage domain sum：**8769**；pin：**MATCH** `b8244bd8…fa454e70`
- built.json H：**569**；pin：**MATCH** `5e45bf5e…d665a`
- 方法：[`../seal-remeasure-20261009.md`](../seal-remeasure-20261009.md)
- 先验钉：1514／1422／1239；Ο1/Ο2 卡 1407（不叠）
- 正交 E：[`paper-a-derive-purify-draft-release-acceptance-not-a-gate-20261011-1558.md`](paper-a-derive-purify-draft-release-acceptance-not-a-gate-20261011-1558.md)
- 父节点：**A**

# Paper A · 真缺口钉 tip-refresh：测量身份 8509 vs 8769 · Draft v0.0.40 seal／assets receipt 之后仍 blocking

- **拍点：** 2026-10-11 ~17:45 Asia/Singapore（UTC+8）
- **本拍切口：** **C. 真缺口钉**
- **主张一句：** tip `16226191`（含 Draft v0.0.40 seal candidate／assets receipt／PR #137 E／PR #138 §8.1#3 C）活 gold `#field` 域积与 `built.json` H **仍只锁 op-87／8769／569**；EN/arXiv 8509／517 仍为稿内 labeled op-74；**Draft seal／GHCR／assets 是产品发布卫生，不是测量身份闭合 → 仍 seal-blocking**。本拍只 supersede tip 证据；**不代裁 Ο1/Ο2（仍荐 Ο1）、不改正文键数、不叠决策卡**。
- **仓库 tip（本拍核对）：** `162261918d9e315c2048207f78bebd8efedf318e`（短 `16226191`；Merge PR #138；相对 1641 tip `b12901c7` 已并入 seal candidate／open-draft tip-advance／assets receipt／E 1715／C 1724）
- **四 blob（git @ tip）：** CN `e2c67f1a0df02a1d5ecd548aa957fa555aaf955b` · EN `ca2bf002cc6d10ae2695c58b78fefb5ced7ab848` · TeX `bd62b23a058b7572a75e8e20264402d84512993d` · abstract `43d7ea7e7297ed8be9a96e3950e014e359467973` — 相对 1641／1724／1715 **SAME**（本拍**零正文 diff**）
- **Softguess：** **NONE×4**（机扫 `Softguess`／`softguess`／`权重就是`／`weights are the logic`／`逻辑本身`／`权重即逻辑`／`Softmax` 于 CN/EN/TeX/abstract 均 0 hit；本钉不粘 Softguess）
- **角色：** 可引用 tip-refresh 钉（citation nail）；**supersede tip 证据 only**；相对 1239／1422／1514／1641 **升格页仍有效**；相对 1641 **并入 Draft seal 后 NEW tip 事实**。**不是**新决策卡；**不叠** Ο1/Ο2。
- **父节点：** **A**
- **DISTINCT：** vs [`paper-a-gap-nail-platform-matrix-tip-refresh-draft040-seal-20261011-1724.md`](paper-a-gap-nail-platform-matrix-tip-refresh-draft040-seal-20261011-1724.md)（**C**：§8.1#3 矩阵）——正交硬缺口；本钉只钉键数双身份。vs [`paper-a-derive-purify-draft-040-seal-candidate-receipt-not-a-gate-20261011-1715.md`](paper-a-derive-purify-draft-040-seal-candidate-receipt-not-a-gate-20261011-1715.md)（**E**：seal ≠ A 闸门）——本钉是 **C**：钉 8509/8769 在 seal 证据落地后 **仍** seal-blocking。vs [`paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-20261011-1641.md`](paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-20261011-1641.md)（**C**：Draft **开窗** tip `b12901c7`）——本页是 **seal／assets receipt 之后** tip `16226191`。

---

## 证据表：NEW tip 事实（相对 1641 Draft-open tip-refresh）→ 对 8509/8769 的含义（do / don't）

| NEW tip 事实（verified @ `16226191`；`gh release list` + tip commits） | 对测量身份 **意味着** | **不意味着** |
| --- | --- | --- |
| `1db30835`：`0.0.40: seal candidate (Draft assets; cand 50ab4c21)` | Draft seal candidate 存在（产品卫生指针） | seal digest＝已选定 8509 或 8769 为唯一投稿测量身份 |
| `729f16a0`：open-draft gap#5 tip advance（policy+guard, not paper-a） | policy+guard tip 前进；自标 not paper-a | policy+guard＝键数双身份闭合 |
| `7bf7ac34`：seal+Draft assets receipt 1645（not Latest） | Draft assets 收据入仓；明文 not Latest | assets receipt＝EN 8509 与 tip gold 8769 已统一 |
| Merge #137 `1ba56c54` + E 1715：seal receipt ≠ A gate | 已登记：Draft seal／receipt **不得**升成 A 理论／投稿闸门 | 本拍可借该 E 把 8509/8769 从 seal-blocking 降级 |
| Merge #138 `16226191` + C 1724：§8.1#3 仍 blocked-on-identity | 正交矩阵硬缺口仍开（仅引用） | 矩阵钉＝键数身份已选；本钉可代裁 Ο1/Ο2 |
| GitHub **Latest** 仍 **v0.0.39**；**Draft** **v0.0.40**（`gh release list`） | Draft 已 seal／开窗；Latest 未再漂 | Draft seal＝投稿测量身份已冻结为 8769 或 8509 |
| tip `16226191` 四 Paper A blob **SAME** as 1641 | tip 产品／研究前进可引用；正文键数未动 | tip 前进＝双身份债消失 |

**结论：** tip 上 Draft seal candidate／assets receipt／open-draft tip-advance／§8.1#3 C 是相对 1641 的**可引用新产品发布卫生＋正交缺口证据**；它**加强**「产品 Draft 卫生 ≠ 测量身份闭合」分界，**不**把 8509/8769 从 seal-blocking 降级。

---

## 本拍重测（方法同 [`../seal-remeasure-20261009.md`](../seal-remeasure-20261009.md)）

对 tip `16226191` 的 `weights/gold/{pp…combo}.tsv`（Table 1 十八阶段顺序：pp, lex, parse, type, scope, irsel, enc, reloc, regmap, tyinfo, pfconv, peep, opinfo, prec, binsel, isel, abi, combo）：

| Check | Value @ `16226191` |
| --- | --- |
| 18-stage `#field` 域积合计 | **8769** |
| Matches CN Table 1 (op-87) | **yes** |
| Matches EN Table 1 labeled 8509 | **no**（差仅 enc/isel/abi/combo） |
| SHA-256 pin（`===<stage>\n` + bytes，Table 1 顺序） | `b8244bd8d9422dd32bb21af7053c38ef009d9d5a027c0924b1cac167fa454e70` |
| Pin vs 2026-10-09 / 1239 / 1422 / 1514 / 1641 | **MATCH** |
| 20-gold `===<stem>\n` sorted pin | `1ef9608742cded03d0a7c1acc74572379efeae228a8f34e15be047bf6ff581b3` **MATCH** |
| enc/isel/abi/combo `#field op` 长度 | **87 / 87 / 87 / 87**（域积 522 / 174 / 522 / 522） |
| `weights/built.json` SHA-256 | `5e45bf5e018c9e1f0417fdbeb17cbf3b87463c761542f676686a7c5fc03d665a`（44,094 B）**MATCH** |
| 18-stage `H` 行加总 | **569**（enc=5, isel=88, abi=69, combo=121 …）对齐 CN；**不是** EN 517 |

→ Draft v0.0.40 **seal／assets receipt 之后** tip **仍未改** 活 gold／built 测量身份。活产物仍锁 **op-87／8769／569**。EN/arXiv **8509／517** 仍为稿内 **labeled** op-74。

### 机械差（与先验钉一致；本拍未改）

- 键差 **8769−8509 = 260**，仅 **enc / isel / abi / combo**（`op` 87 vs 74；分解 78+26+78+78）
- 单元差 **569−517 = 52**，仅 **isel / abi / combo**（`enc` 两侧单元同为 5）

---

## 仍挡封口（still-blocking · IN SCOPE）

1. **单一冻结投稿测量身份尚未选定**（Ο1/Ο2 待政委批；默认仍荐 Ο1＝以 tip gold op-87／8769／569 为唯一投稿身份并三联改写；本拍不代裁、不叠卡——Draft seal／assets **无新测量身份信息** beyond 引用 1641／1715／1724）。
2. **EN op-74 无 tip gold pin**：`b8244bd8…` 钉的是 tip gold 的 **op-87 活表**；op-74 没有对称 frozen gold 复现。
3. **CN/EN/TeX/abstract 三联数字在选定身份前不得互换或单侧改写**（正文免责已在；本拍零正文 diff）。
4. **禁止**把 Draft seal／GHCR candidate／assets receipt／§8.1#3 矩阵钉写成 8509/8769 已闭合。
5. 正交仍开（仅引用）：**§8.1#3** 同身份平台执行矩阵（[`paper-a-gap-nail-platform-matrix-tip-refresh-draft040-seal-20261011-1724.md`](paper-a-gap-nail-platform-matrix-tip-refresh-draft040-seal-20261011-1724.md)）。

### OUT OF SCOPE

- 改正文 CN/EN/TeX/abstract 任何键数/单元；互换 8509↔8769
- 发明 op-74 tip gold / 把 9174 升格为 Table 1 键数
- Softguess 字面粘贴；叠第二张 Ο1/Ο2 决策卡
- 改 kernel / weights / facts / 产品代码；代裁 Draft→Latest／seal/.com
- 重写 1239／1422／1514／1641 钉正文（本页只 supersede tip 证据）
- 重做 1715 E／1724 C 平台钉；把 Draft seal 升成理论闸门

---

## Softguess 行

**Softguess：NONE×4**（本拍机证；与测量身份硬缺口不同类）

---

## 证据指针

- tip：`162261918d9e315c2048207f78bebd8efedf318e`（Draft **v0.0.40** seal 链后；Latest **v0.0.39**）
- Paper A blobs：`e2c67f1a…` / `ca2bf002…` / `bd62b23a…` / `43d7ea7e…`（本钉 **SAME**）
- Softguess：**NONE×4**
- 18-stage domain sum：**8769**；pin：**MATCH** `b8244bd8…fa454e70`
- built.json H：**569**；pin：**MATCH** `5e45bf5e…d665a`
- 方法：[`../seal-remeasure-20261009.md`](../seal-remeasure-20261009.md)
- NEW tip：`1db30835`／`729f16a0`／`7bf7ac34`／`1ba56c54`／`16226191`
- 先验钉：1641／1514／1422／1239；Ο1/Ο2 卡 1407（不叠；仍荐 Ο1）
- 正交：1724 §8.1#3；1715／1558 E Draft seal ≠ A gate
- 父节点：**A**

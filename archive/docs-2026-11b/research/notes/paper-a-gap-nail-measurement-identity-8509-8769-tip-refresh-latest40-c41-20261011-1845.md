# Paper A · 真缺口钉 tip-refresh：测量身份 8509 vs 8769 · §8.1 Latest→40 + c41 K6-1 之后仍 blocking

- **拍点：** 2026-10-11 ~18:45 Asia/Singapore（UTC+8）
- **本拍切口：** **C. 真缺口钉**
- **主张一句：** 在 §8.1 Latest 披露已至 **v0.0.40** 且 tip 已含 c41 K6-1 unisa.bits 产品研究之后，活 gold `#field` 域积与 `built.json` H **仍只锁 op-87／8769／569**；EN/arXiv 8509／517 仍为稿内 labeled op-74（无 tip frozen-gold pin）；**产品 tip 前进与 §8.1 披露刷新 ≠ 测量身份闭合 → 仍 seal-blocking**。本拍只 supersede tip 证据；**不代裁 Ο1/Ο2（仍荐 Ο1）、不改正文键数、不叠决策卡**。
- **仓库 tip（本拍核对）：** `fa3f30829753078be9758d9ef074f5c2bad70e7b`（短 `fa3f3082`；与派单基线一致；相对 1745 tip `16226191` 已并入 Latest 切 40／§8.1#142+#143／c41 K6-1 bits）
- **公开 Latest：** **v0.0.40**（unsigned；`gh api …/releases/latest`）；Paper A 重测冻结身份仍 **v0.0.19**（本拍不改该主张）
- **四 blob（git @ tip `fa3f3082`）：**
  - CN `c37b9291a6cc5fcd24beda00299fdf630da0c02c` — vs 1745 `e2c67f1a…`：**CHANGED**（§8.1 Latest 39→40；PR #142）
  - EN `fad7b3e0b51731f3615de9753aea0144dbee79b2` — vs 1745 `ca2bf002…`：**CHANGED**（同）
  - TeX `5c880b0bafd86cbd10691a149212048a7b7d7279` — vs 1745 `bd62b23a…`：**CHANGED**（同）
  - abstract `43d7ea7e7297ed8be9a96e3950e014e359467973` — vs 1745：**SAME**
- **Softguess：** **NONE×4**（机扫 `Softguess`／`softguess`／`权重就是`／`weights are the logic`／`逻辑本身`／`权重即逻辑`／`Softmax` 于 CN/EN/TeX/abstract 均 0 hit；本钉不粘 Softguess）
- **角色：** 可引用 tip-refresh 钉（citation nail）；**supersede tip 证据 only**；相对 1239／1422／1514／1641／1745 **升格页仍有效**；相对 1745／1808／1831 **并入 Latest-40 披露 + c41 之后 NEW tip 事实**。**不是**新决策卡；**不叠** Ο1/Ο2。
- **父节点：** **A**
- **DISTINCT：**
  - vs [`paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-draft040-seal-20261011-1745.md`](paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-draft040-seal-20261011-1745.md)（**C**：Draft seal 时代；tip `16226191`；Latest 仍 39）——本页是 **Latest=40 + §8.1 披露已改 + c41** tip `fa3f3082`。
  - vs [`paper-a-gap-nail-platform-matrix-tip-refresh-draft040-seal-20261011-1724.md`](paper-a-gap-nail-platform-matrix-tip-refresh-draft040-seal-20261011-1724.md)（**C**：§8.1#3 矩阵）——正交硬缺口；本钉只钉键数双身份。
  - vs [`paper-a-derive-purify-draft-040-seal-candidate-receipt-not-a-gate-20261011-1715.md`](paper-a-derive-purify-draft-040-seal-candidate-receipt-not-a-gate-20261011-1715.md)／[`paper-a-derive-purify-darwin-ctape-compare-acceptance10-not-a-gate-20261011-1759.md`](paper-a-derive-purify-darwin-ctape-compare-acceptance10-not-a-gate-20261011-1759.md)（**E** 衍生净化）——本钉是 **C**：钉 8509/8769 在披露刷新与产品研究落地后 **仍** seal-blocking。
  - vs [`paper-a-decision-card-remeasure-identity-20261011-1808.md`](paper-a-decision-card-remeasure-identity-20261011-1808.md)（**B** 卡；Ο2→v0.0.40；仍荐 Ο1）——**不叠**第二张决策卡；本页只刷新 tip 证据并引用该卡推荐。

---

## 证据表：NEW tip 事实（相对 1745／1808／1831）→ 对 8509/8769 的含义（do / don't）

| NEW tip 事实（verified @ `fa3f3082`；`gh` + `git log`） | 对测量身份 **意味着** | **不意味着** |
| --- | --- | --- |
| Merge #142 `ab0d1192` + `a52a53ca`：§8.1 Latest 披露 v0.0.39→**v0.0.40**（CN/EN/TeX） | 正文领句与公开 Latest 对齐；冻结仍 v0.0.19 | Latest 披露刷新＝已选定 8509 或 8769 为唯一投稿测量身份 |
| Merge #143 `524bc114` + `f80d1393`：§8.1 Latest-40 seal note tip-fill | 1831 钉 tip-after 回填；研究卫生 | tip-fill＝键数双身份闭合 |
| `07bdabfc`：publish-unsigned receipt — Latest 切 v0.0.40 | 公开 Latest 指针已至 40（产品发布） | Latest 切＝EN 8509 与 tip gold 8769 已统一 |
| `a4a631f7`：plans(v0.0.41)+research(c41) K6-1 unisa.bits C ref pair | c41／bits C 参考对入仓（产品／研究卫生） | bits C ref＝测量身份选择或 Table 1 键数改写 |
| `16f6e0ee`：research(c41) K6-1 bits rebase reading（gate unisa-bits rc0） | bits 门禁绿；rebase 阅读记录 | bits gate＝op-74 gold pin 已造出 |
| `fa3f3082`：research(c41)+tests(unisabitscheck) 1841 first-red stop | unisabitscheck 测试／看门狗；产品研究前进 | c41 测试＝8509/8769 seal-blocking 降级 |
| tip 四 blob：CN/EN/TeX **CHANGED** vs 1745；abstract **SAME** | §8.1 披露漂移已入正文；键数／摘要数字未动 | 正文 blob 漂＝双身份债消失 |
| Softguess **NONE×4** @ tip | 无 Softguess 字面污染 | Softguess 扫＝身份已选 |

**结论：** §8.1 Latest→40 披露与 c41 K6-1 unisa.bits 产品研究是相对 1745 的**可引用新产品／披露卫生证据**；它们**加强**「披露刷新／产品 tip ≠ 测量身份闭合」分界，**不**把 8509/8769 从 seal-blocking 降级。

---

## 本拍重测（方法同 [`../seal-remeasure-20261009.md`](../seal-remeasure-20261009.md)）

对 tip `fa3f3082` 的 `weights/gold/{pp…combo}.tsv`（Table 1 十八阶段顺序：pp, lex, parse, type, scope, irsel, enc, reloc, regmap, tyinfo, pfconv, peep, opinfo, prec, binsel, isel, abi, combo）：

| Check | Value @ `fa3f3082` |
| --- | --- |
| 18-stage `#field` 域积合计 | **8769** |
| Matches CN Table 1 (op-87) | **yes** |
| Matches EN Table 1 labeled 8509 | **no**（差仅 enc/isel/abi/combo） |
| SHA-256 pin（`===<stage>\n` + bytes，Table 1 顺序） | `b8244bd8d9422dd32bb21af7053c38ef009d9d5a027c0924b1cac167fa454e70` |
| Pin vs 2026-10-09 / 1239 / 1422 / 1514 / 1641 / 1745 | **MATCH** |
| 20-gold `===<stem>\n` sorted pin | `1ef9608742cded03d0a7c1acc74572379efeae228a8f34e15be047bf6ff581b3` **MATCH** |
| enc/isel/abi/combo `#field op` 长度 | **87 / 87 / 87 / 87**（域积 522 / 174 / 522 / 522） |
| `weights/built.json` SHA-256 | `5e45bf5e018c9e1f0417fdbeb17cbf3b87463c761542f676686a7c5fc03d665a`（44,094 B）**MATCH** |
| 18-stage `H` 行加总（`python3 -m unisa acc`） | **569**（enc=5, isel=88, abi=69, combo=121 …）对齐 CN；**不是** EN 517 |

→ §8.1 Latest 披露 **v0.0.40** 与 c41 K6-1 **之后** tip **仍未改** 活 gold／built 测量身份。活产物仍锁 **op-87／8769／569**。EN/arXiv **8509／517** 仍为稿内 **labeled** op-74。

### 机械差（与先验钉一致；本拍未改）

- 键差 **8769−8509 = 260**，仅 **enc / isel / abi / combo**（`op` 87 vs 74；分解 78+26+78+78）
- 单元差 **569−517 = 52**，仅 **isel / abi / combo**（`enc` 两侧单元同为 5）

---

## 仍挡封口（still-blocking · IN SCOPE）

1. **单一冻结投稿测量身份尚未选定**（Ο1/Ο2 待政委批；默认仍荐 Ο1＝以 tip gold op-87／8769／569 为唯一投稿身份并三联改写；本拍不代裁、不叠卡——引用 [`paper-a-decision-card-remeasure-identity-20261011-1808.md`](paper-a-decision-card-remeasure-identity-20261011-1808.md)；§8.1 Latest→40／c41 **无新测量身份信息** beyond 引用 1745／1808／1831）。
2. **EN op-74 无 tip gold pin**：`b8244bd8…` 钉的是 tip gold 的 **op-87 活表**；op-74 没有对称 frozen gold 复现。
3. **CN/EN/TeX/abstract 三联数字在选定身份前不得互换或单侧改写**（正文免责已在；本拍**零正文键数 diff**；仅索引链入本钉）。
4. **禁止**把 §8.1 Latest 披露刷新／c41 K6-1 bits／unisabitscheck／Draft→Latest 切写成 8509/8769 已闭合。
5. 正交仍开（仅引用）：**§8.1#3** 同身份平台执行矩阵（[`paper-a-gap-nail-platform-matrix-tip-refresh-draft040-seal-20261011-1724.md`](paper-a-gap-nail-platform-matrix-tip-refresh-draft040-seal-20261011-1724.md)）。

### OUT OF SCOPE

- 改正文 CN/EN/TeX/abstract 任何键数/单元；互换 8509↔8769
- 发明 op-74 tip gold / 把 9174 升格为 Table 1 键数
- Softguess 字面粘贴；叠第二张 Ο1/Ο2 决策卡
- 改 kernel / weights / facts / 产品代码；代裁密封 Latest／签名
- 重写 1239／1422／1514／1641／1745／1808／1831 钉正文（本页只 supersede tip 证据）
- 重做 1724 §8.1#3／1715／1759 E；把 c41／bits 升成理论闸门

---

## Softguess 行

**Softguess：NONE×4**（本拍机证；与测量身份硬缺口不同类）

---

## 证据指针

- tip：`fa3f30829753078be9758d9ef074f5c2bad70e7b`（Latest **v0.0.40**；冻结重测仍 **v0.0.19**）
- Paper A blobs：`c37b9291…` / `fad7b3e0…` / `5c880b0b…` / `43d7ea7e…`（CN/EN/TeX **CHANGED** vs 1745；abstract **SAME**）
- Softguess：**NONE×4**
- 18-stage domain sum：**8769**；pin：**MATCH** `b8244bd8…fa454e70`
- built.json H：**569**；pin：**MATCH** `5e45bf5e…d665a`
- 方法：[`../seal-remeasure-20261009.md`](../seal-remeasure-20261009.md)
- NEW tip：Merge #142/#143；`a4a631f7`／`16f6e0ee`／`fa3f3082`（c41 K6-1）；`07bdabfc` Latest切40
- 先验钉：1745／1641／1514／1422／1239；Ο1/Ο2 卡 1808（不叠；仍荐 Ο1）；§8.1 披露 1831
- 正交：1724 §8.1#3；1715／1759 E
- 父节点：**A**

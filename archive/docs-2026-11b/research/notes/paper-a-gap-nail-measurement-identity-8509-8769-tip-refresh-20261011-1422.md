# Paper A · 真缺口钉 tip-refresh：测量身份 8509 vs 8769（活表 pin 复核）

- **拍点：** 2026-10-11 ~14:22 Asia/Shanghai（论文心跳）
- **本拍切口：** **C. 真缺口钉**
- **主张一句：** 双身份（8509/op-74 vs 8769/op-87）仍是 seal-blocking 硬缺口；本拍只复核 origin/main tip 活表 `#field` 域积与 18-stage pin，**不代裁 Ο1/Ο2、不改正文键数**。
- **仓库 tip（本拍核对）：** `25140540988b0922cd03482eda8150d82403a949`（短 `25140540`；Latest 仍 **v0.0.39**）
- **四 blob（相对记忆锚 SAME）：** CN `e2c67f1a` · EN `ca2bf002` · TeX `bd62b23a` · abstract `43d7ea7e`
- **Softguess / 权重就是 / weights are the logic / 逻辑本身 / 权重即逻辑：** 仍 **NONE×4**（本钉不粘 Softguess）
- **角色：** 证据刷新（supersede tip 证据 only）；**不是**新决策卡。先验钉 [`paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)（tip `158ae864`）；Ο1/Ο2 卡 [`paper-a-decision-card-remeasure-identity-20261011-1407.md`](paper-a-decision-card-remeasure-identity-20261011-1407.md)（~14:07）**不叠第二板**。

---

## 本拍重测（方法同 `seal-remeasure-20261009.md`）

对 tip `25140540` 的 `weights/gold/{pp…combo}.tsv`（Table 1 十八阶段顺序：pp, lex, parse, type, scope, irsel, enc, reloc, regmap, tyinfo, pfconv, peep, opinfo, prec, binsel, isel, abi, combo）：

| Check | Value |
| --- | --- |
| 18-stage `#field` 域积合计 | **8769** |
| Matches CN Table 1 (op-87) | **yes** |
| Matches EN Table 1 (op-74) labeled 8509 | **no**（差仅 enc/isel/abi/combo） |
| SHA-256 pin（`===<stage>\n` + bytes，Table 1 顺序） | `b8244bd8d9422dd32bb21af7053c38ef009d9d5a027c0924b1cac167fa454e70` |
| Pin vs 历史（2026-10-09 / 1239 钉） | **MATCH** |

### 四阶段 `op` 词表（活 gold）

| stage | `#field op` 长度 | domain product |
| --- | --- | --- |
| enc | **87** | 522 (=87×3×2) |
| isel | **87** | 174 (=87×2) |
| abi | **87** | 522 |
| combo | **87** | 522 |

→ tip 活表仍对齐 **op-87 / 8769**（CN 身份）。EN/arXiv **8509** 仍为稿内标注的 **op-74** 身份行加总，**不是**本 tip gold 的 live 求和。

### 机械差（与先验钉一致；本拍未改）

- 键差 **8769−8509 = 260**，仅 **enc / isel / abi / combo**（`op` 87 vs 74；分解 78+26+78+78 见 `seal-remeasure-20261009.md`）
- 单元差 **569−517 = 52**，仅 **isel / abi / combo**（`enc` 两侧单元同为 5；见先验钉 + seal-remeasure；本拍未重跑 `unisa acc`）

### `weights/built.json`（廉价只读）

- tip size **44,094 B**；SHA-256 `5e45bf5e018c9e1f0417fdbeb17cbf3b87463c761542f676686a7c5fc03d665a`（与 `seal-remeasure-20261009.md` 历史 pin **SAME**）
- 18-stage `H` 行加总 **569**（enc=5, isel=88, abi=69, combo=121 …）；对齐 CN 单元身份；**不是** EN 517
- 活 gold 的 `op` 长度以 TSV `#field` 为准（上表 **87**）；**未**从 built.json 发明第二套词表长度
- 本拍**不**把 built.json 升成投稿身份裁决

---

## 仍挡封口（still-blocking）

1. **单一冻结投稿测量身份尚未选定**（Ο1/Ο2 待政委批；默认建议仍见 1407 卡 Ο1；本拍不代裁）。
2. **EN op-74 无 tip gold pin**：现有 `b8244bd8…` 钉的是 tip gold 的 **op-87 活表**；op-74 没有对称的 tip frozen gold 复现。
3. **CN/EN/TeX/abstract 三联数字在选定身份前不得互换或单侧改写**（正文免责已在；本拍不改正文）。
4. 正交仍开：§8.1 同身份平台执行矩阵（另钉）；本钉只钉键数双身份。

---

## OUT OF SCOPE（本钉明确不做）

- 改正文 CN/EN/TeX/abstract 任何键数/单元单元格；互换 8509↔8769
- 发明 op-74 tip gold / 把 9174 升格为 Table 1 键数
- Softguess 字面粘贴；叠第二张 Ο1/Ο2 决策卡
- 改 kernel / weights / facts / 产品代码；arXiv 投稿
- 重写 1239 钉正文（本页只 supersede tip 证据）

---

## 证据与机核

- tip before/work base：`25140540988b0922cd03482eda8150d82403a949`
- Paper A blobs：`e2c67f1a` / `ca2bf002` / `bd62b23a` / `43d7ea7e`
- Softguess：**NONE×4**
- 18-stage domain sum：**8769**；pin：**MATCH** `b8244bd8…fa454e70`
- 方法：[`../seal-remeasure-20261009.md`](../seal-remeasure-20261009.md)
- 先验钉：[`paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)
- Ο1/Ο2 卡（不叠）：[`paper-a-decision-card-remeasure-identity-20261011-1407.md`](paper-a-decision-card-remeasure-identity-20261011-1407.md)
- Latest tag：**v0.0.39**
- 父节点：**A**

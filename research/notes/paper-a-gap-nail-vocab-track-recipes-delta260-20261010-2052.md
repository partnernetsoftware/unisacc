# Paper A · 真缺口钉：词表双身份 = 两套 track-facing 冻结配方（R87 / R74）+ Δ=260 阶段归因

- **拍点：** 2026-10-10 ~20:52 Asia/Shanghai（论文心跳）
- **本拍切口：** **C. 真缺口钉**
- **仓库 tip（只读核对 / before）：** `e1fb5dbefa5943a967643cf54621c8063be06d91`（= origin/main 于本拍开拍；相对 ~20:45 tip `88846e8b` 之后 **6** 个产品 test 提交，均不碰 Paper A / gold）
- **四 blob：** CN `74893e3d` · EN `95657738` · TeX `3830ede1` · abstract `43d7ea7e` — **全部 SAME**（相对 ~20:45 / §8.1 后；本钉**零** Paper A 正文 diff）
- **Softguess：** 仍 **NONE×4**（本钉不粘 Softguess；不重写 Softguess 封口边界钉）
- **状态：** 8509/8769 是两套**具名、track-facing**的冻结测量配方，不是「摘要 vs 表 1」横切裂口；Δ=260 可归因到四阶段 `op` 词表差；**不**代裁投稿身份；**不**叠 ~17:08 Ο1/Ο2 卡；父节点仍是 A

---

## 主张一句（本页唯一）

**在 tip `e1fb5dbe` 上，8769 与 8509 仍是两套并存的冻结测量配方——R87（CN 轨：活 tip gold `#field` 域积）与 R74（EN+arXiv 轨：稿内标注的历史 Table 1 行加总）——同轨内摘要与表 1 数字一致，裂口在轨与轨之间；Δ=260 精确等于 enc 78 + isel 26 + abi 78 + combo 78，即仅四阶段上 `(op 87−74)` 乘以各自其余域；PR #61 后的产品 test tip 未漂 gold pin / built.json / 四 blob；§8.1 只披露产物 Latest 漂移、不绑定任一词表配方。**

---

## 为何相对 ~12:39 / ~20:45 仍真（本拍新边界）

| 已有材料 | 已钉什么 | 本拍补什么 |
| --- | --- | --- |
| 测量身份钉 ~12:39 | 8509≠8769 笔误；免责已盖；四阶段键差；单一冻结未盖 | **具名配方 R87/R74** + **Δ=260 阶段公式** + tip `e1fb5dbe` 再 pin |
| 词表≠产物轴钉 ~20:45 | Ο1/Ο2 与 8509/8769 正交；Latest 不统一键数 | **不重钉产物轴**；补 **track-facing ≠ abstract-vs-table** 误读纠正 |
| Softguess 钉 ~12:30 | Softguess = OUT-OF-SCOPE | 本拍只扫：仍 NONE×4 |
| 决策卡 ~17:08 | 产物轴 Ο1/Ο2 | **不叠新卡**；仅引用 §8.1 与 Latest 事实 |

**缺口一句：** 心跳易把「摘要 8509 vs 表 1 合计 8769」读成横切裂口，或把「Latest 又前进 / PR 又合」读成键数已统一。本钉证明：**裂口是 CN 轨 R87 vs EN+arXiv 轨 R74**；同 tip 上 R87 活 pin 与 R74 标注并存；产品 tip 前进不改配方。

---

## 两套冻结测量配方（定义 · 不发明新数）

| 配方 | 身份标签 | 测量方法（已有来源） | tip `e1fb5dbe` 结果 | 稿面落点（同 tip） |
| --- | --- | --- | --- | --- |
| **R87** | op-87 / **8769** / **569** | Table 1 十八阶段顺序，对 `weights/gold/<stage>.tsv` 取 `#field` 词表长度笛卡尔积并求和；单元 = tip `weights/built.json` 构造宽度（`seal-remeasure-20261009.md`） | 域积和 **8769**；18-stage SHA-256 **`b8244bd8d9422dd32bb21af7053c38ef009d9d5a027c0924b1cac167fa454e70`**；built.json SHA-256 **`5e45bf5e018c9e1f0417fdbeb17cbf3b87463c761542f676686a7c5fc03d665a`**（44,094 B）；18 行 per-stage file sha **0 mismatch** vs `seal-remeasure-tip.json` | **CN 摘要**（8,769 / op-87）+ **CN 表 1 合计** + 表下说明 + §5.1 |
| **R74** | op-74 / **8509** / **517** | **不是** tip gold 活求和；是 EN/TeX Table 1 **印刷行**加总（标注历史身份） | tip **无** op-74 frozen gold；EN 表印 enc/isel/abi/combo 为 op **74** → 444/148/444/444 | **EN 摘要** + **EN 表 1** + **TeX abstract/Table 1** + **`abstract.txt`**（均 8,509 / op-74，并声明 CN 为 8,769） |

### NEW 边界：不是「摘要 vs 表 1」

| 轨 | 摘要键数 | 表 1 合计 | 同轨是否一致 |
| --- | --- | --- | --- |
| CN（R87） | 8,769 | 8,769 | **是** |
| EN / TeX / abstract.txt（R74） | 8,509 | 8,509 | **是** |

因此禁止把双身份误读成「摘要一律 8509、表 1 一律 8769」。正确读法：**track-facing dual recipes**（语言/投稿轨分裂），免责声明已写在两侧。

---

## Δ=260 阶段归因（机核 · 引用既有回执 + 本 tip 复核）

来源：[`seal-remeasure-20261009.md`](../seal-remeasure-20261009.md)（「Mechanical gap 8769−8509 | 260 = 78+26+78+78」）+ [`seal-remeasure-tip.json`](../seal-remeasure-tip.json) `hidden_units.rows` 的 `keys` vs `keys_en_table1_if_diff`。

本 tip 对四阶段 `#field`（tab 分隔）复核：

| 阶段 | tip R87 域积 | R74 标注键 | Δ | 公式 |
| --- | --- | --- | --- | --- |
| enc | 522 = 87×3×2 | 444 = 74×3×2 | **78** | (87−74)×(os 3 × arch 2) |
| isel | 174 = 87×2 | 148 = 74×2 | **26** | (87−74)×(arch 2) |
| abi | 522 = 87×3×2 | 444 = 74×3×2 | **78** | (87−74)×(os 3 × arch 2) |
| combo | 522 = 87×3×2 | 444 = 74×3×2 | **78** | (87−74)×(os 3 × arch 2) |
| **合计** | | | **260** | 78+26+78+78 |

其余 14 阶段 tip vs EN 标注 **same**（`vs_en: same` in seal JSON）。单元差 569−517=52 仍仅 isel 14 + abi 17 + combo 21；**enc 单元两侧同为 5**（键差存在、单元差不在 enc）——与 ~12:39 / seal 回执一致，本拍不新造单元数。

**未伪造：** 未声称 tip 上存在可复现的 op-74 gold；未把 9174 升格为表 1 键数。

---

## tip 刷新（相对 ~20:45 · 产品 test 不漂配方）

| Check | tip `e1fb5dbe` | 相对 ~20:45 / seal |
| --- | --- | --- |
| 18-stage field-domain sum | **8769** | SAME |
| 18-stage SHA-256 pin | **`b8244bd8…`** | match |
| 18× per-stage gold sha | 0 mismatch | match |
| `weights/built.json` | **`5e45bf5e…`** / 44094 B | match |
| Paper A 四 blob | `74893e3d` / `95657738` / `3830ede1` / `43d7ea7e` | SAME（自 `b76ba68` §8.1 披露后未再改键数面） |
| `76fa0fbb..HEAD` 提交 | **6** 个，全部 `test:` queuetimeout / Darwin-portable | **零** `research/unisacc-paper*` / `arxiv-paper-a` / `weights/gold` / `built.json` 触碰 |
| Softguess / 「权重就是」 / “weights are the logic” / 「逻辑本身」·「权重即逻辑」 | **NONE×4**（四稿面字面 0 命中） | 预期态 |

---

## §8.1 与产物轴（引用 only · 不叠卡）

- 正文 §8.1（CN tip）：冻结发布身份仍 **v0.0.19**；披露公开 Latest 已至 **v0.0.38**（稿面句；本拍不改正文）。
- 仓内 `research/r39-release-acceptance.json`：`published.latest: true`，tag **v0.0.39**（公开事实；与 ~20:45 一致）。
- **NEW 边界句（相对「Latest 统一键数」误读的再钉，不叠 Ο 卡）：** §8.1 讨论的是**产物/回执同身份**重测义务，全文**不**把 8509 或 8769 写成已随 Latest 统一的单一投稿键数；词表配方选定仍属路线Ι / ~12:39 封口清单，与 Ο1/Ο2 正交（引 ~20:45）。

---

## 仍开 / 本钉 OUT

### 仍开（seal-blocking · 词表轴）

1. 政委选定投稿测量身份：唯一 R87、永久双标、或另具名 tip 重测后再选（引 ~12:39；**本拍不代裁**）。
2. R74 若永久双标：须另给可复现历史 tip/artifact pin（当前 tip **无** op-74 gold）。
3. 选定前禁止 CN↔EN 键数互换或单侧改写正文。

### 正交仍开（本钉不代填）

- 产物轴 Ο1/Ο2（~17:08 卡；不叠）
- 同身份平台矩阵（~16:45）

### OUT OF SCOPE

- 改正文键数 / Softguess 粘贴 / 改 kernel·weights·facts / 叠决策卡 / 发明 op-74 tip gold / 把 9174 当表 1 键数

---

## 证据与机核

- tip（before）：`e1fb5dbefa5943a967643cf54621c8063be06d91`
- Paper A blobs：`74893e3d` / `95657738` / `3830ede1` / `43d7ea7e`（SAME）
- Softguess：**NONE×4**
- pin：18-stage sum **8769**；pin **`b8244bd8d9422dd32bb21af7053c38ef009d9d5a027c0924b1cac167fa454e70`**；built **`5e45bf5e…`**
- Δ 归因：enc 78 + isel 26 + abi 78 + combo 78 = 260
- 先验：`research/seal-remeasure-20261009.md`、`research/seal-remeasure-tip.json`
- 正交：[`paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)、[`paper-a-gap-nail-vocab-vs-product-identity-axes-20261010-2045.md`](paper-a-gap-nail-vocab-vs-product-identity-axes-20261010-2045.md)、[`paper-a-decision-card-remeasure-identity-20261010-1708.md`](paper-a-decision-card-remeasure-identity-20261010-1708.md)（不叠）、[`paper-a-gap-nail-softguess-seal-boundary-20261010-1230.md`](paper-a-gap-nail-softguess-seal-boundary-20261010-1230.md)
- Latest 候选事实：`research/r39-release-acceptance.json`（v0.0.39；不选定）
- 父节点：**A**（neural-network-based compiler；构造非训练；unisacc 实证）

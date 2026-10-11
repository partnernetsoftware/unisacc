# Paper A · 决策卡刷新：投稿/封口重测身份选择（冻结 v0.0.19 vs Latest v0.0.40）

- **拍点：** 2026-10-11 ~18:08 Asia/Shanghai（论文心跳 · 切口 **B** · 决策卡）
- **仓库 tip（开拍 / before）：** `07bdabfc71189bbd951f93bfe18492c6a4bd9c2f`（= `0.0.40: publish-unsigned receipt — Latest切 v0.0.40`）
- **公开 Latest：** tag **v0.0.40**（published 2026-10-11 18:04 SGT / `2026-10-11T10:04:02Z`；`draft=false`；target_commitish `1db3083506e7af178ed604052532d4226511ad01`；URL https://github.com/partnernetsoftware/unisacc/releases/tag/v0.0.40）— **只引 tag / 发布态**；本卡不发明 size 进主张句（NOTE：release 保留 unsigned 资产；签另记；公开 `unisacc.com` sha256 前缀 `50ab4c21…` 与 candidate HASH_OK，见 publish 回执）
- **正文冻结身份：** 仍 **v0.0.19**（§8.1 领句目前仍披露 Latest=**v0.0.39**；**本拍不改正文**；披露滞后单独可走切口 A，不代裁改派冻结）
- **四 blob（本拍 SAME · 不改正文）：** CN `e2c67f1a0df02a1d5ecd548aa957fa555aaf955b` · EN `ca2bf002cc6d10ae2695c58b78fefb5ced7ab848` · TeX `bd62b23a058b7572a75e8e20264402d84512993d` · abs `43d7ea7e7297ed8be9a96e3950e014e359467973`
- **Softguess：** **NONE×4**（机扫四文件：`Softguess`／`softguess`／`权重就是`／`weights are the logic`／`逻辑本身`／`权重即逻辑` 均 0 hit；本卡不粘 Softguess）
- **状态：** **待政委批**（本拍只刷新板，不代裁、不改正文、不改键数/产品）
- **相对 1407 卡：** 仅 **Ο2 目标 Latest 标签** 从 v0.0.39 → **v0.0.40**（公开 Latest 已切；Draft→Published）；**推荐仍 Ο1**；Ο3 仍否决；1407 卡在 Ο2 标签上 **superseded**，其他主张不重写

---

## 1. 标题（本板唯一）

**投稿/封口重测身份选择（冻结 v0.0.19 vs Latest v0.0.40）**

---

## 2. 为何相对 1407 卡是新板（不是空催）

1407 卡（[`paper-a-decision-card-remeasure-identity-20261011-1407.md`](paper-a-decision-card-remeasure-identity-20261011-1407.md)）把 Ο2 写成「密封 Latest **v0.0.39**」。之后才落地：

1. **公开 Latest 已是 v0.0.40**（2026-10-11 18:04 SGT；`gh release edit v0.0.40 --draft=false --latest`；原 Latest v0.0.39 让位）。
2. **tip 入仓 publish-unsigned 回执**（`07bdabfc…`）：[`research/c40-publish-unsigned-1804/receipt-publish-latest-v0.0.40-1804.md`](../c40-publish-unsigned-1804/receipt-publish-latest-v0.0.40-1804.md) + `release/evidence/v0.0.40/…`；**unsigned 公开、签另记**——仍是公开 Latest 身份，不是「仍 Draft」。
3. **先验 E 1715／C 1724／C 1745／E 1759** 均在「Draft v0.0.40 / Latest 仍 39」语境下书写；**不得**再把 Ο2 目标读成 39，也**不得**把「Draft seal 收据」与「已切 Latest」混成同一决策事实。
4. **正文 §8.1 领句仍写 Latest=v0.0.39**（四 blob SAME）——这是**披露滞后**，不是「Latest 仍是 39」的反证；本卡用 release API／回执对齐 Ο2 标签；正文对齐留给后续切口 A（本拍不做）。

因此：本卡不是「再催一次同一 Ο2=39」；是 Latest 公开发布之后，把待批产物身份卡上的 **Ο2 目标标签** 对齐到当前公开 Latest **v0.0.40**。推荐默认、Ο3 否决、与 8509/8769／§8.1#3 正交关系 **不变**。

---

## 3. 选项（恰好 3 + 显式否决）

### Ο1 · 保冻结 v0.0.19（**本拍推荐默认 · 与 1407/1708 同**）

- **做什么：** §8.1 各条（含表 4/5 若恢复、六平台矩阵、键数身份闭合）的**投稿级重测**，严格钉在密封身份 **v0.0.19**（及其历史回执口径）；公开 Latest 只作漂移披露（正文现仍写 39；可另拍 A 改披露到 40，**不等于**改派冻结）。
- **后果：** 论文数字反映较旧产品切片，**不**跟产品最新进度；漂移会随后续 release 继续变大（现已 19→40）；但**不必改派冻结**即可继续封口测量纪律；与「暂不投 arXiv / 产品优先」一致。
- **不做什么：** 不把 v0.0.40 矩阵／unsigned 字节回填进 v0.0.19 冻结口径；不手改 8509↔8769；不把「已切 Latest」误读成「已选定投稿产物身份」。

### Ο2 · 换封到密封 Latest（点名 **v0.0.40** 或下一密封候选）

- **做什么：** 政委显式改派 Paper A 冻结发布身份 → 选定的密封 Latest（**现公开 Latest = v0.0.40**）；在该身份上重算摘要/表 1 键数口径、重测同身份六平台矩阵、再决定表 4/5 升或继续历史。
- **后果：** **大重测成本**；漂移清零到 40；注意 v0.0.40 为 **unsigned 公开**（签另记）——若投稿要求签后 acceptance，政委可另点「下一密封签后 Latest」而非本 tag；仍**不是** arXiv/对外投稿指令（政委另批公开时间戳包时再议）。
- **相对 1407：** 仅把目标标签从 v0.0.39 改为 **v0.0.40**（或政委另点的下一密封 Latest）；选项语义不变。
- **触发再议（建议）：** 政委点头做 GitHub+Scholar 公开时间戳包；或签后 `release_eligible` 齐；或产品不再「shameful / 不好意思公开」时。

### Ο3 · 混合身份（**显式否决 · NO**）

- **禁止：** 用 v0.0.19 历史回执 + v0.0.40（或任意更新）矩阵／二进制拼成「一次」投稿级测量；也禁止用「正文仍写 Latest=39」与「API Latest=40」混读成一次身份裁定。
- **依据：** tip §8.1 领句（PR #124）已明文禁止混读；与平台矩阵硬缺口「须单一冻结身份」同向；publish 回执也不冒充已签。
- **本板把 Ο3 列为否决项，不进入可选默认。**

---

## 4. 推荐（给政委）

| 项 | 裁定 |
| --- | --- |
| **本拍默认** | **Ο1** — 保冻结 v0.0.19 做封口重测纪律；Latest **v0.0.40** 仅披露（正文披露可另拍 A 追上） |
| **何时重开 Ο2** | 政委批准公开时间戳包，或签后密封身份可对外，或政委主动改派冻结身份（目标现读 **v0.0.40**） |
| **Ο3** | **否决**（已禁混读） |
| **与 Softguess** | Softguess 粘贴仍 **UNAPPLIED / 可选命名卫生**，与本板**正交**；本拍确认 **NONE×4** |
| **与 8509/8769** | 词表双身份硬缺口**仍开**（引 1745／1514／1239）；Ο1/Ο2 选定产物身份后，仍须另封词表身份（路线Ι：无回执禁止互换数字） |
| **与 §8.1#3** | 同身份六目标平台矩阵**仍开**（引 1724／1525／1645）；`seal-platform-matrix-inventory.json` judgment：receipts alone **不可**闭合；不因 Latest→40 自动填满 |
| **明确不问** | 不请求现在投 arXiv；不请求现在改键数；不请求改产品/kernel/weights/facts；不代裁签后路径 |

**一句主张：** 公开 Latest 已切 **v0.0.40**（unsigned；签另记）之后，待批产物身份卡的 Ο2 目标须读 **v0.0.40**（不再写 39）；仍推荐 **Ο1** 保冻结 v0.0.19；Ο3 混身份否决；正文 §8.1 仍写 39 是披露滞后≠Latest 反证；8509/8769 与 §8.1#3 正交仍开。

---

## 5. 本拍明确不做

- 不改正文 CN/EN/TeX/abstract（notes-only；仅 `paper-notes` 挂链；**不**本拍做 §8.1 Latest 披露 39→40）
- 不改键数 / 表单元格 / 产品 / kernel / weights / facts
- 不粘 Softguess；不叠 Softguess P0 粘贴板
- 不代裁 Ο1/Ο2（只刷新标签 + 重申默认建议）
- 不发明第二套编译器理论；父节点仍是 **A**
- 不把 unsigned Latest 升成「签后 acceptance 已齐」或 arXiv 指令

---

## 6. 证据锚

| 锚 | 路径 / 引用 |
| --- | --- |
| Latest 切 v0.0.40 回执 | [`../c40-publish-unsigned-1804/receipt-publish-latest-v0.0.40-1804.md`](../c40-publish-unsigned-1804/receipt-publish-latest-v0.0.40-1804.md)；`release/evidence/v0.0.40/receipt-publish-latest-v0.0.40-1804.md`；tip `07bdabfc…` |
| 旧决策卡（Ο2 标签 superseded） | [`paper-a-decision-card-remeasure-identity-20261011-1407.md`](paper-a-decision-card-remeasure-identity-20261011-1407.md)；[`paper-a-decision-card-remeasure-identity-20261010-1708.md`](paper-a-decision-card-remeasure-identity-20261010-1708.md) |
| §8.1 Latest 披露（仍 39） | [`paper-a-seal-section81-latest-drift-20261011-1357.md`](paper-a-seal-section81-latest-drift-20261011-1357.md)（PR #124+#125）；正文四 blob SAME |
| Draft≠闸／Ctape≠闸（正交） | [`paper-a-derive-purify-draft-040-seal-candidate-receipt-not-a-gate-20261011-1715.md`](paper-a-derive-purify-draft-040-seal-candidate-receipt-not-a-gate-20261011-1715.md)；[`paper-a-derive-purify-darwin-ctape-compare-acceptance10-not-a-gate-20261011-1759.md`](paper-a-derive-purify-darwin-ctape-compare-acceptance10-not-a-gate-20261011-1759.md) |
| §8.1#3 仍开 | [`paper-a-gap-nail-platform-matrix-tip-refresh-draft040-seal-20261011-1724.md`](paper-a-gap-nail-platform-matrix-tip-refresh-draft040-seal-20261011-1724.md)；`research/seal-platform-matrix-inventory.json` |
| 8509/8769 仍开 | [`paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-draft040-seal-20261011-1745.md`](paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-draft040-seal-20261011-1745.md) |
| Softguess OUT-OF-SCOPE | [`paper-a-gap-nail-softguess-seal-boundary-20261010-1230.md`](paper-a-gap-nail-softguess-seal-boundary-20261010-1230.md) |
| Latest | tag **v0.0.40**（draft=false） |
| tip before | `07bdabfc` |

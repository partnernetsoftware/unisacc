# Paper A · 决策卡刷新：投稿/封口重测身份选择（冻结 v0.0.19 vs Latest v0.0.39）

- **拍点：** 2026-10-11 ~14:07 Asia/Shanghai（论文心跳 · 切口 **B** · 决策卡）
- **仓库 tip（开拍 / before）：** `53772a942ee89f2f1a08fe951494c2f4bb5ed81a`（= Merge #125 §8.1 Latest-v0039 seal fill；含 Merge #124 正文披露）
- **公开 Latest：** tag **v0.0.39**（published 2026-10-10；target_commitish `e31eadda…`）— **只引 tag**，不发明 size/SHA 进卡体（NOTE 仅：release API 有 asset `unisacc.com`，本卡不引用其 size/digest）
- **正文冻结身份：** 仍 **v0.0.19**（§8.1 领句已披露 Latest=v0.0.39；**不**改派冻结）
- **四 blob（本拍 SAME · 不改正文）：** CN `e2c67f1a` · EN `ca2bf002` · TeX `bd62b23a` · abs `43d7ea7e`
- **Softguess：** **NONE×4**（本卡不粘 Softguess；不叠 ~03:44 P0 粘贴板）
- **状态：** **待政委批**（本拍只刷新板，不代裁、不改正文、不改键数/产品）
- **相对 1708 卡：** 仅 **Ο2 目标 Latest 标签** 从 v0.0.38 → **v0.0.39**（或下一密封 Latest）；**推荐仍 Ο1**；Ο3 仍否决；1708 卡在 Ο2 标签上 **superseded**，其他主张不重写

---

## 1. 标题（本板唯一）

**投稿/封口重测身份选择（冻结 v0.0.19 vs Latest v0.0.39）**

---

## 2. 为何相对 1708 卡是新板（不是空催）

1708 卡（[`paper-a-decision-card-remeasure-identity-20261010-1708.md`](paper-a-decision-card-remeasure-identity-20261010-1708.md)）把 Ο2 写成「密封 Latest **v0.0.38**」。之后才落地：

1. **公开 Latest 已是 v0.0.39**（2026-10-10 发布；相对 1708 时点的 38）。
2. **§8.1 领句披露已合 tip**（PR **#124** / tip `8f2361d1…`）：正文冻结仍 **v0.0.19**；Latest=**v0.0.39**（原披露 38）；禁止混读。
3. **seal 注 tip/blob 已填**（PR **#125** / tip `53772a94…`）：[`paper-a-seal-section81-latest-drift-20261011-1357.md`](paper-a-seal-section81-latest-drift-20261011-1357.md)。

因此：本卡不是「再催一次同一 Ο2=38」；是披露与 seal 合入之后，把待批产物身份卡上的 **Ο2 目标标签** 对齐到当前公开 Latest **v0.0.39**。推荐默认、Ο3 否决、与 8509/8769 正交关系 **不变**。

---

## 3. 选项（恰好 3 + 显式否决）

### Ο1 · 保冻结 v0.0.19（**本拍推荐默认 · 与 1708 同**）

- **做什么：** §8.1 各条（含表 4/5 若恢复、六平台矩阵、键数身份闭合）的**投稿级重测**，严格钉在密封身份 **v0.0.19**（及其历史回执口径）；公开 Latest 只作漂移披露（已写进 tip）。
- **后果：** 论文数字反映较旧产品切片，**不**跟产品最新进度；漂移会随后续 release 继续变大；但**不必改派冻结**即可继续封口测量纪律；与「暂不投 arXiv / 产品优先」一致。
- **不做什么：** 不把 v0.0.39 矩阵回填进 v0.0.19 冻结口径；不手改 8509↔8769。

### Ο2 · 换封到密封 Latest（点名 **v0.0.39** 或下一密封候选）

- **做什么：** 政委显式改派 Paper A 冻结发布身份 → 选定的密封 Latest（**现公开 Latest = v0.0.39**）；在该身份上重算摘要/表 1 键数口径、重测同身份六平台矩阵、再决定表 4/5 升或继续历史。
- **后果：** **大重测成本**；漂移清零；仍**不是** arXiv/对外投稿指令（政委另批公开时间戳包时再议）。
- **相对 1708：** 仅把目标标签从 v0.0.38 改为 **v0.0.39**（或政委另点的下一密封 Latest）；选项语义不变。
- **触发再议（建议）：** 政委点头做 GitHub+Scholar 公开时间戳包，或产品不再「shameful / 不好意思公开」时。

### Ο3 · 混合身份（**显式否决 · NO**）

- **禁止：** 用 v0.0.19 历史回执 + v0.0.39（或任意更新）矩阵／二进制拼成「一次」投稿级测量。
- **依据：** tip §8.1 领句（PR #124）已明文禁止混读；与平台矩阵硬缺口「须单一冻结身份」同向。
- **本板把 Ο3 列为否决项，不进入可选默认。**

---

## 4. 推荐（给政委）

| 项 | 裁定 |
| --- | --- |
| **本拍默认** | **Ο1** — 保冻结 v0.0.19 做封口重测纪律；Latest **v0.0.39** 仅披露 |
| **何时重开 Ο2** | 政委批准公开时间戳包，或产品成熟到可对外；或政委主动改派冻结身份（目标现读 **v0.0.39**） |
| **Ο3** | **否决**（已禁混读） |
| **与 Softguess** | Softguess 粘贴仍 **UNAPPLIED / 可选命名卫生**，与本板**正交**；**不**把 ~03:44「先批 P0」叠回本板 |
| **与 8509/8769** | 词表双身份硬缺口**仍开**（引 12:39）；Ο1/Ο2 选定产物身份后，仍须另封词表身份（路线Ι 精神：无回执禁止互换数字） |
| **明确不问** | 不请求现在投 arXiv；不请求现在改键数；不请求改产品/kernel/weights/facts |

**一句主张：** §8.1 已披露 Latest=**v0.0.39** 之后，待批产物身份卡的 Ο2 目标须读 **v0.0.39**（不再写 38）；仍推荐 **Ο1** 保冻结 v0.0.19；Ο3 混身份否决；8509/8769 词表双身份与本板正交仍开。

---

## 5. 本拍明确不做

- 不改正文 CN/EN/TeX/abstract（notes-only；仅 `paper-notes` 挂链）
- 不改键数 / 表单元格 / 产品 / kernel / weights / facts
- 不粘 Softguess；不叠 ~03:44 P0 决策卡
- 不代裁 Ο1/Ο2（只刷新标签 + 重申默认建议）
- 不发明第二套编译器理论；父节点仍是 **A**

---

## 6. 证据锚

| 锚 | 路径 / 引用 |
| --- | --- |
| §8.1 Latest 披露 v0.0.39 | [`paper-a-seal-section81-latest-drift-20261011-1357.md`](paper-a-seal-section81-latest-drift-20261011-1357.md)（PR #124+#125） |
| 旧决策卡（Ο2 标签 superseded） | [`paper-a-decision-card-remeasure-identity-20261010-1708.md`](paper-a-decision-card-remeasure-identity-20261010-1708.md) |
| 平台矩阵硬缺口 | `research/notes/paper-a-gap-nail-platform-matrix-same-identity-20261010-1645.md` |
| 平台矩阵 Latest→39 刷新 | `research/notes/paper-a-gap-nail-platform-matrix-latest-refresh-20261010-2115.md` |
| 8509/8769 硬缺口 | `research/notes/paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md` |
| Softguess OUT-OF-SCOPE | `research/notes/paper-a-gap-nail-softguess-seal-boundary-20261010-1230.md` |
| Latest | tag **v0.0.39** |
| tip before | `53772a94` |

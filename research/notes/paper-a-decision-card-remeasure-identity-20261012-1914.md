# Paper A · 决策卡刷新：投稿/封口重测身份选择（冻结 v0.0.19 vs Latest v0.0.41）

- **拍点：** 2026-10-12 ~19:14 Asia/Singapore（UTC+8；论文心跳 · 切口 **B** · 决策卡）
- **仓库 tip（开拍 / before）：** `39d3e91ef6556164c235ba544c1af469fa8e68aa`（= #312 tip-after fill after Cut E rc-class #311）
- **仓库 tip（合入 / after）：** `_TIP_AFTER_PLACEHOLDER_`（squash merge 后由 tip-after fill 回填）
- **公开 Latest：** tag **v0.0.41**（`gh api .../releases/latest`；`draft=false`；assets=**7**）— **只引 tag / 发布态**；本卡不发明 size 进主张句
- **Draft 残留：** 仅 **v0.0.31**（draft=true）；与 Ο2 目标无关
- **tip `src/version.h`：** `UNISACC_VERSION "0.0.42"`（在研 tip ≠ Latest；证据记，不改正文）
- **正文冻结身份：** 仍 **v0.0.19**
- **正文 §8.1 Latest 披露：** 已写 Latest=**v0.0.41**／截至 2026-10-12（先验 Cut A 1653/#293；1752 tip-refresh 确认无回漂）— **披露滞后已清**；本卡**不改正文**
- **四 blob（本拍 SAME · 不改正文）：** CN `d4ed54ad8060dcd61831c66bdca5a9a5fa1e1473` · EN `8f4b3dc0540b2eba05415c8fb49620470974ecba` · TeX `05927f621a62fa3a45183e5b65395e1539c09054` · abs `92a6472c92a13cb4e8149bbd798f24e6d6a5f648`
- **Softguess：** **NONE×4**（机扫四文件：`Softguess`／`softguess`／`SoftGuess`／`权重就是`／`weights are the logic`／`逻辑本身`／`权重即逻辑`／`权重≡逻辑` 均 0 hit；本卡不粘 Softguess）
- **状态：** **待政委批**（本拍只刷新板，不代裁、不改正文、不改键数/产品）
- **相对 1808 卡：** 仅 **Ο2 目标 Latest 标签** 从 v0.0.40 → **v0.0.41**（公开 Latest 已切；正文 §8.1 披露亦已对齐）；**推荐仍 Ο1**；Ο3 仍否决；1808 卡在 Ο2 标签上 **superseded**，其他主张不重写

---

## 1. 标题（本板唯一）

**投稿/封口重测身份选择（冻结 v0.0.19 vs Latest v0.0.41）**

---

## 2. 为何相对 1808 卡是新板（不是空催）

1808 卡（[`paper-a-decision-card-remeasure-identity-20261011-1808.md`](paper-a-decision-card-remeasure-identity-20261011-1808.md)）把 Ο2 写成「密封 Latest **v0.0.40**」。之后才落地：

1. **公开 Latest 已是 v0.0.41**（release API：`draft=false`；assets=7；原 Latest v0.0.40 让位）。
2. **正文 §8.1 领句已披露 Latest=v0.0.41／截至 2026-10-12**（Cut A 1653/#293；1752 tip-refresh 确认无回漂）— 披露滞后相对 1808 语境已清；**本卡不重改正文**。
3. **tip version 已至 0.0.42**；Draft 仅剩 v0.0.31；晚间 E 车（c42／opcode／rc-class 等）入仓——均**不**改冻结身份、**不**自动闭合测量缺口。
4. **1808 卡 Ο2=v0.0.40 已与公开 Latest 标签漂移**——再读 1808 会把「待批产物身份」指到已让位的 tag。

因此：本卡不是「再催一次同一 Ο2=40」；是 Latest 公开发布到 **v0.0.41** 之后，把待批产物身份卡上的 **Ο2 目标标签** 对齐到当前公开 Latest **v0.0.41**（或政委另点的下一密封 Latest）。推荐默认、Ο3 否决、与 8509/8769／§8.1#3 正交关系 **不变**。**NEW info** = Latest API tag **41** vs 1808 的 Ο2=**40**。

---

## 3. 选项（恰好 3 + 显式否决）

### Ο1 · 保冻结 v0.0.19（**本拍推荐默认 · 与 1808/1407/1708 同**）

- **做什么：** §8.1 各条（含表 4/5 若恢复、六平台矩阵、键数身份闭合）的**投稿级重测**，严格钉在密封身份 **v0.0.19**（及其历史回执口径）；公开 Latest 只作漂移披露（正文已写 **v0.0.41**；**不等于**改派冻结）。
- **后果：** 论文数字反映较旧产品切片，**不**跟产品最新进度；漂移会随后续 release 继续变大（现已 19→41）；但**不必改派冻结**即可继续封口测量纪律；与「暂不投 arXiv / 产品优先」一致。
- **不做什么：** 不把 v0.0.41 矩阵／字节回填进 v0.0.19 冻结口径；不手改 8509↔8769；不把「已切 Latest」误读成「已选定投稿产物身份」。

### Ο2 · 换封到密封 Latest（点名 **v0.0.41** 或下一密封候选）

- **做什么：** 政委显式改派 Paper A 冻结发布身份 → 选定的密封 Latest（**现公开 Latest = v0.0.41**；或政委另点的下一密封 Latest）；在该身份上重算摘要/表 1 键数口径、重测同身份六平台矩阵、再决定表 4/5 升或继续历史。
- **后果：** **大重测成本**；漂移清零到 41；仍**不是** arXiv/对外投稿指令（政委另批公开时间戳包时再议）。
- **相对 1808：** 仅把目标标签从 v0.0.40 改为 **v0.0.41**（或政委另点的下一密封 Latest）；选项语义不变。
- **触发再议（建议）：** 政委点头做 GitHub+Scholar 公开时间戳包；或签后 `release_eligible` 齐；或产品不再「shameful / 不好意思公开」时。

### Ο3 · 混合身份（**显式否决 · NO**）

- **禁止：** 用 v0.0.19 历史回执 + v0.0.41（或任意更新）矩阵／二进制拼成「一次」投稿级测量；也禁止用「正文写 Latest=41」与「冻结仍 19」混读成一次身份裁定。
- **依据：** tip §8.1 领句已明文禁止混读；与平台矩阵硬缺口「须单一冻结身份」同向。
- **本板把 Ο3 列为否决项，不进入可选默认。**

---

## 4. 推荐（给政委）

| 项 | 裁定 |
| --- | --- |
| **本拍默认** | **Ο1** — 保冻结 v0.0.19 做封口重测纪律；Latest **v0.0.41** 仅披露（正文 §8.1 已对齐；本拍不改正文） |
| **何时重开 Ο2** | 政委批准公开时间戳包，或签后密封身份可对外，或政委主动改派冻结身份（目标现读 **v0.0.41**） |
| **Ο3** | **否决**（已禁混读） |
| **与 Softguess** | Softguess 粘贴仍 **UNAPPLIED / 可选命名卫生**，与本板**正交**；本拍确认 **NONE×4**；**不叠 Softguess** |
| **与 8509/8769** | 词表双身份硬缺口**仍开**（class-(a)；引 0857／1737／2054）；Ο1/Ο2 选定产物身份后，仍须另封词表身份（路线Ι：无回执禁止互换数字） |
| **与 §8.1#3** | 同身份六目标平台矩阵**仍开**（引 1827／0930／1724）；不因 Latest→41 自动填满 |
| **明确不问** | 不请求现在投 arXiv；不请求现在改键数；不请求改产品/kernel/weights/facts；不代裁签后路径；**不声称 seal closed** |

**一句主张：** 公开 Latest 已切 **v0.0.41** 之后，待批产物身份卡的 Ο2 目标须读 **v0.0.41**（不再写 40）；仍推荐 **Ο1** 保冻结 v0.0.19；Ο3 混身份否决；正文 §8.1 披露已对齐 41（本拍不改正文）；8509/8769 与 §8.1#3 正交仍开。

---

## 5. 本拍明确不做

- 不改正文 CN/EN/TeX/abstract（notes-only；仅 `paper-notes` 挂链；§8.1 Latest 披露已由先验 A 对齐）
- 不改键数 / 表单元格 / 产品 / kernel / weights / facts / `src/version.h`
- 不粘 Softguess；不叠 Softguess P0 粘贴板
- 不代裁 Ο1/Ο2（只刷新标签 + 重申默认建议）
- 不发明第二套编译器理论；父节点仍是 **A**
- 不把「Latest→41／tip 0.0.42／E 车入仓」升成「封口已闭」或 arXiv 指令
- 不叠第二决策题；不重钉 Softguess／8509／§8.1#3

---

## 6. DISTINCT（不得并读）

| 拍 / 锚 | 关系 |
| --- | --- |
| **1808 B** | Ο2 标签 **40→41** 为本拍 NEW；推荐／Ο3／正交关系同；1808 在 Ο2 标签上 superseded |
| **1752 A** | §8.1 Latest 披露 tip-refresh 确认已对齐 41；本卡只刷新**待批产物身份**选择，不重做披露、不改正文 |
| **1653 A / #293** | 实际改稿 40→41；本拍 notes-only，零正文 diff |
| **1900 E / #311** | rc-class 债／INHERIT ≠ A gate；本拍不净化、不叠 E |
| **1737 / 0857 C** | 8509/8769 tip-refresh／class-(a)；本拍不重钉、不声称闭合 |
| **1827 C** | §8.1#3 矩阵 tip-refresh；本拍不重钉 |

---

## 7. 证据锚

| 锚 | 路径 / 引用 |
| --- | --- |
| 旧决策卡（Ο2 标签 superseded） | [`paper-a-decision-card-remeasure-identity-20261011-1808.md`](paper-a-decision-card-remeasure-identity-20261011-1808.md)；[`paper-a-decision-card-remeasure-identity-20261011-1407.md`](paper-a-decision-card-remeasure-identity-20261011-1407.md) |
| §8.1 Latest 披露已对齐 41 | [`paper-a-seal-section81-latest-drift-20261012-1752.md`](paper-a-seal-section81-latest-drift-20261012-1752.md)；先验 1653/#293 |
| 晚间 E rc-class（正交） | [`paper-a-derive-purify-rcclass-debt-inherit-ledger-not-a-gate-20261012-1900.md`](paper-a-derive-purify-rcclass-debt-inherit-ledger-not-a-gate-20261012-1900.md) |
| §8.1#3 仍开 | 1827／0930／1724；`research/seal-platform-matrix-inventory.json` |
| 8509/8769 仍开 | 0857 class-(a)／1737／2054 |
| Softguess OUT-OF-SCOPE | [`paper-a-gap-nail-softguess-seal-boundary-20261010-1230.md`](paper-a-gap-nail-softguess-seal-boundary-20261010-1230.md)；0846 NONE≠seal-close |
| Latest | tag **v0.0.41**（draft=false；assets=7） |
| tip before | `39d3e91ef6556164c235ba544c1af469fa8e68aa` |
| tip after | `_TIP_AFTER_PLACEHOLDER_` |

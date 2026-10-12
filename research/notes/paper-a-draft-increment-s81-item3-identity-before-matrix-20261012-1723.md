# Paper A · 成篇定稿增量：§8.1#3 先选密封测量身份再组平台矩阵

- **拍点：** 2026-10-12 ~17:23 Asia/Singapore（UTC+8；论文心跳）
- **本拍切口：** **A. 成篇定稿增量**
- **仓库 tip（开拍 / before）：** `c0e1f8704aea7c073b87aa469ff3dd109dc2b366`（短 `c0e1f870`）— Merge #297 tip-after Cut E Latest-tag churn（#296+#297）；相对任务书 **无漂**
- **tip `src/version.h`：** 在研 tip 证据记（正文不写 tip 版号；开拍核对 Latest **v0.0.41** assets=7；冻结仍 **v0.0.19**）
- **四 blob（开拍 before）：** CN `1a1122863c44205f52e2a2ebaee1e14ffe43aa95` · EN `35cb717952480f6a4e4cbc02dac831f030d2aabb` · TeX `1d057dfa376031ffbf3a3062f0fc8cfbdd19f541` · abstract `92a6472c92a13cb4e8149bbd798f24e6d6a5f648`
- **Softguess：** 机扫四正文 **BEFORE NONE×4** / **AFTER NONE×4**（`Softguess`／`softguess`／`SoftGuess`／`软猜`／`权重就是`／`weights are the logic`／`逻辑本身`／`权重即逻辑`／`权重≡逻辑` 均 0 hit；本拍不粘 Softguess）
- **状态：** §8.1 第 3 条（平台矩阵）CN/EN/TeX 三联硬化为「**先选定单一密封测量身份**，再整理**该身份**回执中的六个 runner + 本机 Windows／Linux 必跑项为一张表；未选定身份前不得把不同身份回执拼表冒充本条已闭合」；abstract **未改**；**不**声称矩阵已闭合；**不**发明矩阵数字；**不**改键数；**不**叠 Ο1 卡；父节点仍是 A。

---

## 主张一句（本页唯一）

**§8.1 第 3 条平台矩阵只能在选定单一密封测量身份之后，用该身份回执组装；把不同身份的回执拼成一张表不得算作本条已闭合。**

---

## 攻击面（改前）

§8.1#3 原文仍写「把 **v0.0.19** 回执中的六个 runner…整理为一张表」，而 §8.1 领句已要求投稿级同身份重测另选单一密封 Latest、不得混读 v0.0.19 与更新二进制。读者可误读为：即使尚未选定测量身份，也可拿 v0.0.19（或任意）回执拼矩阵并宣称 #3 闭合。先验 C 钉 1640 已表明矩阵仍开（tip-refresh，非闭合）；先验 E 1715 切 Latest-tag churn 非闸门——本拍 **不是** 又一次 E tip-refresh，而是正文硬钉「身份先于矩阵」。

---

## 钉（仅 §8.1 第 3 条；领句与其余条目未改）

| 文件 | 改前（摘要） | 改后（摘要） | 相对 before |
| --- | --- | --- | --- |
| CN `research/unisacc-paper.md` | 把 v0.0.19 回执中的六个 runner…整理为一张表… | 在选定单一密封测量身份之后，把**该身份**回执中的六个 runner…整理为一张表…；未选定身份前，不得把不同身份的回执拼成一张表冒充本条已闭合 | **CHANGED** |
| EN `research/unisacc-paper.en.md` | turn the six runners in the v0.0.19 receipt… | after a single sealed measurement identity is chosen, turn the six runners in **that identity's** receipt… Until that identity is chosen, receipts from different identities must not be assembled into one table as if this item were closed. | **CHANGED** |
| TeX `research/arxiv-paper-a/main.tex` | turn the six runners in the v0.0.19 receipt… | 与 EN 同义（`\\textbf{that identity's}`） | **CHANGED** |
| abstract `research/arxiv-paper-a/abstract.txt` | 无 §8.1#3／平台矩阵字面 | 无改动 | **SAME** |

### 改前／改后原文摘录

**CN 改前：**
> 3. **平台矩阵**：把 v0.0.19 回执中的六个 runner、本机 Windows 与 Linux 必跑项整理为一张表，并写明哪些目标只交叉编译、未在本机运行（例如 lnx/x86_64 用模拟器）。

**CN 改后：**
> 3. **平台矩阵**：在选定单一密封测量身份之后，把**该身份**回执中的六个 runner、本机 Windows 与 Linux 必跑项整理为一张表，并写明哪些目标只交叉编译、未在本机运行（例如 lnx/x86_64 用模拟器）。未选定身份前，不得把不同身份的回执拼成一张表冒充本条已闭合。

**EN 改前：**
> 3. **Platform matrix**: turn the six runners in the v0.0.19 receipt and the required local Windows and Linux items into one table, stating which targets are only cross-compiled and not run locally (for example lnx/x86_64 under an emulator).

**EN 改后：**
> 3. **Platform matrix**: after a single sealed measurement identity is chosen, turn the six runners in **that identity's** receipt and the required local Windows and Linux items into one table, stating which targets are only cross-compiled and not run locally (for example lnx/x86_64 under an emulator). Until that identity is chosen, receipts from different identities must not be assembled into one table as if this item were closed.

**TeX：** 与 EN 同义，仅 `x86\_64` 与 `\\textbf{that identity's}` 的 TeX 转义差异。

---

## DISTINCT（相对既有钉／切）

| 对照 | 本拍为何 DISTINCT |
| --- | --- |
| C 钉 1640 `paper-a-gap-nail-measurement-platform-tip-refresh-after-c42-product-closure-20261012-1640.md` | 1640 = platform／measurement **tip-refresh**（矩阵仍开）；本拍 = **正文**硬化「身份先于矩阵／禁混身份拼表」 |
| E 1715 `#296+#297` Latest-tag churn | 1715 = 切 recurring Latest 披露 churn ≠ A 闸门；本拍 **不是** E tip-refresh，是 Cut A 成篇增量 |
| Ο1 决策卡 `paper-a-decision-card-remeasure-identity-20261011-1808.md` | 仍荐 Ο1；**本拍不叠新卡、不代裁**；正文硬化 ≠ 身份选择 |
| Cut A §8.1 Latest 40→41 `#293+#294+#295` | 那是领句 Latest 披露同步；本拍改的是 **条目 3** 闭合条件 |

---

## 明确未做 / OUT OF SCOPE

- **不**闭合 §8.1#3（矩阵仍开；本拍只硬化闭合前提）
- **不**发明／填入任何平台矩阵数字或 runner 结果
- **不**改 8509 / 8769 / 517 / 569 / 9174；表 1–5 单元格
- **不**改 product / kernel / weights / facts / Softguess / `src/version.h` / abstract
- **不**把正文冻结身份从 v0.0.19 改派；**不**叠新 Ο1/Ο2 决策卡（仍荐 Ο1）
- **不**重做 C 1640 tip-refresh；**不** pivot E；父节点仍是 Paper A

---

## 硬缺口仍开（本拍不闭合）／挡粘点

1. §8.1 第 3 条同身份六目标平台执行矩阵（**仍开**；本拍仅钉「身份先于矩阵」）
2. 测量双身份 8509 / 8769（class-(a)）
3. Ο1/Ο2 决策仍开：推荐 **Ο1**；本拍不叠卡、不代裁

---

## tip / blob（合入后填）

| 项 | 值 |
| --- | --- |
| tip before | `c0e1f8704aea7c073b87aa469ff3dd109dc2b366` |
| tip after（squash Merge #298） | `3cd6514ea60fe69ba25fa613744f93f0a11944f3`（短 `3cd6514e`） |
| PR | **#298**（squash merge；branch `paper-a/s81-item3-identity-before-matrix-20261012-1723` @ `efe75308`） |
| Softguess after | **NONE×4** |
| CN/EN/TeX/abs | **CHANGED / CHANGED / CHANGED / SAME** |
| 四 blob after | CN `d4ed54ad8060dcd61831c66bdca5a9a5fa1e1473` · EN `8f4b3dc0540b2eba05415c8fb49620470974ecba` · TeX `05927f621a62fa3a45183e5b65395e1539c09054` · abs `92a6472c92a13cb4e8149bbd798f24e6d6a5f648`（SAME） |

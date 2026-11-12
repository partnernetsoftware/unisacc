# Paper A · 成篇定稿增量：§8.1 Latest 披露 v0.0.40→v0.0.41

- **拍点：** 2026-10-12 ~16:53 Asia/Singapore（UTC+8；论文心跳）
- **本拍切口：** **A. 成篇定稿增量**
- **仓库 tip（开拍 / before）：** `0dd61b613f3249384f1e183b89c687aa7008f1a4`（Merge #292；c42 tip-refresh 1640 后）
- **tip `src/version.h`：** `UNISACC_VERSION "0.0.42"`（在研 tip ≠ Latest；证据记，正文不写 tip 版号）
- **公开 Latest（核对）：** tag **v0.0.41**（`gh api .../releases/latest` + cursor-github `get_latest_release`；`draft=false`；`prerelease=false`；published `2026-10-12T06:21:24Z`＝2026-10-12 14:21 SGT；assets=**7**；target_commitish `440b5c2b16cb7a831f40826873bc2f3f290f6224`）— **只引 tag / 发布日，不发明 size/SHA 进正文**
- **四 blob（开拍 before）：** CN `40cde6dc0c67ac82ab19ecec52202fd3c5f66557` · EN `d9cbfc8cfa9840ea30af3422123b0fec48d1e0e1` · TeX `26c38ff6e2648d8650a61797d743bbb8da6f1748` · abstract `92a6472c92a13cb4e8149bbd798f24e6d6a5f648`
- **Softguess：** 机扫四正文 **NONE×4**（`Softguess`／`softguess`／`软猜`／`权重就是`／`weights are the logic`／`逻辑本身`／`权重即逻辑` 均 0 hit；本拍不粘 Softguess）
- **状态：** §8.1 开篇领句已三语对齐披露「正文冻结仍 v0.0.19 / Latest 已至 **v0.0.41**（原 40）/ 投稿级同身份重测须另选单一密封 Latest」；**不**把冻结身份改派为 v0.0.41；**不**改键数／表单元格／产品；父节点仍是 A。先验 1640 残余显式荐本切：[`paper-a-heartbeat-report-20261012-1640.md`](paper-a-heartbeat-report-20261012-1640.md)。先验 A 切 39→40：[`paper-a-seal-section81-latest-drift-20261011-1831.md`](paper-a-seal-section81-latest-drift-20261011-1831.md)。Ο1/Ο2 **不**叠新卡；仍荐 Ο1；披露刷新 ≠ 改派冻结；披露刷新 ≠ 矩阵闭合。

---

## 主张一句（本页唯一）

**§8.1 必须写明：正文冻结的发布身份仍是 v0.0.19（历史回执与 §8.1 各条登记口径），截至 2026-10-12 公开 Latest 已至 v0.0.41（原披露 v0.0.40）；投稿级同身份重测须另选单一密封 Latest 身份，不得把 v0.0.19 回执与更新二进制混读成一次测量。**

---

## 攻击面（改前）

§8.1 开篇领句仍写 Latest **v0.0.40**／截至 **2026-10-11**，而公开 Latest 已是 **v0.0.41**（2026-10-12 发布；unsigned Draft 材料；assets=7）。1640 拍残余已点名「可选 Cut A：§8.1 Latest 40→41 三联对齐」。本拍只修领句披露，不代裁投稿身份、不叠 Ο1/Ο2、不重写 §8.1#3、不 tip-refresh 8509/8769。

---

## 钉（仅 §8.1 开篇领句；条目 1–6 正文未改）

| 文件 | 改后领句要点 | 相对 before |
| --- | --- | --- |
| CN `research/unisacc-paper.md` | 冻结**仍为** v0.0.19；截至 2026-10-12 Latest **v0.0.41**；投稿级同身份重测须另选单一密封 Latest；**不得**混读 | **CHANGED** |
| EN `research/unisacc-paper.en.md` | remains v0.0.19 … As of 2026-10-12, public Latest is v0.0.41 … one sealed Latest identity … must not mix | **CHANGED** |
| TeX `research/arxiv-paper-a/main.tex` | 与 EN 同义（Appendix `\ref{artifacts-and-evidence}`） | **CHANGED** |
| abstract `research/arxiv-paper-a/abstract.txt` | 无 Latest 字面；仅历史 `v0.0.9-era` 短语，非 Latest 披露 | **SAME** |

条目 1–6（含第 3 条平台矩阵）字面保留；本拍不重写条目正文。不声称矩阵已闭合。

---

## 明确未做 / OUT OF SCOPE

- **不**把正文冻结身份从 v0.0.19 改派为 v0.0.41
- **不**改 8509 / 8769 / 517 / 569 / 9174；表 1–5 单元格
- **不**改 product / kernel / weights / facts / Softguess / `src/version.h`
- **不**闭合 8509/8769 双身份；**不**填同身份六目标矩阵；**不**重测表 4/5
- **不**在正文发明 v0.0.41 产物 size / SHA（仅 tag + 披露日）；target_commitish / assets 仅证据页
- **不**把 tip `0.0.42` 写进正文（tip 在研 ≠ Latest）
- **不**叠新 Ο1/Ο2 决策卡；推荐仍 Ο1；Ο3 混身份仍否决
- 父节点仍是 Paper A；无第二套编译器理论；不重切 derive-purify（本拍不 pivot E）

---

## 硬缺口仍开（本拍不闭合）／挡粘点

1. 测量双身份 8509 / 8769
2. §8.1 第 3 条同身份六目标平台执行矩阵（fill_status 仍 blocked-on-identity）
3. Ο1/Ο2 决策仍开：推荐 **Ο1** 保冻结 v0.0.19；Ο2 目标标签现为 Latest **v0.0.41**；本拍不代裁

---

## tip / blob（合入后填）

| Artifact | SHA |
| --- | --- |
| tip after merge | `86a4669e698930399ca38e032608cba3f99ee702` (Merge #293) |
| CN `research/unisacc-paper.md` | `1a1122863c44205f52e2a2ebaee1e14ffe43aa95` |
| EN `research/unisacc-paper.en.md` | `35cb717952480f6a4e4cbc02dac831f030d2aabb` |
| TeX `research/arxiv-paper-a/main.tex` | `1d057dfa376031ffbf3a3062f0fc8cfbdd19f541` |
| abstract | `92a6472c92a13cb4e8149bbd798f24e6d6a5f648` (**SAME**) |

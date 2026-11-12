# Paper A · 真缺口钉：Softguess NONE×4 + 四 blob SAME ≠ 封口路径闭合

- **拍点：** 2026-10-12 ~08:46 Asia/Singapore（UTC+8）
- **本拍切口：** **C. 真缺口钉**
- **主张一句：** **Softguess NONE×4（四 Paper A blob 相对 #168 基线 SAME）只证明措辞／blob 身份稳定，并不闭合 Paper A 封口路径；挡粘硬缺口仍是（1）测量身份／摘要 8509 vs 表 1 合计 8769、（2）§8.1 第 3 条、（3）待批 Ο1——本拍不叠新 Ο1 卡、不改键数。**
- **仓库 tip-before（本拍开工）：** `dce7d5ce996280cb4382ae778a6871ba070d5492`（短 `dce7d5ce`）— 任务书／observe 0841 tip；= #224 bookkeeping after E #223（0.0.41 freeze／Stage／Python archive≠A gate）；相对任务书 **无漂**。
- **tip-after：** `6781b2db7f04d5b0eccb65670150406a3b9d1135`（短 `6781b2db`；= squash Merge #225 本 C）
- **#168 基线：** merge `ae7f7424585f60fc0184e86e3dbd3b32686125bf`（TeX/abs +image field layout）
- **Latest：** **v0.0.40**（assets=10）；tip version **0.0.41**（freeze start；不 bump Latest）；Paper A 冻结重测身份仍 **v0.0.19**（本拍不改）
- **父节点：** **A**
- **状态：** Softguess **「NONE≠seal-close」可引用钉**（notes-only）；**不**改正文；**不**叠 Ο1/Ο2；**不**重钉 8509 Δ／§8.1#3 矩阵表；**不**粘 Softguess 字面

---

## 与已有钉的正交（为何本拍仍真）

| 已有材料 | 已钉什么 | 本拍补什么 |
| --- | --- | --- |
| Softguess seal-boundary（1230） | Softguess 字面 ∈ OUT-OF-SCOPE · A seal-before；NONE×4＝预期态 | 不重写 OUT-OF-SCOPE；补 **反读**：NONE×4／SAME **不得**被读成「封口路径已闭合」 |
| Softguess tip-refresh after #168（0125） | #168 后 tip Softguess 仍 NONE×4 | 升 tip 证据至 `dce7d5ce`（#223/#224 后）；主对象改为 **SAME+NONE ≠ seal-close** 假蕴含 |
| E 041 freeze／Stage／Python（0825/#223） | 产品 freeze／归档 ≠ A gate | 本钉是 **C 机证边界**，不是 E 升格切；对象 DISTINCT |
| 8509 tip-refresh（0813/#221）／§8.1#3（0801/#219） | 测量／矩阵仍 seal-blocking | 本拍只 **指向** 仍开硬缺口；**不重钉**、不发明数字 |

**缺口一句：** 心跳反复报 Softguess NONE×4 + 四 blob SAME，易被误读成「A 已可封」。本钉证明：**SAME+NONE 只是措辞／blob 稳定机证**，封口路径仍被 8509/8769、§8.1#3、Ο1 挡住。

---

## Softguess 证明什么／不证明什么

### Softguess NONE×4 + SAME×4 **证明**

1. tip 四 Paper A 文件上 Softguess／「权重就是」／“weights are the logic”／「逻辑本身」／「权重即|≡逻辑」字面命中＝0（预期态；义务已在 Def2/3·Alg1.8·§3「不含/no softmax」）。
2. 相对 #168 基线，CN/EN/TeX/abstract **blob SHA 全同**——自 image field layout 成篇增量以来，**零** Paper A 正文漂移。
3. 因此：近期产品 tip 前进（含 0.0.41 freeze E）**未**改动 A 正文措辞身份。

### Softguess NONE×4 + SAME×4 **不证明**

| # | 非法升格 | 反驳 |
| --- | --- | --- |
| 1 | NONE×4 ⇒ A 封口路径已闭合 | 封口硬缺口是测量身份、§8.1#3、Ο1；Softguess 不在该集合（见 1230 OUT-OF-SCOPE） |
| 2 | 四 blob SAME ⇒ 测量债已消 | SAME 只说正文未改；8509 vs 8769 仍开（活 gold op-87／8769 vs 摘要／表口径 8509） |
| 3 | SAME ⇒ §8.1#3 六目标原生矩阵已绿 | 矩阵仍 seal-blocking；产品 freeze／CI ≠ 同身份原生矩阵闭合 |
| 4 | NONE×4 ⇒ 可跳过 Ο1／Ο2 | 投稿／封口重测身份仍待批；**仍荐 Ο1**；本拍**不叠卡** |
| 5 | Softguess 机扫通过＝第四 seal-before 已交 | Softguess 字面粘贴仍属 P0 命名卫生（UNAPPLIED），不是 seal-before 债 |

---

## 机核（可复现 · tip `dce7d5ce`）

1. `gh api repos/partnernetsoftware/unisacc/contents/{path}?ref=main` 拉四文件 blob sha；对照 `#168` merge `ae7f7424…` 同路径 sha。
2. 解码正文机扫：`Softguess`／`softguess`／`SoftGuess`／`权重就是`／`weights are the logic`（空白折叠、ASCII 不敏感）／`逻辑本身`／`权重即逻辑`／`权重≡逻辑`。
3. **判据 NONE×4：** 四文件 Softguess 族＋口号命中合计＝0。
4. **对照：** `softmax` 仅允许「不含／no softmax」义务句（CN/EN/TeX 各 1，abstract＝0）。

### 四 blob @ tip `dce7d5ce` vs #168

| 文件 | tip / #168 blob sha | 相对 #168 |
| --- | --- | --- |
| CN `research/unisacc-paper.md` | `5231dd3bf3e60804c678d48015f1e337edd9dc56` | **SAME** |
| EN `research/unisacc-paper.en.md` | `0a707d704079bfef3b2b5193bfa8dc2b5ec90303` | **SAME** |
| TeX `research/arxiv-paper-a/main.tex` | `aab7bd9b8e3c72d08aa2672a559513bb70e02049` | **SAME** |
| abs `research/arxiv-paper-a/abstract.txt` | `d2eb48f7814b7b578b5644c5e19ff9a76ffe3d3b` | **SAME** |

### Softguess 机扫 @ 2026-10-12 08:46 Asia/Singapore

| 文件 | Softguess | softguess | SoftGuess | 权重就是 | weights are the logic | 逻辑本身 | 权重即逻辑 | 权重≡逻辑 | softmax（对照） |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1（「不含 softmax」） |
| EN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1（「no softmax」） |
| TeX | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1（「no softmax」） |
| abstract | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

- **Softguess_NONE_x4 = true**
- **weights_slogan_NONE_x4 = true**
- **四 blob SAME vs #168 = true**
- 机核工作区：`/tmp/paper-a-softguess-0846/`（不入仓）

---

## 仍开挡粘（本拍不重钉、不叠卡）

| 挡粘 | 状态 | 指针（只引） |
| --- | --- | --- |
| 测量身份 8509/8769 | **仍开** | [`paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-after-k629-041-freeze-20261012-0813.md`](paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-after-k629-041-freeze-20261012-0813.md)（#221/#222） |
| §8.1#3 same-identity six-target native matrix | **仍开** | 0801/#219/#220 及先验平台矩阵钉 |
| Ο1\|Ο2（投稿/封口重测身份） | **仍开**；**仍荐 Ο1** | [`paper-a-decision-card-remeasure-identity-20261011-1808.md`](paper-a-decision-card-remeasure-identity-20261011-1808.md)；**本拍不叠新卡** |
| Softguess 字面粘贴 | NONE×4 预期态；字面仍 P0 UNAPPLIED；**不升** seal-before | 1230 seal-boundary |

---

## 本拍明确不做

- 不粘 Softguess／keep-keycounts／作用域锚进 tip 正文
- 不改正文 CN/EN/TeX/abstract；不改键数身份／kernel／weights／facts
- 不重做 8509 Δ；不重钉 §8.1#3 矩阵表；**不叠** Ο1/Ο2
- 不把 Softguess NONE 重写为 seal-before 硬缺口；也不把 SAME 升成「可封」
- 不代裁产品 H1／Draft／Latest／0.0.41 seal

---

## 证据

- tip-before：`dce7d5ce996280cb4382ae778a6871ba070d5492`
- tip-after：`6781b2db7f04d5b0eccb65670150406a3b9d1135`（#225）
- Softguess／口号：**NONE×4**；四 blob：**SAME×4** vs #168 `ae7f7424…`
- 正交钉：Softguess seal-boundary 1230；tip-refresh 0125；E 041-freeze 0825；8509 0813；§8.1#3 0801；Ο1 卡 1808

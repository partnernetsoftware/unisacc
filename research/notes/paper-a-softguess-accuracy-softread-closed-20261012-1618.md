# Paper A · 真缺口钉：精度/accuracy/1.000 软读面已闭合（body Soft-read CLOSED）

- **拍点：** 2026-10-12 ~16:18 Asia/Singapore（UTC+8）
- **本拍切口：** **C. 真缺口钉**
- **主张一句：** **Paper A 正文（CN/EN/TeX/abstract）上剩余的「精度 / accuracy / 1.000」命中均为 already-glossed（T1 或 keep-as-training ≠ T1）或 false-positive（混合精度＝量化），无 must-nail 裸软读；软读战役可引用闭合；本拍不改正文、不叠 Ο1/8509/§8.1#3。**
- **仓库 tip-before（本拍开工）：** `c005675b9e1dbf03d57216b7a5a08eb3ef3e4fe9`（短 `c005675b`）— Merge #285 Adam table/stability tip-after fix。
- **tip-after：** （merge 后回填；预期本 notes-only PR squash/merge SHA）
- **父节点：** **A**
- **状态：** Soft-read **CLOSED** 可引用钉（notes-only）；**不**改正文；**不**叠 Ο1；**不**改 8509/8769；**不**粘 Softguess 字面

---

## 与已钉 Cut A 系列的正交

| 已钉面（PR） | 钉什么 | 本拍补什么 |
| --- | --- | --- |
| abstract/table1/table3/appendix accuracy→T1 | 发布精度读法钉为 T1 | 不重钉；机证仍 already-glossed T1 |
| #281–#282 Adam「accuracy hit 1.000」 | keep-as-training ≠ T1 | 不重钉 |
| #283–#285 Adam 表头/单表行/稳定性裸 1.000 | keep-as-training ≠ T1 | 不重钉 |
| Softguess NONE≠seal-close（0846） | Softguess NONE≠封口 | 本钉对象是 **accuracy 软读**，不是 Softguess 字面 |

**缺口一句：** 上拍残余「若有裸精度/accuracy 软读可切 C」。本拍全量机扫证明：**must-nail = 0**，软读面闭合可引用。

---

## 机核判据（可复现 · tip `c005675b`）

扫描范围（**排除** `research/notes/*.md`）：

1. `research/unisacc-paper.md`（CN）
2. `research/unisacc-paper.en.md`（EN）
3. `research/arxiv-paper-a/main.tex`（TeX）
4. `research/arxiv-paper-a/abstract.txt`（abs）

模式：`精度` / `accurac`（大小写不敏感）/ `1.000`。

分类桶：

| 桶 | 定义 |
| --- | --- |
| **must-nail-as-T1** | 主张语言里裸「精度/accuracy/1.000」可被误读为样本/训练精度，且同句无 T1／「不是相对样本的拟合精度或训练精度」／「not sample fit or training accuracy」 |
| **must-keep-as-training** | Adam/训练对照语境裸「1.000/accuracy」可被误读为 T1，且同句无 keep-as-training／训练精度／training-run accuracy ≠ T1 |
| **already-glossed T1** | 同句已钉 T1／Def2／「不是…训练精度」／「not sample fit or training accuracy」 |
| **already-glossed keep-as-training** | 同句已钉 keep-as-training／训练精度／training-run accuracy，且显式 ≠ T1 |
| **false-positive** | 非准确度语义（如「混合精度」＝ mixed-precision 量化） |
| **table-cell-under-T1-col** | 表 1 数据格 `1.000`：列头为 **T1**，表题已钉 T1≠训练精度 —— 不构成裸主张软读 |

**闭合判据：** must-nail-as-T1 = 0 **且** must-keep-as-training = 0。

---

## 计数 @ tip `c005675b`（2026-10-12 16:18 SGT）

| 文件 | `精度` | `accurac` | `1.000` |
| --- | ---: | ---: | ---: |
| CN | 16 | — | 26 |
| EN | — | 15 | 26 |
| TeX | — | 15 | 25 |
| abstract | — | 1 | 0 |

### 精度 / accuracy 命中分类（must-nail = 0）

| 文件 | already-glossed T1 | already-glossed keep-as-training | false-positive | must-nail |
| --- | ---: | ---: | ---: | ---: |
| CN `精度` | 11 | 4 | 1（L409 混合精度） | **0** |
| EN `accurac` | 11 | 4 | 0 | **0** |
| TeX `accurac` | 11 | 4 | 0 | **0** |
| abs `accurac` | 1 | 0 | 0 | **0** |

CN keep-as-training 行：L396、L398、L400、L405（§7.3 Adam；#281–#285 已钉）。  
EN/TeX 对应：L393/395/397/402 与 TeX L804/817/825/835。

### `1.000` 表格拉通读

- 表 1 十八行 stage 数据格（CN L123–140 / EN L124–141 / TeX L301–318）：列头 **T1**；表题已含「精度为全域枚举…不是…训练精度」／「accuracy by full-domain enumeration = T1 … not sample fit or training accuracy」→ **table-cell-under-T1-col**（非 must-nail）。
- 其余 `1.000` 出现在已 gloss 的 T1 句或 Adam keep-as-training 句内。

### Softguess slogan NONE×4（对照）

| 文件 | Softguess/softguess/SoftGuess | 权重就是 / weights are the logic / 逻辑本身 / 权重即逻辑 / 权重≡逻辑 |
| --- | ---: | ---: |
| CN | 0 | 0 |
| EN | 0 | 0 |
| TeX | 0 | 0 |
| abstract | 0 | 0 |

→ **Softguess slogan NONE×4**；**weights slogan NONE×4**。

机核工作区：`/tmp/paper-a-scan-1618/`（不入仓）。

---

## 闭合声明（可引用）

1. **must-nail-as-T1 = 0**；**must-keep-as-training = 0**。
2. 正文软读面 **CLOSED**：后续心跳不得再以「裸精度/accuracy 软读」为切口 A 发明假修；若正文再度引入无 gloss 的精度主张，才重新开 A。
3. **不**证明封口路径闭合：挡粘仍是 §8.1#3、8509/8769、Ο1（本拍不叠卡、不改键数）。
4. Softguess NONE×4 仍只是措辞卫生预期态，≠ seal-close（见 0846 钉）。

---

## 仍开挡粘（本拍不重钉、不叠卡）

1. §8.1#3 六目标原生矩阵  
2. 摘要/表口径 8509 vs 表 1 合计 8769（测量身份）  
3. Ο1（荐不叠）

## 残余下一拍建议

切 A/B/E：封口硬缺口（测量身份或 §8.1#3）或成篇非软读增量；**勿**再扫 accuracy 软读除非正文回退。

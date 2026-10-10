# Paper A · 真缺口钉：Softguess 相对 A 封口的作用域（seal-boundary）

- **拍点：** 2026-10-10 ~12:30 Asia/Shanghai（论文心跳）
- **本拍切口：** **C. 真缺口钉**
- **仓库 tip（只读）：** `bd352750`（相对上一心跳记忆 tip `e1a8c5c3` 有产品/release 前进；**Paper A 四 blob 相对锚 tip `04a5087b` 仍 SAME**）
- **四 blob：** CN `000b7ef1` · EN `93fdf052` · TeX `8122353c` · abstract `9e883892` — **全部 SAME**
- **机核：** Softguess / 「权重就是」 / “weights are the logic” / 「逻辑本身」 / 「权重即逻辑」 **仍 NONE×4**（`/tmp/paper-a-heartbeat/paper-a-heartbeat-verify-20261010-1230.json` → `ALL_PASS True`）
- **状态：** Softguess **封口边界钉**（可引用）；**不**粘 tip；**不**改键数/产品/投稿方向；**不**叠 ~03:44 决策卡；**不**重做 Softguess 逻辑钉 addon / P0 merged 粘贴包

---

## 主张一句（本页唯一）

**Softguess 不是 Paper A 封口前硬缺口：** tip 已 seal 的定义 2（严格唯一 argmax）、定义 3（不读 logits + 域外断言失败）、算法 1 步 8 / 出货「有错或并列则拒绝」、§3 验证路径「不含 softmax」已穷尽「拒绝 ≠ Softmax/破平/读 logits」的义务；「Softguess」只是这些义务的**对偶名**。A 可在四文件仍 Softguess **NONE×4** 的状态下封口主张层；把 Softguess 字面写进正文属于 **P0 命名卫生（UNAPPLIED）**，不是 seal-before 理论债。

---

## 与已有钉的正交（为何本拍仍真）

| 已有材料 | 已钉什么 | 本拍补什么 |
| --- | --- | --- |
| Softguess 逻辑钉 addon（~02:34）+ P0 merged（~03:09） | Softguess=纪律对偶名；粘贴 AFTER（**UNAPPLIED**） | 把「要不要 Softguess 字面」从**挡粘口号**收成**封口清单上的 OUT-OF-SCOPE** |
| 权重≡逻辑作用域（~11:54 C） | 「权重就是逻辑」合法= T1+询问纪律；非法含 Softguess 破平仍叫精确 | 本钉不重写作用域合同；只钉 Softguess **本身**对 seal-before 的地位 |
| `seal-refusal-partial-fn-20261009` | 拒绝=偏函数 / 非泛化；摘要拒绝句已对齐 | Softguess ≠ 拒绝语义本体；拒绝已 tip seal，Softguess 只是对偶命名 |
| DeployForm / BootstrapHost（E） | 部署形态 / 自举判据迁出 | 本钉不迁段、不衍生课题 |

**缺口一句：** 心跳反复报 Softguess NONE×4，易被误读成「A 还缺一块理论」。本钉证明：**NONE×4 是预期态**——义务已在 Def/Alg/§3；字面未粘只挡「口语对偶名」卫生，不挡主张封口。

---

## Softguess 是什么 / 不是什么（边界表）

### Softguess ≔（对偶名 · 仅当需要命名时）

在同一套 logits 上，用 **Softmax**、**读取 logits**、或 **任意破平** 仍挑出一个标签的行为。

### Softguess ≠（非法升格）

| # | 非法读法 | 反驳锚 |
| --- | --- | --- |
| 1 | Softguess 是 A 的第三套产品主张 | tip 无 Softguess 字面；义务已在 Def2/3·Alg1.8·无 softmax |
| 2 | Softguess NONE×4 ⇒ A 主张未封 | 主张用「严格 argmax + 拒绝」表述即可；对偶名可选 |
| 3 | Softguess = 留出泛化 / 学习 | 定义 3 + A2：域外是偏函数失败，不是泛化检验 |
| 4 | Softguess = 拒绝语义本身 | 拒绝=报错/断言失败；Softguess=仍挑标签的近似器行为 |
| 5 | 必须先粘 Softguess 才能投 A | 封口 checklist 不依赖该字面；粘贴属 P0 沟通便利 |

---

## 封口清单地位（本拍可引用判决）

### OUT OF SCOPE · A seal-before（本钉）

- Softguess **字面**是否出现在 CN/EN/TeX/abstract
- Softguess 是否需要独立定理 / 新手算 / 新 Lean
- Softguess 是否进入测量身份 / 表口径 / 跨平台矩阵那一类硬缺口

### IN SCOPE · 仅当下列证据出现（命名卫生 / 沟通）

Softguess 字面**才**进入「建议粘贴」队列，当且仅当至少一条成立：

1. **政委批 P0** keep-keycounts + Softguess AFTER（材料已齐：`/workspace/paper-a-p0-paste-keep-keycounts-softguess-merged-20261010.md`）；或
2. **审稿/对外问答**明确问「并列时会不会 Softmax / 破平挑类」——此时用对偶名比复述 Def2/3 更短；或
3. **口语「权重就是逻辑」对外传播**且未附带 ~11:54 作用域收缩——此时 Softguess 句封死非法#5（见作用域钉耦合规则）。

**反证（何时本钉被推翻）：** 若 tip 出现与 Softguess 同义、但**削弱**严格 argmax / 允许读 logits / 允许破平出货的新句，则 Softguess 边界要从 OUT-OF-SCOPE 升回 seal-before 硬缺口。当前 tip **无此削弱**（机核 ALL_PASS）。

---

## tip 已 seal 义务 ↔ Softguess 对偶（只读摘）

| tip 位点 | 密封义务 | Softguess 对偶 |
| --- | --- | --- |
| **定义 2** | \(\operatorname{argmax}\) 唯一且 \(=G_s(k)\) | 并列仍出标签 = Softguess |
| **定义 3** | 不读 logits；\(k\notin K_s\) 断言失败 | Softmax/读 logits 选类 = Softguess |
| **算法 1 步 8 / 出货** | 有错或并列则拒绝发布 | 破平出货 = Softguess |
| **§3 验证路径** | 不含 softmax / 浮点破平 | Softmax 路径 = Softguess |

因此：**REFUSE ≠ Softguess** 已由 tip 义务蕴含；字面是注释，不是新公理。

---

## 与待批板关系（不叠卡）

- ~03:44 两板（**先批 P0 粘贴**；之后**路线Ι 测量身份**）**仍有效**。
- 本钉给第二板之前的「Softguess NONE 焦虑」消掉：**不要把 Softguess 字面缺失当成测量身份同类硬缺口**。
- 本拍**不新开决策卡、不催同口号、不代裁粘贴**。
- 双身份 8509/8769 免责**仍在** tip；本钉不触键数。
- 真 seal-before 硬缺口仍是：测量身份、表口径、跨平台矩阵（及摘要键数差披露纪律）——**不是 Softguess**。

---

## 本拍明确不做

- 不粘 Softguess / keep-keycounts / 作用域锚进 tip
- 不改正文 CN/EN/TeX/abstract
- 不改 kernel/weights/facts/产品代码/键数身份/投稿方向
- 不叠决策卡；不重写 Softguess addon；不新手算
- 不把 Softguess 升格为衍生课题（它是命名对偶，不是独立理论枝）

---

## 证据与机核

- tip：`bd352750`（= origin/main 于本拍核对）
- Paper A blobs：`000b7ef1` / `93fdf052` / `8122353c` / `9e883892`（相对 `04a5087b` SAME）
- Softguess / 权重就是 / are the logic：**NONE×4**
- 双身份免责：仍在（CN op-87 8769；EN/arXiv op-74 8509）
- 机核：`/tmp/paper-a-heartbeat/paper-a-heartbeat-verify-20261010-1230.json`
- 相关 UNAPPLIED（历史材料，本拍不推进）：`/workspace/paper-a-p0-paste-keep-keycounts-softguess-merged-20261010.md`、`/workspace/paper-a-softguess-logic-nail-keep-keycounts-addon-20261010.md`
- 正交钉：`research/notes/paper-a-gap-nail-weights-eq-logic-scope-20261010-1154.md`

# Seal: Table 1 combo row is not a merge theorem (Paper A)

Paper-A research heartbeat — **research-only wording**. **Key counts and product unchanged** (8509 / 8769 / 9174, 517 / 569; no kernel/weights/facts edits).

## Question

Table 1’s **combo** row (hand-merged isel∪abi) is easy to misread as a Paper A **composition theorem**: that merging adjacent stages always preserves T1 exactness, and/or that merged hidden width equals $W_{\mathrm{isel}}+W_{\mathrm{abi}}$. A2 RQ1 owns systematic merge reachability; its experimental baseline $W^*\mathrel{:=}W_1+W_2$ is a study setting, not a theorem proved in Paper A. EN/tex already gloss the combo cell as a control experiment (not on the main path); CN Table 1 has no “What it decides” column. Prior seal `seal-combo-fail-not-t1-20261009.md` covers §7.4 construct-combination **product** failures ≠ falsifying T1 — a different issue.

## Before (no pin after Table 1)

| Locale | Anchor |
|---|---|
| CN | Blockquote on op-87 / op-74 identity only; then §3.3 |
| EN / tex | Total row; combo cell says “control experiment (not on the main path)”; then Deployment kernel |

## After (one paragraph after Table 1)

| Locale | Added (summary) |
|---|---|
| CN | combo = hand-merge control only; 1.000 = T1 on **that** merged table’s domain; not arbitrary-merge T1; not $W_{\mathrm{isel}}+W_{\mathrm{abi}}$ (121 vs 88+69; EN 100 vs 74+52 hygiene); A2 RQ1 owns $W^*\mathrel{:=}W_1+W_2$ |
| EN / tex | Same meaning as CN paragraph |

### Exact sentences (CN)

表 1 的 **combo** 行仅记录将 isel 与 abi **手工合并**为一张真值表后的**控制实验**（不在出货主路径上）。该行精度 1.000 只表明：对**该合并表自身声明的有限域**，构造网络与表逐键一致（即与其它行相同的 T1 读法）；**并不**证明任意相邻阶段合并都能保持 T1，**也不**主张合并后的隐藏宽度等于 $W_{\mathrm{isel}}+W_{\mathrm{abi}}$（本表 combo 为 121 单元，isel 与 abi 分别为 88 与 69，与朴素相加不一致；英文表 1 为 100 对 74+52，同属阅读卫生）。系统性的阶段合并/组合可达性，以及预注册 A2 研究 RQ1 中的基准 $W^*\mathrel{:=}W_1+W_2$，归属 A2；Paper A 不把 combo 行升格为合并定理。

### Exact sentences (EN)

The **combo** row in Table 1 is a **control experiment** only: a hand-merged isel∪abi truth table, not on the main ship path. Accuracy 1.000 on that row means only that the constructed network agrees with **that merged table's own declared finite domain** (the same T1 reading as every other row); it does **not** prove that arbitrary stage merges preserve T1, and does **not** claim merged hidden width equals $W_{\mathrm{isel}}+W_{\mathrm{abi}}$ (the printed Units cells already disagree with a naive sum---100 vs 74+52 here, 121 vs 88+69 in the Chinese Table 1---cited only as reading hygiene). Systematic merge and composition reachability, and the preregistered A2 RQ1 baseline $W^*\mathrel{:=}W_1+W_2$, belong to study A2; Paper A does not elevate the combo row into a merge theorem.

## Files touched

- `research/unisacc-paper.md` (after Table 1 identity blockquote)
- `research/unisacc-paper.en.md` (after Table 1 Total row)
- `research/arxiv-paper-a/main.tex` (after Table 1 `longtable`)
- `research/seal-combo-row-not-merge-theorem-20261009.md` (this file)
- `research/paper-notes-20261009.md` (one-line cross-ref)

`research/arxiv-paper-a/abstract.txt` — not changed (no merge-as-theorem claim).

## Explicit unchanged

- Product / kernel / weights / facts: **unchanged**
- Key counts 8509 / 8769 / 9174 and units 517 / 569; all Table 1 cell numerics: **unchanged**
- Table 4/5 cells, platform receipts, gold remeasure: **not touched**
- `seal-combo-fail-not-t1-20261009.md` (§7.4): **not redone**

## Next beat

Do **not** redo receipt inventory, gold remeasure, or Table 4/5 cell audits in the next heartbeat unless explicitly requested.

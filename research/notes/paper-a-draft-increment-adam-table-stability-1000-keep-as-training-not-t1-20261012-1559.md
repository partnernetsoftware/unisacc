# Paper A draft increment: §7.3 Adam table/stability 1.000 keep-as-training ≠ T1 (20261012-1559)

- **Cut:** A（成篇定稿增量）
- **Timestamp:** 2026-10-12 15:59 Asia/Singapore (UTC+8)
- **Role:** 论文撰写员
- **Tip before:** `bd0d798053d12f730629d1619514c81832d8f4ce` (short `bd0d7980`) — re-fetched at beat start; task cited `a9924191` but main had raced +1 (`docs(release): §29…`); ancestor of tip-before; last beat #281+#282 prose nails remain
- **Tip after:** _(fill after merge)_
- **PR:** _(fill)_

## Claim

§7.3 Adam **表头 / 单表行 / 稳定性段** 残留裸「1.000」钉为 **训练精度／keep-as-training 度量**，显式 **≠ T1**（声明域穷举逐键相等／模型检验）。上拍（#281+#282）已钉散文「accuracy hit 1.000／达到训练精度 1.000」；本拍补齐表与稳定性面，避免表内裸 1.000 被误读成 T1。CN/EN/TeX 三联同改；abstract.txt SAME（无匹配裸 soft-read）；不引入 Softguess 口号。

## Softguess slogan scan (working tree after edit)

| File | Softguess / softguess / soft-guess / Softmax-as-product / 权重就是 / weights are the logic / 逻辑本身 / 权重即逻辑 / 权重≡逻辑 |
|------|------|
| `research/unisacc-paper.md` | NONE |
| `research/unisacc-paper.en.md` | NONE |
| `research/arxiv-paper-a/main.tex` | NONE |
| `research/arxiv-paper-a/abstract.txt` | NONE |

**Softguess_NONE_x4:** yes

**Soft-read HIT (tip before):** §7.3 Adam table header `Adam（首次 1.000 即冻结）` / `Adam (freeze at first 1.000)`；single-table row `全部达到 1.000` / `all reach 1.000`；stability `曾触及 1.000` / `touched 1.000` 及附近裸「首次 1.000 即冻结」／`freeze-at-first-1.000`。

## Blob SHAs (tip before = bd0d7980)

| Path | Blob SHA before | Action |
|------|-----------------|--------|
| `research/unisacc-paper.md` | `1e5a72d644a5095bf161b551628a7003c05d3858` | CHANGED |
| `research/unisacc-paper.en.md` | `f8aa67c4292157b40fffd85d164cd0b5d3473cd4` | CHANGED |
| `research/arxiv-paper-a/main.tex` | `08e78ddf9318b067b70bdc7028c9ede18c9d07c7` | CHANGED |
| `research/arxiv-paper-a/abstract.txt` | `92a6472c92a13cb4e8149bbd798f24e6d6a5f648` | SAME |




## Blob SHAs (working tree / tip after TBD)

| Path | Blob SHA after (pre-merge hash-object) | Action |
|------|----------------------------------------|--------|
| `research/unisacc-paper.md` | `09aa02b8f9365c567127b6da556afc36b1857381` | CHANGED |
| `research/unisacc-paper.en.md` | `d19189a59240a0f38e85b5b0ae09c0acd0a952b9` | CHANGED |
| `research/arxiv-paper-a/main.tex` | `5d262b88f4f3ecf6bd75456c8c88be5f193171b1` | CHANGED |
| `research/arxiv-paper-a/abstract.txt` | `92a6472c92a13cb4e8149bbd798f24e6d6a5f648` | SAME |

## Exact edits

### CN `research/unisacc-paper.md` §7.3

BEFORE:
```
| 情形 | Adam（首次 1.000 即冻结） | 构造 |
| 单表（11 个阶段，21,925 θ） | 全部达到 1.000，18.8 s | 精确 |
…相同 Adam 超参但**不**采用“首次 1.000 即冻结”时，99 次中有 30 次曾触及 1.000 后又回落。
```

AFTER:
```
| 情形 | Adam（首次训练精度 1.000 即冻结；keep-as-training，≠T1） | 构造 |
| 单表（11 个阶段，21,925 θ） | 全部达到训练精度 1.000，18.8 s | 精确 |
…相同 Adam 超参但**不**采用“首次训练精度 1.000 即冻结”时，99 次中有 30 次曾触及训练精度 1.000 后又回落。
```

### EN `research/unisacc-paper.en.md` §7.3

BEFORE:
```
| Situation | Adam (freeze at first 1.000) | Construction |
| Single tables (11 stages, 21,925 θ) | all reach 1.000, 18.8 s | exact |
…but **without** the freeze-at-first-1.000 rule, 30 of 99 runs touched 1.000 and later fell back.
```

AFTER:
```
| Situation | Adam (freeze at first training-run accuracy 1.000; keep-as-training, ≠ T1) | Construction |
| Single tables (11 stages, 21,925 θ) | all reach training-run accuracy 1.000, 18.8 s | exact |
…but **without** the freeze-at-first-training-run-accuracy-1.000 rule, 30 of 99 runs touched training-run accuracy 1.000 and later fell back.
```

### TeX `research/arxiv-paper-a/main.tex` §7.3

BEFORE:
```
Adam (freeze at first 1.000)
…all reach 1.000, 18.8 s
…freeze-at-first-1.000 rule, 30 of 99 runs touched 1.000 and later fell back.
```

AFTER:
```
Adam (freeze at first training-run accuracy 1.000; keep-as-training, $\neq$ T1)
…all reach training-run accuracy 1.000, 18.8 s
…freeze-at-first-training-run-accuracy-1.000 rule, 30 of 99 runs touched training-run accuracy 1.000 and later fell back.
```

### abstract.txt

SAME — no matching bare Adam table/stability soft-read.

## What was NOT changed

- Key counts / identity claims 8509 / 8769（及表 1 合计）
- Product code, kernel, weights, facts
- Draft / Latest / signing / public release
- Ο1 decision card（荐不叠）；挡板仍 §8.1#3、8509/8769、Ο1
- Submission direction / Softguess slogan（仍 NONE×4）
- Adam 1.000 **未**钉成 T1；保持 keep-as-training ≠ T1

## Residual candidates (next beat)

- 扫描其余裸「精度／accuracy」软读面（若仍有，切 C）
- 挡板：§8.1#3、8509/8769、Ο1（荐不叠新卡）

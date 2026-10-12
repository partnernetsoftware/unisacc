# Paper A draft increment: §7.3 Adam accuracy keep-as-training ≠ T1 (20261012-1541)

- **Cut:** A（成篇定稿增量）
- **Timestamp:** 2026-10-12 15:41 Asia/Singapore (UTC+8)
- **Role:** 论文撰写员
- **Tip before:** `a2b03ffa3ad6bad2fd9e8a3af655f051ccf510ef` (short `a2b03ffa`) — re-fetched; = #280 tip; matches task tip-before
- **Tip after:** TBD (fill after merge)
- **PR:** TBD

## Claim

§7.3 Adam 行「accuracy hit 1.000」／「达到 1.000」钉为 **训练对照／keep-as-training 度量**，显式 **≠ T1**（声明域穷举逐键相等／模型检验）；避免在摘要／表 1 等面已把 soft-read accuracy→T1 之后，把 Adam 训练 metric 误读成 T1。CN/EN/TeX 三联同改；abstract.txt SAME（无 Adam accuracy 字面）；不引入 Softguess 口号。

## Softguess slogan scan (working tree after edit)

| File | Softguess / softguess / soft-guess / Softmax-as-product / 权重就是 / weights are the logic / 逻辑本身 / 权重即逻辑 / 权重≡逻辑 |
|------|------|
| `research/unisacc-paper.md` | NONE |
| `research/unisacc-paper.en.md` | NONE |
| `research/arxiv-paper-a/main.tex` | NONE |
| `research/arxiv-paper-a/abstract.txt` | NONE |

**Softguess_NONE_x4:** yes

**Soft-read HIT (tip before):** §7.3 bare Adam 「accuracy hit 1.000」 / 「达到 1.000」 / 「Adam accuracy drops」 / 「Adam 精度跌落」— after prior beats pinned soft-read accuracy→T1 on abstract/combo/table-1 faces, these training-control phrases risk being re-read as T1.

## Blob SHAs (tip before = a2b03ffa)

| Path | Blob SHA before | Action |
|------|-----------------|--------|
| `research/unisacc-paper.md` | `8177afa826d4f2bd1286455641986df6829eab8c` | CHANGED |
| `research/unisacc-paper.en.md` | `0378a8091567f715371f73d9ebc26b1fd827efb6` | CHANGED |
| `research/arxiv-paper-a/main.tex` | `2a05fcc0eff2a13ed60758d148a06117b7f00921` | CHANGED |
| `research/arxiv-paper-a/abstract.txt` | `92a6472c92a13cb4e8149bbd798f24e6d6a5f648` | SAME (no Adam accuracy literal) |

## Blob SHAs (after edit; tip-after TBD)

| Path | Blob SHA after | Action |
|------|----------------|--------|
| `research/unisacc-paper.md` | `1e5a72d644a5095bf161b551628a7003c05d3858` | CHANGED |
| `research/unisacc-paper.en.md` | `f8aa67c4292157b40fffd85d164cd0b5d3473cd4` | CHANGED |
| `research/arxiv-paper-a/main.tex` | `08e78ddf9318b067b70bdc7028c9ede18c9d07c7` | CHANGED |
| `research/arxiv-paper-a/abstract.txt` | `92a6472c92a13cb4e8149bbd798f24e6d6a5f648` | SAME |

## Exact edits

### CN `research/unisacc-paper.md` §7.3

BEFORE:
```
表中 Adam 精度跌落与「须加宽」等行仅是在本节所述协议下的历史示意
…**每个阶段在首次达到 1.000 的 epoch 冻结**。
```

AFTER:
```
表中 Adam 训练精度跌落与「须加宽」等行仅是在本节所述协议下的历史示意
…**每个阶段在首次达到训练精度 1.000 的 epoch 冻结**（此为 Adam 训练对照的 keep-as-training 度量，不是声明域穷举逐键相等的 T1／模型检验）。
```

### EN `research/unisacc-paper.en.md` §7.3

BEFORE:
```
The Adam accuracy drops and ``must widen'' figures in the table below
…**freezing each stage at the first epoch where accuracy hit 1.000**.
```

AFTER:
```
The Adam training-run accuracy drops and ``must widen'' figures in the table below
…**freezing each stage at the first epoch where training-run accuracy hit 1.000** (a keep-as-training control metric under this Adam schedule; ≠ T1 / declared-domain exhaustive key-by-key equality).
```

### TeX `research/arxiv-paper-a/main.tex` §7.3

BEFORE:
```
The Adam accuracy drops and ``must widen'' figures in the table below
…\textbf{freezing each stage at the first epoch where accuracy hit 1.000}.
```

AFTER:
```
The Adam training-run accuracy drops and ``must widen'' figures in the table below
…\textbf{freezing each stage at the first epoch where training-run accuracy hit 1.000} (a keep-as-training control metric under this Adam schedule; $\neq$ T1 / declared-domain exhaustive key-by-key equality).
```

### abs `research/arxiv-paper-a/abstract.txt`

SAME — mentions Adam comparison only; no «accuracy hit 1.000» literal; no invented change.

## Hard bans observed

- Key counts 8509/8769/9174 untouched
- Product/kernel/weights/facts untouched
- Abstract/combo/table-1 soft-read accuracy→T1 faces **not** re-pinned (already closed)
- Softguess slogans not introduced
- No Draft/Latest/sign/publish; Ο1 / §8.1#3 / 8509·8769 decision cards not stacked
- Submission direction unchanged (PartnerNet Software; no ASU)

## Residual candidates (next cuts)

1. §8.1#3 still open (product/empirical blocker; not this cut)
2. 8509/8769 measurement-identity seal still open
3. Ο1 decision card still recommended not to stack this beat
4. Soft-read: scan remaining bare 精度/accuracy near construction claims outside already-closed abstract/combo and this Adam keep-as-training nail (cut C candidate if any remnant)

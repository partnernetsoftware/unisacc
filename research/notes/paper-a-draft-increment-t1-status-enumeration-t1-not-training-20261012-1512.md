# Paper A draft increment: T1 status 「全域枚举」→ T1/Def2 not training (1512)

- **Cut:** A（成篇定稿增量）
- **Timestamp:** 2026-10-12 15:12 Asia/Shanghai (Asia/Singapore UTC+8)
- **Role:** 论文撰写员
- **Tip before:** `f0397027c5e43552ba74235b23b2ae6dc1da1f68` (short `f0397027`) — re-fetched; main tip unchanged from task tip-before
- **Tip after:** _PLACEHOLDER_ (fill after merge)
- **PR:** _PLACEHOLDER_

## Claim

T1 状态格裸「全域枚举／full-domain enumeration」钉成「声明域穷举逐键相等＝T1／模型检验／定义 2，非样本拟合或训练精度」——堵住状态表读成训练 accuracy 的攻击面；CN/EN/TeX 三联同改；不引入 Softguess 口号。

## Softguess slogan scan (tip before)

| File | Softguess / softguess / soft-guess / Softmax-as-product / 权重就是 / weights are the logic / 逻辑本身 / 权重即逻辑 / 权重≡逻辑 |
|------|------|
| `research/unisacc-paper.md` | NONE |
| `research/unisacc-paper.en.md` | NONE |
| `research/arxiv-paper-a/main.tex` | NONE |
| `research/arxiv-paper-a/abstract.txt` | NONE |

**Softguess_NONE_x4:** yes

**Soft-read HIT (tip before):** bare status-cell 「全域枚举」/「full-domain enumeration」 at CN L75, EN L75, TeX L201 (status table T1 row). Note: Table 1 captions / §6.1 / P-3 already carry the T1-not-training gloss; this cut nails only the **status table** cell.

## Blob SHAs (tip before = f0397027)

| Path | Blob SHA before | Action |
|------|-----------------|--------|
| `research/unisacc-paper.md` | `f289fda9f3c0f802d4c5b5eb2b4994f75f454d5a` | CHANGED |
| `research/unisacc-paper.en.md` | `4437f14887526280c1dd223264d90bea660b5ff6` | CHANGED |
| `research/arxiv-paper-a/main.tex` | `7ca29cc5c9c24a6ffb3a7107b9f731915a9a5097` | CHANGED |
| `research/arxiv-paper-a/abstract.txt` | `92a6472c92a13cb4e8149bbd798f24e6d6a5f648` | SAME (no T1 status table) |

## Exact edits

### CN `research/unisacc-paper.md`

BEFORE:
```
| **T1** | 每个发布网络在其声明域上与表相等（即定义 2 的 $K_s$；管线 DSL 所称「声明观察域」同指该有限键域） | 已证：构造式存在性 + 全域枚举 |
```

AFTER:
```
| **T1** | 每个发布网络在其声明域上与表相等（即定义 2 的 $K_s$；管线 DSL 所称「声明观察域」同指该有限键域） | 已证：构造式存在性 + 全域枚举（声明域穷举逐键相等，即 T1／模型检验／定义 2，不是相对样本的拟合精度或训练精度） |
```

### EN `research/unisacc-paper.en.md`

BEFORE:
```
| **T1** | Every released network equals its table on its declared domain (Definition 2's $K_s$; the pipeline DSL's "declared observation domain" names that same finite key domain) | Proved: constructive existence + full-domain enumeration |
```

AFTER:
```
| **T1** | Every released network equals its table on its declared domain (Definition 2's $K_s$; the pipeline DSL's "declared observation domain" names that same finite key domain) | Proved: constructive existence + full-domain enumeration (= T1 / exhaustive key-by-key equality on the declared domain / Definition 2, not sample fit or training accuracy) |
```

### TeX `research/arxiv-paper-a/main.tex`

BEFORE:
```
\textbf{T1} & Every released network equals its table on its declared domain (Definition~2's $K_s$; the pipeline DSL's ``declared observation domain'' names that same finite key domain) & Proved: constructive existence + full-domain enumeration \\
```

AFTER:
```
\textbf{T1} & Every released network equals its table on its declared domain (Definition~2's $K_s$; the pipeline DSL's ``declared observation domain'' names that same finite key domain) & Proved: constructive existence + full-domain enumeration (= T1 / exhaustive key-by-key equality on the declared domain / Definition~2, not sample fit or training accuracy) \\
```

### abs `research/arxiv-paper-a/abstract.txt`

SAME — no T1 status table; no invented change.

## Hard bans observed

- Key counts 8509/8769 untouched
- Product/kernel/weights/facts untouched
- §7.3 Adam "accuracy hit 1.000" kept as training metric
- Softguess slogans not introduced
- No Draft/Latest/sign/publish; Ο1 decision card not stacked

## Residual candidates (next cuts)

1. §8.1#3 still open (product/empirical blocker; not this cut)
2. 8509/8769 measurement-identity seal still open
3. Ο1 decision card still recommended not to stack this beat
4. Soft-read: other bare 「全域枚举」 sites outside status table already glossed in prior cuts (Table 1 / P-3 / §6.1 row); no further status-cell bare left after this nail
5. Possible next residual: §7.3 Adam keep-as-training wording audit (do **not** rewrite training metric as T1)

## Softguess re-scan (after local edit; reconfirm on tip after)

Expect Softguess_NONE_x4 yes.

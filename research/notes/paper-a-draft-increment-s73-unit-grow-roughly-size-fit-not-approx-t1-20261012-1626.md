# Paper A draft increment: §7.3 「单元数近似 / units grow roughly」→ size-fit ≠ table approx / T1 (20261012-1626)

- **Cut:** A（成篇定稿增量）
- **Timestamp:** 2026-10-12 16:26 Asia/Singapore (UTC+8)
- **Role:** 论文撰写员
- **Tip before:** `3fc64cd2524ede9d233acb5da8997486810096f0` (short `3fc64cd2`) — = #287 tip-after fill; Softguess accuracy soft-read CLOSED @ #286+#287
- **Tip after:** _(fill after merge)_
- **PR:** _(fill)_ (nails) + tip-after fill PR

## Claim

§7.3 Adam 对照表「目标数超过 24」构造列裸「单元数近似 / units grow roughly as …」钉为 **精确（T1）+ 单元数规模经验拟合**，显式 **规模拟合 ≠ 表的近似 / 训练逼近 / training surrogate**；堵住在同列表其它行已写「精确 / exact」时，本行「近似 / roughly」被敌意软读成「构造网络是近似的 / T1 放宽」的攻击面。CN/EN/TeX 三联同改；abstract.txt SAME（无该表单元格）；不引入 Softguess 口号；不改键数身份、Adam 训练精度 keep-as-training、product/kernel/weights/facts。

## Softguess slogan scan (working tree after edit)

| File | Softguess / softguess / soft-guess / Softmax-as-product / 权重就是 / weights are the logic / 逻辑本身 / 权重即逻辑 / 权重≡逻辑 |
|------|------|
| `research/unisacc-paper.md` | NONE |
| `research/unisacc-paper.en.md` | NONE |
| `research/arxiv-paper-a/main.tex` | NONE |
| `research/arxiv-paper-a/abstract.txt` | NONE |

**Softguess_NONE_x4:** yes

**Soft-read HIT (tip before):** §7.3 Construction cell bare 「单元数近似 $39+…$」/「units grow roughly as $39+…$」beside sibling cells 「精确」/「exact」— hostile soft-read as approximate construction / weakened T1. (Not the closed accuracy/精度/1.000 soft-read face.)

## Blob SHAs (tip before = 3fc64cd2)

| Path | Blob SHA before | Action |
|------|-----------------|--------|
| `research/unisacc-paper.md` | `09aa02b8f9365c567127b6da556afc36b1857381` | CHANGED |
| `research/unisacc-paper.en.md` | `d19189a59240a0f38e85b5b0ae09c0acd0a952b9` | CHANGED |
| `research/arxiv-paper-a/main.tex` | `5d262b88f4f3ecf6bd75456c8c88be5f193171b1` | CHANGED |
| `research/arxiv-paper-a/abstract.txt` | `92a6472c92a13cb4e8149bbd798f24e6d6a5f648` | SAME |

## Blob SHAs (tip after = _(fill after merge)_)

| Path | Blob SHA after | Action |
|------|----------------|--------|
| `research/unisacc-paper.md` | `40cde6dc0c67ac82ab19ecec52202fd3c5f66557` | CHANGED |
| `research/unisacc-paper.en.md` | `d9cbfc8cfa9840ea30af3422123b0fec48d1e0e1` | CHANGED |
| `research/arxiv-paper-a/main.tex` | `26c38ff6e2648d8650a61797d743bbb8da6f1748` | CHANGED |
| `research/arxiv-paper-a/abstract.txt` | `92a6472c92a13cb4e8149bbd798f24e6d6a5f648` | SAME |

## Before / after（精确）

### CN `research/unisacc-paper.md` L403

- **Before:** `| 目标数超过 24（超出 6 的合成目标） | 塌缩到 0.4419 | 单元数近似 $39 + 4.2\log_2(\text{targets})$ |`
- **After:** `| 目标数超过 24（超出 6 的合成目标） | 塌缩到 0.4419 | 精确（T1）；单元数规模经验拟合约 $39 + 4.2\log_2(\text{targets})$（规模拟合，不是表的近似或训练逼近） |`

### EN `research/unisacc-paper.en.md` L400

- **Before:** `| More than 24 targets (synthetic beyond 6) | collapses to 0.4419 | units grow roughly as $39 + 4.2\log_2(\text{targets})$ |`
- **After:** `| More than 24 targets (synthetic beyond 6) | collapses to 0.4419 | exact (T1); unit-count size grows empirically roughly as $39 + 4.2\log_2(\text{targets})$ (size fit, not table approximation or training surrogate) |`

### TeX `research/arxiv-paper-a/main.tex` L828

- **Before:** `More than 24 targets (synthetic beyond 6) & collapses to 0.4419 & units grow roughly as \(39 + 4.2\log_2(\text{targets})\) \\`
- **After:** `More than 24 targets (synthetic beyond 6) & collapses to 0.4419 & exact (T1); unit-count size grows empirically roughly as \(39 + 4.2\log_2(\text{targets})\) (size fit, not table approximation or training surrogate) \\`

### abs `research/arxiv-paper-a/abstract.txt`

- **SAME**（无该表单元格；不强制同步）。

## 未动

- 键数身份 8509/8769；§8.1#3 六目标同身份矩阵；Ο1 决策卡（不叠）
- Softguess accuracy/精度/1.000 软读面（#286+#287 CLOSED；本拍不重扫）
- product / kernel / weights / facts；Draft/Latest/签/公开；投稿方向
- §7.3 Adam keep-as-training ≠ T1 已钉面（#281–#285）

## 挡粘仍开

§8.1#3；8509/8769；Ο1（荐不叠）

## 残余建议下一拍

- 勿再软读 accuracy 面；本拍已钉 §7.3 unit-grow roughly
- 有新 tip/产品证据再 tip-refresh 8509/8769 或 §8.1#3；Ο1 有新信息才开卡
- 其它非软读成篇增量：intro/methods 表→权重构造松散句、refusal 概率软读（若机扫仍有 must-nail）

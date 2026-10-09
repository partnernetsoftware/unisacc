# Seal: `b_compound` / `b_pp2` ledger vs tape-level (Paper A)

Paper-A research heartbeat — **research-only wording**. **Key counts and product unchanged** (8509 / 8769 / 9174, 517 / 569; no kernel/weights/facts edits).

## Question

Reviewers can collapse three inconsistent readings: §5.5 keeps two R14-8 obligations under **v0.0.14 release identity**; §5.7 reports tape-level clearance on a **v0.0.20 development candidate**; Limitations and §8.1 item 4 previously stated only “v0.0.14 still has two named … (§5.5)” with no pointer to §5.7, so a skim either denies candidate progress or treats tape clearance as published-identity ledger closure.

## Before (short quotes)

| Location | Before |
|---|---|
| CN Limitations (`unisacc-paper.md` §8) | `v0.0.14 仍有两条具名的前端逐字节差异（§5.5），不能把有限语料上的一致写成全域等价。` |
| EN Limitations | `v0.0.14 still has two named front-end byte differences (§5.5); finite-corpus agreement is not full-domain equivalence.` |
| CN §8.1 item 4 | `闭合之前，逐字节主张仍按“已列清单之外”的口径表述。` |
| EN §8.1 item 4 | `until they close, byte-level claims are stated as "outside the listed entries".` |
| CN §5.5 (end) | ended at R14-8 record link only |

## After (semantics pinned)

1. **v0.0.14 published identity**: ledger still open for `b_compound`, `b_pp2` in published-identity byte-parity scope (§5.5 unchanged in substance).
2. **§5.7 development candidate**: tape-level clearance cited in Limitations; explicitly **does not** close v0.0.14 ledger; six-target **image** confirmation still required.
3. **§8.1 item 4**: tape-level clearance ≠ ledger closure; byte claims stay “outside the listed entries” for published identities that still carry those lines until images confirm and ledger closes.
4. **§5.5**: one clause forward to §5.7 for tape-level candidate status; no image-done claim.

## Files touched

- `research/unisacc-paper.md`
- `research/unisacc-paper.en.md`
- `research/arxiv-paper-a/main.tex`
- `research/seal-bdiff-ledger-vs-tape-20261009.md` (this file)
- `research/paper-notes-20261009.md` (one-line cross-ref)

`research/arxiv-paper-a/abstract.txt` — not changed (abstract does not mention these diffs).

## Explicit unchanged

- Product / kernel / weights / facts: **unchanged**
- Key counts 8509 / 8769 / 9174 and units 517 / 569: **unchanged**
- §5.7 measured claims (264/264, first-diff roots, etc.): **unchanged**
- Table 4/5 cells, platform receipts, golden-domain remeasure: **not touched**
- `research/referee.tsv` / isel: **not touched**

## Next beat

Do **not** redo receipt inventory, gold remeasure, or Table 4/5 cell audits in the next heartbeat unless explicitly requested.

# Seal: §7.4 construct-combination failures ≠ T1 falsification (Paper A)

Paper-A research heartbeat — **research-only wording**. **Key counts and product unchanged** (8509 / 8769 / 9174, 517 / 569; no kernel/weights/facts edits).

## Question

§7.4 already states full-domain enumeration table-by-table and that failures were cross-stage / cross-unit combinations (keys of no table). A skim can still read product refusals, ``unknown identifier'', or the 10 gate failures as overturning abstract 1.000 / T1 exactness claims.

## Before (anchor sentence)

| Locale | Quote |
|---|---|
| CN §7.4 | `全域枚举逐表成立，但出错的是跨阶段、跨单元的组合，它不在任何一张表的键里。` |
| EN §7.4 / tex | `Full-domain enumeration holds table by table, but what failed were combinations across stages and units, which are keys of no table.` |

## After (one-sentence pin)

| Locale | Added |
|---|---|
| CN | `此类产品失败并不否定定理 T1：T1 仅断言每张已发布网络在其声明定义域上等于该表自身；跨阶段、跨单元或构造组合缺口属于任一单表键空间之外的工程与产品义务（可纳入 T2/T3 或后续扩键闭包），不构成对「网络等于表」的反例。` |
| EN / tex | `These product failures do not falsify Theorem T1: T1 asserts only that each released network equals its own table on that table's declared domain; cross-stage, cross-unit and construct-combination gaps are engineering and product obligations outside any single table's key space (they may inform T2/T3 or later table-key closure) and are not counterexamples to network = table.` |

## Files touched

- `research/unisacc-paper.md` (§7.4)
- `research/unisacc-paper.en.md` (§7.4)
- `research/arxiv-paper-a/main.tex` (§7.4 / RQ4)
- `research/seal-combo-fail-not-t1-20261009.md` (this file)
- `research/paper-notes-20261009.md` (one-line cross-ref)

`research/arxiv-paper-a/abstract.txt` — not changed (abstract does not imply combination failures falsify T1).

## Explicit unchanged

- Product / kernel / weights / facts: **unchanged**
- Key counts 8509 / 8769 / 9174 and units 517 / 569: **unchanged**
- Table 4/5 cells, platform receipts, gold remeasure: **not touched**
- A2 four questions, new theorems: **not claimed**
- `research/referee.tsv` / isel: **not touched**

## Next beat

Do **not** redo receipt inventory, gold remeasure, or Table 4/5 cell audits in the next heartbeat unless explicitly requested.

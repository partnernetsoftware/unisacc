# Referee ledger: what checks each table against C

The data is `research/referee.tsv`: one row per stage, with its class and a runnable provider. `tests/docs.sh` fails when its stage set is not exactly `unisa/gold.py` `ALL` or when a named provider is missing. The table below is narrative and does not promise to be complete; the TSV is the ledger.

The ledger records facts only (2026-09-26, HEAD 827f2be). The enumeration (`unisa acc`, 8,484 keys) proves **net = table** for every stage. It says nothing about **table = C**. The ledger splits each stage's evidence into three kinds:

- **External**: something outside the project, such as the system cc, OS headers or a disassembler, answers each key.
- **Agreement**: two implementations of ours, driven by the same table, agree. That shows they are consistent, not that the table is right.
- **End-to-end**: whole programs agree with the system cc (`difftest`, `difftest_o`, `corpus`, `ccparity`). Every stage on the path is exercised, but no single key is checked on its own.

`stages` shows that a stage is asked. `ablate` shows that its answer is used. Which stages ablate covers is listed only in the `ABL=` line of `tests/ablate.sh`; this page no longer copies that list, because a hand copy drifted (bdy review).

The per-stage rows, their notes and the totals are in `research/referee.tsv` and nowhere else; the grouping used to be copied here and was a second source (bdy review). Count the classes with:

    grep -v "^#" research/referee.tsv | cut -f2 | sort | uniq -c


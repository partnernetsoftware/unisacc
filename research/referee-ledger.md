# Referee ledger: what checks each table against C

The data is `research/referee.tsv`: one row per stage, with its class and a runnable provider. `tests/docs.sh` fails when its stage set is not exactly `unisa/gold.py` `ALL` or when a named provider is missing. The table below is narrative and does not promise to be complete; the TSV is the ledger.

The ledger records facts only (2026-09-26, HEAD 827f2be). The enumeration (`unisa acc`, 8,484 keys) proves **net = table** for every stage. It says nothing about **table = C**. The ledger splits each stage's evidence into three kinds:

- **External**: something outside the project, such as the system cc, OS headers or a disassembler, answers each key.
- **Agreement**: two implementations of ours, driven by the same table, agree. That shows they are consistent, not that the table is right.
- **End-to-end**: whole programs agree with the system cc (`difftest`, `difftest_o`, `corpus`, `ccparity`). Every stage on the path is exercised, but no single key is checked on its own.

`stages` shows that a stage is asked. `ablate` shows that its answer is used.

| stage | external, per key | agreement | end-to-end | used (ablate) |
|---|---|---|---|---|
| type | **A-50** `gold_audit.py`: cc `_Generic`, 1,400 of 4,275 keys (10 numeric types × 14 operators) | — | yes | yes |
| abi | **A-51** `abi_audit.py`: syscall numbers against this machine's `<sys/syscall.h>`, one cell per machine | — | yes | yes (six heads) |
| lex | none | `lexdiff`: the C lexer vs the Python one | yes | yes |
| peep | none | `optpy`: the C -O0 tape run through `unisa/opt.py` vs C -O1/-O2 | through -O levels (`difftest_o`) | **not covered** (opt.py is off the compile path) |
| opinfo | none | `optpy` | through -O levels | **not covered** |
| enc, reloc, regmap | none (no disassembler referee yet) | `closure`: the C back end vs the Python back end, byte for byte on one tape | yes (`native`, `crossnative` run the images) | yes |
| pp, parse, scope, irsel, binsel, prec, tyinfo, pfconv | none | — (not named yet) | yes | yes, except tyinfo and pfconv, which ablate does not list |
| isel, combo | — | off the C compiler's path | — | — |

Honest totals: two of the 18 tables have a per-key external referee, and `type` is only partly covered by one. Every other table on the path is covered only end to end, through whole programs, plus agreement where the table says so. A stage with no external referee is registered here as such. Skipped is not passed.

# iterate/construct: the weight constructor in C (J10 step 2)

`construct.c` reads one gold table, `weights/gold/<stage>.tsv`, and prints the
net that `unisa/construct.py` `build_net` builds from it. It uses only the C
subset unisacc compiles, and it is standalone. It is not part of the compiler.

    construct [-d] weights/gold/prec.tsv
    python3 iterate/construct/tools/netdump.py [-d] weights/gold/prec.tsv
    construct -u out.uns2 weights/gold/prec.tsv weights/gold/reloc.tsv
    python3 iterate/construct/tools/uns2slice.py ref.uns2 weights/gold/prec.tsv weights/gold/reloc.tsv
    python3 iterate/construct/tools/uns2slice.py --shipped out.uns2 weights/built.uns2
    python3 iterate/construct/tools/uns2round.py out.uns2 weights/gold/prec.tsv weights/gold/reloc.tsv

Both print the same canonical text. It holds the fields of `intnet.to_dict` in
that function's order: `stage`, `H`, `offs`, `b1`, the head with its classes,
`feeds` (one `f c:` line per coordinate), `w2` (one `r j: class,weight ...` line
per unit), `maxlogit`, and then `exact <keys>`. With `-d`, both sides first
print the intermediate steps, so a divergence shows up at the first step where
it occurs:

1. `groups i:`: the quotient groups of field i (value indices).
2. `rule r: cube => class rank l`: the decision list after REDUCE, with its
   winner-vs-loser ranks.
3. `kind` and `unit j: cube`: the chosen representation and its units, in order.

`check.sh [ua]` runs the whole comparison. Run it from the repo root. Every step
has an alarm.

### Selecting stages, and batches (2026-09-25)

A full run is ~32 s; the ceiling for one run is 60 s. So check.sh can run a
subset, and can run the whole list as bounded batches.

- **Stage list.** One table, `TABLE`, at the top of check.sh: stage name,
  UNS2 blob tag (prec and reloc share `single`), whether it has a trace, and
  whether it has invariant negatives (reloc has none). Every per-stage loop
  (dump/UBSan, UNS2, trace, invariant negatives) is derived from it. To add a
  stage, add one row there.
- **Default** (neither `STAGES` nor `GLOBAL` set): every stage and every
  global check, the same output as before, plus `summary:` lines.
- **`STAGES="peep type"`** runs those stages only. `all` means every stage.
  The pseudo-stage `global` turns on the checks that belong to no stage:
  the qset `-Q` self-test, reader negatives, 63-key capacity positive, and
  the capacity, raw-key, field-group and class-count negatives, and (abi
  steps 1-4) the sum self-test and the rule, head and candidate positives
  and negatives (`sum capr caprn caph caphn capk capkn`). `GLOBAL=1/0`
  forces them on or off. Setting either variable makes the selection
  explicit, so `GLOBAL=1` alone runs the global checks only.
- **Fails with exit 2**: an empty selection (`STAGES=`), an unknown stage,
  or any selection that checks nothing (`GLOBAL=0` alone,
  `STAGES=global GLOBAL=0`).
- **Receipts.** Every individual check appends a pass mark at the point it
  succeeds (`P "s peep uns2 cc"`), and only there. `need()` lists the marks
  each item requires: a stage needs dump cc+ua, UBSan, UNS2 cc+ua, shipped
  section cc+ua and round trip cc+ua, plus its trace and its invariant
  negatives (bias, act x cc, ua) when `TABLE` says it has them; each global
  check needs all of its own builds and cases. An item gets a receipt only
  when every mark is present, so a check that fails, is skipped, or was
  deleted leaves its item without one. A run ends with the receipt list,
  one line per item that PASSED and nothing else: `receipt stage peep`,
  `receipt global capq`.
- **Summary.** Built from the marks, not from the selection: per stage and
  per global check `attempted, passed`, `attempted, FAILED, no receipt;
  missing [...]` (naming the missing marks), or `skipped`, then
  `summary: attempted N, passed N, failed N, skipped N`. An attempted item
  without a receipt fails the run by itself (exit 1), even when no check
  set the failure flag. The three builds (cc, ua, UBSan) always happen.
- **`check.sh --batches [ua]`** runs `BATCHES` (a `stages|global` row per
  batch). Each batch is a separate check.sh run, bounded by `alarm 60`, and
  its time is printed. Before it starts, it checks that the PLANNED batches
  together hold every stage in `TABLE` exactly once and the global checks
  exactly once. That is a plan; the result is the receipts. A batch with
  rc != 0 stops the run at once. Otherwise its `receipt` lines must equal
  exactly what that batch requested, or `batch N: RECEIPTS WRONG (rc 0):
  missing [..] extra [..] duplicate [..]` fails the run; at the end the
  receipts of all batches together must be every stage of `TABLE` plus
  every global check, each exactly once (`batches: receipt union = all 15
  stages + 14 global checks, each exactly once (29 receipts)`). The current batches and their times:
  `prec reloc tyinfo regmap pp lex scope`+global 3.9 s,
  `pfconv binsel enc opinfo peep parse` 4.0 s, `type` 27.9 s. type on its
  own takes ~28 s, so the next heavy stage should go in a new batch.
  Since abi step 5 there are four batches: 5.3 s (7 stages + global),
  4.0 s, 28.0 s (type), 12.8 s (abi).


## Status (2026-10-02)

The 2026-09-25 development log (what was proven per stage, capacity steps, audits) moved to
[archive/iterate/construct-log-20260925.md](../../archive/iterate/construct-log-20260925.md).
It no longer passes on the current tables: research/r20-c99-seed-a-audit.md records `abi` rules over 96,
an `isel` class count of 107 and a `combo` difference. The C99 seed constructor of plans/v0.0.21.md (item 0)
lives in `seed/`; this directory is re-tested and reused only, not extended.

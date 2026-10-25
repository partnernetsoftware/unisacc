# exec-prune: measured leaf seal of the prune cell (stage prune)

Scope: the prune cell alone -- stream prune, input **tape.text** (the -O2 tape), output
**tape.text** (the pruned -O2 tape) (exec/pipeline/image-stages.tsv row prune; formats in
exec/pipeline/formats.md); the delta is `gen.py prune`, the product's shared/prune.net.

**Everything upstream is an input, not a dependency.** Every `NAME.input` is a recorded tape.text
fixture, captured once as the reference's `-O2 -S -o -` output on fixed corpus sources (21 of
exec/pipeline/keep-elf.txt, the same subset as exec-opt-e4 and exec-elf; the list is in
tests/reseal-prune.sh) and frozen here as bytes.  Nothing of the pp cell (exec/pp/,
exec/facts/pp-*.tsv, pp-autoinc, src/front_pp.c, tests/seals/exec-pp-literals) and no sealed
header under include/ (only include/unisa_sha256.h, run.c's hash) is read by this cell:
`dependencies.tsv` is the strace read-set of the cell's own chain, and both the capture and the
checker refuse any `exec/pp/`, `exec/facts/pp-`, `autoinc`, `src/front_pp.c`, `tests/seals/` or
other `include/` path in it.  The exec/facts/parse-*.tsv files in the closure are the tape
token/word/constant facts the prune delta is built from, not pp facts.

Cell (the same tools as exec-opt-e4): `exec/build/gen.py prune DELTA` -> `exec/c/tbl.py` ->
`exec/c/net.py` -> `run --check-net`; executor `exec/c/run.c` built with the host cc;
`run prune.net FIXTURE` (the executor's default step budget suffices for every sealed case).

**Reference (self-ruled choice, recorded here):** the frozen expected output of every case is the
bytes the Python referee `unisa.prune.prune_text` (unisa/prune.py -- the same referee
exec/prune/check.py qualifies the cell against) produced for the fixture at reseal time.  The C
reference compiler has no post-prune stream to compare with: its `-S` tape is the pre-prune tape
(src/back_image.c prunes only on the way to an image), so a C-side `-S` post-prune dump would be
a new product feature, not a seal.  Hence the referee is frozen as bytes: tests/reseal-prune.sh
runs it once, untraced (it is the reference, not a dependency; it appears in no closure row), to
write each case's `ref.stdout`; the gate (`tests/streamseal.cx`) replays ONLY the product's prune
net on the frozen inputs and requires byte-equality with the frozen `NAME.stdout`, and never
imports or runs the referee.  Every sealed case is accepted and byte-equal to the referee
(`ref-equal=1`); the reseal's `corpus` step replays all 96 keep-elf sources the same way
(recorded in the reseal's corpus.tsv, not sealed: 96 accepted / 0 refused / 96 byte-equal at
the 2026-10-21 reseal).

Sealed subset (recorded choice): 21 of 96 (the 8 examples/ plus every 7th of the rest).  A replay
costs ~0.02 s per case, so the gate bound is not the limit; the subset keeps the sealed bytes at
~3.7 MB (each -O2 tape fixture and its pruned output are ~85 KB) and mirrors exec-opt-e4 and
exec-elf so the three cells freeze the same sources.

Gate (tests/gate.sh job prune-seal, tests/streamseal.cx --gate, leaf config `cell.tsv`): frozen bytes (dependencies against
the checkout, closure.expected.tsv inventory, case input/stdout/stderr/model bytes,
identity.tsv), then a **replay**: the executor built from exec/c/run.c runs the sealed
`model-<sha>.net` on every frozen input and must give the frozen stdout, stderr and rc; then
the negatives (omitted rule, changed rule/executor source, changed fixture/record/model each
FAIL; every negative baseline re-check replays too).  Verification only: no stamp, no cache or
reuse grant.

Files: `cell.tsv` (stream-seal config incl. `referee_note`; checker is tests/streamseal.cx),
`streams.tsv` (one row per case: verdict, rc, input/stdout/stderr/model with length and sha256,
`ref-rc`, `ref-equal`), `dependencies.tsv` (role, path, length, sha256),
`closure.expected.tsv` (role, path), `measurement.tsv`, `identity.tsv` (core = sha256 of the
sorted dependency+case rows; proof = sha256 over verdict+stdout+stderr shas).

NOT covered: refusal cases (the corpus is all-accept; the checker's refusal path is exercised by
no sealed case; exec/prune/check.py's hand controls -- CR, wide numbers, host slots, oversize --
are not replayed here); -O0/-O1 tapes as inputs (the pipeline prunes the -O2 tape only); the
pp/lex/parse/opt/lower/enc cells and their closures; the driver/package route (`run --bundle`);
the seed-C construction route (seed/gen.c via exec/prune/gen-delta.sh, seed/tbl.c, seed/net.c)
-- the sealed network was compared byte-for-byte with a product build's shared/prune.net at
reseal time (tests/reseal-prune.sh SHARED_PRUNE), not re-derived by the gate; the referee's own
correctness (it is the frozen oracle, qualified separately by exec/prune/check.py).

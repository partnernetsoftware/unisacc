# exec-opt-e4: measured leaf seal of the opt cell (stage e4, -O2)

Scope: the opt cell alone -- stream e4, input **tape.text** (the -O0 tape), output **tape.text**
(the -O2 tape) (exec/pipeline/image-stages.tsv row e4; formats in exec/pipeline/formats.md); the
delta is `gen.py opt --o2`, the product's shared/e4.net.

**The front end is an input, not a dependency.** Every `NAME.input` is a recorded tape.text
fixture, captured once as the reference's `-S -o -` output on fixed corpus sources (21 of
exec/pipeline/keep-elf.txt; the list is in tests/reseal-opt-e4.sh) and frozen here as bytes.
Nothing of the pp cell (exec/pp/, exec/facts/pp-*.tsv, pp-autoinc, src/front_pp.c,
tests/seals/exec-pp-literals) and no sealed header under include/ (only include/unisa_sha256.h,
run.c's hash) is read by this cell: `dependencies.tsv` is the strace read-set of the whole
chain, and both the capture and the checker refuse any `exec/pp/`, `exec/facts/pp-`, `autoinc`,
`src/front_pp.c`, `tests/seals/` or other `include/` path in it.

Cell (the same tools as exec-lex-e1): `exec/build/gen.py opt DELTA --o2` -> `exec/c/tbl.py` ->
`exec/c/net.py` -> `run --check-net`; executor `exec/c/run.c` built with the host cc;
`run e4.net FIXTURE` (the executor's default step budget suffices for every sealed case).

Reference: `ua_ref -O2 -S -o - SOURCE` (tests/build_ref.sh export), the reference stream
exec/pipeline/run.py compares the -O2 tape with.  Every sealed case is accepted and byte-equal
to it (`ref-equal=1`); the reseal's `corpus` step replays all 96 keep-elf sources the same way
(recorded in the reseal's corpus.tsv, not sealed: 96 accepted / 0 refused / 96 byte-equal at
the 2026-10-21 reseal).

Gate (tests/gate.sh job opt-e4-seal, tests/optseal.cx --gate): frozen bytes (dependencies
against the checkout, closure.expected.tsv inventory, case input/stdout/stderr/model bytes,
identity.tsv), then a **replay**: the executor built from exec/c/run.c runs the sealed
`model-<sha>.net` on every frozen input and must give the frozen stdout, stderr and rc; then
the negatives (omitted rule, changed rule/executor source, changed fixture/record/model each
FAIL).  Verification only: no stamp, no cache or reuse grant.

Files: `streams.tsv` (one row per case: verdict, rc, input/stdout/stderr/model with length and
sha256, `ref-rc`, `ref-equal`), `dependencies.tsv` (role, path, length, sha256),
`closure.expected.tsv` (role, path), `measurement.tsv`, `identity.tsv` (core = sha256 of the
sorted dependency+case rows; proof = sha256 over verdict+stdout+stderr shas).

NOT covered: the -O1 delta (`gen.py opt` without --o2, the product's shared/o1.net: a second
cell with the same closure, not sealed here); refusal cases (the corpus is all-accept; the
checker's refusal path is exercised by no sealed case); the pp/lex/parse cells and their
closures; the prune, lower and enc stages; targets other than the host; the driver/package
route (`run --bundle`); the seed-C construction route (seed/gen.c, seed/tbl.c, seed/net.c)
-- the sealed network was compared byte-for-byte with a product build's shared/e4.net at reseal
time (tests/reseal-opt-e4.sh SHARED_E4), not re-derived by the gate.

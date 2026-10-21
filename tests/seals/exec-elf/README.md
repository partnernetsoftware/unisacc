# exec-elf: measured leaf seal of the elf cell (stage elf, target lnx/x86_64)

Scope: the elf cell alone -- stream elf, input **target.text**, output **image.bytes** (the
complete ELF executable) (exec/pipeline/image-stages.tsv row elf; formats in
exec/pipeline/formats.md); the delta is `gen.py enc --elf`, the product's lnx-x86_64/elf.net.

**Everything upstream is an input, not a dependency.** Every `NAME.input` is a recorded
target.text fixture: the reference has no target.text dump, so the fixture is the pipeline's own
`lower.net(prune.net(ua_ref -O2 -S -o - SOURCE))` (prune.net = `gen.py prune`, lower.net =
`gen.py lower --full`, both run UNTRACED as fixture producers by tests/reseal-elf.sh), captured
once on fixed corpus sources (21 of exec/pipeline/keep-elf.txt, the same subset as exec-opt-e4;
the list is in tests/reseal-elf.sh) and frozen here as bytes.  Nothing of the pp cell (exec/pp/,
exec/facts/pp-*.tsv, pp-autoinc, src/front_pp.c, tests/seals/exec-pp-literals) and no sealed
header under include/ (only include/unisa_sha256.h, run.c's hash) is read by this cell:
`dependencies.tsv` is the strace read-set of the cell's own chain, and both the capture and the
checker refuse any `exec/pp/`, `exec/facts/pp-`, `autoinc`, `src/front_pp.c`, `tests/seals/` or
other `include/` path in it.  exec/facts/enc-librarysymbols.tsv and enc-hostbridge-library.tsv
in the closure are library *symbol* facts of the encoder, not the library/header closure.

Cell (the same tools as exec-opt-e4): `exec/build/gen.py enc DELTA --elf` -> `exec/c/tbl.py` ->
`exec/c/net.py` -> `run --check-net`; executor `exec/c/run.c` built with the host cc;
`run elf.net FIXTURE` (the executor's default step budget suffices for every sealed case).

Reference: `ua_ref -O2 -b lnx/x86_64 -o IMAGE SOURCE` (tests/build_ref.sh export), the complete
ELF image the reference writes for the same source.  Every sealed case is accepted and byte-equal
to it (`ref-equal=1`); the reseal's `corpus` step replays all 96 keep-elf sources the same way
(recorded in the reseal's corpus.tsv, not sealed: 96 accepted / 0 refused / 96 byte-equal at
the 2026-10-21 reseal).

Sealed subset (recorded choice): 21 of 96 (the 8 examples/ plus every 7th of the rest).  A replay
costs ~0.01 s per case, so the gate bound is not the limit; the subset keeps the sealed bytes at
~2.6 MB (each target.text fixture is ~100 KB, each image ~25 KB) and mirrors exec-opt-e4's
corpus so the two cells freeze the same sources.

Gate (tests/gate.sh job elf-seal, tests/streamseal.cx --gate, leaf config `cell.tsv`): frozen bytes (dependencies against
the checkout, closure.expected.tsv inventory, case input/stdout/stderr/model bytes,
identity.tsv), then a **replay**: the executor built from exec/c/run.c runs the sealed
`model-<sha>.net` on every frozen input and must give the frozen stdout, stderr and rc; then
the negatives (omitted rule, changed rule/executor source, changed fixture/record/model each
FAIL; every negative baseline re-check replays too).  Verification only: no stamp, no cache or
reuse grant.

Files: `cell.tsv` (stream-seal config: `target`, formats, `encode_note`; checker is
tests/streamseal.cx), `streams.tsv` (one row per case: verdict, rc, input/stdout/stderr/model
with length and sha256, `ref-rc`, `ref-equal`), `dependencies.tsv` (role, path, length, sha256),
`closure.expected.tsv` (role, path), `measurement.tsv`, `identity.tsv` (core = sha256 of the
sorted dependency+case rows; proof = sha256 over verdict+stdout+stderr shas).

NOT covered: refusal cases (the corpus is all-accept; the checker's refusal path is exercised by
no sealed case; the encoder's exec/enc/neg-*.txt controls are not replayed here); the macho, pe
and arm64 targets (`gen.py enc --macho/--pe`, `gen.py enc/arm`: other deltas with other
closures, not sealed here); the pp/lex/parse/opt/prune/lower cells and their closures (the
producers prune.net and lower.net are inputs, re-derived by the reseal but not by the gate); the
driver/package route (`run --bundle`); the seed-C construction route (seed/gen.c, seed/tbl.c,
seed/net.c) -- the sealed network was compared byte-for-byte with a product build's
lnx-x86_64/elf.net at reseal time (tests/reseal-elf.sh SHARED_ELF), not re-derived by the gate.

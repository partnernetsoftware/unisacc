# exec-lex-e1: measured leaf seal of the lex cell (stage e1)

Scope: the lex cell alone -- stream e1, input **pp.text**, output **tokens.typed**
(exec/pipeline/image-stages.tsv row e1; formats in exec/pipeline/formats.md).

**The pp stage is an input, not a dependency.** Every `NAME.input` is a recorded pp.text
fixture, captured once as the reference's `-E` output on fixed corpus sources (examples/,
tests/c/, exec/lex/probes/; the list is in tests/reseal-lex-e1.sh) and frozen here as bytes.
Nothing of the pp cell (exec/pp/, exec/facts/pp-*.tsv, pp-autoinc, src/front_pp.c,
tests/seals/exec-pp-literals, the sealed headers) is read by this cell: `dependencies.tsv`
is the strace read-set of the whole chain, and both the capture and the checker refuse any
`exec/pp/`, `exec/facts/pp-`, `autoinc`, `src/front_pp.c` or `tests/seals/` path in it.

Cell (the same tools as exec-pp-literals): `exec/build/gen.py lex DELTA --typed` ->
`exec/c/tbl.py` -> `exec/c/net.py` -> `run --check-net`; executor `exec/c/run.c` built with
the host cc; `run e1.net FIXTURE` (the USLATTR1 side-car is stripped by run itself).

Reference: `UA_TYPESPELL=1 ua_tdump -dump-tokens FIXTURE -nostdinc` (exec/parse/mkdump.sh on
the tests/build_ref.sh export), the reference exec/pipeline/run.py uses for E1.  Accepted
cases are byte-equal to it (`ref-equal=1`); refusals are `reject: ...` on stderr, empty
stdout, rc 1..125, with the reference also refusing (`ref-rc` nonzero).

Gate (tests/gate.sh job lex-e1-seal, tests/streamseal.cx --gate, leaf config `cell.tsv`): frozen bytes (dependencies
against the checkout, closure.expected.tsv inventory, case input/stdout/stderr/model bytes,
identity.tsv), then a **replay**: the executor built from exec/c/run.c runs the sealed
`model-<sha>.net` on every frozen input and must give the frozen stdout, stderr and rc; then
the negatives (omitted rule, changed rule/executor source, changed fixture/record/model each
FAIL).  Verification only: no stamp, no cache or reuse grant.

Files: `cell.tsv` (stream-seal module config; checker is tests/streamseal.cx), `streams.tsv`
(one row per case: verdict, rc, input/stdout/stderr/model with length and sha256, `ref-rc`,
`ref-equal`), `dependencies.tsv` (role, path, length, sha256),
`closure.expected.tsv` (role, path), `measurement.tsv`, `identity.tsv` (core = sha256 of the
sorted dependency+case rows; proof = sha256 over verdict+stdout+stderr shas).

NOT covered: the pp cell and its closure; locations mode (`--locations`, pp.locations input);
the f1 side-car content (stripped, not compared); the driver/package route (`run --bundle`);
targets other than the host; the seed-C construction route (seed/gen.c, seed/tbl.c, seed/net.c)
-- the sealed network was compared byte-for-byte with a product build's shared/e1.net at reseal
time (tests/reseal-lex-e1.sh SHARED_E1), not re-derived by the gate.

# exec-net-prec: measured leaf seal of the prec net (per-net kernel cell)

Scope: **one constructed net of the classic 18-net grid, alone** -- net `prec` (binary-operator
precedence class; unisa/gold.py STAGES["prec"], 1 field `op` of 19 values, 1 head `y` of 11
classes, 11 units), run by the integer kernel `kernel/unisa_core.c` over its **whole gold domain**
(`weights/gold/prec.tsv`, 19 keys x 1 head = 19 decisions), with the decisions frozen as bytes.
It is the first cell of the per-net kind the 0043 net survey asked for: a net inside a stage does
not count toward the grid until it has its own sealed cell, and until this cell the only exhaustive
check of a net was the whole-grid self-test `kernel/unisa_self.c`, whose include list reaches
`kernel/unisa_model.inc` and through it `kernel/weight.pp.inc`.

**The split is an include-list reorganisation only.** No generated file and no weight byte changes:
the gate copies `kernel/unisa_core.c` and `kernel/weight.prec.inc` into a scratch directory beside a
one-net shim named `unisa_model.inc` (MODEL = the prec literal at offset 0, NSTAGE 1, the net's
shape as STAGE_M/H/OFF/MW/NCLS/VN), appends a self-test main that reads case lines and prints one
decision per line, and builds with the host cc.  `#include "unisa_model.inc"` inside the copied
kernel resolves beside the copy (quoted includes look in the including file's directory first), so
`kernel/unisa_model.inc` is never opened and no pp path is read (measured: strace of the chain; the
compile closure is self.c, the kernel copy, the shim and the weight copy, nothing else).  The shim
and the main live in tests/netseal.cx; nothing is written under kernel/.

**Reference:** the gold table itself.  The gate re-derives the cases from `weights/gold/prec.tsv`
at every check (field vocabularies from the `#field` lines, class lists from the `#head` lines after
the share tag, one case per key per head, indices in table order) and requires them to equal the
frozen `prec.input` byte for byte; the kernel's answers must equal the frozen `prec.stdout` (every
line `head key0 key1 key2 key3 want got`, then `19 decisions, 0 wrong`) and the frozen stderr/rc.
The net's shape (`net.tsv`) is re-derived too: fields/heads/mask width from the gold table, the unit
count from the weight literal's own length (the one H whose layout fills the literal exactly), and
must equal the frozen shape.  The class order of the gold `#head` line is the constructed net's
order (unisa/__main__.py gold_export writes S.heads in net order; `0 wrong` over the whole domain is
the gate's proof of that for this net).

Sealed subset: the whole domain (19 of 19 keys, 1 of 1 heads).  One case (`prec`), one model
(`model-<sha>.inc` = the frozen copy of kernel/weight.prec.inc, 1183 bytes).

Gate (tests/gate.sh job net-prec-seal, tests/netseal.cx --gate): frozen bytes (dependencies against
the checkout, closure.expected.tsv inventory, case input/stdout/stderr/model bytes, net.tsv,
identity.tsv), the derivations above, then the **replay** (the one-net kernel built from the
checkout's unisa_core.c and weight.prec.inc runs the frozen cases and must give the frozen
output/rc), then the negatives (omitted kernel row; changed unisa_core.c / weight literal / gold
table in a private root; changed cases / decisions / model in a scratch seal: each FAIL; every
baseline re-check replays too).  Verification only: no stamp, no cache or reuse grant.  ~3.4 s.

Closure (`dependencies.tsv`, roles kernel / model / reference): kernel/unisa_core.c,
kernel/weight.prec.inc, weights/gold/prec.tsv -- zero pp paths, no kernel/unisa_model.inc, no
weights/built.json, no header under include/.  The checker runtime (unisacc.com running
tests/netseal.cx, which reads the checkout's include/*.h for itself, 51 at the reseal) is the same
for every sealed cell and is not the cell's chain; the capture drops it, as every other seal's
capture never traced its runner at all.  The capture and the checker refuse any pp path, the shared
model files (unisa_model.inc, unisa_self.c, unisa_cases.inc, weight.pp.inc, dense.pp.inc,
weights/gold/pp.tsv, weights/built.json), tests/seals/ and include/ in a dependency row.

Files: `streams.tsv` (one row: verdict, rc, input/stdout/stderr/model with length and sha256,
`ref-rc`, `ref-equal`), `dependencies.tsv` (role, path, length, sha256), `closure.expected.tsv`
(role, path), `net.tsv` (the shape), `measurement.tsv`, `identity.tsv` (core = sha256 of the sorted
dependency+case rows; proof = sha256 over verdict+stdout+stderr shas).

Reseal: tests/reseal-net.sh PRODUCT OUTDIR NET (ungated; strace of `netseal.cx --produce`, then
--capture, --write-seal, --check-core, --negative; the author copies the reviewed folder).

NOT covered: the other 17 nets (each needs its own folder from the same tool; the checker is
table-driven by net.tsv and the gold table: multi-field keys up to 4 fields, multi-head nets up to
16 heads, mask widths above 1 are handled by the derivation but exercised by no sealed case yet);
`kernel/dense.prec.inc` (the net's own dense table, not a reference; not read); the product's
e3 stream (prec lives inside shared/e3.net in the product; this cell seals the kernel per-net
artifact, not the stream); the C front end's use of the prec table at run time (src/); the
`unisa run` route of the kernel (the gate builds with the host cc only, as the self-test does);
the generator (`unisa emit-kernel`, tests/kernel.sh) and the construction of the weights from the
gold table (`unisa build-weights`) -- the literal is taken as the checkout has it, frozen by sha;
refusal cases (a key outside the domain returns -1 from infer; no sealed case asks one).

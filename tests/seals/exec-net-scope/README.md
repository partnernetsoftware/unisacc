# exec-net-scope: measured leaf seal of the scope net (per-net kernel cell)

Scope: **one constructed net of the classic 18-net grid, alone** -- net `scope` (which symbol-table action a declaration kind in a scoping context takes; unisa/gold.py STAGES["scope"], 2 field(s) `ctx` of 6 values and `kind` of 5 values, 1 head `y` of 7 classes, 9 units), run by
the integer kernel `kernel/unisa_core.c` over its **whole gold domain** (`weights/gold/scope.tsv`,
30 keys x 1 head(s) = 30 decisions), with the decisions frozen as bytes.  It is a
cell of the per-net kind the 0043 net survey asked for (batch 2 after exec-net-prec and batch 1), sealed by
the same table-driven checker, tests/netseal.cx, without any per-net code.

**The split is an include-list reorganisation only**, exactly as in tests/seals/exec-net-prec/README.md:
the gate copies `kernel/unisa_core.c` and `kernel/weight.scope.inc` into a scratch directory beside a
one-net shim named `unisa_model.inc` (MODEL = the scope literal at offset 0, NSTAGE 1, the net's shape
as STAGE_M/H/OFF/MW/NCLS/VN), appends the self-test main, and builds with the host cc; the copied
kernel's `#include "unisa_model.inc"` resolves beside the copy, so `kernel/unisa_model.inc` (the
self-test's route to `kernel/weight.pp.inc`) is never opened and no pp path is read (measured:
strace of the chain at the reseal).  Nothing is written under kernel/; no weight byte changes.

**Reference:** the gold table itself.  The gate re-derives the cases from `weights/gold/scope.tsv`
at every check (field vocabularies from the `#field` lines, class lists from the `#head` lines, one
case per key per head, indices in table order) and requires them to equal the frozen `scope.input`
byte for byte; the kernel's answers must equal the frozen `scope.stdout` (every line
`head key0 key1 key2 key3 want got`, then `30 decisions, 0 wrong`) and the frozen stderr/rc.
The shape (`net.tsv`) is re-derived too: fields/heads/mask width from the gold table, the unit count
from the weight literal's own length, and must equal the frozen shape.

Sealed subset: the whole domain (30 of 30 keys, 1 of 1 heads).  One case (`scope`),
one model (`model-<sha>.inc` = the frozen copy of kernel/weight.scope.inc, 1362 bytes).

Gate (tests/gate.sh job net-scope-seal, tests/netseal.cx --gate): frozen bytes, the derivations above,
the **replay** (the one-net kernel rebuilt from the checkout's unisa_core.c and weight.scope.inc runs
the frozen cases and must give the frozen output/rc), then the negatives (omitted kernel row; changed
unisa_core.c / weight literal / gold table in a private root; changed cases / decisions / model in a
scratch seal: each FAIL).  Verification only: no stamp, no cache or reuse grant.

Closure (`dependencies.tsv`, roles kernel / model / reference): kernel/unisa_core.c,
kernel/weight.scope.inc, weights/gold/scope.tsv -- zero pp paths, no kernel/unisa_model.inc, no
weights/built.json, no header under include/.  The checker runtime (unisacc.com running
tests/netseal.cx) is not the cell's chain and is dropped by the capture, as for exec-net-prec.

Files: `streams.tsv`, `dependencies.tsv`, `closure.expected.tsv`, `net.tsv`, `measurement.tsv`,
`identity.tsv` -- same schema as exec-net-prec (net-seal-v1).

Reseal: tests/reseal-net.sh PRODUCT OUTDIR scope (ungated; the author copies the reviewed folder).

NOT covered: the other nets of the grid (each has or needs its own folder from the same tool);
`kernel/dense.scope.inc` (not a reference; not read); the product stream the net lives in; the front
or back end's use of the table at run time (src/); the `unisa run` route of the kernel; the generator
and the construction of the weights from the gold table -- the literal is taken as the checkout has
it, frozen by sha; refusal cases (no sealed case asks a key outside the domain).

# exec-net-combo: measured leaf seal of the combo net (per-net kernel cell)

Scope: **one constructed net of the classic 18-net grid, alone** -- net `combo` (the combined isel and ABI facts of a tape op on an OS and architecture; unisa/gold.py STAGES["combo"], 3 field(s) `op` of 87 values and `os` of 3 values and `arch` of 2 values, 15 head(s) `form` of 5 classes, `symbol` of 107 classes, `sysno` of 96 classes, `arg0` of 19 classes, `arg1` of 19 classes, `arg2` of 19 classes, `arg3` of 19 classes, `arg4` of 19 classes, `arg5` of 19 classes, `ret` of 19 classes, `gate` of 5 classes, `nrreg` of 4 classes, `argshape` of 5 classes, `retconv` of 5 classes, `winimp` of 12 classes, 121 units), run by
the integer kernel `kernel/unisa_core.c` over its **whole gold domain** (`weights/gold/combo.tsv`,
522 keys x 15 head(s) = 7830 decisions), with the decisions frozen as bytes.  It is a
cell of the per-net kind the 0043 net survey asked for (batch 2 after exec-net-prec and batch 1), sealed by
the same table-driven checker, tests/netseal.cx, without any per-net code.

**The split is an include-list reorganisation only**, exactly as in tests/seals/exec-net-prec/README.md:
the gate copies `kernel/unisa_core.c` and `kernel/weight.combo.inc` into a scratch directory beside a
one-net shim named `unisa_model.inc` (MODEL = the combo literal at offset 0, NSTAGE 1, the net's shape
as STAGE_M/H/OFF/MW/NCLS/VN), appends the self-test main, and builds with the host cc; the copied
kernel's `#include "unisa_model.inc"` resolves beside the copy, so `kernel/unisa_model.inc` (the
self-test's route to `kernel/weight.pp.inc`) is never opened and no pp path is read (measured:
strace of the chain at the reseal).  Nothing is written under kernel/; no weight byte changes.

**Reference:** the gold table itself.  The gate re-derives the cases from `weights/gold/combo.tsv`
at every check (field vocabularies from the `#field` lines, class lists from the `#head` lines, one
case per key per head, indices in table order) and requires them to equal the frozen `combo.input`
byte for byte; the kernel's answers must equal the frozen `combo.stdout` (every line
`head key0 key1 key2 key3 want got`, then `7830 decisions, 0 wrong`) and the frozen stderr/rc.
The shape (`net.tsv`) is re-derived too: fields/heads/mask width from the gold table, the unit count
from the weight literal's own length, and must equal the frozen shape.

Sealed subset: the whole domain (522 of 522 keys, 15 of 15 heads).  One case (`combo`),
one model (`model-<sha>.inc` = the frozen copy of kernel/weight.combo.inc, 45663 bytes).

Gate (tests/gate.sh job net-combo-seal, tests/netseal.cx --gate): frozen bytes, the derivations above,
the **replay** (the one-net kernel rebuilt from the checkout's unisa_core.c and weight.combo.inc runs
the frozen cases and must give the frozen output/rc), then the negatives (omitted kernel row; changed
unisa_core.c / weight literal / gold table in a private root; changed cases / decisions / model in a
scratch seal: each FAIL).  Verification only: no stamp, no cache or reuse grant.

Closure (`dependencies.tsv`, roles kernel / model / reference): kernel/unisa_core.c,
kernel/weight.combo.inc, weights/gold/combo.tsv -- zero pp paths, no kernel/unisa_model.inc, no
weights/built.json, no header under include/.  The checker runtime (unisacc.com running
tests/netseal.cx) is not the cell's chain and is dropped by the capture, as for exec-net-prec.

Files: `streams.tsv`, `dependencies.tsv`, `closure.expected.tsv`, `net.tsv`, `measurement.tsv`,
`identity.tsv` -- same schema as exec-net-prec (net-seal-v1).

Reseal: tests/reseal-net.sh PRODUCT OUTDIR combo (ungated; the author copies the reviewed folder).

NOT covered: the other nets of the grid (each has or needs its own folder from the same tool);
`kernel/dense.combo.inc` (not a reference; not read); the product stream the net lives in; the front
or back end's use of the table at run time (src/); the `unisa run` route of the kernel; the generator
and the construction of the weights from the gold table -- the literal is taken as the checkout has
it, frozen by sha; refusal cases (no sealed case asks a key outside the domain).

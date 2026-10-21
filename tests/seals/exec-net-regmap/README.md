# exec-net-regmap: measured leaf seal of the regmap net (per-net kernel cell)

Scope: **one constructed net of the classic 18-net grid, alone** -- net `regmap` (which machine register holds tape register rN on an arch; unisa/gold.py STAGES["regmap"], 2 fields `treg` of 8 values and `arch` of 2, 1 head `y` of 16 classes, 10 units), run by
the integer kernel `kernel/unisa_core.c` over its **whole gold domain** (`weights/gold/regmap.tsv`,
16 keys x 1 head(s) = 16 decisions), with the decisions frozen as bytes.  It is a
cell of the per-net kind the 0043 net survey asked for (batch 1 after exec-net-prec: binsel, tyinfo,
pfconv, regmap), sealed by the same table-driven checker, tests/netseal.cx, without any per-net
code; this net is a 2-field key whose head classes span both back ends (x86_64 and arm64 names in one class list).

**The split is an include-list reorganisation only**, exactly as in tests/seals/exec-net-prec/README.md:
the gate copies `kernel/unisa_core.c` and `kernel/weight.regmap.inc` into a scratch directory beside a
one-net shim named `unisa_model.inc` (MODEL = the regmap literal at offset 0, NSTAGE 1, the net's shape
as STAGE_M/H/OFF/MW/NCLS/VN), appends the self-test main, and builds with the host cc; the copied
kernel's `#include "unisa_model.inc"` resolves beside the copy, so `kernel/unisa_model.inc` (the
self-test's route to `kernel/weight.pp.inc`) is never opened and no pp path is read (measured:
strace of the chain at the reseal).  Nothing is written under kernel/; no weight byte changes.

**Reference:** the gold table itself.  The gate re-derives the cases from `weights/gold/regmap.tsv`
at every check (field vocabularies from the `#field` lines, class lists from the `#head` lines, one
case per key per head, indices in table order) and requires them to equal the frozen `regmap.input`
byte for byte; the kernel's answers must equal the frozen `regmap.stdout` (every line
`head key0 key1 key2 key3 want got`, then `16 decisions, 0 wrong`) and the frozen stderr/rc.
The shape (`net.tsv`) is re-derived too: fields/heads/mask width from the gold table, the unit count
from the weight literal's own length, and must equal the frozen shape.

Sealed subset: the whole domain (16 of 16 keys, 1 of 1 heads).  One case (`regmap`),
one model (`model-<sha>.inc` = the frozen copy of kernel/weight.regmap.inc, 2116 bytes).

Gate (tests/gate.sh job net-regmap-seal, tests/netseal.cx --gate): frozen bytes, the derivations above,
the **replay** (the one-net kernel rebuilt from the checkout's unisa_core.c and weight.regmap.inc runs
the frozen cases and must give the frozen output/rc), then the negatives (omitted kernel row; changed
unisa_core.c / weight literal / gold table in a private root; changed cases / decisions / model in a
scratch seal: each FAIL).  Verification only: no stamp, no cache or reuse grant.  ~3.5 s.

Closure (`dependencies.tsv`, roles kernel / model / reference): kernel/unisa_core.c,
kernel/weight.regmap.inc, weights/gold/regmap.tsv -- zero pp paths, no kernel/unisa_model.inc, no
weights/built.json, no header under include/.  The checker runtime (unisacc.com running
tests/netseal.cx) is not the cell's chain and is dropped by the capture, as for exec-net-prec.

Files: `streams.tsv`, `dependencies.tsv`, `closure.expected.tsv`, `net.tsv`, `measurement.tsv`,
`identity.tsv` -- same schema as exec-net-prec (net-seal-v1).

Reseal: tests/reseal-net.sh PRODUCT OUTDIR regmap (ungated; the author copies the reviewed folder).

NOT covered: the other nets of the grid (each needs its own folder from the same tool);
`kernel/dense.regmap.inc` (not a reference; not read); the product stream the net lives in; the front
or back end's use of the table at run time (src/); the `unisa run` route of the kernel; the generator
and the construction of the weights from the gold table -- the literal is taken as the checkout has
it, frozen by sha; refusal cases (no sealed case asks a key outside the domain).

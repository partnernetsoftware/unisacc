# LB-1 loader side stream (model selftest; not yet connected)

The finite authored rows in hostsymbols-{byte,result}.tsv run on the generic
executor. The input is `USLSYM1\n`, explicit target byte 1..4, program-mode byte0,
and **Tape bytes accepted by tapecheck**, the exact domain of this contract. Target mapping and strict output layout are defined
in research/c43-late-binding-step0.md. Output is withheld until EOF; sorted unique
slot requirements name only loader slots. `.hostcall` is an indirect-register
operation, represented only by its presence flag. This is not a full Tape grammar,
signature checker, symbol resolver or library/bindings migration.

Actual model evidence: 62 states, 8650 canonical network bytes; C seed and Python
constructors agree, and 15740 table/network observations agree. Dynamic C selftest
has 85 cases: 74 accept, 11 refuse, exact reference/model output for accepted cases.
Negative indices4/-1 on all four POSIX targets, Win-target refusal and mode refusal
are explicit. Provider-A/B resources contain different addresses: both side-stream
bytes and a digest-bound historical lower model's main output remain unchanged.
That lower observation is narrow frozen evidence, not current-product freshness.

Python unisa/hostsymbols.py is only the reference oracle. New behavior is a model
selftest run by the old product as a dynamic-C observation tool; no integrated
product green is claimed. A fresh product is required after driver/package wiring.

Out-of-domain `?\n` and `.hostaddr r0, 01` belong to TP-7; bare calls of this
model are not a validation API and must not be compared as though both stages
owned the same domain. The integrated driver must validate the exact bytes first
and fail closed if the validation model is absent.

## Step 2 adapter/driver evidence (base selftest, actually run)

Dynamic C covers two native binding controls, missing slot1/mask13, malformed
provider metadata, two TP-7 domain exclusions from a fixed data file, two driver
routes missing either required model, and three early output alias controls
(default a.out, explicit -MF, automatic .d). Two host-binding reference controls
agree. C/Python package declarations agree (1477 rows plus audit in seedpack).
No integrated new product is claimed yet: observer is the old 92236ce3 product;
the native driver used in route negatives is built from current source.

Source review caught late default/dependency output alias collisions; guards now
normalize those destinations before inference. Side receipts are atomically
renamed after successful lower, before binding; a binding failure may retain the
receipt. Filesystem/encoding errors earlier than that boundary cannot create it.
The existing main-image writer remains unchanged; failure of a later main-image
write may leave the already accepted contract receipt. No ABI/type qualification
or complete library entry-point migration is claimed.

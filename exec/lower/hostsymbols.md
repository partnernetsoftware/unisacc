# LB-1 loader side stream (model selftest; not yet connected)

The finite authored rows in hostsymbols-{byte,result}.tsv run on the generic
executor. The input is `USLSYM1\n`, explicit target byte 1..4, program-mode byte0,
and **validated Tape bytes**. Target mapping and strict output layout are defined
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

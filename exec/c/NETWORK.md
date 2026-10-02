# Constructed transition networks

`exec/pipeline/elf.sh OUT source.c ...` constructs six integer
threshold networks and runs them on the generic C executor. `NETWORK=0` selects the reference table route.
`exec/c/chain.sh` uses the same default and explicit override. Python constructs the models; it does not process
source during the six execution stages. The shipped `.com` now uses the constructed model route; this six-stage
command remains a development and differential-check interface.

For a state's finite observation function `f`, the constructor uses
`base=f(lo)`, hidden units `h_t(x)=[x>=t]`, and signed output weights
`f(t)-f(t-1)`. Only nonzero differences are stored. The two integer linear
outputs are the next-state and action-sequence IDs. Telescoping gives exactly
`f(x)` for every observation in the declared domain. Missing transitions are
`(-1,0)`; EOF is byte 256 and the empty stack symbol is -1. The stack domain
includes non-state PUSH symbols. This construction uses no training.

The current state selects a network bank. This is sparse evaluation of a
state-conditioned network, not a single dense matrix: with state one-hot
`s_q` and a sufficiently large `M`, each active bank's hidden unit is
`H(x-t+M*(s_q-1))`. Inactive banks contribute zero; the baselines are selected
by `s_q`. Runtime evaluates hidden thresholds and signed output sums directly;
it never expands the network into an answer table. A bank's thresholds strictly
ascend (the loader rejects anything else), so the units active for an
observation form a prefix: the kernel binary-searches its length and adds
exactly those weights. That is the same sum; no answer rows or prefix sums are
stored.

A stack bank's rows that send a continuation state `k` back to `k` with one
shared action sequence are a return: pop, then continue at the state the stack
names. That is control over an unbounded structure (the continuation stack),
not a finite decision, so the constructor records it as a declared return set,
`H 3 lo hi n base_next base_seq ret_seq m k1..km units...`, and builds units only
for the bank's other rows (for example the empty-stack row). The kernel answers
`(k, ret_seq)` for a declared `k` before evaluating units. In e3 this removes
the 2,461-unit return bank that dominated every procedure return; `--check-net`
still compares every observation, returns included, against the table. Output codes are integers,
not class logits or argmax. Observation modes and action programs remain
explicit declarations. The generator still contains hand-written compiler
rules, and network construction does not eliminate those rules.

`run --check-net TABLE NET` enumerates all observations of all states through
the same `transition()` used in production, including unreachable combinations
and missing transitions. It also requires identical actions, strings and
observation modes. The checker retains both loaded models for its one-shot
comparison; normal execution loads only one. `netcheck.py` exercises all three
observation modes, a non-state stack symbol, holes, acceptance and a deliberately
changed bias. Every subprocess has a 60-second timeout.

This proves the finite transition implementation equals its reference table.
It does not establish whole-C semantic correctness, full frontend coverage,
or a formal termination/simulation proof. Nor does it imply a smaller or faster
compiler: network weights, declarations, executor and library costs must be
reported separately. Historical size goals do not substitute for the current packaged byte ledger
in [research/model-bytes.json](../../research/model-bytes.json).


Historical runtime-build measurements and earlier switching status are preserved in
[the 2026-09-27 snapshot](../../archive/exec/c/NETWORK-runtime-history-20261002.md).

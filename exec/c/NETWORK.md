# Constructed transition networks

`NETWORK=1 exec/pipeline/elf.sh OUT source.c ...` constructs six integer
threshold networks and runs them on the generic C executor. The default
remains the table route. Python constructs the models; it does not process
source during the six execution stages. This is a development route, not
adoption by the shipped `.com`.

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
it never expands the network into an answer table. Output codes are integers,
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
reported separately. The few-KB executor goal and single-model `.com` remain
unfinished.

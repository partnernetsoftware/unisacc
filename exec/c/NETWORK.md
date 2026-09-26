# Constructed transition networks

`exec/pipeline/elf.sh OUT source.c ...` constructs six integer
threshold networks and runs them on the generic C executor. `NETWORK=0` selects the reference table route.
`exec/c/chain.sh` uses the same default and explicit override. Python constructs the models; it does not process
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

## Building the runtime with unisacc

`EXEC_CC=/path/to/a/native/unisacc exec/pipeline/elf.sh OUT source.c ...`
uses that compiler to build `run.c`. The loader now reads signed decimal model
integers directly, with checked 32-bit indices and 64-bit action arguments;
it does not depend on `fscanf`, `ferror` or `atoll`. Files are read in blocks.
Host libc IO and native POSIX gates are normalised to the same negative-errno
interface. This is OS adaptation, with no parsing or compilation rules.
Native Windows gates still lack full error classification (open failures
cannot identify ENOENT, and ReadFile failure can look like EOF). Windows
self-built runtime IO is therefore not claimed verified.

`nativecheck.sh` first builds a compiler with unisacc, then builds the runtime
with that compiler. It checks all six networks, compares 18 stage outputs on
hello/fib/token-name probes against the cc-built runtime, runs the existing
five error cases, checks signed-64 boundaries, and requires real file-size-limit
write errors to fail on all three runtime builds. The runtime checks its output write and
close results. The macOS full self-source route was also run with a compiler
built by `.com`: its network-built compiler equals the reference and reaches
N1=N2=N3. This still generates the existing C compiler, not an E7 replacement.


The same check now also sends `exec/c/run.c` through the six network stages.
Its resulting macOS ARM64 executable must equal the compiler-built runtime.
That network-built runtime then checks every model's finite domain and runs
all six stages on its own source. Every intermediate output, including the
second runtime image, must match the first route. Thus this measured route
rebuilds the executor itself, not only the old C compiler. Python still
constructs the models; it does not process source in these six stages. The
shipped `.com` has not switched to this driver, and frontend coverage remains
partial. `nativecheck.sh` is in the bounded local gate.

Size ledger, macOS arm64, both `-O2` (2026-09-27): cc runtime `__text` 16,620 B,
file 56,520 B, dynamic libSystem excluded; unisacc runtime `__text` 85,220 B,
file 115,746 B, carried library included. These include loading, verification,
IO and execution; they are not isolated core sizes and do not meet the few-KB
claim. Runtime memory is dynamic and is not measured by these file sizes.

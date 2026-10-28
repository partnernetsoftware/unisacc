# E1 finite dispatch boundary: actual network, independent rules

The byte-stream interface cannot represent twelve different next-byte classes
after the current byte is EOF. This batch does not invent those inputs or reuse
one EOF execution as twelve successful source cases. It evaluates the actual
deployed E1 network at the classification boundary instead.

`stream-e1-lex144-core.c` uses the existing loader and `core_transition` from
`exec/c/run.c` and `exec/c/core.c`. It loads the real `unisacc-stream2.com` and
the formally constructed, same-generation **located `warnlex`** network. The
image/O2 route selects this model in `seed/compilerpack.c`; the unlocated
`shared-e1` network is a different artifact and is refused by the test pins.

Before answering boundary queries, the adapter compares all decoded state
modes, observation domains, next-state decisions and expanded action sequences.
This handles the package's compact sequence encoding without assuming its Q
indices equal the text network's Q indices. The observed result was 654 states
and 168,896 state/observation decisions with identical expanded actions.

The CX test uses the pinned formal JSON's insertion order, plus the documented
table encoder rule that appends its sole absent target `HALT`, for state names.
Its expected labels come from hand rules for whitespace, newline, identifiers,
numbers, quotes, comments and punctuators, motivated by C99 6.4, 6.4.2.1, 6.4.4,
6.4.5, 6.4.6 and 6.4.9. EOF→skip and other→bad belong to the finite dispatch
contract. It checks all 144 gold rows against these rules, then checks the
actual NN decisions and actions against the rules. The gold table never
supplies the expected NN answer.

There are 144 dispatch queries and 24 further dot/slash lookahead queries.
For each EOF pair, the test emits `UNOBSERVABLE` for source reachability and
checks the actual EOF decision goes to `CNT0` without an `ADV` or `BYTE` action.
These are injected boundary observations; the twelve EOF pairs are not twelve
distinct inputs to the NN. A valid same-shape mutation of a diagnostic action
must fail the exhaustive model comparison with empty primary output.

Run each command separately from the repository root:

```sh
tools/bound 55 sh ./unisacc.com tests/stream-e1-lex144-core.c -O2 -fno-trim-libc -o out/e1-lex144-core.elf
tools/bound 55 sh ./unisacc.com tests/stream-e1-lex144.cx -- "$PWD" "$PWD/out/c43-streams-da4e-01" "$PWD/out/e1-lex144-core.elf" "$PWD/out/c43-source-1621-context-01/jobs/gen-warnlex/result.json" "$PWD/out/c43-source-1621-full-pc-01/models/warnlex.net" "$PWD/out/e1-lex144-fresh"
```

The 2026-10-28 local run used `out/c43-e1-lex144-core-02.elf` and
`out/c43-e1-lex144-boundary-02/`: 293 assertions, zero failures, empty batch
stderr. Its outer logs are `out/c43-e1-lex144-boundary-02.stdout` and `.stderr`.
The output directory retains the 168-query plan, actual NN answers, mutation
refusal and before/after identities of the candidate, stream, formal artifacts,
adapter, generic C sources and gold input. Root is `da4e7ef1…`, stream2 is
`68f08deb…`, formal JSON is `a9b31d5d…`, and formal text NN is `a7fe31e5…`.

This is finite E1 NN boundary evidence. It does not claim complete C99 lexing,
144 reachable source cases, eighteen-net `lex` inference, changed product
weights, foreign-host qualification or closure of the current E1 matrix. The
matrix remains unchanged pending review of the boundary contract. The current
`tools/sim.cx` accepts full pre-network delta inputs and coverage, and does not
provide actual-NN state/register injection; no simulator result is substituted
for the deployed network here. Phase times remain UNMEASURED.

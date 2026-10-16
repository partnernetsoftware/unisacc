# Gate diagnostic layers

`gatequeue.py` keeps the full suite list and its measured-duration scheduling.
Layers describe **the breadth of the claim**, not an expected runtime. A stage
test may be slow; the queue's elapsed-time history remains its scheduling input.

| Layer | First question answered | Examples |
|---|---|---|
| `contract` | Are declarations, formats, inventories and runner rules internally sound? | `dsl-ops`, `manifest-entries-pp`, `gate-infra` |
| `stage` | Does one compiler stage or subsystem agree with its reference? | `exec-pp-directives`, `exec-lower`, `seed-construct` |
| `pipeline` | Does a complete compile or product route produce the expected result? | `exec-chain-1`, `difftest-1`, `com-difftest-1` |
| `platform` | Does a target image, ABI or native runtime work? | `bigclosure-win-arm64`, `winposix`, `ccinterop` |

For a first diagnosis, list one layer and run selected names with the existing
bounded queue. Escalate from the relevant stage to its full pipeline and then
the affected platform. `--through-layer` includes every narrower layer; it is
useful for broad preflight, while `--layer` runs exactly one layer.

```sh
python3 tests/gatequeue.py --list-selection --layer stage
python3 tests/gatequeue.py --state /tmp/unisacc-pp-check --layer stage --suite exec-pp-directives --jobs 1
python3 tests/gatequeue.py --state /tmp/unisacc-preflight --through-layer stage --jobs 2
python3 tests/gatelayers.py --check
```

Use the usual `tests/term.sh env ...` wrapper and environment for real macOS
runs. An unfiltered `tests/release.sh --com` remains the only local full release
gate. A layer result is diagnostic evidence and never substitutes for it.

`gatelayers.py` classifies every suite returned by `gate.sh --list --com`.
The `gate-layers` job checks completeness and that the union of exact layers is
the original gate list. Adding a suite without a reviewed classification fails
that job. Layer selection preserves the original job order and the existing
per-suite time and input fingerprints.

Shadow tier planning uses the same gate list, declaration helper and stage map:

```sh
python3 tests/gateselect.py --com --tier dev --base HEAD^ --head HEAD --out /tmp/dev-plan.json
python3 tests/gateselect.py --com --tier merge --base HEAD^ --head HEAD --stage enc --ask opinfo
python3 tests/gateselect.py --root . --verify /tmp/dev-plan.json
MODEL_COM=/absolute/path/to/unisacc.com sh tests/stagerun.sh --plan-only E3
```

`dev` and `merge` scopes add to changed-input, smoke and predecessor obligations;
`candidate`/`release` require `--com`, keep the full list and reject narrowing.
Paper-stage aliases and the optional ask vocabulary live in `gateselect.tsv`;
suite stages still come exclusively from `stagemap.c`. An unmapped ask refuses.
A plan has `coverage_verdict=NOT_ASSESSED` and executes no jobs.

`commitgate.sh` prints the dev recommendation, changed-input closure and whether
it contains the suites observed in that invocation's gate log. A partial/missing
log is `UNKNOWN`, not evidence that all configured suites ran. The report has a
five-second bound; refusal or timeout is `UNAVAILABLE` and leaves the original
suite, coverage, exit-status and derived-check rules intact. Scratch commitgate
self-tests report `UNAVAILABLE` because they have no real gate universe.

`stagerun.sh` delegates that recommendation to the same selector and prints its
difference from the legacy C mapper selection. Execution continues to use the
legacy selection. `--plan-only` exits before queue state or execution is created;
the ordinary invocation retains its frozen-snapshot queue behavior.

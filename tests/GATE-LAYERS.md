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

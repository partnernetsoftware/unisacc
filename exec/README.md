# exec/ — model-driven compiler development route

**This file is a map of `exec/`: what each part is, who writes it and who reads
it.** The specification is [prd.md](../prd.md); the repository-wide map is
[ARCHITECTURE.md](../ARCHITECTURE.md); working rules are [AGENTS.md](../AGENTS.md).

| Path | What it is | Written by | Read by |
|---|---|---|---|
| `c/` | Driver, generic execution core, inference kernel, packaged-library runtime | hand | `make com`, the shipped `unisacc.com` |
| `c/asm/` | The same execution kernel as per-ISA assembly bindings | hand | the assembly route in `c/` |
| `pipeline/` | Stage routing used by construction and the development route | hand | `pipeline/run.py`, `pipeline/elf.sh`, the gate |
| `*/gen.py` | Offline constructors: turn a declaration into a network | hand | `unisa build-weights`, packaging |
| `*/check.py` | Per-stage checks for that constructor's output | hand | the gate (listed in `tests/gate.sh`) |
| `*/fixtures.json` | Inputs those checks compare against | hand | the matching `check.py` |
| `stamp.sh` | Content-keyed artefact cache for the harnesses (`$X`, `fresh`) | hand | every script here that caches |
| `build/` | Generated build output | build | nothing committed |

Authoritative detail, rather than restated here:

- the route and each stage's inputs, outputs, rules and limits:
  [prd §0 current pipeline](../prd.md#pipeline-design);
- the rule-source inventory: [rules.md](rules.md);
- building and the package contract: [c/BUILDING.md](c/BUILDING.md),
  [c/PACKAGE.md](c/PACKAGE.md);
- published versions, current candidate identity and every byte count:
  [prd §0](../prd.md) and the generated ledger
  [research/model-bytes.json](../research/model-bytes.json). **No byte count,
  network count or SHA is restated in this file** — `tests/numberrestatements.py`
  fails the gate if one is.

Historical E0–E7 interfaces and candidate measurements are in [the archived milestones](../archive/exec/README-history-20261002.md). Current acceptance is in [the release receipts](../research/README.md) and [prd.md](../prd.md).

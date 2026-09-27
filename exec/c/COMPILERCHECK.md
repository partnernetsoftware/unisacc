# Compiler driver gate parts

`compilercheck.sh` / `compilercheck.py` accept `core-modes`, `core-contracts`,
`core-dependencies`, `resources`, `language`, and the aggregate aliases `core`
and default `all`. `core` executes its three parts; `all` executes all five.
The aliases retain the entire previous assertion set, but are not bounded gate
units. A complete driver result requires all five gate receipts.

| Gate | Scope |
|---|---|
| `exec-driver-core-modes` | Network-built driver, version queries and 27 mode/optimization-level comparisons |
| `exec-driver-core-contracts` | Compatibility flags, stdin, output preservation, source/output IO and undefined-function checks |
| `exec-driver-core-dependencies` | Dependency resource-read ledger and token dump |
| `exec-driver-resources` | Macro/include resources, printf fallback, isolated ASM package and APE CLI/memory/native checks |
| `exec-driver-language` | Previous 19 language probes plus scalar prefix, conditional dereference and aggregate varargs; ASM network memory O0/O2 and native O2 versus host cc; three conditional rejection macros at O0/O2 |

The six conditional negative cases require rc 1, empty stdout and the observed
`error: not covered: ?: arms of different types` diagnostic ending in
`1 error generated.`. A signal, timeout or rc 3 resource failure cannot pass.

The two isolated ASM consumers use the same setup helper. The language part does
not construct unused C/reference drivers or an APE. Every part retains the
existing `elf.sh` -> `models.py` preparation and hash-verified model cache;
there is no new cache or result reuse. Set `UNISACC_MODEL_CACHE` and `UA` explicitly
for isolated runs. Run separate two-slot batches (never all five in one window):

```sh
./tests/term.sh env JOBS=2 UA=/path/to/private/ua UNISACC_MODEL_CACHE=/path/to/private/cache ./tests/gate.sh --suite exec-driver-core-modes --suite exec-driver-core-contracts
./tests/term.sh env JOBS=2 UA=/path/to/private/ua UNISACC_MODEL_CACHE=/path/to/private/cache ./tests/gate.sh --suite exec-driver-core-dependencies --suite exec-driver-language
./tests/term.sh env JOBS=2 UA=/path/to/private/ua UNISACC_MODEL_CACHE=/path/to/private/cache ./tests/gate.sh --suite exec-driver-resources
```

`scalar_prefix` and `conditional_deref`; ASM network memory O0/O2 and native O2 versus host cc; all three conditional rejection macros at O0/O2 |

The two isolated ASM consumers use the same setup helper. The language part does
not construct unused C/reference drivers or an APE. Every part retains the
existing `elf.sh` -> `models.py` preparation and hash-verified model cache;
there is no new cache or result reuse. Set `UNISACC_MODEL_CACHE` and `UA` explicitly
for isolated runs. Run a two-slot batch, then the remaining core part:

```sh
./tests/term.sh env JOBS=2 UA=/path/to/private/ua UNISACC_MODEL_CACHE=/path/to/private/cache ./tests/gate.sh --suite exec-driver-resources --suite exec-driver-language
./tests/term.sh env JOBS=2 UA=/path/to/private/ua UNISACC_MODEL_CACHE=/path/to/private/cache ./tests/gate.sh --suite exec-driver-core
```

`scalar_prefix` keeps the five unary-promotion `sizeof` outputs in the host
comparison. Its long-double/member widths are now ABI assertions: `__UNISA__`
requires both widths to equal `sizeof(double)` (the product F64 alias); host cc
requires the holder to match its own `long double`. This avoids comparing
unrelated host/product long-double ABIs while retaining the width checks and
exact 3.25/4.5 value comparison.

## Measured scheduling

2026-09-28 arm64 macOS, private freshly built reference and existing model cache:
profiling the former monolithic core reached its outer 55 s limit. Before that
limit, the first 13 reference-compiled C-driver calls alone took 20.5 s; the
system-cc driver completed 31 calls in 3.0 s. This is a partial runtime profile,
not a complete timing attribution. It motivated splitting repeated mode checks
from CLI/error contracts and dependency/token checks, without weakening them.

Terminal two-slot receipts after the split: core-modes **39 s**, core-contracts
**40 s**; next batch core-dependencies **17 s**, language **36 s**; resources
**26 s** in its own batch. All five parts returned rc 0.
Language ran 22 positive probes and six strict rejection variants. Every measured
batch was wrapped in a 55 s process-tree watchdog. The earlier direct non-Terminal
resource/language attempt and monolithic core timeout are not counted as passes.
These are local driver checks, not other-platform or complete-project-suite evidence.

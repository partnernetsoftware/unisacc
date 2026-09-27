# Compiler driver gate parts

`compilercheck.sh` / `compilercheck.py` accept `core`, `resources`, `language`,
and default `all`. `all` executes the union; it is not the bounded gate unit.
No previous test was removed. A complete driver result requires all three parts.

| Gate | Scope |
|---|---|
| `exec-driver-core` | Network-built driver, CLI modes/levels, diagnostics, dependencies and token dump |
| `exec-driver-resources` | Macro/include resources, printf fallback, isolated ASM package and APE CLI/memory/native checks |
| `exec-driver-language` | All previous 19 language probes plus `scalar_prefix` and `conditional_deref`; ASM network memory O0/O2 and native O2 versus host cc; all three conditional rejection macros at O0/O2 |

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

2026-09-28 arm64 macOS measurement: private fresh reference, existing preparation
cache, two-slot Terminal gate: resources **28 s**, language **35 s**, both rc 0;
21 positive probes and six rejection variants ran. Direct non-Terminal execution
hit the outer 55 s limit before a receipt; it is not counted as passing. This
measurement adds no other-platform or complete-project-suite claim. A subsequent
standalone core gate also hit the outer 55 s limit without a receipt; therefore
this run does **not** establish an all-three-parts pass. The core checks themselves
were not removed or weakened by this split.

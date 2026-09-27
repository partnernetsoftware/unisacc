# Compiler driver gate parts

`compilercheck.sh` / `compilercheck.py` accept `core-modes`, `core-contracts`,
`core-dependencies`, `resources`, `language-1`, `language-2`, and the aggregate aliases `language`, `core`
and default `all`. `core` executes its three parts; `all` executes every assertion.
The aliases retain the previous assertion set, but are not bounded gate units.
A complete driver result requires all six gate receipts.

| Gate | Scope |
|---|---|
| `exec-driver-core-modes` | Network-built driver, version queries and 27 mode/optimization-level comparisons |
| `exec-driver-core-contracts` | Compatibility flags, stdin, output preservation, source/output IO and undefined-function checks |
| `exec-driver-core-dependencies` | Dependency resource-read ledger and token dump |
| `exec-driver-resources` | Macro/include resources, printf fallback, isolated ASM package and APE CLI/memory/native checks |
| `exec-driver-language-1`, `exec-driver-language-2` | The same 25 positive probes, split by alternating list positions (13/12); ASM network memory O0/O2 and native O2 versus host cc; all six conditional rejection variants in shard 1 only |

The six conditional negative cases require rc 1, empty stdout and the observed
`error: not covered: ?: arms of different types` diagnostic ending in
`1 error generated.`. A signal, timeout or rc 3 resource failure cannot pass.

The two isolated ASM consumers use the same setup helper. The language part does
not construct unused C/reference drivers or an APE. Every part retains the
existing `elf.sh` -> `models.py` preparation and hash-verified model cache;
there is no new cache or result reuse. Set `UNISACC_MODEL_CACHE` and `UA` explicitly
for isolated runs. Use the existing rolling queue with the six actual gate names:

```sh
./tests/term.sh env UA=/path/to/private/ua UNISACC_MODEL_CACHE=/path/to/private/cache python3 tests/gatequeue.py --state /tmp/private-driver-queue --jobs 2 --suite exec-driver-core-modes --suite exec-driver-core-contracts --suite exec-driver-core-dependencies --suite exec-driver-resources --suite exec-driver-language-1 --suite exec-driver-language-2
```

Each invocation has a maximum 55-second window. Exit 75 means unfinished work:
repeat the same command and state path. Exit 0 means every selected suite passed;
exit 1 means failure. Do not substitute `exec-driver-core`, which is no longer a
gate entry, or put the `core`/`all` aggregate CLI into one bounded gate window.

`scalar_prefix` keeps the five unary-promotion `sizeof` outputs in the host
comparison. Its long-double/member widths are ABI assertions: `__UNISA__`
requires both widths to equal `sizeof(double)` (the product F64 alias); host cc
requires the holder to match its own `long double`. This retains the width checks
and exact 3.25/4.5 value comparison without equating different host/product ABIs.

## Measured scheduling

2026-09-28 arm64 macOS, private freshly built reference and existing model cache:
profiling the former monolithic core reached its outer 55 s limit. Before that
limit, the first 13 reference-compiled C-driver calls alone took 20.5 s; the
system-cc driver completed 31 calls in 3.0 s. This is a partial runtime profile,
not a complete timing attribution. The split preserves all core assertions.

Terminal two-slot receipts for the split: core-modes **39 s**, core-contracts
**40 s**; next batch core-dependencies **17 s**, language **36 s** (22 positives);
resources **26 s** in its own batch. All five parts returned rc 0. After merging
the member-string implementation and registering its probe, language alone was
rerun: **38 s**, rc 0, **23 positives and six strict rejection variants**. The
other four parts were not rerun against that subsequent implementation change.

Every measured batch was wrapped in a 55 s process-tree watchdog. The earlier
direct non-Terminal resource/language attempt and monolithic core timeout are not
counted as passes. These are local driver checks, not other-platform or complete
project-suite evidence. The queue command above documents the existing scheduler;
the measurements used separate bounded `gate.sh --suite` batches.

The unsplit 25-probe language run reached 52.75 seconds on the combined source.
The two shards retain exactly the same list and assertions; `language` and `all`
still execute the full list for callers outside the bounded gate. Both shard
receipts are required. No test result or construction cache is newly introduced.

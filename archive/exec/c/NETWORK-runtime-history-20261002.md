# Runtime construction evidence (2026-09-27 snapshot)

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

The subsequent C-kernel isolation is measured directly as a separate object
in [CORE.md](CORE.md), including all generic action/storage helpers. The older
whole-tool ledger above remains historical; it must not be used as a current
core measurement.

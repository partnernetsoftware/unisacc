# Development and construction support

`sourceflat.cx` is the native complete-source exporter:

```sh
sh ./unisacc.com tools/sourceflat.cx -- out/unisacc-flat.c
sh ./unisacc.com tools/sourceflat.cx -- --verify out/unisacc-flat.c
```

The export and provenance verification use the product directly. `export_ref.sh`
is only a compatibility loader for existing construction callers.

`object-material.cx` prepares three real product-generated Linux unit objects
from two header-free CX sources. Preparation and verification are separate
commands; preparation requires a fresh absolute directory:

```sh
tools/bound 55 sh ./unisacc.com tools/object-material.cx -- prepare ROOT NEW_ABS_MATERIAL
tools/bound 20 sh ./unisacc.com tools/object-material.cx -- --verify ROOT MATERIAL
```

The literal sources are `extern int object_value; int main(void){return object_value;}`
and `int object_value=7;`. Actual product invocations use
`-O2 -c -funit -b TARGET INPUT -o OUTPUT`, producing `main-x86_64.o`,
`value-x86_64.o` and `value-arm64.o` for Linux x86_64/x86_64/arm64 respectively.
Each of the three compilation children is bounded at 8 seconds and must return
0 with empty stdout/stderr. Preparation also makes two fresh full-source
identity observations, each bounded at 8 seconds: explicit child budgets total
40 seconds within the original 55-second parent. It records actual child
arguments, rc and output streams as construction evidence.

The exact `material.tsv` record binds the full source identity, raw product,
tool and netpack module SHA, fixed flags/targets, both literal source files and
all three object byte hashes. Preparation rechecks source identity and input
content before recording PREPARED. Pure verification never compiles objects:
it checks exact literal bytes and receipt/content identity, independently
observes full source before and after, rereads the content record, and finally
rereads the receipt itself. The two source children retain 8-second bounds
within the original 20-second verify command. Neither repeated reads nor
matching endpoints establish an atomic snapshot or detect every transient
concurrent mutation; inputs must remain frozen during execution.

`out/c43-object-material-prepare-02.log` and
`out/c43-object-material-verify-02.log` record successful bounded MAT02
preparation and verification, with exactly
`PREPARED objects-material no-test-result` and
`VERIFIED objects-material no-test-result` status lines (tab-separated).
The tool SHA changed with the endpoint checks, so MAT01 evidence is not
transferred to MAT02. These are material PREPARED/VERIFIED observations only.
They establish no linked-program semantics, ARM execution, object refusal
contract or functional PASS; no unexecuted parent CASE is registered here.
Internal phase and cold/hot timing remain UNMEASURED.

`run-startup-bench.cx` measures first-observed and repeated default-run
end-to-end wall time on Linux/x86_64 LP64. It generates one header-free minimal
CX program and starts four fresh processes with different argv; each must
return0, have empty stderr and match its exact distinct stdout:

```sh
tools/bound 55 sh ./unisacc.com tools/run-startup-bench.cx -- "$PWD" "$PWD/out/run-startup-new"
```

NEW_OUT must be a new absolute directory. Each target process has an8-second
watchdog; full raw-source identity is separately measured before/after with
two8-second controls. All child budgets total48 seconds under the55-second
parent bound. The tool also checks product, identity-source, benchmark-source
and generated-fixture SHA before/after, and saves each sample's stdout, stderr,
rc and duration plus `samples.tsv`. Failed semantic or identity checks invalidate
the aggregate observation; there is no speed threshold or cached test verdict.

The current P53e2cd1f/S447f794a run,
`out/c43-run-startup-bench-01.log`, completed9 assertions with zero failures:

| Sample | End-to-end ns |
|---|---:|
| first | 2008658900 |
| repeat1 | 2007829223 |
| repeat2 | 2076205842 |
| repeat3 | 2026560291 |
| repeat median3 | 2026560291 |

The existing Linux `CLOCK_MONOTONIC` syscall supplies the timestamps; ns is a
representation unit, with clock resolution UNMEASURED. The measured interval
includes watchdog/spawn, compilation, execution and pipe capture, while output
checks and evidence writes occur afterward. Source-identity controls can warm
product/OS state, so first is not an OS-cold or cache-cleared observation.
Startup, preprocessing/modules, construction, loading, binding, inference,
user execution, output, validation and cleanup are not measured separately;
all remain UNMEASURED. Four samples do not estimate p95. This supplies the
current baseline for [C44-RUN-REUSE-01](../plans/v0.0.44.md), not a cache-hit,
phase-cost or acceleration claim. The run-reuse mechanism remains unimplemented.

`run-watchdog-bench.cx` independently compares a guarded resolved `/bin/true`
with one guarded fresh default-run of its own header-free CX fixture. The
fixture receives a fixed argv and must print the exact distinct `WATCHDOG_ARG`
marker; no-op must return0 with both streams empty. Both samples are real
executions, not reused results:

```sh
tools/bound 55 sh ./unisacc.com tools/run-watchdog-bench.cx -- "$PWD" "$PWD/out/run-watchdog-new"
```

Each target and each of the two full-source identity controls has an8-second
child bound:32 explicit child-budget seconds under the unchanged55-second
parent. Before/after hashes bind the full product, controller, netpack, watchdog
wrapper/C source, resolved `/bin/true` binary, generated fixture and identity
source; `/bin/true` realpath and complete raw-source identity are rechecked.
Each sample retains argv/environment, stdout/stderr, rc, duration, and its own
private `noop.trace.raw` or `cx.trace.raw`, explicitly passed through `np_run`
environment additions. Trace bytes and their SHA are retained without parsing
trace events into PASS or internal phase timings. Transport failures record rc
as UNAVAILABLE. Tracing overhead is included in these end-to-end intervals.

On P53e2cd1f/S447f794a, `out/c43-run-watchdog-bench-01.log` completed12/0 under
the original55-second bound. `samples.tsv` records no-op28015979ns and
CX1932617292ns, both with exact outputs MATCH. The raw trace timestamps are:

| Sample | bound_start | child wait returned | exit normal |
|---|---:|---:|---:|
| noop | 19394254.108306 | 19394254.118615 | 19394254.123554 |
| cx | 19394254.137433 | 19394256.053985 | 19394256.058899 |

This observation does not support a fixed two-second watchdog exit wait.
The traces expose child completion and cleanup event order, but do not measure
product startup, compilation, loading or inference separately; those phases
remain UNMEASURED. No no-op subtraction, speed threshold, statistical estimate,
cache result or acceleration implementation is claimed. The original four
startup samples above remain unchanged, and these single traced controls are
not an OS-cold or phase-cost benchmark.

`stream-pack.cx` prepares seven isolated stream artifacts from a candidate's
existing model bytes. Its shared runtime and the new partial contracts are
documented in [streams.md](streams.md). Preparation never records a test PASS.

[nets.md](nets.md) describes the native full-gold integer-kernel batch and its
identity/anti-vacuity controls. [assembly.md](assembly.md) tracks the native .S
construction path, its real encoding checks and the remaining gaps.
[kernels.md](kernels.md) specifies the new bounded construction steps and native
ELF-to-UNIKERN1 linker contracts; it does not claim a completed product rebuild.
[packages.md](packages.md) describes the pinned, source-checked migration of
unchanged P3 models to freshly built native kernels and a rebuilt driver.

The following support was separated from the archived test suite so that
construction is not broken by its removal: `bound`, `bound.c`, `cxbin`,
`safedel.cx`, `lib.sh`, `build_ref.sh`, `refshim.h` and `reffoot.h`.
These are transitional construction adapters, not the new test framework.

Remaining dependencies are explicit: the native watchdog cold bootstrap and
independent reference compiler still use a host C compiler; `cxbin` has an
existing C-oracle bootstrap for the deletion guard. The old watchdog's Python
cold path has been removed. A direct self-hosted run of the old watchdog source
currently fails on the uncarried `sys/prctl.h`; this is an active migration gap.
Existing model coordinators still containing Python are not described as native.

The construction source identity includes the ten support inputs bound, bound.c,
cxbin, safedel.cx, lib.sh, build_ref.sh, export_ref.sh, refshim.h, reffoot.h and
sourceflat.cx under tools/. The native seed/ident.c and compatibility coordinator
declare the same ordered list. A native isolated-tree regression independently
assembles path-NUL plus binary SHA256 payloads and checks each input mutation,
restoration, and missing/directory refusal (15 checks). Run it after constructing
the native identity tool:

```
tools/bound 40 sh ./unisacc.com -O2 -fno-trim-libc seed/ident.c -o out/ident
make identity IDENT_BINARY=out/ident IDENT_OUT=out/identity-fresh
```

This regression checks support-source binding, not the complete build pipeline or
Python runtime parity. Changed source identities invalidate old build manifests.

`cxlib/` retains five native support modules still imported by exec/ callers.
Eight exec/ module symlinks now point here instead of the archived tests tree.
The host watchdog and reference-export paths inside the modules use tools/.
These compatibility callers do not become the new test gate, and their historical
PASS records are not imported. Four caller-family dependency plans and a six-check
default-run support probe passed after relocation; complete historical suites
were not run. Existing external-reference dependencies remain explicit debt.

New `.cx` tools use module interfaces and the implicit runtime without manual
`#include`. Compiled-artifact caching is reserved for measured setup hotspots;
it must not be confused with caching test results.

`pp-gold-material.cx` is a local pp-rule-to-deploy-NN material constructor,
separate from the existing weight-reuse `net-material.cx` path:

```sh
tools/bound 55 sh ./unisacc.com tools/pp-gold-material.cx -- prepare ROOT NEW_ABS_MATERIAL
tools/bound 20 sh ./unisacc.com tools/pp-gold-material.cx -- --verify ROOT MATERIAL
```

Preparation derives six disjoint cubes for9 directives x2 defined states from
the fixed pp rule source, with sparse W2=1, and emits `pp.weights`,
`pp-observe.cx` and `material.tsv`. The142-byte NN uses the actual deploy
kernel's little-endian masks/sparse-entry representation. Its C mathematical
body is extracted exactly from the raw `KERNEL_BODY` in
`unisa/ckernel.py`, stopping before main. No Python interpreter or
`np_convert` runs, and no old weight value supplies the new cube weights.

MAT01's weight SHA is
`2fd7181593523ce306cbcf0825b2d6ebd5dde3f69cf17762e8beeb6ebf25e609`;
the actual C math-body SHA is
`463b02ddd20c31182cbf90458b7cf308992f903b931bbdd8d355d38edee52eb2`.
The receipt binds recipe, ABI, root/source, product/tool/source-text/support
hashes and actual material bytes. Both paths observe fresh source endpoints and
repeat applicable content reads; verify finally rereads the receipt. Verify
admits the fixed142-byte recipe field by field (12 masks, six ordered sparse
unit/head/class entries, W2=1), without calling construction or rebuilding NN.
An altered weight plus a synchronized unsigned receipt therefore remains
ineligible as the baseline.

The actual prepare55/verify20 commands completed rc0 in
`out/c43-pp-gold-material-{prepare,verify}-01.log`. Their statuses are only
PREPARED/VERIFIED no-test-result. Separate tests documented in
[tests/README.md](../tests/README.md) actually completed92/0 for new-weight
inference and23/0 for synchronized-receipt recipe refusal, each within55.
The former runs one real observation child8 between verify20 calls, checking
all18 keys/four logits, boundary refusals/recovery and a private valid W2=2
candidate's actual score change. The latter uses verify20+20 and final source8;
it performs no inference or reconstruction. Neither test verdict is cached.

The `.py` files remain C-source-text/rule dependencies. Full SID observations
also hash old weight files, and the bootstrap product is trusted; this is local
new PP weight-value independence, not complete Python independence or a claim
that the whole execution closure never reads existing model bytes. Current
P3/product/generated sources remain unchanged. No equality with the old NN blob,
all18-net reconstruction, atomic snapshot or foreign-host qualification is
claimed. The19 broad qualification gaps and unmeasured internal/cold/hot phases
remain.

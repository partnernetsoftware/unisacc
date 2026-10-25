# Development and construction support

The working source is Source329; installed P53 was built from S447 and is
currently a trusted bootstrap, not a newly built Source329 product. Material,
runtime and product qualifications below retain their named source identities.
Old PP92/23 and RI/host/source material observations are not transferred merely
because this checkout now contains the unsigned classic C math repair.

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

Source447 MAT01's weight SHA is
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

The unsigned classic table-math repair is exported by
`table-kernel-export.cx`, independently of NN weight construction:

```sh
tools/bound 55 sh ./unisacc.com tools/table-kernel-export.cx -- prepare ROOT NEW_MATERIAL
tools/bound 55 sh ./unisacc.com tools/table-kernel-export.cx -- --verify ROOT MATERIAL
tools/bound 55 sh ./unisacc.com tools/table-kernel-export.cx -- install ROOT MATERIAL
```

All three actual01 commands completed0. The tool extracts the current raw C
KERNEL_BODY, emits only core/self math with its own producer/template hashes,
and preserves39 model/cases/header/weight/dense files byte-for-byte with their
old producer provenance. Install admits exact old generated targets, records
the actual before/after source identities and uses per-file exclusive sibling
temporaries/fsync/rename. Two renames are not one atomic pair; partial failures
retain explicit recovery evidence. It does not forge a new candidate or bypass
the package constructor-input comparison.

`out/c43-table-kernel-math-01/installed.tsv` records Source329 and new
core/self hashes. The saved P53 ran all18 actual tables over8770 keys/22694
decisions, with28868 field refusals/recoveries and72 adapter refusals, all0,
in `out/c43-nets-u64-bootstrap-all-01.log`. Host C99/O2/shift sanitizer
ran the generated self-test with exactly22694 decisions/0 wrong. Permanent
`tests/table-mask.cx ROOT SAVED_BOOTSTRAP ABS_CC NEW_OUT words|all-nets`
uses separate55-second commands: words9/0 and all-nets8/0, in the corresponding
`out/c43-table-mask-*-01.log` records. The words mode extracts actual
functions without forcing unsigned types; sanitizer errors or NOT_RUN cannot
pass. These are classic-math mechanism checks on Source329 using trusted P53
and a nonhermetic host C foundation. They neither reconstruct18 NN parameters
nor qualify a new complete product. The old signed sanitizer failure remains.

The source-P3 pipeline has two distinct controllers:
`source-package.cx` produces native seed tools and model jobs;
`source-package-pack.cx` consumes their verified receipts and a separately
prepared native kernel generation. The commands below document the available
CLI and required order. They are not an execution-success report or a claim
that the complete package/new product is qualified.

Use canonical absolute paths for ROOT and all existing inputs; each NEW path
must be a fresh private absolute directory. Keep source and construction inputs
frozen while executing phases. Each command owns its own55-second watchdog;
do not aggregate the107 jobs under one watchdog or relax individual budgets.

```sh
tools/bound 55 sh ./unisacc.com tools/source-package.cx -- prepare-context ROOT SAVED_BOOTSTRAP ABS_CC NEW_SOURCE_CTX
tools/bound 55 sh ./unisacc.com tools/source-package.cx -- --verify-context ROOT SOURCE_CTX
tools/bound 55 sh ./unisacc.com tools/source-package.cx -- --list ROOT SOURCE_CTX
tools/bound 55 sh ./unisacc.com tools/source-package.cx -- job ROOT SOURCE_CTX JOB_ID
```

There are107 construction jobs: five `tool-gen`, `tool-tbl`,
`tool-net`, `tool-compilerpack`, `tool-pack` jobs, then34 model recipes
with three jobs each. After the corresponding native tool jobs, run
`gen-RECIPE`, `tbl-RECIPE`, `net-RECIPE` in dependency order;
`--list` supplies the exact34 recipe IDs. For example, shared-e2 uses
`gen-shared-e2`, `tbl-shared-e2` and `net-shared-e2`.
Generation invokes the actual C API as STAGE OUT.json FLAG..., table conversion
as INPUT.json OUT.tbl, and threshold construction as INPUT.tbl OUT.net.
Each job creates an exclusive job directory with its actual output, command
argv/streams/rc and canonical dependency receipt. Normal generator/net diagnostic
output is retained. Prior or partial outputs are refused and retained rather
than overwritten. PREPARED jobs do not contain a saved functional verdict.

The context binds source, saved bootstrap and its optional build record,
current root product and its optional build record, CC path/binary/version,
native identity helper, controller and support hashes. Its fixed raw-environ
policy removes SEED_GEN_GENERIC, SEED_GEN_TRACE, UNISACC_SEED_TRACE,
UNISACC_RF_CAPTURE and UNISA_ATTRIBUTES before child execution; both context
and job receipts carry that policy. Presence of an empty environment value is
also removed. A job retains a40-second native command plus two6-second source
observations:52 explicit child seconds, with all controller/IO work still
required to fit the unchanged55-second parent.

The host CC is an explicit construction dependency. Native pack compilation
requires the zlib development headers and library for its `-lz` argument.
The host CC, system headers, linker, compiler subtools, runtime libraries and
zlib remain declared nonhermetic host inputs. The local out/ CX CC adapter and
downloaded/unpacked development package used during development are private
host preparation artifacts; they are not an installed compiler feature,
automatic dependency resolver or portable product capability.

The pack consumer currently pins foundation source
`9c2e71ef2094773bcada4c27a39871f3d0f5947555eb3ebe338aaddf203427ba`.
A foundation change needs explicit consumer compatibility review and fresh
identity material. Prepare KERNEL_GEN separately via
[the native kernel steps](kernels.md), under the same current full source and
root build epoch as SOURCE_CTX. It must contain digest-verified identity/helper
and assembler material, both ISA source/object receipts and arm64/x86_64
UNIKERN1 blobs. Old source-generation receipts are not renewed by renaming them.

```sh
tools/bound 55 sh ./unisacc.com tools/source-package-pack.cx -- prepare ROOT SOURCE_CTX KERNEL_GEN NEW_PACK_CTX
tools/bound 55 sh ./unisacc.com tools/source-package-pack.cx -- model ROOT SOURCE_CTX PACK_CTX MODEL_RECIPE_ID
tools/bound 55 sh ./unisacc.com tools/source-package-pack.cx -- route ROOT SOURCE_CTX PACK_CTX TARGET
tools/bound 55 sh ./unisacc.com tools/source-package-pack.cx -- declare ROOT SOURCE_CTX PACK_CTX
tools/bound 55 sh ./unisacc.com tools/source-package-pack.cx -- pack ROOT SOURCE_CTX PACK_CTX
tools/bound 55 sh ./unisacc.com tools/source-package-pack.cx -- seal ROOT SOURCE_CTX PACK_CTX
tools/bound 55 sh ./unisacc.com tools/source-package-pack.cx -- --verify ROOT SOURCE_CTX PACK_CTX
```

After pack-context preparation, run the model command once for each of the34
recipe IDs, then the route command once for each of six TARGET values:
`lnx-arm64`, `lnx-x86_64`, `osx-arm64`, `osx-x86_64`,
`win-arm64`, `win-x86_64`. These CLI target IDs use hyphens;
the resulting route rows use OS/ARCH names. Each model phase verifies its
tool/gen/tbl/net ancestry and copies the actual paired table/network bytes into
PACK_CTX/models without reconstructing weights. Route phases bind the actual
image-stages declaration and seven verified stage models. The complete
inventory requires exactly34 model pairs, six route files and two private
kernel files, including directory/mode/full-content observations.

Declare then executes native seed/compilerpack with the verified routes,
shared O1/E2/nativeabi, model directory and kernels, writing declaration and
audit material. Pack executes native seed/pack with compression and explicit
header/kernel/predefine mounts, writing PACK_CTX/compiler.pkg. Each command
has one40-second native child plus source6 before/after; prepare additionally
uses the existing kernel-context check bound15. These bounds leave no promise
that arbitrary inputs or maximum ancestry fit55 seconds. Each phase must
actually complete under that same limit.

Seal rereads all107 job dependencies,34 pairs, six routes, kernel construction
receipts, actual declaration/pack command evidence, carried headers and full
package bytes. It writes exclusive PACK_CTX/model-inputs.tsv and package.tsv
with source-package-v1 identity. The consumer's final `--verify` is pure read,
rechecking content/source endpoints without generating NN/package bytes.
The foundation `--verify-context` checks context only; its own pack and
package-level `--verify` entries remain NOT_READY. Use the consumer CLI for
these later stages. This source-package path is separate from the pinned
P7 package-refresh migration and does not invoke archived aggregation gates.

The v1 root build epoch binds ROOT/unisacc.com and the presence/content of its
.build.json as well as the saved bootstrap. Keep those bytes fixed through
context, kernel-generation and pack verification. Installing another root
product or changing its build record invalidates this epoch even when other
source bytes match; begin new matching material instead of silently relabeling
old receipts. Saved P53/S447 can bootstrap current Source329 while their
identities remain separate. That use alone does not create a Source329 product.

A precompiled native controller may avoid repeated controller compilation.
Its code-cache identity must bind actual ELF bytes, compiler/options/bootstrap,
controller/modules/source and the applicable context; revalidate this identity
when reusing it. Every job/phase still executes and freshly validates its own
inputs, output and dependency receipts. A listing, context verification or
prior verdict cannot substitute for construction or functional execution.
Endpoint-local digest observations are discarded before the next fresh
endpoint; no PASS cache is provided.

PREPARED/VERIFIED package status is content/source consistency evidence.
Independent consumer behavior, source-to-model semantics, same-candidate
product construction and platform qualification remain separate. The34
delta-model recipes are distinct from the classic18 gold-table NN parameters,
whose existing numeric artifacts and .py source files remain full-SID inputs.
Trusted initial bootstrap, root-assisted initial measurement, unsigned
receipts, host dependencies and remaining Python-source/construction debts
must stay explicit; no complete N22, historical process proof, zero-Python
chain, foreign-host execution or whole-source atomic snapshot is claimed.

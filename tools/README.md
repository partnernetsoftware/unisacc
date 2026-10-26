# Development and construction support

The final measured working source is
`0d59237f3250b59514a55ff529c77b665033e5feb71061f6fcaded0c7ef925c0`.
Scanner repair and legitimate carried-header regeneration completed; cf2270 is
header-only intermediate evidence. Installed Root remains Source329. P53 raw bytes
were fully reconstructed and locally installed with a new record; the prior
S447 bootstrap and its original record remain explicitly preserved. Material,
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
context, kernel-generation and pack verification. A changed Root/record cannot
silently satisfy the original epoch, even with equal artifact bytes. Current
sourcepackage explicitly resolves that original epoch to the saved checkpoint
only after checking BOTH original raw and record hashes. Installing P53/S329
does not relabel PkgCTX03's S447/e084 epoch; its original bytes remain available.
Without that exact provider observation, prepare new matching material. Saved P53/S447 can bootstrap current Source329 while their
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

The retained Source329 material milestone has gen13's nine kernel phases at
actual rc0, with a fresh context and both blobs. The old `5c7ad479...` consumer's
PC01 prepare, all34 model copies and six routes completed at actual rc0,
including fresh two-endpoint verification and copying of warnparse material.
The original producer's outer142/no-PREPARED outcome remains unchanged.
PC01 declare01 instead reached outer55 rc142 during full input verification:
no phases/declare output was created and the native command was not reached.
The prior frozen consumer `56a0be5e2d321456c825d3f8997fcd598303295b62e095af714a2f12dfc12117`
was actually compiled native with -O2. New PC03 prepare, all 34 copies, six
routes, declare, pack, seal and --verify each completed rc0 under original
outer55 limits. The source-package-v1 compiler.pkg is 1588745B, SHA256
`1c6a503090ee8d4189d6326139fa4a18b5db14b9682f1db0949e8ee79519db3e`;
model-inputs.tsv SHA256 is
`fbabeffcc27d9ca0a677ce793aea7645aa8d30a97893e483755a3aa14c950b8b`.
PC01 declare142 and PC02 pack142 remain retained failures. PC02's same-SHA
package does not convert its incomplete outer run into success. PC03 material
is PREPARED/VERIFIED only, with no functional-test conclusion. The independent
old-SP full-package observation and current PC04 results are recorded below;
product-v2/installer qualification, a new candidate and same-candidate
selfhost03 completed58/0, with further function checks pending. Root is now locally installed P53/S329; the prior S447 bootstrap remains saved. Source329 candidate A was fully constructed under ctx699
and archived; current candidate B's new-context reconstruction and58/79 gates
completed. Local installation and GEN14 construction are complete; Source329 selfhost
now completed58/0 in run03 within the unchanged original30-second gate;
run01 timeout remains historical; fclose is now SOURCE_FIXED with explicit
working headers, while the new-source product is NOT_REBUILT.

The compiled `tools/source-job-verify.cx`, using the prior frozen SP module a8aebdd,
actually verified net-warnparse twice in
`out/c43-source-job-verify-warnparse-01.log`: each endpoint physically read 53
files with six ancestry jobs/one model, at approximately 0.58s per snapshot.
This pure-read material observation reconstructs no NN and does not rewrite
the original producer's142 outcome. The independent `tests/source-job-snapshot.cx` API gate was actually compiled
native -O2 and completed outer55 rc0 with 67 assertions/0 failures in
`out/c43-source-job-snapshot-01.log`. Both endpoints observed 53 files/six jobs/
one model; exact negative reasons, complete failed-output clearing and recovery
all passed. A compiled controller caches code only: every invocation still
physically reads/verifies the applicable inputs, never reuses a PASS. All
original limits and 19 broad gaps remain.

Current `cxlib.sourcepackage`407f5ef7 retains the same independent pure-read API
`sp_verify_job_snapshot(ROOT, SOURCE_CTX, JOB_ID, FRESH_SID, SECONDS, &result, why, cap)`.
FRESH_SID must come from an actual current identity-helper execution, not a saved
receipt. Success returns0 with sp_result material identities, witness, physical
read count and measured elapsed_ns; failure returns-1 with a bounded reason,
clearing every byte of a supplied result. SECONDS must be1..55 and does not add
time to the caller's outer limit. Each call uses a fresh forked child/view;
retained per-endpoint bytes do not survive as a future verification result.
The companion package snapshot API verifies a complete prepared package.
Both APIs prove unsigned observed consistency, not historical construction,
atomic filesystem snapshots, NN behavior or new-candidate qualification.

The prior a8aebdd SP module's independent full-package double observation
actually completed rc0 on PC03 in `out/c43-sp-package-probe-run-01.log`:
1004 physical files,107 jobs/34 models per endpoint,7744849441ns and
7753816590ns. Four SP endpoints alone take approximately31s, beyond the
original selfhost30 limit. Before candidate construction, the hybrid threshold
was therefore changed from1MiB to64KiB/provider-v3. No30/55-second limit,
physical input read or assertion is removed; no PASS is cached.

New frozen consumer SHA is
`dd6f5e6068289d04346f202035fb1ae3c84a10e254630c7ba5a650c7a06deb46`,
and sourcepackage SHA is
`407f5ef7bd8cf0b65d1086db12b36ab6e19627dc8a0ed5f2e4002df8eae22442`.
KS8ad, Source329 and foundation9c2 are unchanged. Current PC04 prepare,
all 34 model copies, six routes, declare, pack, seal and --verify each completed
actual rc0 under original outer55 limits, reusing verified NN bytes without
reconstructing weights. compiler.pkg is 1588745B with the same1c6a503... SHA
as PC03; current model-inputs.tsv SHA256 is
`b10f49023fbcd78b813ec2adc1df05e42cfa6cd068bf2e383b4a80b38131f822`,
and package.tsv SHA256 is
`431639124a24de143a82dbb455fceb3fd8fa815b48085d5199853138952b6e3f`.
New407/dd6 full-SP probe02 actually completed rc0:1004 physical files,
107 jobs/34 models per endpoint,5110674321ns and5088791923ns. Every content
read remains real; these observations do not count as SELFHOST passing.
The strict new-pin gate's native02 build completed rc0, then original outer55
execution recorded67/0 in `out/c43-source-job-snapshot-02.log`, with53 files
per endpoint at approximately0.55/0.52s. Old PC03 and prior67/0 logs remain
bound to their original identities. All material is only PREPARED/VERIFIED,
not functional PASS. Root is now locally installed P53/S329, while historical Source329
candidate A was fully constructed and archived. Current new-context candidate
B construction and58/79 gates completed, followed by actual local installation;
current GEN13/14 construction and original30 selfhost03 completed58/0,
including complete after-validation, retained as Source329 baseline only.
Current fclose is SOURCE_FIXED/PRODUCT_NOT_REBUILT, with338/0 using -I include.

Source329 candidate A was fully reconstructed under GEN13 product context
`699ccb6169f39f77868f0835cd5dd13b9ee1bd7c773fc938cc968cad19a0b00d`:
all five slices and wrap actually completed rc0. Its4911193B complete artifact
SHA256 is `53e2cd1f669c92fda2c16dd93a9021cd4db97f05c57ab2d1acf9a835cbac9857`,
identical to old Root bytes. This is a full source reconstruction observation,
not a timestamp refresh. Candidate version0.0.43, direct smoke16/0 and
historical-bootstrap candidate admission58/0 actually completed; see
`out/c43-source-candidate-13-01.log`. Old rb813/product2872/test0e7 were
committed as9abe0e86cf. Before B installation, Root retained its original
S447 bootstrap record, SHA256
`e0843c0c025f05e728c8584a7fd5db51965345316b65c6316815bd57787dbe45`.

The old installer legacy-header branch checked kernel context/embedded bytes
without complete P7 admission. New rbce7f/installer e37 drafts use explicit
schema contracts:mode2 accepts only legacy kind1 and runs full P7 BOM,
corecompat and v1 verification; mode1 accepts only source kind2; mode0 retains
its original contract. The relevant source58 and schema/invalid-BOM79 gates
now passed for candidate B; these are API controls, not positive legacy
qualification or successful installer execution.
After checking ctx699 and actual hashes, Root's CX archived12 old product/native
receipt files in
`out/c43-native-kernel-13/product-context-699ccb61-before-legacy-admission`
(`out/c43-product-13-a-archive.log`), preserving the nine ASM phases and NN.
Current GEN13 candidate B's five slices and wrap each actually completed
rc0 under original55-second limits, with context
`14954ce18cff8a1b087c22a60b56546b19a3fb149005240ff8333fb108b98cba`.
Its complete4911193B artifact retains P53's original raw SHA53e2cd1f...
Candidate B's version command completed rc0, direct smoke16/0 passed, and new
rbce7 source-candidate native02/outer55 completed58/0 in
`out/c43-source-candidate-13b-02.log`; this is independent new-context evidence.

The initial schema gate's72/1 in `out/c43-candidate-schema-13b-01.log` was a
test expectation error:the old BOM's unisa/ckernel.py row1042 has old SHA
`db184ffec27565ad0a6d725d49710e7818af7d8e686fb642b67c862b88503aa3`,
while the current template is
`d3400bfb0d8dab72c8851eec67fba9c732815cb46ac90d9466791be027e5b9cc`;
it precedes kernel/unisa_core.c row1115. Production correctly rejected the
first changed input with exact model-input-digest. The original red log remains.
The test retains all72 obligations, adding seven independent unique-old-row,
order, fixed-hash and current-template-stability prerequisites. Native02 and
original outer55 then actually completed79/0 in
`out/c43-candidate-schema-13b-02.log`:all three exact schema/BOM refusals,
A5 output clearing, recovery and unchanged-material checks passed. This is an
invalid-old-BOM probe, not positive legacy qualification or selfhost.

Before positive installation, Root retained its original S447/e084 record. Installer
e37's native coordinator actually built rc0 in
`out/c43-candidate-install-runner-build-01.log`. A real installer negative
control on the old known-P7 schema/BOM returned child rc2 with exact
model-input-digest path=unisa/ckernel.py. Root's own thin CX caller checked
that refusal and completed outer rc0 in
`out/c43-installer-reject-probe-run-01.log`. Complete Root/candidate hashes
were unchanged; all six candidate metadata/backup/staged paths were absent
with ENOENT specifically. No installation occurred in that negative control. This is neither positive
legacy qualification nor successful installation.
Current installation:commit10d2a688a4 contains full-schema rbce7/installer e37/
schema79 admission. The real installer completed original outer55 rc0 with
INSTALLED development-only-no-release in
`out/c43-candidate-13b-install-01.log`. Root is now P53/S329, rather than the
historical S447 bootstrap-only identity. Its full raw SHA remains
`53e2cd1f669c92fda2c16dd93a9021cd4db97f05c57ab2d1acf9a835cbac9857`,4911193B:
identical bytes were actually reconstructed, not timestamp-refreshed.
Current Root.build.json SHA is
`21704e50abc28930d9a0792fdb9f895aee49b13cd163dc783451784d3bb73ef5`,
record.commit is `10d2a688a491d6b4740377cbe46d9eaabec00ce1`, source is329,
and native SHA is
`3d75d3557afa43d441a8bdfe6a7cbc992161da90326bfd0d751e9e3d0a115a7b`.
An independent helper/Root-record check actually completed rc0 in
`out/c43-installed-329-record-check-01.log`.
GEN13/bootstrap-before-install.com preserves raw53 and the original e084/S447
record, identical to the checkpoint originals. PkgCTX03's original Root epoch
continues through the explicit checkpoint provider's two raw/record hashes;
its old epoch is not relabeled. Retained Source329 fixed-point baseline (not transferred to changed source):RB SHA
`699b0a2cc5ffb1a791c024aa0b2cb0c21a9e2ffe213e77f69c4535132ceebd92`,
product coordinator SHA
`68d55d5548922930d1223445098d7e8418539e6d5d84e1e08ddab85fdbacd15c`.
GEN13/14 each actually completed five slices/wrap (12 independent original55
phases), without reconstructing PC04/NN/ASM. Current GEN13 v2 context is
`1660a13221c0d3034b396af62d4621290ba8bb61deb173128582629595df091d`,
with saved447/e084 builder. GEN14 context is
`5cf8787fec970a934c9d50caa2e9ead3116ca19ccdff0d3e9e24d756c080c437`,
with Root329/21704 builder. Full raw remains P53.
Actual original30 selfhost completed rc0,58/0 in
`out/c43-selfhost-source329-03.log`; bash builtin time, including watchdog,
recorded wall26.414s/user7.701s/sys18.999s. All four SP endpoints physically
read107 jobs/34 models/1004 files each, with5072600419/5155174425/5069025671/
5094686385ns elapsed. Eleven after-checks of Root/record/original builders and
frozen files were OK. Runner source/flags/context profiles are preserved in out/. Assertions, all
four physical reads, after-validation
and the original30 bound remain intact; no PASS cache is used.
Original01 timeout142 (older c275/149 contexts) remains. Run02 only failed to
find /usr/bin/time (wrapper127), so never started the test; it is neither a
test failure nor PASS. This is a local native two-generation fixed point, not
HOST-seed three-stage N22, foreign native qualification or version completion.

Current state:SOURCE_FIXED / PRODUCT_NOT_REBUILT. Final measured working
source is `0d59237f3250b59514a55ff529c77b665033e5feb71061f6fcaded0c7ef925c0`
after precise scanner repair and legitimate carried-header regeneration.
Header-only cf2270b2e103ead1d13859489a34d6eb9e57a127eb0bef5f347b17822cb6c16e
remains intermediate source-header evidence (identity01/helper8 rc0). Installed Root
remains P53/record21704/Source329; no new-source product has been rebuilt or
installed. Passing Source329 selfhost58 is retained as a baseline/history,
never transferred to the changed source.

Current stdio source SHA is
`a6f5cc020e04e547f2668f162f736b5eac423bf1ec1e241406a745d06e2b01e0`;
include/sys/_exit.h SHA is
`1b35e59300e2120420d3261c7239159c08332c43d2efb87bde9ad2594ed88701`.
Permanent tests/stdio-close.cx is unchanged at
`7106b12654dec939d6657a71b95ad379cb0247c585273d5c119d9f9cf3ecc8b5`.
Old default-run actually returned1 with338 assertions/48 failures (24 rounds'
EOF/ENOSPC checks) in `out/c43-stdio-close-before-01.log`. Explicit
`sh ./unisacc.com -I include tests/stdio-close.cx -- ABS_NEW_OUT` uses the
working headers:original bound55 actually returned0,338/0, empty stderr in
`out/c43-stdio-close-source-01.log`. The minimal -I include probe also returned0
under original bound12 in `out/c43-fclose-devfull-source-01.log`, with EOF/-1,
ENOSPC28 and closed FD/EBADF9. These are source-header observations, not a new
candidate or evidence that Root's default embedded library headers changed.
The old Source329 red and clean host02 reference remain historical evidence.

The scanner is now precisely repaired:seed/unisa_libneed.c SHA
`5f0469bcb6268bf979d8bb872424ae238c8f991b51543337b6f4b2750b2fdbb1`.
Host CC compiled it rc0 with POSIX200809/c99/O2/Wall/Wextra and implicit
declarations treated as errors. out/c43-libneed-attributes-01.elf SHA is
`5c6a7756fa4de143a3e74ec14f392fc8ee736755ad55043cb976ace0fb2bb011`.
Actual original55 `out/c43-libneed-attributes-contract-run-01.log` completed
rc0,12 assertions/0 failures for flat-attribute exact spans/closure/refusal/
stability. The same implementation is now permanent tests/libneed-attributes.cx,
SHA `fd7fa200469f1f5aecb3f7567ba0ab7c77759e48d6f417a562295b53988c8679`
(only comments differ from the original out/ contract). New -I include
bootstrap default-run actually completed original55 rc0,12/0 in
`out/c43-libneed-attributes-permanent-run-01.log`:

```sh
tools/bound 55 sh ./unisacc.com -I include tests/libneed-attributes.cx -- ROOT LIBNEED_NATIVE NEW_WORK
```

Both construction.libneed-attributes and library.stdio-close are registered
as independent SOURCE55 batches/coverage. IMPLEMENTED records an implemented
entry, not execution/PASS; broad library remains NOT_READY.

New tools/carried-header-export.cx SHA is
`432bf38606d945a1acb8508e04ab784127842593c3ac2a51013019ac1c00acc4`.
Its -I include/O2 native build actually completed rc0. prepare/--verify/install
each independently completed rc0 under original55, with empty stderr in
`out/c43-carried-header-{prepare,verify,install}-01.log`. The complete old
header names/text/routing/LIBKEY/OFF/DEP/BODY/ROOT canonical oracle actually
passed. The tool legitimately installed generated kernel/unisa_headers.inc,
SHA `482d376cc8eb2c5152512a4ead9841f47b32e23b948024a10718eb82b5806d9f`.
Old e63 generated-header bytes remain in the capsule and new material's
previous files. Old/new complete libneed tables actually share SHA
`aea1293b96da78c6a0cfaf0354e47e840d414de41bdea735bead30d736c8c853`;
autoinc projection probe out/c43-autoinc-projection-probe-01.cx SHA
`6363a2235de1781dbeb150a34f5694b60cb02814bdb292557a3781054e3cd6a3`
actually completed -I include default-run original55 rc0,3/0 in
`out/c43-autoinc-projection-run-01.log`. Actual six-header ordering and Latin1
line rules produced byte-identical complete canonical projections both
including printf and excluding printf. Both complete include trees and all
support inputs were freshly stable at four endpoints. Together with the equal
complete aea129... libneed table, this is necessary-condition evidence for NN
adoption. Adoption remains unimplemented; neither complete PP-fact equivalence
nor completed reuse/new-product construction is claimed.

Final fresh working SID is
`0d59237f3250b59514a55ff529c77b665033e5feb71061f6fcaded0c7ef925c0`
(`out/c43-fclose-source-identity-02.log`, original helper bound8 rc0).
cf2270 remains the header-only intermediate measured identity. Root remains
P53/record21704/Source329; the new-source product is NOT_REBUILT/not installed.
At `out/c43-source329-origin-01`, all seven source trees/typekw/ten support
files and old Root raw/record are actually sealed and checked. The original
CTX03 helper rescanned capsule/root and returned the complete329 SID, rc0 in
`out/c43-origin-capsule-snapshot-sid-01.log` and capsule snapshot-sid.stdout.
The origin capsule is now complete:706MiB,19 parts plus final verification,
each under original55 with actual rc0. SEALED SHA is
`87f7d1fd1469d170556f8a0cc2861d27e14de05f687e97e4805db135268c3273`;
the original helper twice actually rescanned the full329 SID. See
`out/c43-origin-capsule-final-verify-01.log`. Old receipts/epoch remain intact.
Current Source0d GEN15 actually completed all nine phases under original55,
context `67fb6c9b0dacf0c385bf05e49b49a67493d34bdb489afd5f91589a6d3471cd3f`.
The two blobs match old4d6e(7096B（含40B头，body7056B）)/32ce(7144B（含40B头，body7104B）); this is PREPARED, not functional
PASS. Root/record/runner/builder-source after-checks were OK.

Historical origin SP API98848f9f... and wrapperbf9a331c... were actually compiled native
with saved CAP/root/unisacc.com plus absolute-I current include/O2. ELF SHA is
`de8511f0501a1bafb1e2038b3efa89352022d19dccaa0130b0442346f1d8588c`.
Its independent saved compiler is53/record21704/source329, not live Root raw.
--write-profile completed0, profile SHA
`4793f9d98c489b224f020cbc58d0f217c33500cb496147474d4fe73ce8a54bba`.
The first --snapshot completed original55 rc0 in
`out/c43-source-origin-snapshot-01.log`:origin329,107 jobs/34 models/1004
physical reads, execution_reads148 and wrapper6,elapsed6397320828ns. Current
executing SP988 is separate from origin specification407. This is fresh old
DAG verification, not current Source0d construction or test PASS. That initial point remains historical; the later indexed37-check gate is recorded below.

Earlier NI module SHA was
`8becdff327ca5bfed12bbee94367cf59cd3dd91ab68933d0fae66394a4627fac`.
Its native rebuild completed0; original55
`out/c43-nninputs-probe-run-02.log` completed6/0,7791 reads=current3888/
origin3888/support15. This covers all four trees at four endpoints,ten Cdeps,
directory modes; complete includes and origin DAG have separate domains.
Two native calls' internal reads remain UNMEASURED. elapsed3627775504ns,
reader3626988686ns,native_current1503902949ns,native_origin1455617847ns;
four tree observations each took approximately0.157-0.173s. Old NIcd50's
3.4738s first observation remains historical. These are not OS-cold/statistical
acceleration, complete PP equivalence or completed NN adoption.

Current indexed SP a832 uses new verifier ELF02 52fc/profile02 6acb.
The permanent origin gate completed original55 with37/0; its two positive
snapshots read107 jobs/34 models/1004 files each in5082765804/5145743979ns.
The old98837 logs and out/c43-sourcepackage-before-index-988.cx remain
historical. Current execution code and old origin specification407 are
separate identities; this verifies Source329 material, not new Source0d
construction.

tools/libneed-material.cx221119 actually prepared material; derived CX6b48
and nativeE4 actually built. Both complete include-table controls completed8/0:
39810B, exact SHA
aea1293b96da78c6a0cfaf0354e47e840d414de41bdea735bead30d736c8c853.
Native observations1.452/1.473s versus CX0.161/0.159s are individual timings,
not OS-cold statistics. fast-attrs01 retained12/0 from old fd7 test logic.
The later report-label correction to REFERENCE_C_SOURCE avoids claiming a
host-C compilation history for CX; that labelled version has not yet rerun.
No seed/header/Source0d/KGEN15 input changed for this optimization.

NI a95 and profile writer954 actually built and created the reader profile.
E4.cache.tsv (dfab...,3318B) binds compiled headers NI1ed1cb..., original
material, saved bootstrap, source and module identities. tests/nn-inputs.cx9217
native02 built and completed original55 with104/0, including two complete
positive comparisons at approximately1.14s each. The original93/1 log remains:
Root.com was non-ELF, so its exact expected shape refusal was corrected.
All93 obligations remain, plus11 shape/private non-executable ELF-clone and
unknown-artifact refusal premises. The clone is only read, never executed.
SP/wrapper/origin-test commit02bb4c614d and NI/material/profile/test commit
789e7d4c6f record these mechanisms.

The new adoption producer a546/native03 9457 actually completed prepare03,
pack01, seal01 and verify01, each within original55, for
out/c43-adopted-package-01. P3 is1589556B, SHA
e486e9523c30c6c4985050f12c5e6a4323f8f6d8d84b9088709083f83b3e52a5;
manifest20e5ac180c3e88a0fd415ca670a59e666731fa65d044a57948ade77657075a47,
receipt24ee9ffc4fb6e1b2d757b7f55e8dec03e424cf63804454fb6ff180e229adbbc3,
context2b1a53bd337b1899a76271139171372b16c71dcd11e913dec8b600c2173f45f4.
All34 NN and the declaration are explicitly ADOPTED from Source329.
All107 origin jobs receive fresh full-DAG verification, plus NI and current
GEN15 kernels; only pack is EXECUTED with current headers. Earlier prepare01/02
rc2 logs remain: the kernel suffix mapping was corrected from KGEN *.kernel
to private P3 kernels/arm64 and kernels/x86_64 without removing checks.
Raw kernels are7096/7144B, including40B headers (bodies7056/7104B).

out/c43-adopted-package-snapshot-01.log completed original55 rc0:
single-endpoint8.8639s, origin5.1836s, NI1.1473s, body9.0065s.
These timers have different scopes and must not be added. Four such fresh
endpoints are projected to exceed original selfhost30; a complete kind3
selfhost has not run, so this is not a measured selfhost failure. Next:
measure and reduce helper/controller overhead while retaining30, all four
full endpoints and all assertions; implement RB kind3/installer integration,
integrate the adopted-snapshot adapter with RB kind3, then
construct and qualify a new candidate. Package PREPARED/VERIFIED/snapshot
results are material evidence, not functional PASS. Installed Root remains
P53/Source329; current Source0d has no rebuilt candidate. The19 gaps,
foreign hosts, classic18 source construction and signing/publication
restrictions remain. The separately authorized csr private C44 v6 report of
approximately0.07s is not main43 qualification; its size+mtime shortcut is
not accepted into43, and -run reuse is not implemented in main.

Two separate SOURCE/lnx_x86_64/55 batches are now registered as
IMPLEMENTED, not PASS. Strict --list actually returned0 in
out/c43-adoption-testplan-list-01.log: metadata_valid1,98 batches,
79 implemented/19 planned/63 unprepared,113 coverage,
94 implemented/19 unresolved,executed0/NOT_EXECUTED. This parses metadata;
it does not execute batches.

* construction.origin-package: finite37-check origin gate, with materials
  source-origin, source-origin-verifier, source-origin-profile and
  logical-source-package. Actual CLI:
  sh ./unisacc.com tests/origin-package.cx -- ROOT CAPROOT LOGICAL_PC VERIFIER_ELF VERIFIER_PROFILE NEW_WORK
* construction.nn-input-applicability: finite104-check NI gate, with
  source-origin and libneed-reader. Actual CLI:
  sh ./unisacc.com tests/nn-inputs.cx -- ROOT SNAPSHOT_ROOT TRUSTED_READER

A measured native -O2 controller may reuse compiled code; it never caches
a verdict. These gates do not cover DAG-leaf/status/extra-directory/
execute-bit/parent-symlink/normal-v1-alias mutations or NI data/Cdep/domain
mutations, OS-cold behavior, foreign hosts or full PP equivalence.

The adopted-snapshot adapter also completed default-run development testing:
out/c43-adoptedsnapshot-probe-run-01.log, original55,15/0,
seven non-mutating negative controls and one full positive endpoint,
own51 reads/9.355677247s. This is module development verification, not RB3
integration or candidate qualification.

Historical registry evidence before the two new registrations: original20 --list completed rc0 in
`out/c43-fclose-testplan-list-01.log`, metadata_valid1,96 batches (77
implemented,19 planned,61 unprepared),111 coverage (92 implemented,19
unresolved),executed0/NOT_EXECUTED. Subsequent original20 --all actually
returned3 with empty stderr in `out/c43-fclose-testplan-all-01.log`:
metadata_valid1/111 coverage/92 implemented/19 gaps; SMOKE16/0 only,
95 non-smoke batches and76 coverage NOT_RUN,executed_subprocess_batches0.
Unknown/unrun entries are not PASS. Historical109 coverage/94 batches are not
an updated count after these two registrations. Implementation metadata does not replace actual tests,
NN adoption or new-candidate qualification. Local installation is not complete qualification.
The19 gaps, foreign hosts, classic18 weight-source construction and-run44
remain unfinished; signing/publication restrictions are unchanged. Complete
classic18 source work,19 broad gaps and foreign-host qualification remain open;
no signing/Draft/Latest/publication or implemented `-run` reuse is claimed.

`cxlib.kernelsha` provides `ks_sha256_hex(input, length, hex65)`:0 succeeds;
a negative Linux errno reports failure (`-22` invalid arguments, `-95`
unsupported platform/layout). Caller errno is unchanged; module-local errno
is not a cross-module error ABI. Failure clears the first output byte when the
output pointer is non-NULL. It requires Linux/x86_64-LP64 AF_ALG hash/sha256,
creates fresh transform/operation sockets for every call, and has no fallback
or digest/result cache. Each sendmsg update is at most256KiB; socket and message
layouts are checked, interrupted/short sends handled, and all owned FDs closed.

The old ce019 module's actual formal probe matched empty/abc/65MiB fixed
SHA256:load51028690ns and memory-hash380484652ns, measured separately.
The current8ad60 module corrects the explicit error ABI. Its own fresh native
probe in `out/c43-kernelsha-module-probe-run-02.log` also matched all three
vectors:load55760174ns plus hash386130270ns (441890444ns total for this input).
The earlier software4501887772ns is a single-point comparison, not OS-cold or
statistical acceleration evidence. Fresh source observation still yields329,
recorded in `out/c43-source-after-afalg-01.sha256`. Its permanent gate
recorded144 assertions/0 failures within outer55 in
`out/c43-kernelsha-regression-02.log`; the original142/2 first-run failure is
retained in `out/c43-kernelsha-regression-01.log`. This establishes a bootstrap
kernel-crypto mechanism, not new-product/P3 qualification or a measured
implementation of `-run` compilation reuse. Producer integration and PC02
still require their own actual execution; the19 broad gaps remain open.

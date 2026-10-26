# Tests

Source identity boundary: final fresh working SID is
`0d59237f3250b59514a55ff529c77b665033e5feb71061f6fcaded0c7ef925c0`,
after scanner repair and legitimate carried-header regeneration. Header-only
cf2270 remains intermediate measured evidence. No new-source product is built.
Root is now locally installed P53/S329, with a new actual construction/installation
record and unchanged raw bytes. The original P53/S447 bootstrap remains saved. Source329 candidate A was fully reconstructed
under ctx699, checked and archived. Current new-context candidate B was fully
rebuilt and passed direct smoke16/candidate58/schema79, then was actually
installed development-only. GEN13/14 reconstruction and original30 Source329 selfhost03 completed58/0;
current fclose is SOURCE_FIXED/PRODUCT_NOT_REBUILT (explicit -I include338/0); the old
S447 provenance remains historical and is not relabeled.
P53/S447 results below, including PP92/23, RI and fixed-point observations, are
not transferred to changed source; Source32958/0 is also only a retained baseline. The PP low-mask observations remain valid within
their original scope; they are not relabeled failures.

## UJS product (`ujs.com`) — **不在本目录**

Host1 船闸在 **[`../ujs/tests/`](../ujs/tests/)**（与 unisacc 套件分离）：

```
./unisacc.com -run ujs/tests/run.cx -- --list
./unisacc.com -run ujs/tests/run.cx -- --smoke
./unisacc.com -run ujs/tests/run.cx -- --complete
```

host1 语料在 [`../ujs/tests/corpus|neg`](../ujs/tests/)。本树 [`ujs2wasm/`](ujs2wasm/) 仅留 archive_only（expect.json 等）。

## Native test entry and explicit gaps (unisacc CX)

This is a separate batch, with no imported PASS records and no legacy runner dependencies. It does **not** gate `ujs.com`.

From a checkout with unisacc.com:

```
sh ./unisacc.com tests/run.cx -- --smoke
sh ./unisacc.com tests/run.cx -- --list
sh ./unisacc.com tests/run.cx -- --all
```

Neither .cx source uses an explicit #include directive. The runner imports cases.product using the implemented #pragma unisa use/module mechanism and relies on the existing automatic .cx edition prelude for carried library declarations. One compiler invocation builds both source units in memory, and one process computes all sixteen outcomes and compares them with hand-written expected values. It never invokes Python, Clang, shell algorithms, old suites, or child compilers, and writes no executable artifact or result cache. The shell in the command only loads unisacc.com.

Exit status: smoke 0 means this local batch passed; 1 means an assertion failed; 2 means invalid usage, metadata or runner failure. --list and --all read coverage.tsv and batches.tsv through the same strict parser, resolving the checkout from the actual runner source path so another CWD is supported. --list validates and inventories metadata without executing batches. --all preserves the sixteen in-process smoke assertions, lists every other registered batch and implemented coverage item as NOT_RUN, and reports every unresolved obligation with its declared status. It returns 3 while this complete qualification is unavailable; a local smoke failure takes priority. Metadata errors are named refusals, not an empty inventory or a pass.

coverage.tsv separates implemented assertions from missing obligations; batches.tsv binds sources, modes, platforms, named materials, argument templates and per-command budgets. IMPLEMENTED is code availability, not a persisted PASS. New source bytes must actually run for fresh evidence. No preparation result is treated as a test pass. The inventory does not execute templates or spawn separate batches, and cannot silently use old material or test results.

The metadata-only commands are `tools/bound 20 sh ./unisacc.com tests/plan.cx -- --validate ROOT` and `tools/bound 20 sh ./unisacc.com tests/plan.cx -- --selftest ROOT`. The parser's 30 local assertions include malformed mappings, missing files, duplicate rows and borrowed-view callback lifetime controls. Metadata validity always reports executed_batches=0; it is not functional or host qualification.

The batch covers basic functions, recursion, struct arrays and pointer mutation, callback execution, loops, short-circuit behavior, overlap memmove, snprintf bytes and length, strtol value and tail, allocation, a published SHA-256 known answer, and imported module private state. It provides basic product behavior evidence on the current local host only.

Batch execution, validation, compiler startup and internal construction/loading/inference/output are explicitly UNMEASURED. The former sys/time.h dependency is removed; no unverified OS ABI declaration replaces it. A verified module time API and phase probes remain missing. An external wall-clock measurement may report the whole invocation but cannot substitute for missing phase probes.

Pointer mutation, callbacks, overlap operations and malloc/free are tests of the existing unsafe C boundary. The allocation fixture checks its allocation result and frees its buffer, but this is runtime fixture discipline, not a static safety guarantee. Existing module imports provide source organization; this MVP does not implement Rust ownership, borrowing, alias checking or memory safety guarantees.

A separate reusable runtime module lives in cx/owned.cx. Run `make owned` for its
551-assertion batch; tests/run.cx does not execute this separate batch. It checks
bounds, stale handles, move/drop, capacity and generation exhaustion. This is
additional partial library coverage, not a completed library or ownership gate.

Complete stream/interface, RI/resource, compiler diagnostics, six-target/host,
bootstrap, library/ABI and candidate CI obligations remain unfinished. The
separate eighteen-net local kernel contract is implemented; the main --all
entry marks it NOT_RUN until it is scheduled there. This MVP is not release qualification.

The separate `streams.cx` batch now covers part of the seven-stream and framing
obligations, with no imported legacy PASS records. Preparation, commands, actual
coverage and limits are documented in [tools/streams.md](../tools/streams.md).
Its 40 local assertions do not clear the remaining broad NOT_READY obligations.

The [eighteen-net batch](../tools/nets.md) uses the actual integer infer kernel,
strict full gold input and contextual field-boundary refusal/recovery. It passed
all eighteen nets locally, including three separate anti-vacuity controls. The
[assembly batch](../tools/assembly.md) checks the extended C encoder with native
default-run source adaptation and handwritten ISA byte/rejection oracles. These
separate batches do not inherit a persisted PASS in the main runner.

`sh ./unisacc.com tests/formats.cx -- ROOT ABS_NEW_OUT` is the separate six-image
container batch. It checks architecture, bounded load/section headers and the
Linux x86_64 return-7 oracle against the current root product; the other five
native hosts are explicitly NOT_RUN. The earlier fourteen checks on Pa7e8 remain
historical evidence. Current P53 was independently rerun with14/0 in
`out/c43-formats-installed-11.log`, including actual Linux x86_64 execution;
the other five native hosts remain NOT_RUN. This is partial format evidence;
full object/boundary/host obligations remain NOT_READY in the main runner.

The source tree is read-only while running. Apply a caller-owned watchdog of at most 30 seconds for this MVP; the runner performs no child process spawning and has no inner scheduling steps.

`sh ./unisacc.com tests/resources.cx -- ROOT ABS_NEW_OUT` exercises the current
product's resource capture and strict replay, checks exact output and CWD
independence, then mutates private ledgers for identity, field, key, blob and
embedded-NUL refusals. Each refusal requires rc2, its named reason and empty
stdout; a fresh replay of the original capture must recover afterwards. Empty
and NUL-valued extra entries check binary ledger loading; they do not prove
every consumer's presence/empty/absent behavior. An optional final native ELF
driver argument supports a narrow pre-packaging check against the root model
container. Both identities are checked for changes. This is partial RI evidence;
packaged-product snapshots, full lifecycle/resource budgets and foreign hosts remain NOT_READY.
The same optional argument also accepts a full packaged APE candidate, executing
it directly through its launcher before installation, with its own model package.

RI construction and execution are separate. [tools/ri.md](../tools/ri.md)
describes SC2 ordered capture/replay, SC1 map compatibility and the original C
substrate adapter. The registered `resources` batch retains its original
30-second budget. The separate `ri-trace` and `ri-multi` batches each have a
55-second budget and use distinct new output directories:

```sh
tools/bound 30 sh ./unisacc.com tests/resources.cx -- ROOT ABS_NEW_OUT
tools/bound 55 sh ./unisacc.com tests/resources.cx -- ROOT ABS_NEW_OUT --trace
tools/bound 55 sh ./unisacc.com tests/resources.cx -- ROOT ABS_NEW_OUT --multi
tools/bound 55 sh ./unisacc.com tests/resources.cx -- ROOT ABS_NEW_OUT --trace-output
```

`ri-trace` covers event order, missing/surplus events, strict recapture, metadata
refusal and constrained trace-to-map conversion. `ri-multi` checks mixed CX/C
translation units whose replies change, exact SC2 replay and an independently
executed return-7 image. These are separate obligations from the existing
ledger replay/refusal/blob rows; none is dropped or replaced. The separate
`ri-trace-output` batch retains its55-second budget and passed17 assertions
on the actual P53 packaged candidate. It checks exact primary/symbol replay, rejects an
unconsumed event before either output file exists, preserves SC1 --version/help
compatibility and refuses conflicting routes in both flag orders. These three
rows are supported by `out/c43-ri-trace-output-candidate-11.log`; registry
availability by itself is never execution evidence.

Prepare each C substrate once with the appropriate fixture, then execute its
permanent parent runner. MATERIAL is the corresponding absolute output directory
containing `runtime-native`, `runtime.c`, `material.tsv` and `native.tsv`:

```sh
tools/bound 55 sh ./unisacc.com tools/ri-material.cx -- --build ROOT NEW_ABS_CAPTURE_MATERIAL tests/cases/ri-fetch.cx
tools/bound 55 sh ./unisacc.com tests/ri-capture.cx -- ROOT CAPTURE_MATERIAL NEW_ABS_CAPTURE_RUN
tools/bound 55 sh ./unisacc.com tools/ri-material.cx -- --build ROOT NEW_ABS_CONSUMER_MATERIAL tests/cases/ri-consumers.cx
tools/bound 55 sh ./unisacc.com tests/ri-consumers.cx -- ROOT CONSUMER_MATERIAL NEW_ABS_CONSUMER_RUN
tools/bound 20 sh ./unisacc.com tests/core-compat.cx -- ROOT
```

`--build` records PREPARED material, not a functional PASS. It binds source,
nine direct inputs, full bootstrap, generated C, explicit compile flags and
actual ELF bytes, with checks around the real compilation. Both parent runners
call `--verify-native` before and after execution, independently hash the binary,
and retain each fresh process's stdout/stderr/status. Material verification is
bounded to18 seconds per call; ordinary child cases to2 seconds, capture quota
to8 seconds, and the whole parent invocation to55 seconds. The single compiled
C substrate is reused; test outcomes are executed anew. Materials from changed
inputs must be regenerated, never silently reused.

`ri-capture` covers six positive groups with18 literal fixture assertions and
seven fatal cases, including actual4096-event exhaustion and exact named
refusals with empty stdout. `ri-consumers` covers45 fixture assertions for RI /
synthetic RES / disk precedence, opaque keys and bytes, owned blob/cache slots,
snapshot PRESENT/EMPTY/ABSENT/MISSING, strict SC1 map behavior, and the isolated
zero-length-copy allocation/growth contract. Strict missing has two additional
parent checks for rc2 and exact empty-stdout error. Synthetic RES lookup does
not establish P3 loading; the narrow empty-copy guard does not establish general
memory safety.

The `core-compat` module batch checks16 assertions for one pinned full-file
transition only: current guard is kind1, exact old reversal is kind0, and
side-byte/allocation/capacity/guard mutations are refused. Its receipt contains
the full current core and helper SHA. No old archived PASS is consumed.
Historically installed Root `P53e2cd1f/S447f794a` had a packaged candidate that passed
smoke16 and the four product-facing RI batches30/15/8/17, all with zero failures.
Fresh current-root materials were then built and verified: capture16 parent
checks /18 fixture assertions /7 fatal cases, consumer10 parent checks /45
fixture assertions plus strict-missing2, and the separated NN chain17 checks
(including2 PRECONDITION) all passed their original55-second outer bounds.
The logs are `out/c43-ri-{capture,consumers,standalone}-installed-11.log`.
These are actual new-root runs; the old capture04/consumer04 and repaired
prototype results are retained separately and were not transferred. The
core-compat module's16/0 source-eligibility observation is also separate from
production execution. No foreign-host or general memory-safety qualification
follows from these local checks.

The `ri-standalone` batch requires two separate materials: the actual C runtime
built from `tests/cases/ri-cli.cx` (`ri-cli`) and the static synthetic NN chain
(`ri-chain`). Each preparation has its own bound and new directory:

```sh
tools/bound 55 sh ./unisacc.com tools/ri-material.cx -- --build ROOT NEW_ABS_CLI_MATERIAL tests/cases/ri-cli.cx
tools/bound 12 sh ./unisacc.com tools/ri-chain-material.cx -- ROOT NEW_ABS_CHAIN_MATERIAL
tools/bound 12 sh ./unisacc.com tools/ri-chain-material.cx -- --verify ROOT CHAIN_MATERIAL
tools/bound 55 sh ./unisacc.com tests/ri-standalone.cx -- ROOT CLI_MATERIAL CHAIN_MATERIAL NEW_ABS_STANDALONE_RUN
tools/bound 55 sh ./unisacc.com tests/ri-material-controls.cx -- ROOT CLI_MATERIAL tests/cases/ri-cli.cx NEW_ABS_CONTROL_RUN CASE_ID
```

Its exact `ri.standalone-attribute-commit` contract runs two private synthetic
threshold NN models through the actual standalone C CLI. Valid replay must
produce the expected attribute bytes; an unconsumed trace event must refuse
before modifying an existing attribute sidefile or creating an absent one.
This does not qualify production E1/E3 models. Chain construction records only
PREPARED; its `--verify` is read-only and never converts the models again. The
v1 NN profile binds its recipe/module, netpack and material CLI hashes plus the
exact `exec/c/core.h` action ABI SHA256
`9d1eaeec134834433ad11fdb62be27bfb03a9c9b28d8c7058b191ec6edbcfe30`.
Both preparation and verification reject a changed action ABI; matching only
the converter or recipe is insufficient. Preparation remains PREPARED-only.
The parent verifies chain material directly through its module before and after
execution. The two NN structure checks are PRECONDITION, not functional PASS
assertions. Runtime verification remains two18-second calls, and each of the
four case processes retains its3-second limit within the original55-second
parent bound. Neither material preparation is folded into that parent budget.

The original prototype first reported17 assertions with two sideeffect
failures. A subsequent repaired development prototype completed17/0; that
observation did not transfer to the new parent. The new parent with separate
runtime/chain inputs was independently executed on the current root:17/0,
`out/c43-ri-standalone-installed-11.log`. Production source feature commit589db6c39f is
recorded separately; generation11 is installed as `P53e2cd1f/S447f794a`. Its current-root
observations bind actual runtime, chain and bootstrap identities; full product
and foreign-host qualification remain separate.

Native-material controls use the same `ri-cli` material but run one CASE_ID per
new output directory and55-second invocation: `control-valid`,
`native-byte-flip`, `missing-native-receipt`, `symlink-native`, `fifo-receipt`,
`corrupt-runtime.c`, or `corrupt-flags-receipt`. Seven separate registry batches
map to `construction.ri-native-material-controls`; one successful case cannot
qualify the other six. The runner first verifies the original material, copies
actual bytes and native permissions into a private directory, mutates that copy,
and requires the specific rejection reason with rc2 and empty stdout. Valid
control requires exact verification output. All four original hashes are read
again afterwards. Each tool invocation is bounded to18 seconds; the seven cases
are not combined under an expanded timeout. All seven were actually executed
on P53:70 checks with zero failures, `out/c43-ri-native-control-*-11.log`.
The initial valid01 failed during setup because an empty inherited capture
variable was rejected by the bootstrap; it did not reach a material mutation
and supplies no semantic control evidence. The runner now clears that variable
rather than injecting an empty value. Source, fixture, tool and bootstrap bytes
must remain fixed during any control run.

The seven `ri-chain-*` control batches separately run `valid`, `NNflip`,
`tableflip`, `receiptflip`, `missingnet`, `symlinknet` and `FIFOreceipt`, each
with a new private output directory and the unchanged55-second outer bound:

```sh
tools/bound 55 sh ./unisacc.com tests/ri-chain-controls.cx -- ROOT CHAIN_MATERIAL NEW_ABS_CHAIN_CONTROL CASE_ID
```

They share the exact `construction.ri-chain-material-controls` obligation;
each case still needs its own execution evidence. Verification reads only,
does not convert or prepare a new NN, and preserves the original five material
hashes. All seven were actually executed on P53:68 checks with zero failures,
`out/c43-ri-chain-control-*-11.log`. Native and chain control preparation or a
single valid clone cannot stand in for the other negative controls.

The four `ri-flow-*` batches use actual current P53 production P3 models, with
two explicit materials (`ri-flow-runtime`, `ri-flow-data`) and one CASE per
55-second parent command. [tools/ri.md](../tools/ri.md) describes their full
scope and identity contract. Prepare source, compile, seal and derive data as
separate bounded commands:

```sh
tools/bound 55 sh ./unisacc.com tools/ri-material.cx -- ROOT NEW_RUNTIME tests/cases/ri-flow.cx
tools/bound 55 sh ./unisacc.com tools/ri-flow-material.cx -- compile ROOT RUNTIME
tools/bound 55 sh ./unisacc.com tools/ri-flow-material.cx -- seal ROOT RUNTIME
tools/bound 20 RUNTIME/flow-native prepare ROOT/unisacc.com NEW_DATA
tools/bound 55 sh ./unisacc.com tests/ri-flow.cx -- ROOT RUNTIME DATA NEW_ABS_RUN CASE_ID
```

The initial generic command prepares source only; do not replace it with the
generic `--build`. Profile `compile` records COMPILED pending material, and
only `seal` records PREPARED. This profile uses default libc trimming with
explicit `-O2 -b lnx/x86_64 -o -` flags. DATA preparation runs the actual E2,
E1 and E3 models and binds all derived bytes and directory inventory, raw
product, native runtime and literal source identities. It constructs no new
weights and adds no P3 decoder. None of these preparation states is a test PASS.

Select `e3-empty`, `e3-absent`, `e3-missing` or `e3-to-lower`, each with a fresh
run directory. The first three share `ri.production-e3-presence`, which requires
all three cases: actual E3 SC2 events must show PRESENT-empty, explicit ABSENT
or MISSING fallback to real E1 attributes, followed by native exit oracles
0/0/7. The fourth maps to `ri.production-e3-lower-binding`: upstream snapshot
identity uses real E3 tokens, while lower capture uses the distinct actual
lower input after tapecheck/e4/prune. The final constructor image must exit7;
lower is not assumed to fetch attributes. Runtime module verification, complete
data identity and input/product/runtime hashes are checked before and after.

Profile compile and seal each completed within55 seconds and DATA preparation
within20 seconds. All four independent parents completed12 checks with zero
failures each under their original55-second bounds:
`out/c43-ri-flow-{e3-empty,e3-absent,e3-missing,e3-to-lower}-01.log`.
The generated ELF programs actually exited0/0/7/7. The first three runs checked
the real E3 attributes fetch: PRESENT with length0, ABSENT with length0, and
MISSING fallback PRESENT with length13 and exact E1 bytes. Lower capture
observed zero attributes fetches; its case proves the actual input identity
and production chain, not lower consumption of attributes. These functional
observations remain separate from preparation. The adapter retains actual C P3/model,
RI and snapshot implementation under its existing library guard, excluding the
standalone CLI. This is production-model consumer evidence, not installed CLI
integration, complete RI qualification or foreign-host execution. The broad
`ri` obligation remains NOT_READY. The separately registered fixed-P3 HOST chain
below does not turn this local RI evidence into complete N22 or foreign-host
qualification. Internal phases remain UNMEASURED.

The eleven independent flow-material control batches use the same separately
prepared materials above. Each command owns a fresh absolute output directory
and its own55-second limit; there is no aggregate invocation:

```sh
tools/bound 55 sh ./unisacc.com -O2 tests/ri-flow-controls.cx -- ROOT RUNTIME NEW_ABS_PROFILE_CONTROL CASE
tools/bound 55 sh ./unisacc.com tests/ri-flow-data-controls.cx -- ROOT RUNTIME DATA NEW_ABS_DATA_CONTROL CASE
```

The nine profile CASEs are `valid`, `unsealed`, `elf-flip`, `pending-flip`,
`final-flip`, `closure-field`, `old-v1`, `recompile` and `reseal`.
The required `ri-flow-runtime` material is a successfully sealed v2 profile:
`runtime.c`, `material.tsv`, `flow-native`, `flow-native.pending.tsv`
and `flow-native.tsv`. Pending COMPILED output alone and old single-step
receipts are ineligible. The test clones these five leaves, verifies the original,
then verifies or requests an immediately rejected compile/seal on the private
copy. It checks precise refusal names, empty stdout, unchanged originals and
unchanged private leaves after each command, including an absent final receipt.
It does not successfully compile, seal or execute a model.

Profile controls import the actual profile module and run its functions in a
child reexecution of the live test ELF. Linux `/proc` access is required.
The still-live parent's `/proc/PID/exe` is bound to `/proc/self/exe` by
device/inode/size, and each child checks the full executing-image SHA. Passing
literal `/proc/self/exe` through the watchdog would select the watchdog image.
This avoids recompiling a thin CLI for each child; it caches no artifact or
verdict. Both child commands retain18-second bounds. Each child writes
`original.status` or `control.status` alongside stdout/stderr, containing
the actual rc; a runner error without a reliable rc records UNAVAILABLE and
fails. The original valid01 failure and separate O2 probe remain development
failures with incomplete rc evidence; their precise causes are not established.
The live-image valid02 completed13 checks with zero failures within55 seconds,
with original/control rc0. The O0 elf-flip02 run subsequently failed its original
material prerequisite with actual rc142 and empty stderr, retained in its
original.status; it reached no negative-control assertion and is not a PASS.
All nine03 cases, explicitly invoking the controller with -O2 as above, then
completed133 assertions with zero failures (13 valid plus eight times15), each
within the original55-second parent and18-second child bounds. Their status
files retain actual original/control rc0/0 for valid and rc0/2 for refusals.
This optimization applies to the controller and its reused live ELF; no profile
tool, module, material or deadline was changed.

The twelve-column batch schema has no compilerflags field. SOURCE entry must
remain exactly the source path, and args describes runtime arguments. The table
is metadata without an executor, so -O2 is documented in this manual command
and the exact coverage description; it is not inserted into entry or args.
No future default dispatch configuration or automatic -O2 execution is claimed.

DATA CASEs are `extra-empty` and `declared-empty-remove`. Both require the
same sealed runtime plus separately PREPARED `ri-flow-data`. They clone every
regular file and directory, including empty directories, and reject special
leaves. The second case first establishes a valid private receipt containing
an explicit empty DIR row. An added undeclared directory or removal of the
declared directory must then yield rc90, empty stdout and the exact
`data identity or full inventory changed` reason; restoring only the directory
state must verify successfully. Three native verify calls each retain an8-second
bound. Original DATA tree, all five runtime leaves and raw product hashes are
checked before/after. Each DATA case completed15 checks with zero failures
within55 seconds in `out/c43-ri-flow-data-control-{extra-empty,declared-empty-remove}-01.log`.

The exact IDs are `construction.ri-flow-profile-eligibility` and
`construction.ri-flow-data-dir-inventory`. All nine and both cases respectively
belong to their finite obligations. These are material identity/refusal controls;
they do not add NN/RI functional coverage, installed CLI or foreign-host
qualification. The19 broad unresolved obligations remain open. Internal phase
and cold/hot timing remain UNMEASURED.

The `hostseed-fixed-p3` batch registers only
`bootstrap.fixed-p3-driver-lineage`, with explicit context, models, payload and
three-generation material inputs. Preparation uses one step per command and
one new context; all paths supplied to these tools must be absolute:

```sh
tools/bound 55 sh ./unisacc.com tools/hostseed-step.cx -- --bootstrap-ident ROOT NEW_CONTEXT /usr/bin/cc
tools/bound 55 sh ./unisacc.com tools/hostseed-step.cx -- --bootstrap-host ROOT CONTEXT
tools/bound 55 sh ./unisacc.com tools/hostseed-step.cx -- ROOT CONTEXT native CONTEXT/HOST MODELS PAYLOAD STAGE1 slice-lnx/x86_64
tools/bound 55 sh ./unisacc.com tools/hostseed-step.cx -- ROOT CONTEXT native CONTEXT/HOST MODELS PAYLOAD STAGE1 wrap
tools/bound 55 sh ./unisacc.com tools/hostseed-step.cx -- ROOT CONTEXT ape STAGE1/unisacc-seed.com MODELS PAYLOAD STAGE2 slice-lnx/x86_64
tools/bound 55 sh ./unisacc.com tools/hostseed-step.cx -- ROOT CONTEXT ape STAGE2/unisacc-seed.com MODELS PAYLOAD STAGE3 slice-lnx/x86_64
tools/bound 55 sh ./unisacc.com tests/hostseed-chain.cx -- ROOT CONTEXT MODELS PAYLOAD STAGE1 STAGE2 STAGE3 NEW_ABS_CHECK
```

Before each generation's wrap, run all five independent slice steps:
`slice-win/x86_64`, `slice-lnx/x86_64`, `slice-lnx/arm64`,
`slice-osx/x86_64`, `slice-osx/arm64`. The abbreviated commands above show the
Linux slice only; every generation also needs its own `wrap`, using that
generation's indicated builder. No wrapper aggregates the five compilations
under a relaxed timeout. Ident compilation has a15-second child bound, native
C HOST compilation35 seconds, each slice or wrap40 seconds; parent commands
retain55 seconds. The checker uses bounded read-only verification, not builds.

The native HOST is compiled by the explicitly resolved host CC from current
`exec/c/compiler.c`, using the C core with no ASM defines. Each five-target
driver is built from `exec/c/asmcompiler.c` at `-O2`; wrap executes `seed/ape.c`
through the actual builder's default-run, without `APE_ZLIB`. New receipts bind
full source identity, CC path/version/binary SHA, helper source/binary SHA,
actual builder kind/path/full SHA and its parent wrap receipt. They also bind
MODELS and PAYLOAD: the payload must exactly match the declared current P53
product's terminal P3 bytes. These are preparation receipts, not saved PASS.

The current tool SHA is
`293e069e71de8bdc8b27a62a70dbb8d452cdd47e9d192334840716f8c7080b1f`.
Its CTX02 identity helper and HOST were actually rebuilt, then all18 independent
slice/wrap steps of HOST-to-stage1-to-stage2-to-stage3 completed rc0 under the
unchanged55-second parent. All three full products are P53. The strengthened
checker in `out/c43-hostseed-chain-03.log` completed41 assertions with zero
failures: source and actual parent edges are reverified before/after, all five
stage2/3 slices and complete wraps are compared, and both stage1 and stage3
are compared byte-for-byte with the current root product. CTX02 chain02's40
checks and old CTX01 chain01's40 checks remain historical evidence; none is
silently upgraded to41. This is separate from the two-generation58 contract.

Single-step APE-parent admission now reads stage fields only to find ancestors,
then reconstructs canonical stage bytes and all six actual artifact/full receipts
from the context HOST forward. It binds source/context/HOST, actual parent edges,
models/payload, builder identities and exact flags. Two independent physical
reads and fresh source/dependency observations precede any new stage output;
the actual step's completion rereads the ancestry and compares its full witness.
Canonical-path, cycle and64-generation depth guards are implemented; cycle and
depth refusals have not been independently tested. These endpoint observations
do not establish atomicity or resistance to changes restored between reads.
The explicit child maxima are40 plus three source observations of6 seconds,
totalling58 before controller overhead, exceeding the outer55 hard bound. Actual
success of the18 finite steps does not promise that every permitted64-generation
ancestry or every combination of child maxima completes within55. The timeout
has not been raised, and no assertions were removed.

NN models and packaged ASM kernel binaries remain explicit trusted inputs;
their Python NN/source and ASM construction was not replayed. The initial full-source observation
uses the installed root before the new CC-built identity helper independently
remeasures it. Host CC subtools, linker, system headers and libraries remain
declared dependencies, not a hermetic toolchain proof. Unsigned receipts establish
consistency, not independent historical execution proof. The broad `bootstrap`
and planned `full-bootstrap` obligations remain NOT_READY: this finite fixed-P3
driver chain is not complete host-seed/N22 or same-candidate foreign-host
qualification. Internal phase and cold/hot timing remain UNMEASURED.

All new registrations describe available code, not qualification state.
The metadata inventory labels named-material batches UNPREPARED; an operator
must separately supply and verify the actual material. `--all` still lists
separate batches as NOT_RUN rather than executing them. Broad `ri` and
`resources` remain NOT_READY; full model routes,
all resource budgets and foreign hosts are not qualified. Startup, construction,
loading, inference, output, validation and cold/hot phases remain UNMEASURED.

`tools/bound 55 sh ./unisacc.com tests/snapshots.cx -- ROOT ABS_NEW_OUT` runs
the separate POSIX snapshot batch in a new absolute output directory. It prepares
the actual shared C ledger through `tools/ledger-material.cx`, executes that
source through default-run, then regenerates and verifies the material. The
adapter retains the original C implementation and binds the complete source,
bootstrap product and fixture SHA identities. PREPARED/VERIFIED material is
not functional PASS evidence or a CX rewrite of the shared implementation.

`out/c43-snapshots-07.log` records 265 assertions with zero failures and 87
allocation fault points. The three exact coverage IDs are `ri.snapshot-commit`,
`ri.snapshot-identity` and `ri.snapshot-lifecycle`: atomic no-replace publication
with COMMIT last, retained verified bytes without reopening, heap/fd/FILE release,
empty clearable output after failed transactions, and refusal/recovery for bad
input, duplicates, corruption, paths, FIFO, growth, readdir, link, close,
permissions and capacity boundaries. This is Linux x86_64 source-adapter evidence
using root bootstrap Pb8eae71b; it does not qualify snapshot behavior in the
installed product, a new rebuilt candidate, Windows or other native hosts.
The broad `ri` obligation remains NOT_READY, and the main runner does not
execute this separate batch. Internal phase and cold/hot timing are UNMEASURED.

`sh ./unisacc.com tests/optimization.cx -- ROOT ABS_NEW_OUT [CANDIDATE]`
compiles six Linux x86_64 images (-O0, -O1, -O, -O2, -O3, -Os), then executes
each image with runtime inputs 7 and 11 and checks every row against hand-derived
literal byte oracles. It also executes the same module through default-run for both inputs.
Fourteen cases cover integer arithmetic, negative division/remainder, unsigned
shift/wrap, narrow promotion, same/distinct pointer aliasing, loop control and
induction, switch/loop joins, short circuit, recursion and struct copy. No signed
overflow, invalid shifts or dangling pointers are used. A final `--tape` instead
runs the separate bounded Tape contract: default equals O0, O/Os equal O1, O3
equals O2; O1/O2 actually change the fixture; O4/Oz are named refusals.

CANDIDATE may be an ELF driver using the root model container or a complete APE
product tested before installation. Candidate/model bytes must remain unchanged.
The semantic batch emits real native images specifically to exercise image
construction and execution; it does not replace normal dynamic tool execution.
A final `--run` instead executes all six explicit flag spellings directly in
memory, with both runtime inputs, as its own <=55-second batch. The driver
narrow checks passed 242 native semantic, 194 direct-run and 17 Tape assertions. This is
partial optimization evidence, not the full old corpus, optimizer closure,
resource domain, same-candidate bootstrap or foreign-host qualification. The
broad optimization contract remains NOT_READY. All internal timing phases and
cold/hot breakdowns are UNMEASURED; this is not a speed benchmark. Use separate
new directories for each native, direct-run and Tape batch and a <=55-second watchdog.

The complete development candidate `Pb8eae71b` (SHA256 `b8eae71ba2651709dbd5c927dd4d774050c10b4cefed85da0ca3600dc79730e5`) passed
241 native, 193 explicit direct-run and 16 Tape assertions in generation 07.
The three `out/c43-optimization-candidate-{native,run,tape}-07.log` records print
the same full candidate hash. The installed root also passed the 24-check RI
batch and 16 smoke checks; `--all` remains rc3 with its full coverage gaps.
This is local Linux x86_64 development evidence, not release qualification.

`tools/bound 55 sh ./unisacc.com tests/properties.cx -- ROOT ABS_NEW_OUT [CANDIDATE]`
runs the separate `properties.uint32-finite` batch. The optional candidate must
be a complete APE product. O0 and O2 each compile the fixture once; each image
then receives all domain axes through runtime argv and evaluates six uint32
words `{0,1,0x80000000,0xffffffff,0x12345678,0xaaaaaaaa}` across all five
rotations `{0,1,7,16,31}`, plus loop bounds `{0,1,31,32}`. Forty independent
handwritten literal byte rows check modular affine inversion, byte-swap
involution, rotate inversion and bit weight, and ascending/descending loop
masks and visit counts. Invalid argc, uint32 overflow, rotation32 and loop33
must return 2 with empty stdout/stderr before evaluation.

`out/c43-properties-01.log` records an actual 100 assertions with zero failures
on complete candidate `P46cfa11a` (SHA256
`46cfa11a8fe4ad05e450f87ae95fdf76bd1be541e89e62f74f2632724ebb4e2f`).
Both optimization levels matched the literal gold and each other; candidate,
fixture, driver and generated image identities were checked for changes.
This is an exact finite Linux x86_64 sub-contract. The broad `properties`
obligation and its `full-properties` planned batch remain NOT_READY; full
properties, optimization and other native hosts are not qualified. The main
runner does not execute this separate batch. Internal phase and cold/hot
timing remain UNMEASURED.

The local two-generation rebuild check is `tools/bound 30 sh ./unisacc.com tests/selfhost.cx -- ROOT PRIOR_GENERATION PRIOR_PACKAGE FRESH_GENERATION FRESH_PACKAGE`. Both generations must already be constructed. The prior builder is the explicit saved `PRIOR_GENERATION/bootstrap-before-install.com` and its record; the fresh builder is the current root product. The checker validates each generation's kernel/assembler/source/object/package/slice/wrap receipts before and after the original artifact comparisons. P7 validates its complete flat BOM; source-package validates its complete construction DAG, with source job/model/read domains reported separately from the legacy flat model_inputs count. Each validation invocation owns a bounded set of actual artifact bytes for repeated receipt consumers, releases it on return, and starts the after-check with fresh physical reads. It retains no PASS or cross-phase identity cache.

For historical `P53e2cd1f/S447f794a`, generations11/12 passed all58 assertions under the original30-second bound: `out/c43-native-selfhost-12.log`. Both complete products equal the installed root byte for byte. Source,1241 material inputs and construction receipts are checked before and after; the after-check starts a fresh context and physically rereads inputs. Unsigned receipts establish consistency; the original baseline and prior builder's historical source were not replayed, and product-context-v1 omits the bootstrap. This is not host-seed N22 or foreign-host qualification.

The older `P46cfa11a/Sb9c0f819` generations08/09 also passed58 under30 seconds in `out/c43-native-selfhost-09-retained-01.log`; that evidence was not transferred to the new source. Their earlier142 failures remain failures. The exact coverage ID is `bootstrap.native-two-generation`; the full-bootstrap obligation remains NOT_READY.

Receipts are unsigned consistency records, not proof of historical process execution; product-context-v1 does not bind its builder. The saved prior builder's historical source and the original frozen7f709 source tree were not reproduced. The09 wrapped candidate needs no installation record; absence is explicitly NOT_RECORDED while its real construction receipts remain mandatory. This is neither host-seed three-stage N22 nor foreign-host execution qualification. Internal phase and cold/hot timing remain UNMEASURED.

The eight `hostseed-control-*` batches register the separate exact obligation
`bootstrap.fixed-p3-lineage-refusals`. They require the same prepared context,
full current terminal-P3 MODELS product, exact declared PAYLOAD and three distinct
complete generation directories as `hostseed-fixed-p3`. Run one CASE per fresh
absolute output directory:

```sh
tools/bound 55 sh ./unisacc.com -O2 tests/hostseed-controls.cx -- ROOT CTX MODELS PAYLOAD G1 G2 G3 NEW_ABS_OUT CASE
```

CASE is `duplicate-generation`, `swapped-generations`, `wrong-payload`,
`raw-models`, `wrong-native-builder`, `wrong-parent`, `duplicate-step` or
`parent-receipt-builder-field`.
The controller performs exactly one target command bounded at 20 seconds,
including that target's default-run compilation, plus two independent source
identity observations bounded at 8 seconds each. The explicit child budgets
total 36 seconds within the unchanged 55-second parent. No generation, HOST,
compiler or weights are rebuilt.

Before running the negative target, the controller reconstructs exact canonical
context, HOST, stage and artifact receipts against the actual source, parent
paths, parent receipt hashes and bytes. This positive prerequisite prevents an
unrelated pre-existing invalid receipt from faking a mismatch rejection.
Canonical receipts are checked again after the target. The controller also
records complete sorted before/after manifests for the context and all three
generation trees, including directories, regular-file hashes and modes, plus
MODELS, PAYLOAD, raw product, tools, control/parent/identity sources, host CC and
watchdog. Special input leaves are refused. The full raw source identity must
remain unchanged. Actual target stdout, stderr, rc and argument vectors are
retained. Each target must return 2, leave stdout empty and emit its exact named
reason; a rejected new output must remain absent.

`wrong-parent` clones the complete 13-leaf G2 generation and supplies the
wrong actual parent builder. `duplicate-step` clones the complete 13-leaf G1
generation and requests an already published step. Both writable probes target
private copies only and compare complete private manifests afterward, preserving
successful artifacts and receipts. The new `parent-receipt-builder-field` case
copies all13 G1 leaves, changes only its private wrap receipt's unique builder
hash to64 zeroes, and requests `invalid-probe` through that private product. It
requires exact `receipt-bytes-mismatch` before any new stage exists, plus complete
preservation of both original materials and the private already-mutated copy.
No compiler executes for `invalid-probe`.

The old CTX01 seven01 cases completed with zero failures under the original
55-second parent, 20-second target and two 8-second source-observation bounds:
60 assertions in total (five cases with 8 each, plus `wrong-parent` and
`duplicate-step` with 10 each). Actual logs are
`out/c43-hostseed-control-{duplicate-generation,swapped-generations,wrong-payload,raw-models,wrong-native-builder,wrong-parent,duplicate-step}-01.log`.
`duplicate-generation` used default-run compilation; the other six controllers
used explicit manual `-O2`. Each target's expected refusal returned actual rc2
with empty stdout and its exact named reason; each enclosing case returned rc0.
The two historical private-copy cases also preserved their complete manifests.
Those60 assertions are historical and do not qualify the new tool/context.
New-tool CTX02 `parent-receipt-builder-field`02 completed11/0 before the
chain checker's source changed to add its41st assertion; it remains that earlier
observation. All eight current-checker03 cases have now independently returned
actual outer rc0 with71 assertions and zero failures (five times8, twice10 and
one11), using tool
`293e069e71de8bdc8b27a62a70dbb8d452cdd47e9d192334840716f8c7080b1f`.
All eight controllers used explicit manual `-O2`, with unchanged55-second
parents, 20-second targets and two8-second source observations. Logs are
`out/c43-hostseed-control-{duplicate-generation,swapped-generations,wrong-payload,raw-models,wrong-native-builder,wrong-parent,duplicate-step,parent-receipt-builder-field}-03.log`.
These71 checks are fresh CTX02/current-checker evidence, separate from the
historical60/0 and earlier field-only11/0. The current CTX02 chain03 result is
41/0, including the first generation's full bytes versus the root; none of the
old chain40 results is silently upgraded.

The registry has no compilerflags field or executor. SOURCE entry remains the
exact source path and args remains a runtime argument template; the controller
`-O2` requirement is expressed by this manual command and coverage description,
not encoded as a dispatcher feature.

Current single-step admission canonically verifies the complete parent chain
before writing a new stage; the earlier weak-preflight limitation belongs to
the historical tool, not this current contract. The new field-mutation case
tests that precise former gap. Other parent-field mutations and the cycle/depth
guards are not claimed independently tested. These finite refusals do not establish hermetic host dependencies,
NN/ASM source reconstruction, historical process execution, complete host-seed
N22 or foreign-host qualification. The 19 broad gaps remain open. Internal phase
and cold/hot timing remain UNMEASURED.

The nine `objects-*` batches register three finite obligations:
`formats.linux-unit-objects`, `negative.unit-linker` and
`negative.unit-unsupported-targets`. Their named `objects` material is prepared
and verified independently; [tools/README.md](../tools/README.md) describes its
recipe and identity boundaries:

```sh
tools/bound 55 sh ./unisacc.com tools/object-material.cx -- prepare ROOT NEW_ABS_MATERIAL
tools/bound 20 sh ./unisacc.com tools/object-material.cx -- --verify ROOT MATERIAL
tools/bound 55 sh ./unisacc.com tests/objects.cx -- ROOT MATERIAL NEW_ABS_OUT CASE
```

Use a fresh absolute output directory for each CASE: `link`, `mixed-target`,
`missing-definition`, `duplicate-definition`, `old-unit-version`,
`unsupported-osx-x86_64`, `unsupported-osx-arm64`,
`unsupported-win-x86_64` or `unsupported-win-arm64`. Every batch is SOURCE,
Linux x86_64, bounded at 55 seconds. The controller command above uses default
compilation; no controller optimization option has been assumed or qualified.
The material constructor's explicit `-O2 -c -funit` object flags are separate
from controller compilation.

The header-free `main.cx` references `object_value` and returns it, while
`value.cx` defines it as 7. The constructor produces real product objects
`main-x86_64.o`, `value-x86_64.o` and `value-arm64.o`. The parent never
recompiles these source units. It independently checks literal ELF64 little
endian ET_REL=1, machine62/183 and bounded section/name/symbol/RELA tables.
ARM is a container-format observation only; native ARM execution remains NOT_RUN.
The `link` CASE requires real product linking, an independently checked
x86_64 executable and actual return7 with empty stdout/stderr.

Four linker controls require exact rc1, empty stdout, the complete literal
diagnostic and no created output: mixed Linux architectures, an undefined
`object_value`, a duplicate definition and an old unit version. The last
locates exactly one bounded UNISATAPE1 metadata section and its first
`.unit 2` record before changing only the version byte in a private object;
the original material and private mutation must remain unchanged by refusal.
Four other controls require the exact unsupported object-target diagnostic for
both architectures on macOS and Windows. Each negative CASE then links the
valid x86_64 objects and executes return7 as recovery.

Every parent verifies the material before and after, with two independent
20-second child bounds. A negative target is bounded at 2 seconds; the recovery
or positive link at 8 seconds and the generated executable at 2 seconds.
The old-unit metadata mutation is a bounded private precondition, not an
arbitrary whole-file first-match edit. Eleven original input hashes, including
the product, constructor, controller, support/identity source, receipt, literal
sources and objects, are checked before/after. Exact stdout/stderr, actual rc
and argument vectors are retained for each child. Timeouts do not satisfy a
refusal predicate.

The actual `link01` parent completed20 assertions with zero failures using
MAT02 within the original55-second limit, recorded in
`out/c43-objects-link-01.log`. Its real product-linked x86_64 image returned7
with empty stdout/stderr. The actual `mixed-target01`, `missing-definition01`
and `duplicate-definition01` parents each completed22/0; `old-unit-version01`
completed24/0. All five independent parents returned actual rc0 within their
original55-second limits, totaling110 assertions with zero failures
(20+22+22+22+24). Their logs are
`out/c43-objects-{mixed-target,missing-definition,duplicate-definition,old-unit-version}-01.log`
and the link log above. All four unsupported-target parents subsequently
completed22/0 each with actual outer rc0 under the same55-second limit, in
`out/c43-objects-unsupported-{osx-x86_64,osx-arm64,win-x86_64,win-arm64}-01.log`.
All nine independent parents therefore completed198 assertions with zero
failures (20+seven times22+24), using the unchanged MAT02 recipe and original
budgets. Unsupported-target results are exact product refusals followed by
Linux x86_64 return7 recovery; they are not macOS/Windows object generation or
native execution. The earlier independent link probe is not parent evidence. MAT02 preparation and verification completed
within the original 55/20-second limits, recorded in
`out/c43-object-material-prepare-02.log` and
`out/c43-object-material-verify-02.log`, solely as PREPARED/VERIFIED
no-test-result observations. These material records do not establish any parent
PASS. The three exact coverage rows describe code availability, not stored
results. The 19 broad gaps remain open, including complete formats and negative
domains. Mach-O/COFF unit objects, system-linker interoperability and foreign
native execution are not qualified. Internal phase and cold/hot timings remain
UNMEASURED.

Two separate PP batches now use the explicit `pp-gold` material:
`pp-source-constructed` maps to `net.pp-source-constructed`, and
`pp-gold-controls` maps to `construction.pp-gold-recipe-refusal`. Both are
SOURCE/Linux x86_64 commands with independent55-second limits and fresh
absolute output directories. Prepare once, verify separately, then execute
each parent independently:

```sh
tools/bound 55 sh ./unisacc.com tools/pp-gold-material.cx -- prepare ROOT NEW_PP_MATERIAL
tools/bound 20 sh ./unisacc.com tools/pp-gold-material.cx -- --verify ROOT PP_MATERIAL
tools/bound 55 sh ./unisacc.com tests/pp-constructed.cx -- ROOT PP_MATERIAL NEW_PP_RUN
tools/bound 55 sh ./unisacc.com tests/pp-gold-controls.cx -- ROOT PP_MATERIAL NEW_PP_CONTROL
```

The constructor derives six disjoint cubes from the fixed pp rule source for
nine directives and two defined states. It writes new142-byte mask/sparse-W2
weights, an observation harness and an identity receipt. The real deploy C math
is precisely extracted from `unisa/ckernel.py`'s raw `KERNEL_BODY` before
its main; neither Python nor `np_convert` executes. No old weight values are
used to construct the new NN. The Source447 MAT01 weights SHA is
`2fd7181593523ce306cbcf0825b2d6ebd5dde3f69cf17762e8beeb6ebf25e609`;
the extracted math-body SHA is
`463b02ddd20c31182cbf90458b7cf308992f903b931bbdd8d355d38edee52eb2`.

The functional parent owns independent handwritten18-label and72-score oracles.
A single8-second default-run observation child calls the actual extracted
`infer()` for all18 keys, all four logits and unique winning classes,22 field
refusals/recoveries and four stage/head refusals/recoveries. It reports raw
results without PASS labels. In the same process it loads a separate private,
structurally valid candidate with one W2 changed from1 to2 and must actually
produce scores `{2,0,0,0}`, versus baseline `{1,0,0,0}`; a receipt refusal
is not inference-sensitivity evidence. The parent checks actual loaded-model
SHA, exact rows, no extra output, lifecycle and input identities. Before/after
material verification each retains its20-second child bound.

The material control first verifies an exact private three-leaf clone, changes
only its first sparse W2 to2 and synchronizes the private receipt's unique
model-SHA field. Verification must still return2 with empty stdout and the exact
canonical-unit/class/weight reason, preserving both originals and private
mutated bytes. Its two verify children retain20-second bounds and its final
fresh source observation8 seconds. Baseline verify is pure read: it checks every
one of the142 bytes against the fixed six-cube mask/count/unit/head/class/W2=1
fields, repeats source/content endpoints and rereads the receipt; it does not
call the constructor or reconstruct weights. W2=2 is a valid NN for the separate
inference experiment, but is not this canonical baseline recipe.

On P53/S447, MAT01 prepare55 and verify20 completed rc0 in
`out/c43-pp-gold-material-{prepare,verify}-01.log`, solely as PREPARED/VERIFIED
no-test-result material observations. The actual functional parent completed
92 assertions with zero failures in `out/c43-pp-constructed-01.log`; the
separate recipe control completed23/0 in `out/c43-pp-gold-controls-01.log`.
Both enclosing commands returned0 within their original55-second limits;
child budgets were not relaxed or assertions removed.

Python files remain text dependencies for the gold rules and C math. Full source
identity observation hashes the existing closure, including old weight files;
the startup product remains trusted and may carry existing models. Thus new PP
weight-value construction is independent of old weight values, while the entire
execution closure is not claimed Python-source-free or globally model-unread.
No P3/product/generated-kernel source was changed. Neither byte equality with old
weights, all18 NN source reconstruction, foreign-host qualification nor atomic
snapshot/ABA protection is claimed. The19 broad gaps remain open. Internal
phase and cold/hot timings remain UNMEASURED.

`tests/kernelsha.cx` is a standalone Linux/x86_64-LP64 AF_ALG mechanism gate,
with no arguments or prepared weights:

```sh
tools/bound 55 sh ./unisacc.com tests/kernelsha.cx
tools/bound 55 sh ./unisacc.com -O2 tests/kernelsha.cx
```

It checks literal SHA256 for empty/abc/one million `a`, software cross-checks
at55/56/63/64/65 and262143/262144/262145 bytes, invalid NULL arguments,
explicit negative-errno returns, unchanged caller errno, recovery and repeated
calls with `/proc/self/fd` counts before/after. The observer directory FD is
included identically in each count and closed before hashing. AF_ALG or kernel
hash/sha256 unavailability fails explicitly; there is no software fallback or
cached outcome. Actual execution in `out/c43-kernelsha-regression-02.log`
completed within outer55, rc0,144 assertions/0 failures, using module8ad60.
The original142/2 failure in `out/c43-kernelsha-regression-01.log` remains:
module-local errno did not propagate to callers. The new ABI returns negative
Linux errno directly and preserves caller errno.

The old ce019 formal probe separately measured65MiB load51028690ns and
hash380484652ns with matching fixed SHA. Current8ad60 also passed its own fresh native three-vector probe in
`out/c43-kernelsha-module-probe-run-02.log`:load55760174ns and hash386130270ns,
total441890444ns for the same input. The earlier software4501887772ns is only
a single-point comparison, with no OS-cold or statistical acceleration claim.
Fresh source observation remains329 (`out/c43-source-after-afalg-01.sha256`).
These are bootstrap kernel-mechanism observations, not Source329 product/P3-package or `-run` startup-cache evidence.
Source329 kernel gen13's nine phases and old consumer PC01 prepare/34 copies/
six routes completed at rc0, including warnparse material verification/copy;
the original producer timeout remains. PC01 declare01 hit outer55 rc142 in
input verification before native execution or phases/declare output. Current
prior consumer56a0be5e was compiled native -O2 and new PC03 prepare/all 34 model
copies/six routes/declare/pack/seal/--verify each actually completed rc0 under
the original outer55 limit. Its 1588745B source-package-v1 compiler.pkg SHA is
`1c6a503090ee8d4189d6326139fa4a18b5db14b9682f1db0949e8ee79519db3e`,
and model-inputs.tsv SHA is
`fbabeffcc27d9ca0a677ce793aea7645aa8d30a97893e483755a3aa14c950b8b`.
This is PREPARED/VERIFIED material, not functional PASS. PC01 declare142,
PC02 pack142 and the original producer142 are retained; identical package
bytes in PC02 do not supply a successful outer terminal status.

Prior SPa8aebdd and the actually compiled source-job verifier completed two
pure-read net-warnparse observations, each 53 files/six ancestor jobs/one model
and approximately 0.58s, in `out/c43-source-job-verify-warnparse-01.log`.
No NN was reconstructed. `tests/source-job-snapshot.cx` is the new 67-assertion
API mechanism gate; native O2 compilation actually completed rc0, followed by
outer55 rc0 and 67 assertions/0 failures in
`out/c43-source-job-snapshot-01.log`. Both endpoints actually observed 53 files,
six ancestor jobs and one model. Wrong valid64 SID and unknown job have exact
child refusal reasons; argument boundaries return named errors, all supplied
failed outputs are cleared entirely, and a final fresh observation/snapshot
recovers the original identities/witness/physical counts. No input is written
or NN reconstructed. Compiled-controller reuse is code caching, never PASS
caching; each run must execute every observation and check.

A native controller can be prepared separately with the trusted bootstrap and
identity-bound compiler flags/source/modules/ELF, then run under the original
limit (the reported 67/0 used native O2 execution):

```sh
tools/bound 55 sh ./unisacc.com -O2 -b lnx/x86_64 -o /ABS/NEW_CONTROLLER tests/source-job-snapshot.cx
tools/bound 55 /ABS/NEW_CONTROLLER ROOT SOURCE_CTX net-warnparse
```

The test now strictly expects consumerdd6f5e60/sourcepackage407f5ef7, with
independent material/API contracts. The reported67/0 used prior56a/a8ae;
the current new pins/64KiB gate independently passed67/0 in native02 build/run
(`out/c43-source-job-snapshot-02.log`); no prior PASS is inherited. Its maximum child budgets total32 seconds (identity8 twice,
positive6 twice, two negative children2 each); other argument failures fork
no child. The outer55 limit remains unchanged. This bootstrap mechanism gate
is not new-product, NN-function or historical-producer-result qualification.
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
not functional PASS. Root is now locally installed P53/S329; historical Source329 complete
candidate A was constructed, checked under ctx699 and archived. Current
new-context B construction and58/79 gates completed, then local installation
actually completed. GEN13/14 reconstruction and current original30 selfhost03 completed58/0;
the first142 remains historical; fclose is now SOURCE_FIXED with explicit
working headers, and the new-source product remains NOT_REBUILT.

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
its original contract. Relevant candidate B source58 and schema/invalid-BOM79
gates have now actually passed; this is not positive legacy qualification or
successful installer execution.
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

Historical Source329 library red (current SOURCE_FIXED/PRODUCT_NOT_REBUILT):default-run
`out/c43-fclose-devfull-probe.cx` actually returned1 in
`out/c43-fclose-devfull-product-01.log`:buffered fwrite1/ferror0/errno0, then
incorrect fclose rc0/errno0 despite closed FD (EBADF9). Same-literal host
system-header reference using /usr/bin/cc (POSIX200809L,O2,implicit-declaration
errors enabled) built02/ran02 rc0, correctly reporting EOF/-1/ENOSPC28 and
closed FD in `out/c43-fclose-devfull-host-run-02.log`. Host01's implicit-fileno
warning remains; only clean02 supplies the reference. Root/record/CC/two
sources/stdio hashes were unchanged at that historical observation. Old stdio SHA was
`22ede652a598774465af4d8ecf8cd3195b7fe948a1b9492ab717a39cab179b9e`.
Permanent `tests/stdio-close.cx` (Linux, `ABS_NEW_OUT`) now records the same
failure across24 distinct stream descriptor numbers. Default-run under the
original55-second bound actually returned1,338 assertions/48 failures in
`out/c43-stdio-close-before-01.log`: only EOF and ENOSPC fail in each round.
The other290 checks passed, including descriptor release, exact binary regular
file contents, state-slot reuse and final descriptor count. This is an observed
red regression, not complete library qualification. The test never accesses a
FILE after close and missing /dev/full is NOT_RUN solely for ENOENT.
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
Large CTX03/PC04/KGEN/CC materials still require copying; the complete origin
package is not finished. New product-build/RB drafts explicitly pass
-I ROOT/include for source-package driver/wrapper builds and bind v2 flags
(legacy argv unchanged); these drafts are not compiled/constructed evidence.
Next:finish origin-package copying, explicitly qualify NN adoption or construct
anew, then rebuild using current headers and verify the same candidate. Old329
receipts must not be relabeled fresh;19 gaps, unimplemented44 startup reuse
and signing/publication restrictions remain.

Current registry evidence:original20 --list completed rc0 in
`out/c43-fclose-testplan-list-01.log`, metadata_valid1,96 batches (77
implemented,19 planned,61 unprepared),111 coverage (92 implemented,19
unresolved),executed0/NOT_EXECUTED. Subsequent original20 --all actually
returned3 with empty stderr in `out/c43-fclose-testplan-all-01.log`:
metadata_valid1/111 coverage/92 implemented/19 gaps; SMOKE16/0 only,
95 non-smoke batches and76 coverage NOT_RUN,executed_subprocess_batches0.
Unknown/unrun entries are not PASS. Historical109 coverage/94 batches are not
the current registry. Implementation metadata does not replace actual tests,
NN adoption or new-candidate qualification. Local installation is not complete qualification.
The19 gaps, foreign hosts, classic18 weight-source construction and-run44
remain unfinished; signing/publication restrictions are unchanged. Complete
classic18 source work,19 broad gaps and foreign-host qualification remain open;
no signing/Draft/Latest/publication or implemented `-run` reuse is claimed.
New-threshold full-package observations and old-ctx candidate A construction
are complete, as are current B construction/candidate58/schema79 controls.
Installer and fresh GEN14 construction are complete development-only. Current
Source329 selfhost03 completed58/0 within unchanged30, including complete
after-checks, retained as Source329 baseline only. Current fclose is
SOURCE_FIXED/PRODUCT_NOT_REBUILT; explicit-I include338/0 is not default
embedded-header or new-candidate qualification.
Root is installed P53/S329; saved P53/S447 remains the explicit historical
bootstrap. Candidate A is historical, while installed B has smoke16/source58/
schema79 and actual-install evidence. GEN14 construction passed, but the
Source329 original30 selfhost03 completed58/0 with complete after-validation;
original01 timeout remains; fclose is now SOURCE_FIXED/PRODUCT_NOT_REBUILT,
and old Source329 qualification is not transferred to changed source;
classic18 source construction, foreign hosts and 19 broad gaps remain open.
Original time limits are unchanged.

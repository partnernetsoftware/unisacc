# Tests

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
native hosts are explicitly NOT_RUN. Fourteen checks passed on development
product Pa7e8. This is partial format evidence; full object/boundary/host
obligations remain NOT_READY in the main runner.

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
Root `P53e2cd1f/S447f794a` is installed. Its actual packaged candidate passed
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

The local two-generation rebuild check is `tools/bound 30 sh ./unisacc.com tests/selfhost.cx -- ROOT PRIOR_GENERATION PRIOR_PACKAGE FRESH_GENERATION FRESH_PACKAGE`. Both generations must already be constructed. The prior builder is the explicit saved `PRIOR_GENERATION/bootstrap-before-install.com` and its record; the fresh builder is the current root product. The checker validates each generation's kernel/assembler/source/object/package/slice/wrap receipts and all 1241 model input identities before and after the original artifact comparisons. Each validation invocation owns a bounded set of actual artifact bytes for repeated receipt consumers, releases it on return, and starts the after-check with fresh physical reads. It retains no PASS or cross-phase identity cache.

For current `P53e2cd1f/S447f794a`, generations11/12 passed all58 assertions under the original30-second bound: `out/c43-native-selfhost-12.log`. Both complete products equal the installed root byte for byte. Source,1241 material inputs and construction receipts are checked before and after; the after-check starts a fresh context and physically rereads inputs. Unsigned receipts establish consistency; the original baseline and prior builder's historical source were not replayed, and product-context-v1 omits the bootstrap. This is not host-seed N22 or foreign-host qualification.

The older `P46cfa11a/Sb9c0f819` generations08/09 also passed58 under30 seconds in `out/c43-native-selfhost-09-retained-01.log`; that evidence was not transferred to the new source. Their earlier142 failures remain failures. The exact coverage ID is `bootstrap.native-two-generation`; the full-bootstrap obligation remains NOT_READY.

Receipts are unsigned consistency records, not proof of historical process execution; product-context-v1 does not bind its builder. The saved prior builder's historical source and the original frozen7f709 source tree were not reproduced. The09 wrapped candidate needs no installation record; absence is explicitly NOT_RECORDED while its real construction receipts remain mandatory. This is neither host-seed three-stage N22 nor foreign-host execution qualification. Internal phase and cold/hot timing remain UNMEASURED.

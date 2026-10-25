# Tests

## UJS product (`ujs.com`) — **不在本目录**

Host1 船闸在 **[`../ujs/tests/`](../ujs/tests/)**（与 unisacc 套件分离）：

```
./unisacc.com -run ujs/tests/run.cx -- --list
./unisacc.com -run ujs/tests/run.cx -- --smoke
./unisacc.com -run ujs/tests/run.cx -- --complete
```

host1 语料在 [`../ujs/tests/corpus|neg`](../ujs/tests/)。本树 [`ujs2wasm/`](ujs2wasm/) 仅留 archive_only（expect.json 等）。

## New native test MVP (unisacc CX)

This is a separate batch, with no imported PASS records and no legacy runner dependencies. It does **not** gate `ujs.com`.

From a checkout with unisacc.com:

```
sh ./unisacc.com tests/run.cx -- --smoke
sh ./unisacc.com tests/run.cx -- --list
sh ./unisacc.com tests/run.cx -- --all
```

Neither .cx source uses an explicit #include directive. The runner imports cases.product using the implemented #pragma unisa use/module mechanism and relies on the existing automatic .cx edition prelude for carried library declarations. One compiler invocation builds both source units in memory, and one process computes all sixteen outcomes and compares them with hand-written expected values. It never invokes Python, Clang, shell algorithms, old suites, or child compilers, and writes no executable artifact or result cache. The shell in the command only loads unisacc.com.

Exit status: smoke 0 means this local batch passed; 1 means an assertion failed; 2 means invalid usage or runner failure. --all runs the batch and exits 3 while mandatory contracts remain incomplete or unscheduled (1 takes priority if smoke fails). Its 19 broad gaps are NOT_READY; the eighteen implemented separate net batches are NOT_RUN in this entry. --list is inventory only, not evidence of execution.

coverage.tsv separates implemented assertions from missing obligations. IMPLEMENTED is code availability, not a persisted PASS. New source bytes must actually run for fresh evidence. No preparation result is treated as a test pass.

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
snapshots, full lifecycle/resource budgets and foreign hosts remain NOT_READY.
The same optional argument also accepts a full packaged APE candidate, executing
it directly through its launcher before installation, with its own model package.

`sh ./unisacc.com tests/optimization.cx -- ROOT ABS_NEW_OUT [CANDIDATE]`
compiles six Linux x86_64 images (-O0, -O1, -O, -O2, -O3, -Os), then executes
each image with runtime inputs 7 and 11 and checks every row against hand-derived
values. It also executes the same module through default-run for both inputs.
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
The driver narrow checks passed 242 semantic and 17 Tape assertions. This is
partial optimization evidence, not the full old corpus, optimizer closure,
resource domain, same-candidate bootstrap or foreign-host qualification. The
broad optimization contract remains NOT_READY. All internal timing phases and
cold/hot breakdowns are UNMEASURED; this is not a speed benchmark. Use separate
new directories for the semantic and Tape batches and a <=55-second watchdog.

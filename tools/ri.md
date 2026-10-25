# ResourceInput capture and consumer development checks

Identity boundary: these recorded P53 results belong to Source447. The current
working source is Source329, whose complete product has not been built; root
P53 is presently a trusted bootstrap/historical candidate. None of the RI,
runtime-material or fixed-P3 lineage results below is silently transferred to
Source329. Current unsigned classic-math checks are recorded separately in
[the PRD](../prd.md), not as RI qualification.

These are local Linux/x86_64 checks. The product batch uses the actual driver;
the two substrate batches compile the original C executor and headers with a
header-free CX witness. That adaptation is not a CX rewrite of the C executor.
Preparation is separate from execution and never records a test verdict.

Capture now writes **SC2**, an ordered trace of actual `core_host_fetch` calls.
Repeated keys, including replies that change between translation units, retain
separate events. Strict SC2 replay requires each next key and consumes the whole
trace before committing output. Per-core resource cache hits do not fetch and
do not consume an event. SC1 remains a map: duplicate keys are rejected, unused
rows remain legal. A snapshot is a complete SC1 map and rejects an SC2 trace.
Converting a trace to a map is valid only when every repeated answer has equal
presence, length and actual bytes. It is not valid for every mixed-unit trace.

The capture directory must be new, empty, or contain only a regular empty
`resources.tsv` for compatibility. Initialization owns a private guard; blobs
are published without replacing existing targets. I/O and quota failures refuse
the operation. Failed partial directories remain unqualified: this is not an
atomic whole-ledger transaction or a durable COMMIT protocol. Capture admits
at most 4096 host-fetch events and keys of 1–512 bytes; exhausting that budget
is an explicit failure. This does not establish the complete resource budget.

Use new output directories for each command. The root product must be rebuilt
after production inputs change; an old root can still serve as a named bootstrap.

```sh
tools/bound 30 sh ./unisacc.com tests/resources.cx -- "$PWD" "$PWD/out/ri-map-new"
tools/bound 55 sh ./unisacc.com tests/resources.cx -- "$PWD" "$PWD/out/ri-trace-new" --trace
tools/bound 55 sh ./unisacc.com tests/resources.cx -- "$PWD" "$PWD/out/ri-multi-new" --multi
tools/bound 55 sh ./unisacc.com tests/resources.cx -- "$PWD" "$PWD/out/ri-output-new" --trace-output

tools/bound 55 sh ./unisacc.com tools/ri-material.cx -- --build "$PWD" "$PWD/out/ri-capture-material-new" tests/cases/ri-fetch.cx
tools/bound 55 sh ./unisacc.com tests/ri-capture.cx -- "$PWD" "$PWD/out/ri-capture-material-new" "$PWD/out/ri-capture-run-new"

tools/bound 55 sh ./unisacc.com tools/ri-material.cx -- --build "$PWD" "$PWD/out/ri-consumer-material-new" tests/cases/ri-consumers.cx
tools/bound 55 sh ./unisacc.com tests/ri-consumers.cx -- "$PWD" "$PWD/out/ri-consumer-material-new" "$PWD/out/ri-consumer-run-new"

tools/bound 55 sh ./unisacc.com tools/ri-material.cx -- --build "$PWD" "$PWD/out/ri-cli-material-new" tests/cases/ri-cli.cx
tools/bound 12 sh ./unisacc.com tools/ri-chain-material.cx -- "$PWD" "$PWD/out/ri-chain-material-new"
tools/bound 12 sh ./unisacc.com tools/ri-chain-material.cx -- --verify "$PWD" "$PWD/out/ri-chain-material-new"
tools/bound 55 sh ./unisacc.com tests/ri-standalone.cx -- "$PWD" "$PWD/out/ri-cli-material-new" "$PWD/out/ri-chain-material-new" "$PWD/out/ri-standalone-new"
tools/bound 55 sh ./unisacc.com tests/ri-material-controls.cx -- "$PWD" "$PWD/out/ri-cli-material-new" tests/cases/ri-cli.cx "$PWD/out/ri-material-valid-new" control-valid
```

The material tool binds the complete source identity, full bootstrap bytes,
nine direct input hashes, generated C bytes, compiler flags, actual ELF bytes
and receipt. It checks inputs before and after compilation. Both runners verify
the material before and after execution, read the native image again, run fresh
child processes, and retain stdout/stderr/status. They never cache test results.

The capture witness covers changing/repeated replies, nonempty-directory and
FIFO refusal, legacy empty files, busy guards, session reset, actual 4096-event
exhaustion and injected open/flush/close/append failures. The consumer witness
covers binary keys/values, owned blob copies, positive empty IDs versus absent
zero, repeated lookup slots, RI → synthetic RES → disk precedence, snapshot
PRESENT/EMPTY/ABSENT/MISSING behavior, and SC1 strict refusal. Synthetic RES
lookup is not actual P3 loading or a complete model route.

The standalone witness uses two tiny threshold networks prepared separately by
the existing `np_convert`, through the actual C `--chain` entry. The material
tool binds complete NN byte pins, literal tables, its supporting tools and a
fixed `core.h` action ABI. Verification only reads these materials; it never
converts weights. The runner verifies the chain material before and after each
batch and reports the two NN admission checks as PRECONDITION. Literal binary
output and final attribute records are checked independently. An unconsumed
SC2 event must leave an existing attribute sentinel unchanged and must not
create a missing attribute sidefile. Attribute data stays in memory until the
entire chain and trace accept. These synthetic networks are not production
E1/E3 qualification. The separate `--trace-output` driver batch covers the host
symbol sidefile, SC1 metadata compatibility and unsupported run/object routes.

Material controls run one case per invocation, each with its own 55-second
bound. Besides `control-valid`, select `native-byte-flip`,
`missing-native-receipt`, `symlink-native`, `fifo-receipt`, `corrupt-runtime.c`,
or `corrupt-flags-receipt`, always with a new output directory. They copy the
four material leaves, mutate only the private copy, require the specific
refusal, and physically reread the original leaves afterward.

The isolated NULL,0 probe retains an active memcpy hook and checks allocation,
slot count, growth, nonempty copies and cleanup. Its original red observation
is preserved separately. The production guard skips only a zero-length memcpy;
it retains `n+1` allocation and slot occupancy. `corecompat.cx` permits just
this exact whole-file transition from a pinned old SHA. Package refresh retains
all selected constructor files and their current hashes, records the exception
and helper SHA explicitly, and preserves model and non-kernel resource bytes.
It does not authorize arbitrary runtime changes or establish ABI equivalence.
The rebuild verifier checks current source and receipt consistency; it does
not independently reread the frozen baseline tree used by package refresh.

The `ri-flow` adapter adds four separate production-model consumer cases over
the current full P53 P3 package. It uses the actual C package loader and model
executor, without another decoder or weight construction. Its existing
`UNISA_RUNTIME_LIBRARY` guard excludes the unrelated standalone CLI and model
enumerator; this is a source adapter, not installed CLI integration.

Preparation is deliberately split. The generic material command below writes
source only. Profile compilation uses default libc trimming with explicit
`-O2 -b lnx/x86_64 -o -`, emits COMPILED pending material, and cannot claim
PREPARED until a separate seal verifies it. Use a new runtime directory and
new DATA directory, then a new run directory for every CASE:

```sh
tools/bound 55 sh ./unisacc.com tools/ri-material.cx -- "$PWD" "$PWD/out/ri-flow-runtime-new" tests/cases/ri-flow.cx
tools/bound 55 sh ./unisacc.com tools/ri-flow-material.cx -- compile "$PWD" "$PWD/out/ri-flow-runtime-new"
tools/bound 55 sh ./unisacc.com tools/ri-flow-material.cx -- seal "$PWD" "$PWD/out/ri-flow-runtime-new"
tools/bound 20 "$PWD/out/ri-flow-runtime-new/flow-native" prepare "$PWD/unisacc.com" "$PWD/out/ri-flow-data-new"
tools/bound 55 sh ./unisacc.com tests/ri-flow.cx -- "$PWD" "$PWD/out/ri-flow-runtime-new" "$PWD/out/ri-flow-data-new" "$PWD/out/ri-flow-empty-new" e3-empty
```

The other CASE_IDs are `e3-absent`, `e3-missing` and `e3-to-lower`, each as its
own55-second command. DATA is derived by the real production E2/E1/E3 stages:
literal constructor source yields actual E1 tokens and attributes, then E3
tape and snapshots. The material inventory binds every file and directory,
including empty directories, plus the full product/runtime and literal-source
hashes. PREPARED DATA contains no saved test verdict.

The three E3 cases require real model fetches observed through SC2, rather
than added C lookup probes. PRESENT-empty and explicit ABSENT each mask the
lower RI attributes; MISSING falls back to the attributes actually produced by
E1. Their final native exit oracles are0/0/7. A legal ABSENT row is re-signed in
COMMIT; the production snapshot writer is not changed. In `e3-to-lower`, the
upstream snapshot binds actual E3 tokens. Tapecheck, E4 and prune remain in the
route, and only the actual lower invocation is captured against its real input,
whose bytes must differ from the upstream input. The final constructor image
must exit7. Its observed lower attributes-fetch count does not justify a claim
that lower consumes that key. Each parent directly verifies the runtime profile
before/after, and the child verifies the entire DATA identity before/after.

The profile compile/seal commands each completed under55 seconds and DATA
preparation under20 seconds. All four independent parents completed12 checks
with zero failures each under the unchanged55-second bound. Their actual logs
are `out/c43-ri-flow-{e3-empty,e3-absent,e3-missing,e3-to-lower}-01.log`, and
the generated ELF exits were0/0/7/7. Actual E3 fetches respectively showed
PRESENT/length0, ABSENT/length0 and MISSING fallback PRESENT/length13 with exact
E1 attribute bytes. Lower capture observed attributes-fetch count0: that case
proves the true lower input identity and production route, not consumption of
the attributes key. None of these results turns preparation or broad RI into
PASS. The exact
coverage IDs are `ri.production-e3-presence` and
`ri.production-e3-lower-binding`. All four independent cases are required;
this remains local source-adapter evidence over the named current package.

Flow-profile eligibility and DATA-directory inventory controls are separate
from the four functional parents. After the existing source/compile/seal/data
preparation, use new private output directories and run one CASE per55-second
command:

```sh
tools/bound 55 sh ./unisacc.com -O2 tests/ri-flow-controls.cx -- ROOT RUNTIME NEW_PROFILE_CONTROL CASE
tools/bound 55 sh ./unisacc.com tests/ri-flow-data-controls.cx -- ROOT RUNTIME DATA NEW_DATA_CONTROL CASE
```

Profile CASEs: `valid`, `unsealed`, `elf-flip`, `pending-flip`,
`final-flip`, `closure-field`, `old-v1`, `recompile`, `reseal`.
All require the sealed v2 five-leaf runtime, not pending-only output. Original
verification and the private control each use an18-second child. The child
reexecutes the live parent ELF through Linux `/proc/PID/exe` and directly calls
the actual profile module; parent file identity and full child executable SHA
are checked. Linux proc access is a precondition. No thin CLI recompilation,
successful profile construction, NN evaluation or verdict cache is involved.
Actual child rc/stdout/stderr are retained in `original.status`/`control.status`
and corresponding stream files; unavailable rc is explicit and fails.
The old valid01 failure and separate O2 probe retain incomplete rc evidence,
with precise causes uncertain. New valid02 passed13/0 with both child rc0.
The later O0 elf-flip02 failed at original-material prerequisite rc142 with empty
stderr, saved in original.status; no negative-control assertion ran, so that
failure is not negative-control PASS evidence. All nine03 cases using manual
-O2 controller invocation above passed133 assertions with zero failures
(13 plus eight times15), each under the unchanged55/18-second parent/child
limits. Actual original/control status files show rc0/0 for valid and rc0/2
for refusals. Only the controller compilation option changed; profile tools,
modules and prepared inputs were unchanged.

The batch table has no compilerflags column and currently no executor. SOURCE
entry is an exact source path; args is a runtime-argument template. The manual
-O2 command and exact coverage description express this requirement without
misplacing flags in entry/args or claiming future dispatcher support.

DATA CASEs `extra-empty` and `declared-empty-remove` require the same runtime
and separately prepared DATA. Both copy all regular leaves and directories,
prove a valid baseline, demand exact rc90/empty stdout on the directory-only
inventory mutation, and restore a valid state. The explicitly declared empty
DIR case first proves its private modified receipt valid. Each of three native
verify calls is bound8. Both cases actually passed15/0 within55; no NN is
executed. Original DATA/full runtime/raw product hashes remain unchanged.

The exact finite IDs are `construction.ri-flow-profile-eligibility` and
`construction.ri-flow-data-dir-inventory`; [tests/README.md](../tests/README.md)
documents all cases and evidence. Material controls do not clear the19 broad
qualification gaps or turn PREPARED into functional PASS. Phase timings remain
UNMEASURED.

The separate `bootstrap.fixed-p3-driver-lineage` contract is now registered in
[tests/README.md](../tests/README.md). Its actual HOST-to-stage1-to-stage2-to-stage3
CTX02 run completed41 checks with zero failures in
`out/c43-hostseed-chain-03.log`, including stage1 versus root complete bytes;
all three full products are P53. The tool SHA is
`293e069e71de8bdc8b27a62a70dbb8d452cdd47e9d192334840716f8c7080b1f`;
its HOST and all18 slice/wrap steps were actually rebuilt under their original55
limits. Canonical parent admission reconstructs full stage and six artifact
receipts from HOST, with repeated physical reads before output and after-step
ancestry verification. Canonical paths, cycle and64-generation depth guards
are implemented; cycle/depth controls remain untested. The child maxima
40+6+6+6 total58, exceeding outer55 before overhead: these18 actual successes do
not guarantee the worst-case permitted ancestry fits55, and the bound remains
unchanged. The eighth refusal case alters only a private parent wrap receipt's
builder field and requires named rejection before any new stage output. All
eight CTX02/current-checker03 controls now independently returned outer rc0:
71 assertions with zero failures (five8, two10, one11), each using manual `-O2`
under the unchanged55-second parent, target20 and two-source8 budgets. Their
actual logs are `out/c43-hostseed-control-*-03.log`; each target required exact
rc2, empty stdout and its named refusal, preserving the complete original
manifest and private clones where applicable. The earlier field-only02
observation11/0 and old CTX01 seven-case60/0 remain historical, separate results;
neither was transferred to this fresh71/0 qualification.

The fixed declared P3 payload still trusts NN and packaged ASM kernel binaries;
Python NN/ASM source reconstruction is not replayed. Initial source measurement
remains root-assisted, host-toolchain dependencies are not hermetic, and unsigned
receipts are consistency observations rather than historical execution proof.
This evidence neither replaces RI consumers nor clears broad
`bootstrap`/complete host-seed/N22 or native foreign-host qualification.

Full RI/model-route coverage, all resource budgets, native foreign hosts and
general memory safety remain separate qualification obligations. Phase timings
and cold/hot startup measurements are UNMEASURED in these batches.

# ResourceInput capture and consumer development checks

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
tools/bound 55 sh ./unisacc.com tests/resources.cx -- "$PWD" "$PWD/out/ri-map-new"
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

Full RI/model-route coverage, all resource budgets, native foreign hosts and
general memory safety remain separate qualification obligations. Phase timings
and cold/hot startup measurements are UNMEASURED in these batches.

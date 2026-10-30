# Release evidence and material audit, 2026-10-28

This is a read-only evidence audit during the main construction window, not a
new test result. Initial inspection covered `723f6f5a46`/`7e5f3f18a0`; HEAD also
advanced to the polling-rule documentation commit `c0d593c1e2`. No compiler or
heavy batch was run, no construction input was changed, and no remote message,
artifact upload or CI dispatch was issued.

The installed product is Root `d65c1474…`, source `f9fff160…`. Private E3 package
`661c2b02…` is not admitted. The running regular source build is rooted at
`/home/devali/repos/unisacc-epoch-aggregate-13d8cfe478`, with context
`out/epoch-context-01/context.tsv`, source
`ace3b44700ce593e99084edf299011be5730e4dac1f157f0b97e816f16ee5916`
and bootstrap Rootd65. This audit does not infer process completion from files.

## The newly registered 35 batches

The proposal has 39 physical lines: three comments, one header and **35 data
rows**. It contributes 15 coverage rows. Formal registration now has 204 batches,
193 coverage rows and 19 unresolved wide contracts. Metadata registration and
historical proposal counts are not current-candidate execution evidence.

| Group / exact registered IDs | Count | Required material | Executable input/oracle and remaining restriction |
|---|---:|---|---|
| `stream-e4-peep-0..7` | 8 | `streams` | `weights/gold/peep.tsv`, slices 0/200 through 1400/232, two-instruction witnesses generated inside the actual batch. Preserve UNREALIZABLE and TABLE_SUSPECT classifications. |
| `stream-e4-opinfo-0..1` | 2 | `streams` | `weights/gold/opinfo.tsv`, slices 0/40 and 40/40; class eligibility witnessed through E4. |
| `stream-e4-o2` | 1 | `streams,ref` | Twelve inline C programs; independent classic `-O0/-O2` tape and non-vacuity witnesses. |
| `stream-prune-cases` | 1 | `streams` | 27 declared graph cases; verbatim `src/tapeprune.c`/`src/tapescan.h` oracle is compiled by the batch itself, so it does not need the external `ref` material. |
| `properties-cases-{det,meta,bound,tu}` | 4 | `streams` | Inline literals, six-target byte comparisons, environment/jobs/metamorphic and integer/refusal contracts. |
| `properties-cases-{lower,random}` | 2 | `streams,ref` | Lower replay/frame oracle and 16 seeded UB-free programs; six-target differential requires the independent reference. |
| `properties-cases-layout-{len12,len3-0..5}` | 7 | `streams,ref` | All 258 length 1..3 data sequences and independent layout arithmetic, native execution and six-target reference bytes. |
| `optimization-corpus-0..6`, `optimization-{closure,cli}` | 9 | `ref` | Seven explicit programs, six optimization spellings and targets, closure/non-vacuity and named refusals. |
| `formats-macho-pe` | 1 | none | Product-generated Mach-O/PE structure, signature/page-hash and mutation oracles. This is not foreign-host execution. |

Thus the material plan is **15 streams-only, 10 streams+ref, 9 ref-only and one
material-free** batch. The 19 reference-dependent batches must receive a real
canonical reference executable, not an absent placeholder or a self-comparison.
The proposal comment that `{ref}` is unknown is stale: `run-batch.cx` already
resolves declared material names generically. No special `ref` branch is needed.

These `*-gen.cx` entries are execution batches, not prepare-only generators.
Invoking them starts compilers/streams; `prune`'s `oracle-only` still compiles and
runs its oracle. They must not be launched as supposedly light preparation while
the regular constructor holds the heavy window.

## Material paths and the first effective final-candidate wave

Existing Rootd65 material is `out/c43-fullname-streams-01/streams.json`: source
package `e154c747…`, runtime prefix `abe915c3…`, resources `b68d8a75…`, stream4
`f7164370…`, stream5 `efd16281…`, stream6 `8d7fc94a…`. The private E3 stream set
`out/c43-aggregate-return-streams-02` has package661 and is not a release input.
The old proposal's stream4 `9d3119df…`/stream5 `1306de94…` observations are
historical. The new corpus drivers accept supplied stream paths; they do not
compile those historical hashes as mandatory pins. Fresh material identity and
same-candidate member verification are still required.

Let `F` be the final admitted candidate's canonical test/construction root, `M`
a new stream directory and `S` a new session under `F/out`. After the heavy
window is transferred, the existing shortest material path is:

```
F/tools/bound 55 sh F/unisacc.com -O2 -fno-trim-libc F/tools/stream-runtime.c -o F/out/final-stream-runtime.com
F/tools/bound 30 sh F/unisacc.com F/tools/stream-pack.cx -- F/out/final-stream-runtime.com F/unisacc.com M
F/tools/bound 55 F/tools/build_ref.sh F/out/final-ref.c F/out/final-ref
```

Here `F` is notation to replace with the actual absolute root, not a literal
shell variable expansion. The reference script's own root must be `F`; it
exports that frozen source through `sourceflat.cx`, wraps it with refshim/reffoot
and builds the independent classic implementation with host `cc -O2`. Preserve
its generated source, source JSON, compiler path/version/flags and executable
hash. Host C/system headers remain declared dependencies. The existing
`out/c43-ref-pp-ucn-proto-cdx/ref` is an old experiment and has no demonstrated
applicability to the final source. Do not choose it merely because it exists.

Prepare the session material map with canonical `streams<TAB>M` and
`ref<TAB>F/out/final-ref`, then use existing `all-waves init`, `material` and
`executor`. First execute `e2-input-binding` on the actual final seven-stream
set; only then schedule the 35 new batches in single-heavy waves. Reuse exact
unchanged material bytes only after content and applicability checks. Every
batch still executes afresh. Finally run `tests/all-aggregate.cx` and
`tests/run.cx -- --all ABS_SESSION`, retaining nonzero incomplete verdicts.
Bare `--all` is inventory plus smoke, not execution of the entire registry.

All 35 must run again on the final candidate. The same applies to other
candidate qualification leaves, host observations and current-epoch matrix
evidence; old Root94, d65 or private661 PASS records do not migrate.

## Immediate execution blockers beyond missing files

1. `e4.gold.peep-1632` is not closed by eight successful process exits.
   `peep-cases-gen.cx:195` deliberately returns rc0 with TABLE_SUSPECT rows.
   Historical proposal data has 277 executed passes, 1339 UNREALIZABLE keys and
   16 TABLE_SUSPECT keys. The wide row still requires every gold key's declared
   outcome. Independently prove the classifier's unreachable key domain and
   resolve the 16 inconsistent gold relations through facts/generation and
   oracle review; do not silently rewrite them to keep or count them as PASS.
2. O1 material is absent from the standard seven-stream packer, which selects
   `image/O2` only. `e4.in.o1-model` and `e4.pos.o1-save-restore-to-mov` need an
   actual selected O1 member and separate literal/reference O1 oracle.
3. Existing formal identity/admission registry rows still embed Root7a/SID4fe,
   old record/commit/plan literals. The local hosts test also hardcodes Root7a
   in its product assertion and HOST_CELL output. A new candidate cannot satisfy
   those old tuple assertions; parameterize/bind the new exact tuple while
   preserving the original mismatch controls.
4. E1 independent source still pins `68f08deb…`, whereas the Rootd65 material is
   stream2 `d23b7b41…`. E3 defaults to `8605c8f3…`; its optional explicit pin is
   not passed by its formal row. Inspect all related fixed pins against the
   actual final material before freezing the final verification plan.
5. Three source-material rows declare `current-source-id`/`recorded-source-id`
   as ordinary materials. `run-batch` currently requires canonical absolute
   paths, while their receiving tests require a 64-character hex literal.
   This cannot be solved with a clever filename. The implementation prepared
   in this same change adds generic `name:sha256` declarations (bare names
   remain paths), validates strict lowercase 64-byte hex, and carries the
   declared type through run-batch, the session manifest, split snapshot,
   receipt and independent aggregate. Literal identities use a separate hash
   domain and are rebound to the immutable material map; they are never opened
   as filenames. Three SID rows, eighteen stream pin rows, and three candidate
   profile rows now declare their literal inputs explicitly. The independent synthetic
   contract is `tests/typed-materials.cx`: `prepare ROOT NEW_FIXTURE`, then use
   that fixture's real product and `tools/all-waves.cx` to `init` a new session
   with `out/map.tsv`, register `literal` and `asset`, run `--heavy typed-wave`,
   prepare the native `executor`, and `start/run/finish` `typed-split` with the
   actual generated nonce. Finally `verify FIXTURE SESSION RUN_BATCH_ELF`
   checks qualification of those two synthetic rows, separately re-hashes and
   re-chains forged material/receipt records, verifies independent aggregation
   refuses them, restores the original bytes and verifies recovery. Invoke
   each phase separately under the existing 55-second bound. This small
   registry deliberately claims no coverage of the repository's 204 batches.

   This contract was executed after the external cc-session process exited:
   fixture02 prepare **11/0**, actual heavy wave **1 PASS** (unchanged 12-second
   child budget, 11.864 seconds measured), formal executor/start/native-run/
   finish **typed-split PASS**, then independent verify **14/0**. Verify includes
   internally re-chained literal substitution, unknown wire type, map literal
   mutation, cross-type receipt mutation with recomputed receipt SHA and chain,
   refusal/recovery, bad literal input, and ENOTDIR versus ENOENT path errors.
   Only the miniature two-batch registry was QUALIFIED. Root d65 was the real
   candidate; the miniature tree's measured SID is
   `d21b321eb28c7227b388bf1c005ed188631571f932e413e0f6a29c0cf257d715`,
   not the installed product's build-source SID and not a release export.

   Reproducible evidence is `out/c43-typed-prepare-03.log`,
   `out/c43-typed-{init,literal,asset,wave,executor,start,run,finish,verify}-02.log`,
   and `out/c43-typed-fixture-02/out/session-01/`. The restored final manifest
   SHA is `daaa485a4b47e6f0da514099ace67dc64a37863c065f316f44f8da57ee8e759c`;
   fixture batches SHA is
   `30e3893af4a40d3a06544a1f879f6578450fb462740555f9f61d2d04a7890dff`.
   Both initial red observations remain: prepare01 failed to compile the test
   because it called private `as_join`; fixture01 wave3 was UNKNOWN because
   `sizeof material[nmaterial].name` as an argument is rejected by the product.
   The harness uses an equivalent local `Material *` spelling; the product bug
   remains explicit in `tests/pending/sizeof-indexed-member-arg.cx` (expected
   default-run exit 7, not run here and not credited PASS).

   The full new registry was independently listed after all 24 row edits:
   `out/c43-typed-registry-list-01.log`, rc0, 204 batches / 193 coverage /
   19 unresolved, executed0, NOT_EXECUTED. Its batches SHA is
   `903bfa7aed6019293a0fe84b34c38622d3c90f685e38fc3f8b24dd7fd083ea8c`,
   coverage SHA
   `676404f458285d88ce7f4dcfa774c60f6fc0487c7134420edcb9fb786c72528b`.
   Any final candidate verification profile must pin this new plan input;
   an earlier plan digest cannot authorize the revised registry. The real
   release's unresolved coverage and fresh final-candidate execution obligation
   are unchanged. Typed source identities: testplan `33ce8e81b0aa5ea5416d3e1531669ce88c51d45603c773828f714d305b15ef47`,
   run-batch `4f476d8fd9af80e8194a94250c1516c15a55be3a7ba2de9e15964c7e17ad62ad`,
   all-waves `fb579426ee38d4fe3ea52661927a541a3900626ed2f26ee48e74af1f885d72a1`,
   allsession `e9ae993283770f4c83973c08cdd5fafc47c7409dc24b6779c43f076ca312b461`,
   typed contract `0962054b012d829787aec98e1160f50bd33492018dbcbec37d923153d74c98c8`.

   The compatible profile input module was independently exercised by
   `tests/candidate-profile.cx`: `out/c43-candidate-profile-loader-01.stdout`
   reports 28 assertions / 0 failures, stderr is empty. This verifies immutable
   expected-input schema, ownership, rejection and recovery only. The three
   registry entry CLIs match their profile argument counts and consume the
   separately declared `candidate-profile` path and `candidate-profile-sha`
   literal; admission retains its explicit GEN/PC material bindings. Actual
   candidate identity/admission/host profile execution is still NOT_RUN.
   Admission's existing `rebuildcontext` import crosses the independently owned
   package/admission reader domain, which must be frozen before its formal
   candidate run; typed allsession/all-waves themselves import only netpack and
   testplan and do not cross that domain.
6. `selfhost-mixed-*` expects kind4/156-job material; the active regular chain
   produces kind2/107. Do not substitute a regular GEN directory as a mixed
   endpoint. Preserve the kind4 contract with actual matching material or
   implement an explicit applicable kind2 branch with the same fixed-point
   obligations; `selfhost-pair` already has a kind2-aware verification route.
7. The ancestry-cycle/depth control source exists but is not registered. Its
   existing assertions must be wired to real three-stage materials before
   it can close the ancestry cell. Having the source on disk is insufficient.

## N22: what the current regular construction does and does not provide

Completion of 107 jobs plus native kernels/product is preparation, not complete
N22. Retain the actual source context, PC context, declaration/models/resources,
both generation directories, prior builder+record and current kind2 admission
result. Preserve their literal canonical roots; receipts cannot simply be moved
or relabelled. Then execute existing `p3-member-consume` (source ctx, pack ctx,
tool ELF), `p3-library-members` (pack ctx), `p3-nn-asm-provenance` (source/pack
contexts, both GENs, elf tool) and kind2 identity/admission cells against that
same candidate.

The remaining three-stage route is the actual hostseed/native-first builder:
HOST context+models+the final live PC payload+three distinct stage directories;
run `--verify-chain`, then `hostseed-source-bind` and every lineage/ancestry
negative control. `hostseed-source-bind` explicitly requires payload equal to
`PACK_CTX/compiler.pkg`, matching host/source SIDs, and fresh source-job/model
verification. Two equal generations and a fixed P3 chain do not prove complete
NN-source or host trust edges.

Local outstanding N22 obligations include loader/libc/preload binding, actual
product-free orchestration where required, current member/oracle crossing,
canonical two-ISA assembler/layout/refusal proof, AF_ALG-vs-software digest
crosscheck and a candidate-bound dependency declaration. Host CC/subtools,
system headers/libc, zlib headers/private loader, classic18 Python emission,
watchdog/reference host-C bootstrap and unsigned same-user trust limits remain
explicit declared dependencies, not zero-dependency claims. Existing old matrix
notes are often stale: kernel-route now has nine registered finite batches;
refresh their real evidence before judging that older MISSING/BLOCKED text.

## The 19 wide contracts: material work still needed after the 35

| Wide row | Existing executable route/material | Remaining local/host work not supplied by the 35 |
|---|---|---|
| E2 | streams, CLI/header/diagnostic/input-binding batches | Complete refused/positive domain and same-candidate host cells; final pins/member binding. |
| E1 | stream2 + lexical gold + input-binding | Current NN-key oracle and observable-source attribution; old293/0 does not replace the still-open144-key obligation. |
| E3 | selected error/warn models, units/tapecheck adapters | Full type/scope/UNITOK2/attributes/refusal corpus; aggregate-return private green needs installed-candidate rerun. |
| E4 | 8+2 gold slices and O2/optimization cases | O1 material, all-key treatment, refusal/budget/input/semantic breadth and foreign execution. |
| prune | new27-case three-way oracle + stream5 | Export/library signatures, refusals/recovery, executed semantic preservation and hosts. |
| lower | stream6, seven layout slices, per-target stream packs | Full ABI/gates/sidecars/credentials/refusals and five real native target hosts. |
| elf | per-target stream7 + format/object/kernel-route batches | Full encoding/relocation/branch/memory/refusal domains; host execution. |
| interfaces | seven streams, units/tapecheck, attributes fixtures | Consumer grammar, UNITOK2, sidecar ownership, memory/O1/prune placement and foreign chains. |
| RI | ri-cli/chain/capture/consumers/flow runtime+data | Full presence/empty/missing, ledger authority, stage/input/target/optimization identity and refusal coverage. |
| properties | new13 property slices + existing finite uint32 | Complete matrix/host mapping; reject vacuous or partial no-REF execution. |
| resources | existing resource/hostile and watchdog controls | fd/disk/memory/CPU/step budgets, process-tree cleanup, UNKNOWN and recovery. |
| negative | diagnostics/model loader/qprefix/hostile/kernel-route | Remaining malformed resource/schema/action/routes, runtime integrity and macro-column gap. |
| optimization | new9 optimization leaves + existing native/run/tape | Independent reference, full corpus including blocked c-testsuite obligation, closure and host proof. |
| formats | Mach-O/PE structural96, ELF/objects/kernel-route | Complete object/ABI/layout/refusal domains and actual foreign execution. |
| hosts | final candidate tuple + host adapter | All six actual host/process observations; WinARM native process is still missing. |
| bootstrap | source/PC/GEN/hostseed/kernel-route material | N22 edges, strict three-stage chain and foreign native rebuilds as above. |
| library | library ABI, P3 library API fixture, native library | Complete API/errors/ownership/concurrency/FFI/callback/layout/export surface, not merely55 ABI assertions. |
| candidate | final raw com+record+SID+plan+verification commit | Applicable tuple rows, kind2/refusals, immutable producer artifact and six-host receipts. |
| phase timing | internal stamps + external timing batches | Final-candidate cold/hot attribution, cache/memory/output/validation coverage; unmeasured phases remain explicit. |

Most `full-*` entries still point to absent pending sources and remain PLANNED.
They need complete domain oracles/receipts and actual native host evidence,
not just material paths or changing registry status. The new leaves do not
automatically discharge these broad parent contracts.

## Five foreign hosts: shortest valid path and current hard limits

Freeze one final admitted raw product+record tuple, source SID and exact
verification commit plus six-file plan digest. Produce an immutable artifact
containing exactly the two raw root files, with real completed producer run and
artifact IDs. Checkout the exact verification commit without newline/symlink
substitution, independently hash bytes/record/plan, observe actual host **and
process** architecture, and execute actual applicable batches with native
tree-watchdog/path adapters. Each foreign host then needs its own chain/rebuild
and required contract cells; cross-target bytes made on Linux do not count.

`candidate-native.yml` lists all six axes but intentionally exits3: POSIX runs
identity/metadata and bare `--all`; Windows runs only `--version`. It supplies
neither full batch/material execution nor strict three-stage construction.
WinARM explicitly expects PE/x86_64, so it cannot satisfy the required native
ARM64-process row. Windows also lacks the functional JobObject/path adapter.
POSIX non-Linux adapters and unavailable native runners must be measured, never
emulated into PASS. The simple `ci.yml` also assumes a root product after checkout
although `unisacc.com` is untracked; it is not an immutable candidate producer.
Signing/Defender artifact workflows are not a replacement producer of the
required raw product+build-record pair.

Old documents' blanket signing/Draft/Latest/public prohibitions were revoked by
the current AGENTS rules. They are not technical blockers. This audit itself
has no external-dispatch task; no such action was taken. Real missing artifact
IDs, adapters, native ARM64 PE and complete host execution remain blockers.

## Safe parallel work now, heavy work later

While the main constructor owns the heavy window, independent owners can review
and prepare: typed material/tuple/pin adapters and negative controls; immutable
material-map/profile templates; reference source/flags provenance inventory;
peep relation feasibility and16 suspect gold oracles; O1 selector design;
Windows watchdog/path and native ARM64 compiler slice; CI producer/schema and
six-host receipt protocol; missing RI/library/resource/phase-domain fixtures.
These edits belong in separate files, not the running frozen source tree.

After handover, one heavy owner finishes construction/admission, prepares final
streams and independent REF, freezes test plan+harness identities, runs the full
implemented registry in bounded waves and independently aggregates it. Other
owners may continue light review/oracle work without mutating those inputs.
No current PASS is created or migrated by this document.

# Rule-source migration and completion boundary

Source inventory refreshed after `11a0d56` (2026-09-28). This is a remaining-work
list, not a completion percentage. Historical batch results and artifact hashes
remain in `prd.md`; they are not repeated here.

The route is **declarations → finite transitions/actions → constructed threshold
networks → generic execution over stage byte streams**. Moving a rule to TSV
removes its Python control implementation; it does not erase the rule or imply
smaller weights. Shared action sequences and dynamic gold/catalog values must
remain shared inputs, not copied answers.

| Stage | Declaration sources already used | Remaining handwritten construction |
|---|---|---|
| E1 | `lex/` declarations | Data assembly and bindings; runtime model is separate from the old product lexer |
| E2 | [pp/rules.md](pp/rules.md) | Header-name extraction, resource ordering, initialization and assembly bindings |
| E3 | `parse2/tape-*`, `scope-*`, `declaration-*`, `width-*`, `type-tape.tsv`, `control-*`, `ladder-*`, `initializers-*`, `strings-*`, `constexpr-*`, `statics-*`, `vla-*`, `unresolved-*`, `printfallback-*`, `type-entry/default/follow`, `operator-*`, `shape-*`, `conversion-*`; gold type/tyinfo/prec and other existing facts | Dynamic type/gold/template bindings and initialization; fixed expression, declaration, literal, diagnostic, warning, unit and location controls now have declarations |
| E4 | `opt/scans-*`, `local-*`, `stfuse-*`, `peep-*`, `analysis-*`, `parsers-*`, `rounds-*`; gold opinfo/peep | LEVEL selection, dynamic data initialization/dispatch and generic assembly in `opt/gen.py` |
| E5 lowering | `lower/data-*` sparse layout, `lower/code-scan-*`, `lower/code-print-*`, `lower/code-entry-*`, `lower/code-sysprep-*`, `code-abi-sources.tsv`, `code-syscall-*`, `armfuse-*`; existing gold/catalog facts | Dynamic ABI fact selection and template bindings in `lower/code.py` (fixed syscall/termination control now uses `code-syscall-*` and `code-shell-*`); target/escape/layout bindings in `data.py`; ARM destination-shape policy and shared optional u64 reader are declared |
| E5 encoding | `enc/armint-*`, `armmem-*`, `armbranch-*`, `armfp-*`, `arminput-*`, `armlayout-*`, `armwin-*`, `armbase-*`, `armcontract-*`, `x86-operand-*`, `x86-line-*`, `x86-procs-*` (including relaxation); catalog opcode facts | Dynamic instruction/shape and format-field bindings; machine-code templates including Windows command-line setup remain shared sources; see [enc/CONTROL_AUDIT.md](enc/CONTROL_AUDIT.md) |
| E6 images | `enc/elfimage-byte/result.tsv` shared image control and `enc/pedelta-byte/result.tsv` PE control, `enc/machodelta-*` Mach-O layout/signature-page control, `enc/sha256-*` shared digest control; existing target layout facts | Format header/section/CD fields and import/string enumeration, child installation and dynamic layout/hash-constant bindings in `enc/` |

`finite_rules.py` expands declared observations, bindings and action sequences;
it must not acquire compiler-specific predicates. The retired grammar in
`parse/gen.py` was deleted. Its fixed numeric/character token reader, numeric
formatting/conversion and automatic-header scanner now use `parse/tokenread-*`,
`numeric-*` and `autoscan-*` declarations. PRN and PRNW instantiate one rule with
different widths. The module retains the generic assembler, dynamic word-trie
assembly and vocabulary/qualifier selection. `autonames()` still extracts header
resources. `optext()` binds binsel/irsel results to shared tape templates.
Any remaining compiler-specific control must stay visible in this inventory;
calling the file “shared support” does not remove that obligation. Keep a single implementation of each shared
routine when migrating it; E3, the unit reader, E4 and lowering use this module.
`src/` and `unisa/` remain the behavior reference until the specified switch.

## Work required before calling the refactor complete

- Finish the rule-source inventory above: remove the replaced control code,
  preserve actual dynamic dependencies, and enumerate old/new local transitions.
- Close source and failure compatibility gaps in the chosen product contract.
  The current fixed candidate restores the original 216-pass corpus baseline.
  This finite suite does not establish complete C99 compatibility.
- Validate the actual packaged network compiler, not just the table simulator:
  CLI, diagnostics, source-to-image, optimization, six targets and native execution
  on the available platforms, with input and artifact provenance.
- Reconcile the carried kernels, resource/template bytes and the one-copy model
  package with the S-17 size ledger; compare performance on the same inputs.
- Only then adopt the model path as the default product and update its docs.
  The current shipped `unisacc.com` has not switched.

## Fixed-control inventory

The final ordinary unary/dereference group is declared in `b128044`, including
shared ID.inc/dec and the static-auto rejection. Startup/unit markers,
identifier/update, error recovery, token positions and unit framing are now
also declared. Their combined five expanded E3 graphs at `11a0d56` match the
complete `70d3952` baseline; unit framing has separate ordinary/located graph
and actual-network checks. This is transition equality, not byte-identical
packaging or a new product acceptance result.

Source review of lower/opt and enc found no further complete input-dependent
fixed-control group to migrate. Python still binds dynamic ABI/target facts,
initializes data and connects parameterized templates. Encoder field schemas,
shared byte writers and target-program itoa/WINARGS machine-code templates
remain explicit sources. They must not be described as zero Python logic or
as migrated template algorithms. See [enc/CONTROL_AUDIT.md](enc/CONTROL_AUDIT.md).
The final parser helper inventory is checked separately before the candidate
is frozen; a shorter gen2.py alone is not completion evidence.

## Final candidate snapshot (acceptance pending)

The candidate built from source `85eaeb9` has SHA-256
`9a0ae470718ea4db28a59ea838344004c741ea0119c771c1370454209bdecd46`.
It contains a 6,095,823-byte package (SHA-256
`a1f364e119ea1be07cd3c8fa2ee9b9fe7c06be89e05ddbe362e6fa52c53332a8`)
inside a 6,279,167-byte `.com` container. The complete package occurs
physically once. Its 32 distinct constructed networks, 938 stage-route rows
and 21 resources have an artifact-specific inventory; all 32 network/table
pairs passed full declared observation-domain checks, including actions and
strings. Those checks prove table equivalence, not complete C99 semantics.

The fixed-package model driver reached N1=N2=N3 as identical complete
115,746-byte Mach-O files (SHA-256
`6e31c9664f4274f9bac02f46c9b773db7b5e53e0200725985c6b3b4b866dde03`).
The N3 successor passed empty-PATH O0/O2 memory and native probes against the
host; missing-package execution returns 2. The network package and APE
container were held fixed, so this does not establish their self-construction.

The specified six-target smoke passed: local macOS arm64 and Rosetta x86-64
container hello; Linux arm64/x86-64 and Windows arm64/x86-64 hello/fib/convert
through model-run and compiled/native execution against host output. Linux
x86-64 used Lima/QEMU; Windows used an ARM64 UTM guest and OS emulation of the
x86-64 container driver. This is bounded smoke, not complete platform-suite
acceptance or evidence from six physical machines.

### Measured compile-time cost

On macOS arm64, five fresh APE-process samples per compiler/input measured
`-O2 -b osx/arm64` through completed image write, excluding execution of that
image. The classic and model output SHA-256 matched for each input:

| Input | Classic median | Model median | Model/classic |
|---|---:|---:|---:|
| `examples/fib.c` | 0.06078575 s | 0.62056504 s | 10.209× |
| `unisacc.c` | 0.77623196 s | 19.42672129 s | 25.027× |

The timings include startup and model/package loading. They measure compiler
latency, not produced-program runtime. Raw samples, input hashes and compiler
hashes are in the local `/tmp/unisacc-final-perf-plan-928/summary.json`;
the final ledger still needs a durable reference. This cost remains to be
optimized while preserving byte-identical outputs.

The **complete current 143-item gate remains pending**. Old queue passes and
timeout attempts are historical records, not inherited results. `make com` and
the shipped `unisacc.com` still take the classic path. Complete the immutable
candidate's gate, retain failures/refusals/skips and artifact hashes, and
reconcile the final result ledger before changing the default or publishing.
Specified smoke and fixed-package bootstrap do not substitute for that gate.

## Earlier candidate evidence

The immutable candidate from `70d3952` is 6,278,823 bytes, SHA256
`a1115126d8281c93a1b38c3c3fc491779994201d14cf60cd2005933faa19ffb0`.
It runs actual constructed networks, not the Python table simulator:

- Original 220-file corpus: 216 pass, zero wrong/refused/slow, four existing
  known failures. Four shards, two slots, 21.86 seconds; no inputs removed.
- The 22 shared/target network-table checks pass over their full observation
  domains. This is local transition equality, not a proof of C semantics.
- Combined source validation: 25 host/ASM-network language probes (memory
  O0/O2 and native), six conditional rejections; E3 self-source tape equals the
  reference (the separate self-source check uses tables).
- CALL, function declarations and shared addr/fmtwalk controls were migrated in
  parallel. Each complete expanded graph matches before/after in five modes,
  including dynamic source perturbations. Function-pointer array support fixes
  the last original corpus refusal, 00209.

The same `a1115126` candidate also passed 429 optimization comparisons and
fixed-package driver N1=N2=N3 (115,746-byte Mach-O), with empty-PATH successor
execution. Linux arm64/x86_64 and Rosetta x86_64 each passed hello/fib/convert
model-run and compiled execution; Windows ARM64 guest passed those three on both
target ISAs. Linux x86 uses QEMU and Windows x86 uses emulation. These are smoke
checks, not full guest suites or model/container regeneration. Later rule-source
migrations are not silently credited to this older package.

Earlier immutable candidates retain separate evidence: `4d288bab` passed all
429 differential comparisons and the product CLI/diagnostic/tools/closure groups;
its two initial capacity/diagnostic test-contract failures were resolved by
requiring explicit rejection or host-correct execution. `6e60e3d1` demonstrated
fixed-package driver N1=N2=N3 (115,746-byte Mach-O). These results do not by
themselves validate the new candidate or prove model/container regeneration.

The older `81c04285` candidate has actual Linux arm64/x86_64, macOS Rosetta and
Windows ARM64 guest smoke evidence. Linux x86 uses QEMU; Windows x86 binaries use
ARM64 emulation. These are bounded smoke tests, not full six-platform acceptance,
and they do not validate the newer candidate.

Historical closure plan for those earlier candidates: parser/helper inventory,
full rebuild, corpus ratchet, bounded product and platform checks, and a physical
size ledger. The final `9a0ae470` candidate's current status is recorded above;
earlier artifact results do not transfer to it.

Resource and language checks now run in separate queue jobs. Core checks are split
into modes, contracts and dependencies; see [c/COMPILERCHECK.md](c/COMPILERCHECK.md).
Stage construction is independently schedulable before a validated final pack;
see [c/BUILDING.md](c/BUILDING.md). Parallel jobs use private artifact paths and a
frozen source tree. Passing a selected batch never implies the entire gate passed.

Joint source check: all five complete expanded E3 modes at `2d27cbf` match
`70d3952`, including observations, actions, strings and top-level metadata.
This checks the combined migrations, not only isolated branches; packed bytes
may still differ due to insertion order. Fixed `a1115126` separately passed all
15 selected product gate jobs, including 11 real tools. Neither result substitutes
for rebuilding and checking the final adopted artifact.

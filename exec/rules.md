# Rule-source migration and completion boundary

Source inventory refreshed after `70d3952` (2026-09-28). This is a remaining-work
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
| E3 | `parse2/tape-*`, `scope-*`, `declaration-*`, `width-*`, `type-tape.tsv`, `control-*`, `ladder-*`, `initializers-*`, `strings-*`, `constexpr-*`, `statics-*`, `vla-*`, `unresolved-*`, `printfallback-*`, `type-entry/default/follow`, `operator-*`, `shape-*`, `conversion-*`; gold type/tyinfo/prec and other existing facts | `parse2/gen2.py` expression/initialization control and dynamic type bindings (CALL, FN and shared address/format control now declared); helper modules for scopes, other literals, diagnostics, warnings, units and locations; string ESC data and shared initialization bindings |
| E4 | `opt/scans-*`, `local-*`, `stfuse-*`, `peep-*`, `analysis-*`, `parsers-*`, `rounds-*`; gold opinfo/peep | LEVEL selection, dynamic data initialization/dispatch and generic assembly in `opt/gen.py` |
| E5 lowering | `lower/data-*` sparse layout, `lower/code-scan-*`, `lower/code-print-*`, `lower/code-entry-*`, `lower/code-sysprep-*`, `code-abi-sources.tsv`, `code-syscall-*`, `armfuse-*`; existing gold/catalog facts | Dynamic ABI fact selection and template bindings in `lower/code.py` (fixed syscall/termination control now uses `code-syscall-*` and `code-shell-*`); ARM destination-shape policy; target/escape/layout bindings in `data.py`; shared optional u64 reader in `modelinput.py` |
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

## Current evidence and remaining work

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

Remaining closure obligations:

1. Finish E3's remaining hand-written control groups (notably expressions,
   and initialization glue), while retaining shared
   type/gold facts and deleting replaced Python control. E4's runtime algorithm
   and encoder runtime branches already use declarations; initialization and
   machine-code/format templates remain separately disclosed inputs.
2. Preserve the restored fixed corpus baseline while validating the remaining
   changes. Do not copy known classic-reference defects to achieve byte equality.
3. Run bounded joint CLI/diagnostic/optimization/platform and performance checks
   on the same immutable full model container. Update the size ledger for that
   artifact, separating weights, kernels, drivers, libraries and templates.
4. Switch default product construction and documentation only after that evidence
   passes. `make com` and shipped `unisacc.com` still use the classic route.

Resource and language checks now run in separate queue jobs. Core checks are split
into modes, contracts and dependencies; see [c/COMPILERCHECK.md](c/COMPILERCHECK.md).
Stage construction is independently schedulable before a validated final pack;
see [c/BUILDING.md](c/BUILDING.md). Parallel jobs use private artifact paths and a
frozen source tree. Passing a selected batch never implies the entire gate passed.

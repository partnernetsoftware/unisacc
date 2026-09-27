# Rule-source migration and completion boundary

Current source checkpoint: 4d351c5, including shared compound assignment conversion and sparse lowering operand namespaces, plus the earlier string/void fixes. This is a source
inventory, not a new specification language or a completion percentage.

The route is **declarations → finite transitions/actions → constructed threshold
networks → generic execution over stage byte streams**. Moving a rule to TSV
removes its Python control implementation; it does not erase the rule or imply
smaller weights. Shared action sequences and dynamic gold/catalog values must
remain shared inputs, not copied answers.

| Stage | Declaration sources already used | Remaining handwritten construction |
|---|---|---|
| E1 | `lex/` declarations | Data assembly and bindings; runtime model is separate from the old product lexer |
| E2 | [pp/rules.md](pp/rules.md) | Header-name extraction, resource ordering, initialization and assembly bindings |
| E3 | `parse2/tape-*`, `scope-*`, `declaration-*`, `width-*`, `type-tape.tsv`, `control-*`, `ladder-*`, `initializers-*`, `strings-*`, `constexpr-*`, `statics-*`, `vla-*`, `unresolved-*`, `printfallback-*`, `type-entry/default/follow`, `operator-*`, `shape-*`, `conversion-*`; gold type/tyinfo/prec and other existing facts | `parse2/gen2.py` expression/call/initialization control and dynamic type bindings; helper modules for scopes, other literals, diagnostics, warnings, units and locations; string ESC data and shared initialization bindings |
| E4 | `opt/scans-*`, `local-*`, `stfuse-*`, `peep-*`, `analysis-*`, `parsers-*`, `rounds-*`; gold opinfo/peep | LEVEL selection, dynamic data initialization/dispatch and generic assembly in `opt/gen.py` |
| E5 lowering | `lower/data-*` sparse layout, `lower/code-scan-*`, `lower/code-print-*`, `lower/code-entry-*`, `lower/code-sysprep-*`, `code-abi-sources.tsv`, `code-syscall-*`, `armfuse-*`; existing gold/catalog facts | ABI fact selection, initialization/termination shells and template bindings in `lower/code.py`; ARM destination-shape policy; target/escape/layout bindings in `data.py`; shared optional u64 reader in `modelinput.py` |
| E5 encoding | `enc/armint-*`, `armmem-*`, `armbranch-*`, `armfp-*`, `arminput-*`, `armlayout-*`, `armwin-*`, `armbase-*`, `armcontract-*`, `x86-operand-*`, `x86-line-*`, `x86-procs-*` (including relaxation); catalog opcode facts | x86 concrete instruction encoding and setup; ARM dynamic instruction/shape bindings and image-layout bindings; the Windows command-line template remains a shared source |
| E6 images | `enc/elfimage-byte/result.tsv` shared image control and `enc/pedelta-byte/result.tsv` PE control, `enc/machodelta-*` Mach-O layout/signature-page control, `enc/sha256-*` shared digest control; existing target layout facts | Format header/section/CD fields and import/string enumeration, child installation and dynamic layout/hash-constant bindings in `enc/` |

`finite_rules.py` expands declared observations, bindings and action sequences;
it must not acquire compiler-specific predicates. The retired grammar in
`parse/gen.py` was deleted. Its fixed numeric/character token reader, numeric
formatting/conversion and automatic-header scanner now use `parse/tokenread-*`,
`numeric-*` and `autoscan-*` declarations. PRN and PRNW instantiate one rule with
different widths. The module retains the generic assembler, dynamic word-trie
assembly and vocabulary/qualifier selection. `autonames()` still extracts header
resources. `optext()` binds binsel/irsel results to shared tape templates.
Those live rules also remain to be declared; calling the file “shared support”
does not remove this obligation. Keep a single implementation of each shared
routine when migrating it; E3, the unit reader, E4 and lowering use this module.
`src/` and `unisa/` remain the behavior reference until the specified switch.

## Work required before calling the refactor complete

- Finish the rule-source inventory above: remove the replaced control code,
  preserve actual dynamic dependencies, and enumerate old/new local transitions.
- Close source and failure compatibility gaps in the chosen product contract.
  The current differential candidate rejects bit-fields. Other unsupported
  forms remain outside that finite test set; a correct rejection is not support.
- Validate the actual packaged network compiler, not just the table simulator:
  CLI, diagnostics, source-to-image, optimization, six targets and native execution
  on the available platforms, with input and artifact provenance.
- Reconcile the carried kernels, resource/template bytes and the one-copy model
  package with the S-17 size ledger; compare performance on the same inputs.
- Only then adopt the model path as the default product and update its docs.
  The current shipped `unisacc.com` has not switched.

The earlier checkpoint d5dcda4 candidate was 6,005,111 bytes, SHA256
`3ffcb9bdada262e13bc9a9e8ac3c638059e8eab465690e90ffe7c44f3565cf64`.
It grew by 19,592 bytes after rule renumbering; this is not a compression result.
The E3 graph is identical to checkpoint 2273f3e, including every action, and
its hidden-unit count remains 9,806, with changed state/sequence order.
It produced reference-identical hello images for six targets; six programs, including floating arithmetic and conversions,
at O0/O1/O2 ran identically to host cc on macOS arm64. These are bounded
checks, not full product acceptance or cross-platform native verification.
Exact run records are in `prd.md`. Historical prototype results must not
substitute for the remaining completion obligations.

Actual packaged real-tools check at 7fd1514: 6/11 pass, 0 wrong, 5 refused.
The prior d5dcda4 artifact had 6 wrong and 5 refused. Braced character-string
initialization was also wrong in the classic C reference; matching that reference
alone was insufficient. The 7fd1514 candidate is 6,017,681 bytes, SHA256
`13b2b3221ed3b5f046761518ec2a00af2a35e75c758a2a1fb16f29ee983abb7d`.
It does not yet include 080e638 scalar void casts. The 11-project baseline remains
required; this is not product acceptance. `TOOLS_UA` explicitly selects the binary
for `tests/tools.sh`; its default still tests the Python compiler.

The follow-up 9157c00 candidate includes scalar void casts: 6,017,901 bytes,
SHA256 `d8358e4b1b3f87a076715fead6c15b5216a3143fe82544ea6f9f93f630e06ae7`.
Real tools still report 6/11: regex2 passes parsing but fails downstream encoding.
Investigation found a lowering ARG/TXT address-region collision on the larger
input. Commit 3be914f separates the regions and widens operand indexing; the
original regex2 then passed the complete constructed-network-to-native route
against host cc in isolated verification. That fix is not in the artifact above. Root strings, fixed-size string rows and void
casts now run in the existing ASM network driver regression, with host-cc results
and both in-memory and native execution. Inferred leading dimensions remain a
separate compatibility gap.

Compound assignments now use the shared operator result and destination
conversion paths (961c50c), removing 13 Python lines. The old 316-probe set has
309 unchanged outputs and seven necessary narrowing changes, all matching the
corrected C reference. Persistent integer and pointer probes also compare host
cc with the actual ASM network driver at O0/O2 and native O2. Fixed-array shape
work is still isolated; it has not yet closed the real-tools baseline.

The 9a38e3b candidate closes the original 11-project tools baseline through
the complete network compiler: 11 pass, zero wrong/refused/skipped. It is
6,019,097 bytes, SHA256
`d07fed1dd459731bd6b7a5a00e24aefa1f455de44b6890d639a3c1cc891199f0`.
Shared fixed-array shape declarations and operand namespace fixes are included.
The shape change adds 36 Python lines and 277 TSV lines; it closes real input
gaps and is not code reduction. Array-typedef extra suffixes, T**, T* members,
and T* function returns still reject; no wider compatibility is implied.

Expanded differential acceptance keeps all 141 existing input files and all
three optimization levels. The same candidate has 393 matching runs and 30
explicit refusals (10 files); no accepted run differs. Refusals cover parameter
declarators, address/compound-literal forms, wide strings, bit-fields, and
preprocessor expression/paste forms. The gate remains failing for these gaps.
Four bounded queue shards replace the monolithic timed-out job; the classic
reference passes all 423 comparisons under that stronger harness.

The 66709ab candidate adds shared parameter suffixes and narrow/wide string
walking. Full-network differential acceptance is now 405/423 matching runs,
zero wrong outputs, and 18 refusals across six original inputs; all four
shards finish in a 34.01-second two-worker window. The candidate is 6,100,290
bytes, SHA256 `b66aaba72e2c3681faec4d04e3ac8d2e240e56fc1236eaf754122595b615133c`.
The remaining refusals are compound/address expressions, bit-fields, and
preprocessor expression/paste forms. Unicode universal-character escapes
still refuse. E2 uintmax rules added afterwards are verified independently;
they are not in this candidate, and function macros in #if remain pending.

The following source-only migrations retain complete transition graphs under
ordinary and perturbed dynamic inputs: ARM input contracts remove 38 Python
lines (86 declaration lines added), x86 operand scanning removes 34 Python
lines (90 declaration lines added). Shared address control removes 25 Python
lines; anonymous compound storage removes seven while reusing INITVALUE and
AS.struct. Their full packaged acceptance is pending; classic bit-field
assignment results were corrected independently before the model migration.

At 4d351c5, the complete six-target package runs the original 11 tools (11/11)
and 420 of 426 optimization differential cases with zero wrong outputs. Only
the original bit-field test and the added bit-field assignment-result test
refuse, at all three levels. The four-shard queue takes 36.99 seconds; refusals
still fail it. Package: 6,150,292 bytes, SHA256
`95900b79bad936f874edd6368066be352f2974b86d3e10757bd68a846e6722d2`.
These results do not cover arbitrary C99 inputs or complete native platform
acceptance, and the default shipped compiler remains the classic route.

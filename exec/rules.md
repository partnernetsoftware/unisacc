# Rule-source migration and completion boundary

Current source checkpoint: 080e638, including VLA, unresolved-call, printf fallback, shared x86 control and string/void compatibility fixes. This is a source
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
| E3 | `parse2/tape-*`, `scope-*`, `declaration-*`, `width-*`, `type-tape.tsv`, `control-*`, `ladder-*`, `initializers-*`, `strings-*`, `constexpr-*`, `statics-*`, `vla-*`, `unresolved-*`, `printfallback-*`, `type-entry/default/follow`, `operator-*`; gold type/tyinfo/prec and other existing facts | `parse2/gen2.py` expression/call/initialization control and dynamic type bindings; helper modules for scopes, other literals, diagnostics, warnings, units and locations; string ESC data and shared initialization bindings |
| E4 | `opt/scans-*`, `local-*`, `stfuse-*`, `peep-*`, `analysis-*`, `parsers-*`, `rounds-*`; gold opinfo/peep | LEVEL selection, dynamic data initialization/dispatch and generic assembly in `opt/gen.py` |
| E5 lowering | `lower/data-*` sparse layout, `lower/code-scan-*`, `lower/code-print-*`, `lower/code-entry-*`, `lower/code-sysprep-*`, `code-abi-sources.tsv`, `armfuse-*`; existing gold/catalog facts | ABI/SYSCALL body in `lower/code.py`; ARM destination-shape selection and dynamic bindings; target/escape/layout bindings in `data.py` |
| E5 encoding | `enc/armint-*`, `armmem-*`, `armbranch-*`, `armfp-*`, `arminput-*`, `armlayout-*`, `armwin-*`, `x86-procs-*` (including relaxation); catalog opcode facts | x86 concrete instruction encoding and setup; ARM core/formatting and dynamic image-layout bindings; the Windows command-line template remains a shared source |
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
  The checkpoint candidate still rejects bit-fields, several declarator/compound
  literal forms, and wide strings. A correct rejection is not support.
- Validate the actual packaged network compiler, not just the table simulator:
  CLI, diagnostics, source-to-image, optimization, six targets and native execution
  on the available platforms, with input and artifact provenance.
- Reconcile the carried kernels, resource/template bytes and the one-copy model
  package with the S-17 size ledger; compare performance on the same inputs.
- Only then adopt the model path as the default product and update its docs.
  The current shipped `unisacc.com` has not switched.

The latest rebuilt candidate, from checkpoint d5dcda4, is 6,005,111 bytes, SHA256
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
input; its repair remains pending. Root strings, fixed-size string rows and void
casts now run in the existing ASM network driver regression, with host-cc results
and both in-memory and native execution. Inferred leading dimensions remain a
separate compatibility gap.

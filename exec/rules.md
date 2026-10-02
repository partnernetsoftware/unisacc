# Rule-source migration and completion boundary

Source inventory refreshed after `11a0d56` (2026-09-28). This is a remaining-work
list, not a completion percentage. The inventory describes offline construction sources; it does not declare
product adoption incomplete. The authoritative [pipeline design](../prd.md#pipeline-design)
and [S-17 migration archive](../archive/s17-migration-log-20260928.md) separate
current behavior from historical acceptance evidence.

The route is **declarations → finite transitions/actions → constructed threshold
networks → generic execution over stage byte streams**. Moving a rule to TSV
removes its Python control implementation; it does not erase the rule or imply
smaller weights. Shared action sequences and dynamic gold/catalog values must
remain shared inputs, not copied answers.

| Stage | Declaration sources already used | Remaining handwritten construction |
|---|---|---|
| E1 | `lex/` declarations | Data assembly and bindings; runtime model is separate from the classic reference lexer |
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
`src/` and `unisa/` remain behavior references after the default switch;
the explicit classic fallback is not an automatic retry for model refusals.

## Current validation boundary

Each generated δ is checked against its table over the finite observation domain; the packaged product is then tested on source, tape, image, diagnostics and native execution. The current adopted route, coverage limits and release identity are tracked in [prd.md](../prd.md), [the package contract](c/PACKAGE.md) and the release receipts. Old candidate identities and fixed-control milestones are preserved in [the rule history](../archive/exec/rules-history-20261002.md).

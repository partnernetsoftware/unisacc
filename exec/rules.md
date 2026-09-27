# Rule-source migration and completion boundary

Current source checkpoint: 975ee7c, after array helpers and address/output rules. This is a source
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
| E3 | `parse2/tape-*`, `scope-*`, `declaration-*`, `width-*`, `type-tape.tsv`, `control-*`; gold type/tyinfo/prec and other existing facts | `parse2/gen2.py` grammar/control, types and initialization; helper modules for scopes, literals, diagnostics, warnings, units and locations |
| E4 | `opt/scans-*`, `local-*`, `stfuse-*`, `peep-*`, `analysis-*`, `parsers-*`, `rounds-*`; gold opinfo/peep | LEVEL selection, dynamic data initialization/dispatch and generic assembly in `opt/gen.py` |
| E5 lowering | `lower/data-*` sparse layout, `lower/code-scan-*`, `lower/code-print-*`; existing gold/catalog facts | `lower/code.py`, `armfuse.py`; target/escape/layout bindings in `data.py` |
| E5 encoding | `enc/armint-*`, `armmem-*`, `armbranch-*`, `armfp-*`, `arminput-*`, `armlayout-*`; catalog opcode facts | x86 encoding and setup, ARM Windows setup; dynamic image-layout bindings, shared formatting helpers |
| E6 images | Existing target layout facts | ELF, Mach-O, PE, memory layout and signature construction in `enc/` |

`finite_rules.py` expands declared observations, bindings and action sequences;
it must not acquire compiler-specific predicates. The retired grammar in
`parse/gen.py` was deleted, but that module still contains shared token reading,
numeric output and decimal-float conversion rules as well as the generic
assembler. Those live rules also remain to be declared; calling the file
“shared support” does not remove this obligation.
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

The latest rebuilt candidate, from the earlier 0ae4ad2 checkpoint, is 5,985,531 bytes, SHA256
`9b5b710bd97cc590371a9fc73de159ee8aa6f0fcfffa34f297ae0d3a54e1bbc7`.
It produced reference-identical hello images for six targets; four programs
at O0/O1/O2 ran identically to host cc on macOS arm64. These are bounded
checks, not full product acceptance or cross-platform native verification.
Exact run records are in `prd.md`. Historical prototype results must not
substitute for the remaining completion obligations.

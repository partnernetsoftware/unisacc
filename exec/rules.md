# Rule-source migration and completion boundary

Current checkpoint: 84cfcc8, after structured control, analysis and sparse layout. This is a source
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
| E5 lowering | `lower/data-*` sparse layout; existing gold/catalog facts | `lower/code.py`, `armfuse.py`; target/escape/layout bindings in `data.py` |
| E5 encoding | `enc/armint-*`, `armmem-*`, `armbranch-*`, `armfp-*`; catalog opcode facts | x86 encoding and setup, ARM input/layout/Windows setup, shared formatting helpers |
| E6 images | Existing target layout facts | ELF, Mach-O, PE, memory layout and signature construction in `enc/` |

`finite_rules.py` expands declared observations, bindings and action sequences;
it must not acquire compiler-specific predicates. `parse/gen.py` is now only
shared construction support; the retired grammar implementation was deleted.
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

The latest measured candidate before this checkpoint is from d487647:
5,985,198 bytes, with the actual network reader-error fix. The later rule-only
batch was checked by complete transition comparisons and a combined gate; it
was not represented as a newly rebuilt candidate. Exact hashes and run records
are in `prd.md`. Historical prototype results must not substitute for these
remaining completion obligations.

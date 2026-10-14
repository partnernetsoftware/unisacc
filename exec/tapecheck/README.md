# Tape domain stage (C43, implementation in progress)

`gen-manifest.tsv` installs the authored byte/result transition tables through the
existing generic graph assembler. The C seed constructor and Python constructor
produce identical JSON. The product runs the resulting integer network using the
shared execution core; there is no C Tape grammar in the driver.

The model reads only input bytes. Successful validation emits the entire original
stream, byte for byte, after EOF. Rejection emits no stdout and names the cause and
physical line in stderr. CRLF counts as one line. Target syscall/ABI/library
capability checks remain in lower. Validation has no filesystem or host-symbol
resource lookups.

Covered rules: the 71 `unisa.tape.SHAPE` opcodes, operand counts, r0–r7, signed
integer lexical forms (decimal/base prefixes), grouped register displacements,
labels/comments, `.str` quoted/escaped bytes and the reference reader's 1 MiB
literal bound, `.bss`, `.unit 2`, and linkage records (including the unit prerequisite
for `.gdef`). `shape.tsv` documents the ordinary opcode schema; the transition
tables are the executable rule source, not generated runtime output.

Known coverage boundaries are not acceptance claims: underscore integer spellings
are explicitly refused as `not covered`; quoted ordinary symbol operands are
likewise not covered. Exotic label spellings and duplicate-resource corner cases
are not certified by this sample set. This lexical stage does not claim undefined
reference-resolution rules or target capability checks. No trusted flag substitutes
for running the model on the exact bytes passed to lower.

## Actual measurements before driver/package integration

- All 303 `.c`/`.cx` files recursively under `examples/` and `tests/c/` were attempted
  with the existing 0.0.42 product's `-S`. Its 282 successful tapes were accepted,
  emitted unchanged, and independently accepted by `unisa.tape.parse`.
- The remaining 21 sources were refused by the old product before an output Tape
  was produced: one reason family, undefined external functions (setpgid, curl and
  Windows APIs). They are not validation-stage failures or passes.
- 418 fixed opcode/record/boundary/point-mutation inputs: both accept 89; both refuse
  329; model-refuse/reference-accept 0; model-accept/reference-refuse 0. Every
  model refusal had an empty stdout, named cause and line; `unreachable`/missing-line
  count 0. These are narrow samples, not exhaustive reference equivalence.
- The initial diagnostic branch error and missing grouped displacement coverage
  were fixed. A replay-helper shared-output race was repaired and both affected
  runs discarded before independent reruns. Old first-red evidence is retained
  under `/tmp/c43-tapecheck`; it is not rewritten as an initial pass.

`check.sh 1|2|3` builds the model and execution adapter privately, compares the two
constructors, and runs the dynamic C contract `tests/tapecheckmodel.cx` on bounded
shards. Python is only the existing parse reference and construction oracle.
Driver/raw/Tapebin/forwarding/package integration is a separate next change.

Observation artifact: old 0.0.42 product SHA256 `ba11d636b54adcdc519a81af7553d15ab882ecb85622bdc16968d966acdb8f5f`. The final native-execution model SHA256 is `a111de7d94ad82c849391cb333b086c0b526d7373dd5cb0f199b09f452ed27a5` (508 banks, 116058 bytes). `tests/tapecheck-corpus-ledger.tsv` binds all 303 source hashes and the 282 captured Tape hashes. These records prove model behavior only; they do not bypass product freshness or qualify a new product.

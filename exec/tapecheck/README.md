# Tape domain stage (C43)

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
Driver/raw/Tapebin/forwarding/package integration is now in this working change; its fresh-product evidence is separate from those historical model measurements.

Observation artifact: old 0.0.42 product SHA256 `ba11d636b54adcdc519a81af7553d15ab882ecb85622bdc16968d966acdb8f5f`. The final native-execution model SHA256 is `a111de7d94ad82c849391cb333b086c0b526d7373dd5cb0f199b09f452ed27a5` (508 banks, 116058 bytes). `tests/tapecheck-corpus-ledger.tsv` binds all 303 source hashes and the 282 captured Tape hashes. These records prove model behavior only; they do not bypass product freshness or qualify a new product.

## TP-9 word range (owner ruling implemented)

The owner chose the host-independent word interval `[-2^63, 2^64-1]`.
Integer operand validation checks magnitude **before** multiplication/addition;
negative magnitude is additionally limited to `2^63`. Errors name `integer` and
physical line. The model uses existing A64/C64U core actions, not a driver parser.
`unisa.tape._tape_integer` performs the same bounded accumulation and never asks
Python to convert an unbounded decimal string. `.bss` uses decimal spelling and
must be nonnegative. Record/index/target displacement restrictions remain separate
contracts; word-width acceptance alone is not proof of those narrower capabilities.

Signs precede prefixes and apply to every supported base. Decimal and `0x` signs
follow the C reference `ts_integer` (`src/tapescan.h:10-13`): `-0x8000000000000000`
is legal, `-0x8000000000000001` is not. Python/reference/model already supported
`0o`/`0b`; their existing lexical forms retain the same sign and word rule.
The C raw reader's scanner accepts only decimal/hex; acceptance of binary/octal
here does not claim C reader or target encoder coverage. `s` operands beginning with a sign/digit follow the C raw reader: validate them as
word literals, retaining their spelling. `L` and nonnumeric `s` operands remain
symbolic tokens, not rewritten or resolved against the host.
Underscore spellings remain a registered model undercoverage: the fixed
four `registered-underscore*` cases expects reference acceptance, model `not covered`,
empty stdout and a line. The selftest reports it separately, never as product
acceptance or matched refusal.

cc's former 4301-digit disagreement exposed Python's implicit host conversion
limit, not a Tape rule. It is now an ordinary refusal fixture: both implementations
reject it for explicit word range. A 4301-zero fixture is accepted, showing that
a character-count ceiling was not substituted. The obsolete known-difference
entry has been removed after actual model/reference replay. Host environment
cannot change either result.

### Inspected C/backend paths (code review, not extra execution evidence)

- `src/front_parse.c:6219` (`fe_read`) copies bytes; raw `-S` alone was not a range
  witness before this change.
- `src/tapescan.h:5-24` bounds magnitude before multiply/add against `~0UL`;
  `src/back_lower.c:385-391` limits negative magnitude, and
  `src/back_lower.c:531-541` applies that scanner to raw immediate/numeric operands.
  The repository's 64-bit path accepts positive words through `2^64-1` and negative
  words down to `-2^63`, refusing wider words via `bk_tape_bad` at line 380.
  The unguarded catalog helper `bk_num` at line 365 is a different path.
- `.bss` has an additional structural cap of `2^31-1`
  (`src/back_lower.c:450`); that is not a host integer digit limit.
- Lower ARM fusion's bound skips to GENERIC rather than universally rejecting
  Tape (`exec/lower/armfuse-result.tsv:88-98`). ARM encoder accumulation is guarded
  (`exec/enc/armcontract-result.tsv:12-28`) and has narrower field checks.
  x86's general numeric scan accumulates before an ITOA-specific guard
  (`exec/enc/x86-operand-byte.tsv:8`, `exec/enc/x86-operand-result.tsv:2-12`);
  that was not evidence of a universal backend overflow refusal. The front word
  guard now prevents out-of-word literals from reaching those paths.

The prior 418-case measurement above remains historical. The new fixed set is
445 cases: 441 matched-decision fixtures plus four registered undercoverages, including
all four bases' positive/negative boundaries and the two 4301-digit observations.
Post-integration fresh-product evidence is reported separately; Windows library
mode still awaits its reference contract and native evidence.

### Additional reference behavior changes

Negative `.bss` sizes now explicitly report `invalid .bss size`, following the
nonnegative C check at `src/back_lower.c:450`; the fixed record-negative fixture
contains `.bss s -1`. Previously Python converted the negative integer and passed
it onward. Numeric-looking `s` operands now undergo integer validation, following
`src/back_lower.c:531-541`; `numeric-symbol-malformed` fixes `1oops` as a refusal.
An actual scan of the 282 accepted compiler-produced tapes found zero numeric-leading
label/resource/linkage symbol definitions. These tapes provide sampled compatibility evidence;
this does not establish a universal prohibition on every possible symbol spelling.

The integrated model has 526 states and 117816 network bytes (the earlier 117785
measurement preceded the numeric-symbol transitions). The fresh private artifact
is `/tmp/c43-tape-domain-range/unisacc-next.com`, SHA256
`92236ce330ed5ea109922337f13d85ceaddbde488c57346e38ab862bc63729ab`.
The product black-box check actually passed all 78 mode cases; no release seal or
whole queue was run.

## Integrated regression evidence

Actually run on the fresh artifact above: all three model shards returned rc0
(the final shard includes four registered underscore-integer forms), and the
black-box product check returned rc0 for all 78 mode cases. cc independently ran
`exec-driver-core-modes` with the original limit after bounded preparation warmed
the existing content-keyed cache: rc0 in 24 seconds, all 27 mode/level matches.
cc also compared 50 sampled sources against the old product `ba11d636`: Tape
text, Tapebin packages, images from both inputs and exit codes were identical.
These independent runs are reported by cc, separate from our own executions.

`com-tapebin-1..3` are previously ruled baseline timeouts on this host; our
attempts supplied no new product evidence. Their native-machine evidence is left
to CI. The cold-preparation timeout is recorded separately from model rejection
and integer-domain NEG results; no limit was increased or fixture removed.

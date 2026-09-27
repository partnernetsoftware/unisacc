# Stage formats (prd.md S-17)

Each format has a checker `check_<fmt>.py FILE` (dots become `_`; exit 0
valid, 1 invalid with the reason on stderr).  `run.py` checks every stream.

- **src.c** -- a C source file: bytes, no NUL.
- **pp.text** -- the reference's `-E` output.  Measured byte-identical to the
  buffer the reference's lexer reads (`UA_LEXIN`, exec/lex/mkpre.sh), so E2's
  output is E1's input.  Text, no NUL, no `#include`/`#define` line left.
- **tokens.plain** -- the reference's `-dump-tokens`: one token per line,
  `kind` or `kind=value`, then `eof`, then `N tokens` (N counts eof).  A `type` token carries no spelling.
- **tokens.typed** -- tokens.plain where every `type` token is `type=SPELLING`:
  E3 needs the spelling and stages share no symbol table.  E1's delta writes
  it with `exec/lex/gen.py --typed` (the type token's bytes copied by the same
  generic SPAN action as id/num/str); the reference side is the patched
  `/tmp/ua_tdump` (exec/parse/mkdump.sh, `UA_TYPESPELL=1`).  Without --typed
  the delta is byte-identical to the tokens.plain one.
- **tape.text** -- the reference's `-S -o -` tape: `label:` lines, `.dir`
  lines, indented instructions.

## Preprocessor diagnostic envelope (development)

`exec/pp/gen.py --locations` emits **pp.locations**, while its default remains
plain **pp.text**. The envelope is produced by model actions; the C/ASM host
does not interpret its fields. The optional E1/E3 location modes consume it as described below. It is not
yet connected to the compiler CLI route and does not make `-Wall` available.

- Magic: `UNIPP1` followed by NUL (7 bytes total).
- Five little-endian unsigned 32-bit words: preprocessed text byte length,
  forced-include prefix line count, automatically inserted include line count,
  splice-record count, include-record count.
- One 32-bit byte offset for each joined continuation, in encounter order.
- Each include record: 32-bit insertion line, inserted line count, name byte
  length, then the name bytes (no terminator). As in the current reference
  diagnostic map, names are limited to their first 62 bytes. Records retain
  insertion order; nested regions must be undone from last to first.
- Exactly the declared number of preprocessed text bytes, with no trailer.

These are the reference's diagnostic inputs, not raw-source offsets: macro
expansion can change columns, and inserted headers change line counts. The
future consumer must apply forced-prefix, reverse include-region, automatic
prefix and continuation adjustments in the reference order, and suppress
warnings inside header regions. It must not recover locations by searching
for token text. Source path is the route's existing source-name input.

`exec/pp/locationcheck.py` builds an instrumented reference only in scratch
and compares all records and text byte for byte, including nested/repeated
includes, forced/automatic includes, LF/CRLF continuations and macros. Its
Python decoder is a test oracle, not part of the product route.

## Token offsets (development)

`exec/lex/gen.py --positions` consumes ordinary **pp.text** and emits
**tokens.positions**: before each typed token line, including `eof`, it writes
`@`, four little-endian offset bytes, then LF. The final `N tokens` line has
no prefix. Offsets index the preprocessed byte buffer; EOF is its length.
The four bytes are binary, so a consumer must not split the entire stream on
newlines. Token spelling follows the existing tokens.typed contract.

This mode implies `--typed`; ordinary/default generation is unchanged.
Offsets are emitted by model actions from the lexer's saved start cursor,
not reconstructed by searching for spelling. `positioncheck.py` compares
against the reference lexer's actual `tpos` array in a scratch build, plus
ordinary typed output after removing the fixed-size prefixes. The optional joined envelope below carries this stream and the preprocessing
map together. `-Wall` is still unavailable.


## Joined token locations (development)

`exec/lex/gen.py --locations` consumes **pp.locations** and emits
**tokens.locations**: `UNITOK1` plus NUL (8 bytes total), one little-endian
32-bit length, that many bytes containing the complete pp.locations envelope,
then tokens.positions through its final count line. It validates framing,
record extents and splice positions before emitting anything. Declared lengths
must fit nonnegative signed 32-bit input cursors. The lexer scans a bounded
copy of the preprocessed text, so its offsets remain text-relative.

`exec/parse2/gen2.py --locations` consumes tokens.locations. It retains the
source text blob, forced/automatic line counts, splice offsets and include
records for diagnostic use. It reads each token's position prefix before
entering the ordinary token reader; parser token-buffer positions still point
to the prefix. This preserves original-buffer rewinds and bounded views used
for strings and initialisers. `source_pos` is the current text offset;
`TOKEN_POS[token_buffer_position]` retains it across subsequent reads. The
normal tape output has no location prefixes or metadata. The location reader
also passes through the ordinary stable-token-ordinal registration, so static
local storage labels are unchanged by diagnostic mode.

`exec/lex/locationcheck.py` checks the E2/E1 join and malformed frames.
`exec/parse2/locationcheck.py` checks the E2/E1/E3 join against ordinary tape
output and reads every saved map field back through a test-only model
continuation. Neither test inserts a language-specific executor primitive.
The compiler CLI does not select these modes yet. Multi-unit location framing,
warning selection and -Werror handling remain to be connected; these location
tests alone do not claim warning compatibility.


## Diagnostic rendering and warning rules (development)

E3 `--warnings` implies `--locations`. It implements the reference's `-Wreturn-type` decision, including its
last-statement heuristic and main/void exemptions, plus `-Wint-conversion`
for assignments and local scalar initializers. The latter preserves the
reference's syntactic lone-zero and call-result exemptions. Included-header
warnings are suppressed. It is not a
full `-Wall` mode and is not selected by the compiler CLI. Multi-unit integration and `-Werror` remain pending.

The model renderer derives file, line and column from retained preprocessing
records and emits the source line/caret to stderr. `diagnosticcheck.py` compares
it with the actual reference diagnostic functions, including EOF/clamped
positions, tabs, splices and header suppression. `returnwarningcheck.py`
compares both tape and diagnostics and also checks that the quiet route emits
no warnings. Neither adds a language-specific executor primitive.

`returnwarningcheck.py --int-conversion` exercises the second rule on the
same harness: it compares complete tape and diagnostic bytes, and checks the
quiet route separately. This migration preserves the reference behavior;
these checks do not establish full C conversion-constraint correctness.

The optional mode also implements block-local `-Wunused-variable`. Binding
and use records follow the existing scope undo stack, preserve shadowed
bindings, and report in declaration order when a block closes. Function
parameters and static/global objects are not warned about by this reference
rule. Identifier use follows the reference token-context test (a standalone
simple write is not a read), rather than tape liveness. `--unused` on the
warning check exercises the rule and its interaction with the other warnings.


The optional mode also implements the reference `printf` format preflight and
`-Wformat` checks. It parses arguments for their types, retains diagnostics,
then rewinds emitted tape and the literal-pool cursor before the real call.
Reference label allocation is retained; therefore warning-mode tape need not
equal quiet-mode tape. Each mode is compared with its matching reference.
`--format` checks 36 cases, including nested calls and all four warning kinds
together; `--shard 0/2` and `--shard 1/2` split this bounded check. This retains
the existing reference policy, not a claim of complete format validation.

The checks also exposed a missing scalar-float default argument promotion in
the ordinary parser route. An argument with no recorded formal kind now goes
through the existing float-to-double conversion. `probes/vararg_float.c`
checks both a float variable and a cast; this does not assert completeness of
all parameter conversions.

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
does not interpret its fields. It is not yet connected to the compiler route
or consumed by E1/E3, and does not make `-Wall` available.

- Magic: seven bytes `UNIPP1` followed by NUL.
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
ordinary typed output after removing the fixed-size prefixes. The optional
pp.locations envelope is still separate: connecting its text and mapping to
this stream and E3 is remaining work, and `-Wall` is still unavailable.

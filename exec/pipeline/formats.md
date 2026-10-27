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

The E1/E3 connection also carries constructor and destructor marks. With
`\0cli/f1`, E1 frames its typed tokens as `USLATTR1`, two LE32 words (mark
count and token byte length), then count records of LE32 token ordinal and
one kind byte, followed by the ordinary typed tokens. `run.py` removes this
frame before comparing or checking `tokens.typed`; it adds unit zero to each
record and passes the resulting `\0cli/attributes` resource to E3. The
reference front end retains these marks outside its token stream. Dropping
the resource can make E3 accept a source while omitting a constructor call
from the tape, as `probes/f1_header_ctor.c` demonstrates.

## E3 token trailer (enforced by every E3 model)

E3 checks the end of its token input after the startup scan reaches the
final `eof` token (statics `TT.*` rows; the located readers route a failure
to their own frame refusal):

- **tokens.plain / tokens.typed and UNITOK1 (single unit):** right after the
  `eof` record comes exactly one count line, `N tokens` LF, and then the end
  of the input. N is canonical decimal (no sign, no leading zero, at least
  1) and must equal the number of token records from the start of the token
  stream through `eof`. Records are counted structurally, not through the
  parser: a UNITOK1 record is `@`, four offset bytes, LF and its token text;
  a tokens.typed record is one line. A line that does not start a record
  (a blank, white-space- or quote-led line in tokens.typed; any line not led
  by `@` in UNITOK1) continues a multi-line string token. Folding by the
  parser (parenfold) and look-ahead do not change N.
- **UNITOK2:** there is no count line; the input must end right after the
  final prefixed `eof` record.
- A missing, wrong, padded or repeated count line, or any byte after it, is
  refused: located E3 (UNITOK1/UNITOK2, all CLI routes) with
  `reject: not covered: malformed token locations`; the plain E3 with
  `reject: not covered: truncated token dump`.

The USLATTR1 frame's own length fields are validated by the C adapter that
removes the frame, not by E3. After the 16-byte header it requires exactly
`count * 5` record bytes plus the declared payload byte length; an inconsistent
count/extent or payload length is refused as `run: malformed USLATTR1 side-car`
with status 2 and no output. The single-stream adapter separately refuses a
truncated header (recognized magic but fewer than 16 bytes) as
`run: invalid attribute frame`, also with status 2 and no output.

## target.text (lower -> elf input)

`target.text` is the lowering stage's output and the image stage's input:
header lines, then target instruction lines (labels, `.dir` and instruction
forms of the selected ISA). Header lines start with `@`; a key is followed by
one or more spaces and its value up to LF. The ELF/Mach-O/PE/object and memory
image routes of all six targets accept these header keys (x86_64 in
`exec/enc/address-*`, arm64 in `exec/enc/armlayout-*`, shared image input in
`exec/enc/elfimage-*`):

| key | value | required | contract |
|---|---|---|---|
| `@target` | `lnx/x86_64` ... `win/arm64` | yes | selects the address layout; must match the image format |
| `@data` | lowercase hex byte pairs, or `-` | no (absent = empty) | stored initial data prefix, through its last nonzero byte |
| `@sym` | `NAME DECIMAL` | no, repeatable, unique | named data offset (DATA_BASE 256 = first data byte) |
| `@src_os` | os name | no | carried |
| `@data_len` | decimal digits only, at most 2^31-1, not below the stored `@data` length | yes | full logical data extent including the implicit zero tail |
| `@bss` | decimal | no | Linux/macOS images require 0; Windows: extra reserved extent |
| `@relocs` | `-` or comma-separated decimal slot offsets | no | each slot `offset + 8` must lie inside the extent |
| `@threads` | `1` | no | thread-cell layout |
| `@argc` / `@argv` | decimal | memory route (required there; ignored by file images) | argument cells inside the extent |

Lower always emits `@target`, `@data`, `@src_os`, `@data_len`, `@bss`, and
`@relocs`. The image stage refuses, with no output:

- a missing `@target` or `@data_len`, an empty input, or a header-only input,
  and (file images, not objects or the memory route) no `_start` label or a
  `_start` that is not inside the code: `reject: not covered: address or
  target declaration` on x86_64; the ARM64 encoder reports these through its
  existing header failure (`arm64: ...` then `reject: not covered: ARM64
  operand or instruction`);
- an `@sym` value or an instruction data reference (named or numeric address,
  `argsave`/`argvget`, itoa tables, Windows gate cells, ARM `AD.data`) whose
  offset lies past the end of the extent, DATA_BASE + `@data_len` (+ Windows
  `@bss`): `reject: not covered: ELF input or relocation` (`PE`/`Mach-O` in
  that name for those formats). One past the end is allowed (the ARM stack top
  and C one-past-end pointers use it); the width of the access at the last
  bytes is not checked.

`@relocs` slot values are not range-checked; only slot positions are. The
image stage adds the data base to each slot's 64-bit value as the reference
`unisa/image` relocate does, so a pointer one past an object (or any other
value) is carried unchanged in meaning.

## Preprocessor diagnostic envelope (development)

`exec/build/gen.py pp --locations` emits **pp.locations**, while its default remains
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

`exec/build/gen.py parse2 --locations` consumes tokens.locations. It retains the
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
The development compiler now selects these location modes for normal and
warning compilation, including multiple units. The location tests alone do
not claim complete diagnostic compatibility.


## Diagnostic rendering and warning rules (development)

E3 `--warnings` implies `--locations`. It implements the reference's `-Wreturn-type` decision, including its
last-statement heuristic and main/void exemptions, plus `-Wint-conversion`
for assignments and local scalar initializers. The latter preserves the
reference's syntactic lone-zero and call-result exemptions. Included-header
warnings are suppressed. It is not a
complete warning implementation for every input. The development compiler CLI
selects it for single- and multi-unit `-Wall`/`-Wextra`/`-Werror`.

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


## Warning CLI routes

`compilerpack.py` constructs target-specific located preprocessors and shared
located lexer/warning parser networks, under `<target>/warn/...`. The driver
selects those routes for a source with `-Wall`, `-Wextra`, or `-Werror`.
Preprocessing and token-dump modes keep their ordinary routes. Public token
dumping deliberately disables implicit header selection, like the reference.

`-Werror` supplies the opaque `\0cli/werror` resource. After rendering the
warning summary, the parser rejects with an empty reason if its count is
nonzero and the resource is nonempty. Existing runtime rejection carries
stderr and discards partial stdout. The driver never opens destinations or
runs code after that rejection. No C-side diagnostic-text parsing or new
executor action is involved.

Multi-source warning mode uses `<target>/warn/unit` to preprocess and lex each
source independently, then `<target>/warn/multi/...` to isolate static names
and parse one program. `c/warningcheck.sh` compares 60 complete single-unit
results across host-C, unisacc-C and assembly-backed drivers;
`c/multiwarningcheck.sh` compares 120 multi-unit results.

## Located translation-unit directory (`UNITOK2`)

The driver frames each unit as LE32 payload length followed by LE32 filename
byte length, filename bytes, and its existing UNITOK1 envelope. It interprets
no source/token data. `units.py --locations` reuses the static-declaration
scanner, preserving physical qualifiers and multiline adjacent string tokens.
Its output is:

- Eight-byte magic `UNITOK2\0`, then LE32 unit count (1..64).
- In unit order: LE32 filename byte length, filename bytes, LE32 UNIPP1 length,
  and the exact UNIPP1 envelope (source, splice map and include map).
- Typed token stream. Each prefix is `@`, LE32 unit index, LE32 source offset,
  LF, then the ordinary typed token. The existing @unit0/@unit+ marker tokens
  also receive a prefix. There is one final prefixed eof token.

E3 stores a source context for each unit and selects it at every token read,
including rewinds. Include/splice tables use disjoint unit-indexed regions;
unit records live at 48<<40. The separate framing network uses 49<<40 for its
map blobs. Parser warning-context token ordinals also include the unit epoch,
so a later unit cannot overwrite an earlier unit's neighboring-token facts.
Diagnostics and format recognition use original preprocessed spelling rather
than the static-isolation suffix. Counts/extents/unit indices/source offsets
are checked before access. `unitlocationcheck.py` independently serializes a
valid two-unit case, checks UTF-8 filenames and reference diagnostics, and
rejects ten bad map containers and seven bad input frames.


## Parser errors and error-limit resource

`--errors` implies locations and can accompany `--warnings`. The driver
passes `-ferror-limit=` unchanged as NUL-terminated bytes under the opaque
`\0cli/error-limit` resource. An absent/empty resource means the default 20;
a supplied empty argument has a NUL byte and parses as zero (unlimited).
The model reads the leading decimal value, counts errors across units and
implements the stopping message. C does not parse diagnostic text.

Mapped grammar errors render reference diagnostics, discard the failed
continuation/scope, and rescan to the next balanced top-level boundary.
Nonzero error count prevents output publication. Unmapped prototype
limitations acquire a location but keep their not-covered reason and stop.
`parse2/errorcheck.py` compares 17 complete results and checks one such
limitation separately. `c/multiwarningcheck.sh` additionally compares 18
cross-unit error results, covering order, recovery and limits.

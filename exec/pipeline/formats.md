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
  E3 needs the spelling and stages share no symbol table.  Today only the
  patched reference `/tmp/ua_tdump` (exec/parse/mkdump.sh, `UA_TYPESPELL=1`)
  writes it; E1's delta does not, so E1 -> E3 is declared not connected
  (`tokens.typed?` in stages.tsv) rather than faked.
- **tape.text** -- the reference's `-S -o -` tape: `label:` lines, `.dir`
  lines, indented instructions.

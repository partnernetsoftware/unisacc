# E3: the parser / code walker as a finite delta (minimum slice)

Status 2026-09-26. Code: `exec/parse/gen.py` (derives the table),
`exec/parse/compare.py` (runs it, compares with the reference), probes in
`exec/parse/probes/`. The executor is **E2's `exec/pp/sim.py`, unchanged**:
no primitive was added for E3.

## 1. Recursive descent -> delta + stack

- Every parse function of `src/front_parse.c` in the slice becomes a
  procedure; every call site inside it becomes a **continuation state**. A
  call is `PUSH ret` + go to the callee's entry; a return is the shared `RET`
  state, which reads the stack top (`t` mode) and pops. The stack alphabet is
  the finite set of continuation labels.
- Precedence: one procedure pair `BINL`/`LOOPL` per level of the **prec stage**
  (`weights/gold/prec.tsv`, 10 levels); the operators of a level are read
  from the table, and each operator has its own continuation state (after the
  right operand returns, that state emits the operator). ALU spelling:
  `binsel.tsv` -> `irsel.tsv`, `_rev` swaps the operands.
- `id` followed by `=` needs one token more than LL(1) gives: the id's span is
  saved, the next token read, and on not-`=` the delta emits the operand and
  **pushes the ten LOOP continuations at once** (`BINCONT`), i.e. resumes the
  ladder as if it had descended.
- `for (i; c; s) body` emits `s` after `body`: the reader position of `s` is
  saved (`MARK`), the tokens skipped by paren depth, the body parsed, the
  reader `JUMP`ed back to `s`, then forward past the body. Generic cursor
  moves, no C.
- The frame size is printed in the prologue before the body is seen (the
  reference patches it). The delta runs **two passes** over x: pass 1 records
  each function's maximum slot count in W, `OCLR`, `JUMP` to 0, pass 2 emits.

## 2. Observation

`obs = (q, r, b, t)` as in delta-framework §2.2. The token reader (`NEXT`) is
a byte trie over the dump's lines (`b` mode) and leaves the token class in
`W[tk]`, the payload span in `W[ps]..W[pe]`; parser states do `RLD tk` and
branch on `r`. So the parser's effective observation is
(state, token class, stack top, r); the class is computed by delta, not by the
executor.

## 3. Symbol table and attributes in the generic store

- Name -> id: `INTERN v ps pe` hash-conses the byte span (the key is the span
  the delta itself delimited). That is the only name operation.
- Locals: `W[LOC+v]` = slot (0 = not a local). Declaring writes an undo pair
  (v, old) at `W[UNDO+usp]`; leaving a block unwinds to the saved `usp`
  (block entry saves `usp`, `cur` on a value stack `W[VS+vsp]`). Slots are
  reused across sibling blocks; the frame is 8 x max live slots, which is what
  the reference prints.
- Functions: `W[FND+v]` = pass number that defined it; a call checks it.
- A typedef-name lookup would be the same thing: `INTERN` the id span, `LDX`
  a region `TDEF+v` written when a `typedef` declarator was parsed and undone
  by the same unwind log; the delta branches on the loaded value via
  `CMPI`/`r`. Not built in this slice (typedef is out of scope).
- Type attributes: not needed in the slice (everything is `int`); they would
  be further W regions keyed by v. Not measured.
- Numbers printed (offsets, labels, `.frame %6d`): `DIVMOD10` digit loop,
  digits stored in W and printed reversed.

## 4. Slice and what is transcribed

Covered: `int f(int a, ...) {...}` definitions (<= 6 params), block-scoped int
locals with initializers, decimal literals (<= 9 digits), locals, `=` to a
local, unary `- ! ~ +`, all 19 binary operators incl. `&& ||`, parens, calls
(<= 6 args) to a function defined earlier or itself, statements block /
expression / empty / `return e` / if-else / while / `for(e;e;e)`.
Everything else is rejected `not covered: ...`.

Transcribed from measured reference tapes (not derived from a table): header,
footer, prologue/epilogue, push/pop, local load/store, return, branch/label
shapes, label allocation order (if: else-label at `jumpz`, end-label at
`else`; while: top, end before the condition; for: top, end, continue before
the condition), `.` prefix on div/mod.

**Input gap:** `-dump-tokens` prints every type keyword as `type`, so the
delta cannot tell `int` from `char`. `compare.py` therefore counts a file
whose source spells any other type word as not-covered before running the
delta (`src-type`). That filter is outside the delta.

## 5. Measured (2026-09-26)

Table: 477 states (412 r-mode, 64 byte-mode, 1 stack-mode), 122,480 full
entries (every state total over 257 keys), **940 sparse entries** (one default
+ exceptions per state), 287 action sequences / 3,006 actions (45,898 B JSON),
JSON 2,390,087 B dense.

| set | files | equal | reject-agree | not covered | differ |
|---|---|---|---|---|---|
| probes | 8 | 5 | 1 | 2 | 0 |
| tests/c + examples | 104 | 0 | 0 | 104 | 0 |

The corpus contributes nothing: 96 files spell `void`/`char`/`long` (every
`main(void)`), the rest call `printf` without a prior definition or use
constructs outside the slice (`b_ops2.c`: expression; `b_args8.c`: 8 params;
`b_stdio.c`: top-level). So the 0 differ is evidence from the probes only
(p1, p5-p8: 29,556 steps, 448/477 states visited). Next slices must at least
add `(void)` parameter lists, prototypes/calls to undefined externals and
`++`/compound assignment before the corpus measures anything.

No reference bug found in this slice.

## printf defined in the unit (measured 2026-09-26, not yet handled)

Allowing a call to the bundled `<stdio.h>` printf as an ordinary variadic call
exposed 14 differences. (B) The reference keeps its builtin printf lowering
even when printf is defined: each argument still gets a fresh frame slot
(`.frame` grows by 8 per argument). So the fix is to keep the builtin path,
not to treat it as a call. (A) With several headers, the reference's body
order is the reverse of their prepending (a later `#include` lands above an
earlier one); the delta's reading of the prelude must follow the token
stream's physical order. Both stay rejected ("printf defined in the unit")
until the structured E3 (research/e3-structured.md) handles them.

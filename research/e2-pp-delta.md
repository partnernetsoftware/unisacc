# E2: the preprocessor as a finite delta (design + minimum-slice measurement)

> Status: design draft + one measurement (2026-09-26). This is the E2 milestone
> of prd.md S-17. The machine is the one in [`delta-framework.md`](delta-framework.md)
> §2, with the trust boundary of §3 **option A**. It sizes the preprocessing
> layer only and says nothing about the other layers.
> Artifacts: `exec/pp/sim.py` (generic executor), `exec/pp/gen.py` (generates δ
> for the minimum slice), `exec/pp/compare.py` / `run.sh` (compare with the
> reference), `exec/pp/mknoauto.sh` (reference variant without `autoinc()`, src/ untouched).

## 1. The reference

The reference is `src/front_pp.c` as `fe_load` runs it: shebang blanking →
`splice` → `decomment` → `autoinc` → `preprocess` → `expandsrc`. `-E` writes
`src[0..nsrc)` after `expandsrc`. **That is exactly the buffer `lex()` reads**
(E1's `UA_LEXIN` hook dumps the same bytes), so comparing `-E` covers both
outputs the task names. Some properties of the reference shape the whole
design, so they come first:

- **It is in place and keeps line structure.** Directive lines and dead lines
  are overwritten with spaces (same length, newlines kept). `#include`
  replaces its line with the file text plus `\n`, then **re-runs splice and
  decomment over the whole buffer** and resumes at the line start.
- **Expansion is not C99 rescanning.** `expandsrc` runs up to **8 whole-buffer
  rounds**. Each round copies the buffer and substitutes every macro call it
  finds, and bodies are *not* rescanned within the round. Arguments are
  expanded recursively, with a depth bound of 8. There are **no hide sets**.
  Which definition a name has at a given byte comes from a **segment number
  per byte**: each `#define`/`#undef`/`pop_macro` starts a new segment, and
  each table entry records the segments it is live in (`macfrom`/`macto`/`macprev`).
- **`#if` does not expand macros.** An identifier's value is `macval`, which
  is the leading decimal digits of the body, and only when the body starts
  with a digit.
- **`autoinc`** scans the source for calls to functions that the bundled
  headers define (`static …(…){` on one line), and prepends
  `#include <h>` for each header needed. This is a heuristic over the whole
  file.

E2's criterion (S-17) is **identity with the reference**, so δ reproduces
these behaviours, bugs included (§8), and does not implement the C99 ones. §5
still spells out how the C99 rules would be written, because the reference is
expected to move to them.

## 2. Finite control vs unbounded data

| Finite control (in δ's states) | Unbounded data (in the executor's generic store, moved by generic actions) |
|---|---|
| Pass sequencing (P0 splice, P1 decomment, P2 autoinc, P3 directives, P4 rounds, DIAG) | The pass input `x` and output `o`; one attribute word per byte (the segment) |
| Byte classes: letter, digit, blank, quote, `#`, `/`, `*`, `\`, newline, EOF (δ sees the raw byte, as in E1) | |
| Directive recognition: the directive word is interned and looked up in a dictionary that init fills from **DIRV**. What each (directive, defined) pair does comes from **weights/gold/pp.tsv**; both are derived by gen.py | The interned strings (hash-consing: bytes → id) |
| Conditional-stack *transitions* (take / skip / pop, from pp.tsv) | The conditional stack itself: `TAKE[k]`, `SEEN[k]`, `NDEPTH` in W |
| `#define` parameter-list grammar; body scan (`#`, `##`, literals, identifiers) | The macro table: entries (name id, body blob id, fn, var, np, params, from, to, prev) in W; `NEWEST[id]` |
| `#if` precedence levels as return points in Γ (recursive descent, like the E0 toy) | The `#if` value stack in W; 32-bit values |
| Macro-call argument scanning (depth counter, strings/chars skipped, `,` at depth 1) | The argument table `ARGO/ARGL/NARGS` and the per-call frames in W (FP-addressed) |
| Include lookup order (source dir, include/, bundled) | The file dictionary path → bytes (read-only data) and the include-region table (`IRLN`, `IRNL`, names) |
| Diagnostic kinds (finite: unterminated comment, no such file, …) | The splice table `spl_at`, the line and column counters |

The macro table, include stack, argument lists and token buffers are all
unbounded. None of them is ever an action parameter. Action parameters are
named slots, constant bytes and Γ symbols (framework §2.3). Data moves
between named places only through `LDX/STX` (indexed by W values), `SPAN`,
`INTERN` and `BLOBSAVE`.

## 3. Executor primitives (all language-independent)

These are E1's primitives plus a few generic data-structure operations. The
complete list is in `exec/pp/sim.py`.

| group | primitives | why E2 needs it |
|---|---|---|
| E1 | `ADV MARK JUMP COPY OUT SPAN SPAN2 PUSH POP ACCEPT REJECT DIVMOD10` | unchanged |
| E0 | `LDI COPYW ALU ALUI CMP CMPI LDX STX BYTE` (int32, wrap, x/0 := 0, shift count mod 32) | table and stack arithmetic, `#if` |
| result register | `RLD s` (r := small W value), `OLAST` (r := last output byte) | branch on a stored flag; paste trims trailing blanks |
| output | `ODROP OLEN OCLR OSEL` | `##` trimming and `, ## __VA_ARGS__` comma removal (δ decides which bytes, the executor only drops the last one); include resume point; the "no change" round; stderr |
| byte attribute | `SETOT s` (stamp for new output), `XATTR d` (read x's stamp), `COPYT/SPANT` (copy with a new stamp) vs `COPY/SPAN` (keep it) | the segment of every byte (`srcseg`) across passes and rounds |
| strings | `INTERN d s e` (bytes → id), `BLOBSAVE d s e`, string builder `SBCLR SBOUT SBSPAN SBBLOB SBINTERN SBSAVE` | macro names, parameter ids, bodies; constants such as `_Pragma` built from constant bytes |
| reader frames | `INPUSH b` (read a blob), `INPUSHX s` / `INPUSHXE s e` (a view on x, open-ended or bounded), `INPOP`, `XLEN`, `BLEN` | read a macro body; read an argument range of x; read an included file |
| data | `SBFIND d` (path in the builder → blob id or 0) | `#include`: header bytes are **data** in a dictionary, not code |
| passes | `SWAP` (x := o, o := ε) | the reference's pass structure, including the re-run after `#include` |

**Not allowed** (each would carry C preprocessing semantics into the
executor): "expand this macro", "substitute arguments", "stringize", "paste
tokens", "evaluate #if", "is this a directive", "find the matching `)`",
"C integer promotion", "resolve #include". Summaries of macros (live
definition, parameter index, value) must be **computed by δ** from the generic
store. In this design they are: `MFIND` is a δ loop over the `prev` chain
comparing segments, and `PARAMAT` is a δ loop over parameter ids. **No
trusted component was needed** for the covered slice. For the design as a
whole, the one candidate is `autoinc` (§6): it is a heuristic, and it could be
kept as a listed, not-yet-migrated trusted component if porting it is not
wanted.

## 4. Passes as δ

- **P0 splice**: `\`+LF and `\`+CR+LF are dropped, and every join appends
  `OLEN` to `spl_at` (for the line map). A shebang first line becomes blanks.
- **P1 decomment**: `"`/`'` literals are copied through escapes. `//`
  becomes one space. `/*…*/` becomes one space plus its newlines. Running off
  the end is `reject(unterminated comment)` at the comment's start.
- **P2 autoinc** (designed, not built): one scan of x interns every maximal
  `[A-Za-z0-9_]` run and marks `CALLED[id]` when the run is followed by
  blanks/newlines and `(`. It then matches parens with a depth counter
  (MARK/JUMP back afterwards) and marks `DEFD[id]` if `{` follows. The
  `printf` format check is a small byte automaton. Each bundled header is
  then read as a blob, and its `static … name(… {` lines put name ids in a
  per-header list in W. `NEED[h]` = some listed id is `CALLED` and not
  `DEFD`. The output is the prepended lines in the reference's reverse
  order, then x.
- **P3 directives**, line by line: `live` = no open level has `TAKE = 0`,
  recomputed per line as a δ loop. A `#` line interns its word, gets `d` from
  the DIRV dictionary, computes `flag`, then takes `a` from the pp.tsv
  transitions and applies the reference's action for `(d, a)`. The line is
  blanked. A live `#include` looks the path up (source dir → `include/` →
  bundled), writes o = prefix + file bytes + `\n` + x[LE..], records the
  include region, SWAPs, and runs P0, P1, P3 again. P3 then resumes at the
  saved `RESUME = OLEN`, which is the reference's `i = ls; continue`.
  `#define`/`#undef` start a new segment (`CURSEG += 1`, `SETOT`).
- **P4 rounds**: `emitrange` and `emitbody` are δ subroutines with Γ return
  labels. Their locals live in a W frame at `FP` (depth, the 12 saved
  argument pairs, the macro, `ni`, pastejust). An identifier reads its
  segment with `XATTR` and looks up `MFIND(id, seg)`. An object-like macro
  pushes its body blob and copies it. For a function-like macro: blanks and
  newlines are skipped; if `(` follows, **collectargs** runs in an open-ended
  view of x (the reference scans to `nsrc`, not to `to`). The trimmed start
  and end of each argument are tracked in a forward scan: the first
  non-blank is marked, and so is the position after the last non-blank. No
  backward reads are needed. The reference's argument normalisation (empty
  argument list, empty `...`, `...` absorbing commas) is a chain of
  `CMP`-states. The 12 argument pairs are saved into the frame **after**
  collectargs and restored after the body, as the reference does (this is
  the source of bug B1, §8). In a body, a parameter becomes an
  `emitrange(argo, argo+argl, depth+1)` on a bounded view of x. `#param`
  becomes `"`, then the argument bytes with `"` and `\` escaped, then `"`.
  `##` becomes `OLAST`/`ODROP` while the last byte is a blank, the body's
  blanks are skipped, and pastejust (emit one space after the next token) is
  set. `, ## __VA_ARGS__` with an empty variable part becomes `OLAST` = `,`
  → `ODROP`. A round with no change outputs x (`OCLR` + `SPAN2`). Otherwise
  the round SWAPs, and stops after 8.
- **`#if`** (designed, not built): `ppcond … ppprim` becomes 12 levels. Each
  binary operator pushes V onto a W stack, calls the lower level with a Γ
  label, then pops and combines. Multi-byte operators use MARK/JUMP
  look-ahead. `ppprim` covers `! ~ - + ( )`, numbers (base 8/10/16; the
  suffix `uUlL` is skipped), character constants (`\n \t \0 \r`), `defined`
  (interned and compared with the id of the constant `defined`), and
  identifiers → `HAS ? VAL : 0` via `MFIND`. The end of the directive is the
  byte `\n`/EOF. After a character constant, a `CMP` with `LE` clamps a
  cursor that has run past the end.

## 5. Macro expansion and hide sets with generic primitives

The reference has no hide sets, so δ does not implement them. The C99
6.10.3.4 rule ("painted blue") would take no new primitive:

- *Disable while expanding*: when a body is pushed (`INPUSH`), δ sets
  `W[DISABLED + entry] = 1`. When that frame ends (EOF of the body frame,
  then `INPOP`), δ clears it. An identifier read while its entry is disabled
  is painted.
- *Painted tokens stay painted*: when rescanning happens inside the same pass
  (the reader stack gives C99's rescan-with-rest-of-input for free), a
  painted identifier is emitted and never looked up again. An argument that
  is pre-expanded and then substituted carries its paint as an **attribute
  bit** on its bytes (`SETOT` with a paint bit; `XATTR` reads it). That is
  the same generic attribute mechanism that carries segments here.
- Stringize and paste in C99 work on tokens. Here they work on the byte
  text, as in the reference: `#` escapes `"`/`\`, and `##` trims blanks.
  Doing them on tokens would mean δ recognising token boundaries (E1's δ,
  reused as a sub-machine), not a new primitive.

## 6. `#if` arithmetic

The reference's `int` arithmetic, as run on this host (arm64, `cc -O2`),
**defined** as δ's: 32-bit two's complement with wrap on `+ - * ~` and unary
`-`. `/` and `%` by 0 give 0; the reference checks this itself, and δ does
the same with a `CMP` rather than relying on the ALU. `INT_MIN / -1` =
`INT_MIN`: measured, `#if (-2147483647-1)/-1 == 5` runs and does not trap on
arm64. It would trap on x86-64, so this is a **host dependence of the
reference** (C UB). The shift count is taken mod 32 (measured: `#if 1<<33` is
true, i.e. `1<<1`), and `>>` is arithmetic. Comparisons are signed. `&&` and
`||` evaluate both sides; the reference does not short-circuit, and has no
side effects that would show the difference.

## 7. Diagnostics and the line map as outputs of δ

`diag_at` is a pure function of state that δ holds: the buffer, the position
p, `spl_at`, the include regions `(ln, nl, name)`, `nautoinc` and `prelines`.
Designed as a DIAG pass (not built in this slice):

1. Build the buffer the reference sees. For a decomment error that is
   o ++ x[|o|..], because decomment works in place. Then SWAP.
2. Scan to p, counting line and column, and MARK each line start.
3. Walk the include regions from innermost out with `LDX` and `CMP`, to find
   `inside` and the adjusted line. Then subtract `nautoinc`, and add one per
   `spl_at < p`.
4. `OSEL 1`. Write the file name: the source-path blob, or the region's name
   blob. Write the numbers with `DIVMOD10` onto Γ digits, as E1 does. Write
   the message constant, the line (`SPAN2` from the line start to `\n`), and
   the caret line (a tab where the line has a tab, a space elsewhere). Then
   `REJECT k`.

In this slice δ produces `reject(k)`, and the comparison checks the kind and
the exit status. **The rendered `file:line:col` text is not compared.**

## 8. Reference bugs found (not fixed; src/ untouched)

Each repro is run with `/tmp/…/ua_ref -E t.c`, where the reference is built
by `tests/build_ref.sh`.

- **B1: an argument is lost after a nested macro call inside an argument.**
  ```c
  #define F(a,b) [a|b]
  #define G(x) x
  F(G(1), 2)
  ```
  The reference gives ` [ 1 |] `; C99 gives `[1|2]`. Cause: `emitbody(F)`
  expands parameter `a` through `emitrange`. That call's `collectargs(G…)`
  overwrites the **global** `argo/argl/nargs`, and `emitrange` saves them
  only *after* collectargs, so F's `nargs` comes back as 1 and `b` expands to
  nothing. `F(3, G(4))` is correct, because `G` is the last argument.
- **B2: self-reference is expanded 8 times** (there are no hide sets). Given
  `#define foo foo + 1`, `foo` becomes
  `foo + 1  + 1  + 1  + 1  + 1  + 1  + 1  + 1`; C99 gives `foo + 1`. In the
  same way, `#define f(a) a` with `f(f)(1)` gives `1`; C99 gives `f(1)`.
- **B3: `#if` ignores any body that does not start with decimal digits.**
  With `#define X 0x10`, `#if X` is false, because the value is the leading
  decimal digits `0`. `#define Y (1)` gives `#if Y` = 0. C99 expands macros
  in `#if`.

## 9. Measurement (minimum slice)

**Slice built:** shebang, splice, decomment (with the unterminated-comment
reject), directive recognition, `#ifdef #ifndef #else #endif`, object-like
`#define`, `#undef`, `#include "…"`/`<…>` (source dir → `include/` → bundled;
the include re-runs splice and decomment), function-like `#define` *recorded*
(visible to `#ifdef` and to redefinition), unknown directives and other
`#pragma`s blanked, and **object-like expansion with segments and ≤ 8
rounds**, including `##` in object-like bodies. **Rejected as not covered:**
`#if`/`#elif`, the invocation of a function-like macro, `_Pragma`, and live
`#pragma push_macro`/`pop_macro`. **Not modelled:** `autoinc` (so the
comparison also runs against the reference built without it), -D/-U/-I/-include,
and the diagnostic text.

Commands, each step bounded to 58 s:
`E2TMP=… exec/pp/run.sh gen`, then `ex.aa … ex.af`, `corpus.aa … corpus.ag`.

**Table** (`python3 exec/pp/gen.py`):

| quantity | value |
|---|---|
| states | 154 (b 107 / r 46 / t 1) |
| table entries (each state total over the one component it reads) | 39,335 |
| of which filled with `unreachable` | 10,673 |
| action sequences / actions in the pool (pool bytes) | 121 / 513 (3,882 B) |
| sparse encoding (a default per state, 5 B per exception, plus the pool) | **9,127 B** |
| dense encoding (4 B per entry, plus the pool) | 161,376 B |
| Γ (return labels + BOT) | 14 |
| action kinds used | 38 (list: `gen.py` output) |

**Comparison** (stdout + exit status; a reject is compared by kind + exit status):

| corpus | files | equal (vs ref) | equal (vs no-autoinc ref) | reject agrees | not covered | **differ** |
|---|---|---|---|---|---|---|
| `examples/*.c tests/c/*.c` | 103 | 73 | 19 | 0 | 11 | **0** |
| `corpus/` (all 249 `.c`) | 249 | 184 | 21 | 6 | 38 | **0** |
| total | 352 | 257 | 40 | 6 | 49 | **0** |

- *equal vs no-autoinc*: δ matches the reference with `autoinc()` removed, and the real reference differs only because autoinc prepended headers. Autoinc is not modelled, so these 40 files do **not** count as E2 passes against the real reference.
- *reject agrees*: the 6 `tiny-regex-c/tests/*` files (`#include "re.h"` is missing). δ rejects `no such file for #include`, and the reference exits 1 with that message. The file:line:col text is not compared.
- *not covered* (49 in total): function-like macro invocation 32, `#if` 15, `#elif` 1, `#pragma push_macro` 1 (c-testsuite 00206). Before push/pop_macro was rejected, 00206 was the one real difference: δ blanked the pragma and printed `"333"` where the reference printed `"222"`.
- Steps: 64.1 M in total (Python state-machine lookups). Coverage per shard: 83–122 of 154 states, and at most 1,830 of 39,335 entries. Low entry coverage is the norm for dense rows; it is evidence on the tested inputs only (S-17).

**What runs** (S-17, "what the model is"): `sim.py` looks δ up in a Python
table (a state machine). This is not network inference.

## 10. Next

In order: function-like invocation (the P4 design above, including B1-faithful
argument saving), `#`/`##` in function-like bodies, `#if` (§4), `autoinc`
(P2, or a listed trusted component), the DIAG pass, then `pushpop`, `-D/-U/-I`.
The measurement above says nothing about the size of those parts.

## 10. Against the fixed reference (2026-09-26, main 61e4044)

The reference now expands on tokens (f74b61a, Prosser hide sets) and pads
replacements with spaces: `((FILE *)1)` is emitted as `( ( FILE * ) 1 )`.
The byte-segment P4 of this slice reproduces the OLD textual expansion, so
object-like expansion itself now differs: shard ex.aa = 17 equal, 2 DIFF
(tests/c/a_callres.c: spacing of an expanded body), 1 not covered; table
154 states, dense 161376 B, sparse 9127 B, pool 3882 B. Function-like
calls are not yet added: P4 must first be re-cut as a token-level pass
(token emitted with its hide set, body re-tokenised with inter-token
spaces) before function-like argument collection is layered on it.

### 10.1 First token-level P4 attempt (2026-09-26, not adopted)

Reference rule, measured with `ua_ref -E`: a top-level object-like expansion
is emitted as ` ` + its fully rescanned tokens joined by one space + ` `
(empty body -> one space); nested expansions add no padding. An attempt
(active-macro flags F_ACT/F_UP as the object-like hide set, one round,
pp-number / literal / longest-match punctuator tokenising of bodies; `#`/`##`
in bodies rejected) built 184 states, sparse 10349 B, but raised DIFFs:
ex.aa 2, ab 3, ac 9, ad 2, ae 9, af 1; corpus aa 0, ab 4, ac 2, ad 9, ae 34,
af 19 (ag timed out). Not committed; the cause is still to be found.

### 10.2 Token-level P4 adopted (2026-09-26)

Diagnosis of the §10.1 attempt: the ex shards were 26 DIFF on main as well
(not 2; aa alone was 2). All 26 DIFFs, main and attempt alike, were ONE class,
header macro bodies used inside the bundled headers themselves, e.g.
`fputc(__u_c, stdout)` (ref `fputc(__u_c,  ( ( FILE * ) 1 ) )`) and
`return NULL;` (ref `return  0 ;`). On main the cause was body spacing (byte
copy `((FILE *)1)`); in the attempt the cause was that nothing expanded at
all: the new end-of-body state `EBX` never set `CHANGED`, so `P4END` took the
"unchanged" branch and re-emitted the unexpanded input. No newline,
punctuator-splitting or nested-spacing difference was observed. Fix:
`EBX` sets `CHANGED`, and since the token rescan (hide sets via F_ACT/F_UP)
is complete in one pass, `P4END` accepts after one round. ex.aa..ex.af:
26 DIFF -> 1 DIFF (ad), 0 otherwise.

### 10.3 Nested object-like spacing; all shards 0 DIFF (2026-09-26)

The last ex DIFF (ex.ad `tests/c/b_mmap.c`: `0x2 |  0x20` vs reference
`0x2 | 0x20`) and the one corpus DIFF it hid (corpus.af
`c-testsuite/single-exec/00211.c`: `[  0x1E + 1 ]`) were the same defect: a
body identifier emitted its separating space (or set SEP) before it was known
whether it expands, and the nested expansion's first token then brought its
own. The reference joins the tokens of the fully rescanned expansion with one
space, so a nested expansion contributes no padding of its own. Fix
(exec/pp/gen.py): the space is pending in PS and emitted, and SEP set, only on
the paths where the name does not expand (EBNX). In slice, fixed; not rejected.

| shard | files | equal | equal-noauto | not-covered | reject-agree | DIFF |
|---|---|---|---|---|---|---|
| ex.aa..af | 127 | 73 | 18 | 16 | 0 | 0 |
| corpus.aa | 40 | 38 | 1 | 1 | 0 | 0 |
| corpus.ab | 40 | 32 | 0 | 8 | 0 | 0 |
| corpus.ac | 40 | 37 | 0 | 3 | 0 | 0 |
| corpus.ad | 40 | 31 | 0 | 9 | 0 | 0 |
| corpus.ae | 40 | 31 | 4 | 5 | 0 | 0 |
| corpus.af | 40 | 12 | 13 | 15 | 0 | 0 |
| corpus.ag | 9 | 0 | 0 | 3 | 6 | 0 |
| corpus total | 249 | 181 | 18 | 44 | 6 | 0 |

Function-like macros are still out of scope (the bulk of not-covered).

### 10.4 Function-like spacing measured; delta not yet extended (2026-09-26)

Reference (`/tmp/ua_ref -E`) on `#define F(a,b) a+b*a`, `#define G(x) [x]`:

| input | output |
|---|---|
| `int y=F(1,2);` | `int y= 1 + 2 * 1 ;` |
| `F( p , q )z;` | ` p + q * p z;` |
| `x F (u,v) y;` | `x  u + v * u  y;` |
| `G(F(1,2));` | ` [ 1 + 2 * 1 ] ;` |
| `F` NL `(3,4);` | ` 3 + 4 * 3 ` NL `;` (the call's newline moves after it) |
| `G(G)(5);` | ` [ G ] (5);` (hide set: inner G not re-invoked) |
| `G(a  b);` | ` [ a b ] ;` (argument re-tokenised, one space between tokens) |

So the rule is the object-like one: every body token (argument tokens
included, blanks around arguments dropped) is joined by one space, the
whole expansion padded by one space each side, and text between the name
and `(` (blanks, newlines) is consumed, a newline re-emitted after the
expansion. The delta was not extended in this round (time budget): P4
still rejects any function-like invocation as `not covered`, so all
shards stay at the 10.3 totals (ex 0 DIFF; corpus 181 equal / 0 DIFF).
Next: argument spans recorded by the executor (MARK pairs per argument,
stride table as for macros), body scanned with parameter ids mapped to
an INPUSH of the argument span, hide set as in 10.2.

### 10.5 Function-like macros, smallest form (2026-09-26)

exec/pp/gen.py: `#define F(p1,...,pn) body` records n (F_NP) and the
parameter ids (F_P0.., at most 8) in the fixed-stride entry (FSZ 17);
F_FN = 1 covered, 2 not covered (zero parameters, variadic, malformed, > 8).
A call skips blanks/newlines (counted) to `(`, MARKs each argument's span and
BLOBSAVEs it (split at commas, literals skipped; an argument containing `(`
rejects `not covered: nested call args`), checks the count, and scans the
body with PUSHM.  Directly in the body (CUR = the called entry) a parameter
name pushes its argument blob through a fixed argument entry ARGE with the
same PUSHM, so the hide-set rule of 10.2 applies unchanged (`G(G)(5)` ->
` [ G ] (5);`).  Spacing per 10.4, the counted newlines emitted after the
closing pad; measured extra: an expansion that emits nothing is padded by
ONE space, not two (`return F(, 1) 0;` -> `return   0;`; corpus 00122.c and
tests/c/b_decl2.c DIFFed until fixed).  No new primitive.  Still not
covered: F(), variadic, # / ##, nested call arguments, a function-like name
inside a body.

| shard | equal | equal-noauto | not-covered | reject-agree | DIFF |
|---|---|---|---|---|---|
| ex.aa..af (127) | 77 | 20 | 8 | 0 | 0 |
| corpus.aa..ag (249) | 191 | 23 | 29 | 6 | 0 |

## §11 #if / #elif, smallest form (2026-09-26)

Covered: a line that is exactly `0`, `1`, `defined X` or `defined ( X )`
(whitespace free).  A dead `#if` is not evaluated (as the reference); its
level is pushed as skip.  `#elif` is always evaluated.  Everything else --
other constants, operators, `!`, macro expansion of the line -- still rejects
`not covered: #if expression`.  The full integer-constant-expression
evaluator (64-bit ALU width, signed/unsigned, precedence climbing) is NOT
done: the executor ALU is int32 and widening it is the next step.
Measured: ex 104 files, 0 DIFF, 6 not covered; corpus 249 files, 0 DIFF,
25 not covered (was 29), 6 reject-agree.

### 11.1 64-bit ALU actions (executor, language-agnostic)

Added beside the int32 `ALU`/`ALUI`/`CMP` (whose semantics are unchanged, so
E3 is unaffected): `A64 op d a b`, `A64I op d a v`, `C64 a b`, `C64U a b`.
Width/overflow rules: a word is read as a signed 64-bit two's-complement
value (its value mod 2^64); every result is wrapped mod 2^64 and stored
signed.  `add sub mul` wrap; `and or xor not` bitwise (`not` ignores b);
`shl` / `shr` (logical) / `sar` (arithmetic) take the count mod 64; `sdiv`
truncates toward zero, `srem` = a - (a sdiv b)*b, INT64_MIN sdiv -1 =
INT64_MIN, INT64_MIN srem -1 = 0; `udiv`/`urem` operate on the unsigned
images.  Divide-by-zero is a result code, not a trap: `A64`/`A64I` set
r := 1 and W[d] := 0 when a div/rem divisor is 0, else r := 0, so the delta
decides what a zero divisor means.  `C64` / `C64U` set r := 0,1,2 for <,=,>
on the signed / unsigned images.

### 11.2 #if / #elif expressions without macros (2026-09-26)

`exec/pp/gen.py` XE: a shunting-yard evaluator in the delta. Operator stack
at W[64e6..], value and poison stacks at W[65e6..] / W[66e6..], precedence
table at W[67e6..] (written at init). Covered tokens: decimal (<= 18 digits)
and hex (<= 15 digits) constants without suffix, `0`, `defined X`,
`defined(X)`, `( )`, unary `! ~ - +`, binary `* / % + - << >> < <= > >= ==
!= & ^ | && ||`, `?:`. Arithmetic is A64 / C64 (intmax_t, signed).

Division by zero: each value carries a poison bit; `/ %` with a zero divisor
sets it, `&& || ?:` drop the poison of the operand they do not evaluate
(matches `ppskip` in front_pp.c). A poisoned result is **not covered**: the
reference prints `division by zero in #if` on stderr and still writes the
text with exit 0 under -E, which the one-channel delta cannot reproduce.

Not covered: any other identifier (`#if macro name`; macro expansion in #if
is the next step), char constants, u/l suffixes, octal, malformed lines.

Measured: 18 probes (12 equal, 6 not covered, 0 DIFF); ex + corpus (353
files): 276 equal, 43 equal-noauto, 6 reject-agree, 28 not covered, 0 DIFF.

### 11.2a #if: identifiers that are not macros read as 0

Slice of the expansion step: an identifier other than `defined` on a live
#if/#elif line is looked up (MFIND); not a macro -> the value 0 is pushed
(the reference expands first and reads leftover identifiers as 0; with no
macro on the line the two agree). A macro name is still not covered -- the
expansion of the line into a scratch area through P4 is the next step.
Probes 10 (/tmp/xp/q*.c): 7 equal, 3 not covered (macro name, f(1) syntax,
division by zero). Per shard, like for like against main's gen.py:
ex.aa..af 107: 78 equal, 20 equal-noauto, 9 not covered, 0 DIFF (unchanged);
corpus.aa..ag 249: 199 equal (+1), 23 equal-noauto, 21 not covered (-1),
6 reject-agree, 0 DIFF.

### 11.2b #if: object-like macro names expand before evaluation

XE meeting an identifier that MFIND finds: an active macro (F_ACT, the
hide-set rule the body rescan uses) reads 0 like any leftover identifier;
an inactive object-like macro pushes its body (INPUSH via PUSHM, active
chain CUR / F_UP, counter xdp) and XE keeps reading operands and operators
from it; at the body's end (EOF with xdp > 0, in XO or XR) the frame is
popped, the mark cleared, and the rest of the line continues. So `A -> B
-> 2`, `#define X X` (0), `A B / B A` (0), empty bodies and hex / paren
bodies evaluate as the reference does. A function-like name is not covered.
Probes 12: 10 equal, 2 not covered (F(1); `2L` suffix), 0 DIFF.
Shards: ex.aa..af (104 files in this split): 79 equal, 20 equal-noauto,
5 not covered, 0 DIFF; corpus.aa..ag 249: 201 equal (+2), 25 equal-noauto
(+2), 17 not covered (-4), 6 reject-agree, 0 DIFF.

## autoinc (measured 2026-09-26, not yet in the delta)

Reference: autoinc() runs after decomment(), before preprocess(). For each header in the fixed order assert.h ctype.h stdlib.h string.h wchar.h stdio.h, if some `static NAME(...){` definition line of include/H is *called* in the source and never *defined* there (srcuse==1; printf excluded), it PREPENDS the line `#include <H>` at byte 0 (so later headers land above earlier ones), which P3 then processes as a normal include. Afterwards, a printf format with a width/flag/precision prepends `#include <stdio.h>`. Measured: strcpy+strlen with no include -> ref -E emits string.h text, noauto does not. gen.autoinc_map() derives the trigger names from include/*.h (ctype 14, stdlib 28, string 15, wchar 1, stdio 40, assert 0). Not wired: the trigger needs a call/definition scan (paren matching to `{`) before P3; verdicts unchanged (45 noauto, 0 DIFF).

# CX2/CX3 module design for `.cx`: `#pragma unisa use`, derived interfaces (CXIF), strict refusals

> **Design only.** Nothing here is implemented. The only things actually run are marked **[measured]**.
> Inputs: plans/cx-modules.md (CX0..CX4), docs/cx-spec-draft.md v0.3 §2/§3/§3.1/§4/§8, include/cx.h,
> research/flownet-w0-feasibility.md, tests/flownetw0{,mod}.cx, exec/c/compiler.c `unit_includes` (l.397, 705-714,
> 822, 864), src/front_parse.c `fe_load` prelude insertion (l.6583-6603) / `fe_units`, src/front_pp.c `quotedinc` + F3.

## 0. Facts this design rests on

| Fact | Source |
|---|---|
| A `.cx` unit gets `cx.h` forced in front of the `-include` list. The decision is made per unit and keyed on the suffix (`unit_includes` in the product; `cxu` in `fe_load` in the reference). `.c` units keep exactly the CLI list | compiler.c:397, front_parse.c:6586 |
| Several sources make **one** program (`fe_units`: "there is no linker"). Product units are compiled per unit and may run in a parallel forked pool, and the unit cache is keyed by path | front_parse.c:6638, compiler.c:805-870 |
| Shared driver code already has a precedent: `src/fwdstub.c` is `#include`d by both `exec/c/compiler.c:169` and `unisacc.c:16` | grep |
| `#pragma` is not honoured: E2 only handles push/pop_macro and drops the rest. `-E` emits **no** `# line "file"` markers | front_pp.c:1540, 2727; -E probe **[measured]** |
| An undefined extern **object** reads as 0 (`extern int zz; printf zz` gives `0`, rc 0) | W0 + re-probe **[measured]** |
| **An implicit function declaration is accepted across units**: `a.cx: return g(3);` with no prototype, plus `b.cx: int g(int x){return x+1;}`, gives rc 4 | probe **[measured]** |
| Local-header users: `cxlab.h`×3 (examples/cx-lab), `snapbase.h`×2, `../exec/c/codec.h`×2, `../exec/c/bind.h`×1, and `.cx`-includes-`.cx`×1 (`librarycallbacke3check.cx` is just `#include "librarycallbacklowercheck.cx"`, a whole program with `main`) | grep |
| `cxlab.h`/`snapbase.h` are header-only libraries: `static` functions, a macro (`CXLAB_MAXCOL`) inside an exported typedef, and their own system includes (`<sys/time.h>`) | read |
| `extern char **environ;` appears in 3 tracked `.cx` files (draftguardcheck, libraryresolverhostcheck, revivedscan) | grep |
| `tests/cxinclude.tsv` (the CX0 ratchet) does not exist yet | ls |

The implicit-declaration finding matters most. Without a fix, "remove the `use` and it must go red" can pass silently whenever the module is still linked by another route. §4 and §6 address it.

## 1. Module naming and file resolution

**Syntax** (pragmas only; spec §2):

```c
#pragma unisa use flownetw0mod        /* sibling file: ./flownetw0mod.cx */
#pragma unisa use util.tsv            /* subdirectory: ./util/tsv.cx */
#pragma unisa module flownetw0mod     /* optional; when present it must equal the file stem */
```

- **Grammar.** NAME is `seg(.seg)*` with `seg = [a-z_][a-z0-9_]*`. Paths, quotes, `..`, uppercase and `-` are refused by name: `cx: a.cx:3: bad module name "../x"`.
- **Placement.** Only on a physical line at file scope, before the first declaration, and outside any `#if`. Comments and strings are skipped by the scanner. Anything else is `cx: a.cx:9: unisa pragma after first declaration`. An unknown `#pragma unisa WORD` in a `.cx` file is refused. In a `.c` file, `#pragma unisa` keeps today's behaviour (ignored), because the scanner never runs on `.c` units.
- **Resolution in v1 is relative only.** A non-`std` NAME resolves against the **directory of the using file**, and dots map to `/`. There is no search path, no `-I`, no parent directory, and no glob. One name maps to exactly one candidate file, so resolution never depends on what else exists on disk. A shared repo library root (`-cxroot DIR`) is deferred until a second directory actually needs one. Today every user sits beside its header.
- **The `std.*` namespace is reserved and resolved inside the compiler, never on disk.** `std.cx` means the current prelude and is always implicit. Writing it is accepted as a no-op. Any other `std.X` gives `cx: unknown std module std.io` until CX1 editions split the prelude (std.core implicit; std.proc, std.fs and so on opt-in). The split happens at an edition boundary only, so old scripts keep compiling.
- **Closure and order.** The driver starts from the command-line sources, in the order given. It takes a breadth-first closure over `use`, visiting each file's uses in source order, and appends each newly discovered module after the CLI sources. Identity is the lexically normalised path: no realpath, no filesystem queries beyond `stat` of the one candidate. A diamond (a→b, a→c, b→d, c→d) yields d once. If a module is both used and passed on the CLI (`-run a.cx b.cx`, today's gate form), it is one unit, so existing gate commands keep working.
- **Cycles are refused** (spec §3, v1): `cx: module cycle: flownetw0 -> m -> flownetw0`. Mutual recursion stays inside one module.
- **A used module must not define `main`**: `cx: module b defines main (a used module is a library)`. This rule is what makes the `.cx`-includes-`.cx` alias pattern migrate cleanly (§5).
- **Shebang scripts** (`#!/usr/bin/env -S unisacc.com -run`) work with no new flags, because the closure comes from the source and not from argv.

## 2. Exported interface (CXIF v0 = a generated, normalised C99 declaration text)

**Decision.** In v0, CXIF is **text**: a C99 declaration header projected from the module source. The spec's record form (type graph, §3.1) is a later step, and it must reproduce this text. Text is the only form the system cc can consume, so the oracle sees exactly the bytes the product sees.

**Projector** (`src/cxmod.c`, shared by product and reference like `fwdstub.c`, written in the `src/` dialect). It does a single pass over the module's raw text: splices joined, comments removed, strings respected. It tracks brace and paren depth at file scope and splits items at depth-0 `;` or at the `}` that closes a function body.

| Item at file scope | v0 rule |
|---|---|
| non-`static` function **definition** | export the prototype: item text up to the body `{`, whitespace collapsed, then `;` |
| `static` function or object | private; omitted |
| `typedef …;`, `struct/union/enum TAG {…};`, anonymous `enum {…};` | exported verbatim, in source order |
| non-`static` object definition (`int n;`, `const int t[2]={…};`) | **refused**: `cx: b.cx:4: module b exports object fw0_n; make it static and export a function`. This codifies the W0 "functions only" rule. Exporting `const` tables is reconsidered in CX4 |
| non-`static` prototype with no definition in the module | refused (it would be a hand-written extern; see CX3) |
| `#define NAME` | recorded, never exported (spec §2). If an exported item mentions a module macro: `cx: cxlab.cx:16: exported cxlab_row_t uses macro CXLAB_MAXCOL; make it an enum constant` |
| exported item inside `#if/#ifdef` | refused in v0 (`conditional interface`). Target-conditioned interfaces need the condition in the key (spec §3) |
| exported name starting `cx_` / `_cx_` / `__u_` | refused (reserved for the prelude) |
| `#pragma unisa opaque TAG` (the only new pragma in CX2; slice CX2b) | export only `struct TAG;`. Justification required by spec §2: C has no syntax for "this definition is private" |

**Output** is byte-deterministic, and its bytes depend only on the module bytes and the generator version:

```c
/* CXIF v0 module flownetw0mod src-sha256 12ba95b478b94fa5 -- generated, do not edit */
#ifndef CXIF_flownetw0mod
#define CXIF_flownetw0mod
int fw0_radix(int i);
int fw0_lookup(int a, int b);
int fw0_tsv(const char *path, long *sum, char *why, int whylen);
#endif
```

- **Self-containment** is a gate check, not a per-build cost. The gate compiles a synthetic `.cx` unit made of `prelude + CXIF` alone and requires a clean compile. This catches a leaked type from a module's private system include (for example `struct timeval` from cxlab's `<sys/time.h>`) as a red result rather than a consumer-side mystery.
- **What crosses a module boundary:** function prototypes, types, and enum constants. **What does not:** macros, objects, `static` items, and bodies.
- **Transitive types.** If b's exported signature names a type from c, b's CXIF must carry it. v0 handles this by giving every consumer the **type part** of the CXIFs in its use closure (each once, in topological order, include-guarded). The **function part** comes from **direct** uses only. Calling a transitive module's function then becomes an undeclared call, which CX3c refuses for `.cx` (§4).

**Where CXIF lives.** It is never committed and is regenerated on every build (a linear scan that takes milliseconds). In the product driver, each module's CXIF is written once per process to a content-addressed `$TMPDIR/ua-cxif-<sha16>.h` and appended to the consumer unit's forced-include list after `cx.h`. There is precedent for this: the parallel pool already writes `$TMPDIR/ua-unit-<pid>-<j>`. The file must survive the forwarding second pass. Inspection flags:

- `unisacc -cx-interface m.cx` prints the CXIF.
- `unisacc -cx-plan main.cx` prints TSV: `unit<TAB>path<TAB>forced-includes…`, in compile order.

An in-memory resource (no temp file) is a later refinement once the E2 header-resource route is understood. It must keep the CXIF bytes identical.

**System cc oracle** (`tests/cxcc.cx`, itself a `.cx` script):

```sh
unisacc.com -cx-plan tests/flownetw0.cx                    # -> units + their CXIF list
unisacc.com -cx-interface tests/flownetw0mod.cx > $T/flownetw0mod.h
cc -std=c99 -Wno-unknown-pragmas -x c -include include/cx.h -include $T/flownetw0mod.h -c tests/flownetw0.cx -o $T/0.o
cc -std=c99 -Wno-unknown-pragmas -x c -include include/cx.h -c tests/flownetw0mod.cx -o $T/1.o
cc $T/0.o $T/1.o -o $T/fw0 && $T/fw0 .                     # stdout must equal unisacc -run's
```

cc compiles each unit separately, so `static` items in cx.h are per unit, which is the same as unisacc. The plan TSV is the single source of truth, so the oracle cannot drift from the product's resolution.

## 3. Visibility and collisions

The mapping onto C stays exactly C:

- **pub** = a non-`static` definition, with external linkage and an unmangled name.
- **private** = `static`.
- No renaming and no mangling (§7).

Collisions are checked when the driver merges a consumer's interfaces, **before** compilation, so they get better names than today's `reject: multiple definitions of this function`:

| Case | Refusal |
|---|---|
| two modules in one program export the same function | `cx: fw0_lookup exported by both b (b.cx:12) and c (c.cx:3)` |
| consumer defines a non-static name a used module exports | `cx: a.cx:20: fw0_lookup is exported by flownetw0mod` |
| two visible CXIFs define the same typedef/tag/enum constant differently | `cx: type row_t from b and c differ`. Byte-identical repeats are impossible because each CXIF is emitted once |
| statics with the same name in different units | allowed (already **[measured]** in W0) |

There is one external namespace per program, as in C. The API name prefix (`fw0_`, `fn_`) is a lint ratchet, not a rule, in v1 (spec §3: "auditable C name prefixes").

## 4. Minimal first slice (CX2a), with its negative tests

**Scope**, about 300 lines of shared C plus driver hooks:

1. `src/cxmod.c`: pragma scan, resolution, closure/cycle, projector, collision merge.
2. Product: in `compiler.c`, before the unit loops, but only if some source ends in `.cx`. Expand `sources[]`, then make `UNIT_INCLUDES(path)` pick a **per-unit** list: `cx.h` followed by the CXIF files of that unit's uses. Reference: the same expansion in `unisacc.c`'s driver, plus `fe_load` taking the per-unit list instead of the single `cxu` flag.
3. The two flags `-cx-plan` and `-cx-interface`.
4. Pilot migration of `tests/flownetw0.cx`: delete its 3 hand prototypes, add `#pragma unisa use flownetw0mod`, and change the gate to `-run tests/flownetw0.cx -- .`.

**Not in CX2a:** opaque, `std.X` other than `std.cx`, CX3 refusals, record-form CXIF, and caching.

**Gate `cxmodule`**: fixtures under `tests/cxmod/*.cx` with zero includes, and each step bounded at ≤58 s. Every negative must give rc 1, empty stdout, and the exact stderr line:

| # | Fixture | Expected |
|---|---|---|
| N1 | `use nosuch` | `cx: tests/cxmod/n1.cx:1: module nosuch: no file tests/cxmod/nosuch.cx` |
| N2 | n2a uses n2b, n2b uses n2a | `cx: module cycle: n2a -> n2b -> n2a` |
| N3 | `use ../x`, `use Bad-Name` | `bad module name` |
| N4 | `#pragma unisa usee x` | `cx: …: unknown unisa pragma usee` |
| N5 | module exports `int n5_count;` | `exports object n5_count` |
| N6 | exported typedef uses a module `#define` | `uses macro N6_MAX; make it an enum constant` |
| N7 | two used modules both export `dup_f` | `dup_f exported by both` |
| N8 | used module defines `main` | `module n8b defines main` |
| N9 | `use std.io` | `unknown std module std.io` |
| N10 | `use` after a declaration, or inside `#if` | `unisa pragma after first declaration` / `inside conditional` |
| N11 | flownetw0 with the `use` removed, run alone | existing negative: `no host function fw0_radix`, rc 127. This is runtime red, which is accepted until CX3c |

**Positives:**

- P1: flownetw0 gives `module reference ok (2 units, 9 checks)` via `use`.
- P2: a diamond produces one copy of d.
- P3: `-run a.cx b.cx` where a uses b gives one copy of b.
- P4: `-cx-interface` output is byte-identical across two runs and equals a golden file.
- P5: `-o` output is byte-identical with `TMPDIR` set to two different directories (temp-path independence).
- P6: the CXIF self-containment compile is clean.
- P7: `tests/cxcc.cx` cc-oracle stdout equals unisacc's.
- P8 (**the `.c` red line**): a `.c` file containing `#pragma unisa use x` compiles to the same sha256 as at the pre-slice commit, and the existing difftest/closure suites are unchanged.
- P9: under `-ftrim-libc` the flownetw0 output size is unchanged. The quoted CXIF include must be read by `quotedinc` (F3), not disable trimming.

## 5. Migration order (each step is its own commit; the local-include ratchet only goes down)

1. **flownetw0**: hand prototypes become `use` (CX2a pilot).
2. **librarycallbacke3check.cx** (`.cx`-includes-`.cx`, and the included file has `main`). Move the body of `librarycallbacklowercheck.cx` into a module `tests/librarycallbackplan.cx` exporting `int lcb_main(int, char **)`. Both check files then become 3-line mains: `#pragma unisa use librarycallbackplan` plus `int main(int c,char**v){return lcb_main(c,v);}`. This keeps both gate names, and the `.cx`-in-`.cx` count goes from 1 to 0.
3. **snapbase.h** becomes `tests/snapbase.cx`: `static` is dropped from the 4 API functions and `snapbase_seg` is exported as a typedef. Its 2 users get `use snapbase`.
4. **cxlab.h** becomes `examples/cx-lab/cxlab.cx`: `CXLAB_MAXCOL`/`CXLAB_COLW` become `enum` constants (N6 forces this) and the API loses `static`. Its 3 users get `use cxlab`. Its `<sys/time.h>` stays private to the module body until CX1 puts `gettimeofday` in the prelude; self-containment (P6) proves it does not leak.
5. **`../exec/c/codec.h`×2 and `bind.h`×1** test the product's own C. They are C-boundary code, not modules. **Decision:** rename these three checks to `.c` with an explicit `#include <cx.h>` (spec §1.1: a `.c` may opt into the toolkit). This needs no new mechanism. A trusted `#pragma unisa cimport` waits until a real `.cx` needs a C library (spec §7 inventory).
6. The remaining system-header includes are handled by CX0/CX1, not CX2. Local-header count reaches 0, which is the CX2 exit condition (plans §2).

**CX3**, a negative gate per rule, with a count ratchet first and the refusal second:

- **CX3a.** `#include` in `.cx` gives `cx: x.cx:5: #include in .cx (use a module or the prelude)`. It lands once the CX0 ratchet total is 0. A macro that expands to a header name is also refused (spec §2).
- **CX3b.** The `extern` keyword in `.cx` text, at file or block scope, gives `cx: z.cx:1: extern object zz: modules share no objects; export an accessor function`. Before this lands, CX1 moves `extern char **environ;` into the prelude and the 3 copies are deleted. Modules never export objects and an undeclared identifier is already a compile error, so this **per-unit, lexical** rule closes the zero-read gap with no whole-program pass. The prelude's own `extern`s are in headers, not in `.cx` text, so they are unaffected.
- **CX3c.** An implicit function declaration in a `.cx` unit gives `cx: a.cx:7: g called without a declaration (missing #pragma unisa use?)`. C99 already requires the declaration, so this is conforming, and it applies to `.cx` only. A report-mode count over tracked `.cx` comes first. After CX3c, N11 becomes a compile-time refusal.

## 6. Memory-safety hooks (CX4) and why module boundaries help

- **Prelude slices** (spec §5 "rich locally, conservative at the ABI"):
  - `typedef struct { unsigned char *p; size_t n; } cx_slice;` with `cx_slice_at(s, i, &out)` returning -1 when out of range, `cx_slice_sub(s, off, len)` refusing on overflow, and `cx_read_slice(path)`.
  - `cx_fmt(cx_slice *dst, fmt, …)`, a vsnprintf wrapper that reports truncation.
  - `snapbase_seg` is already `{p, n}`, so it is the first adopter.
- **Banned-call ratchet.** The `cxmod.c` scanner counts call-position identifiers: `strcpy strcat sprintf vsprintf gets strtok atoi` (and `scanf` `%s`). The ratchet starts at today's count and only goes down. New `.cx` files must have 0.
- **Ownership at the boundary only, in v1.** Contracts are keyed by *module + symbol + signature* (spec §4), not positionally:
  ```c
  #pragma unisa contract fw0_tsv borrows_shared(path) borrows_unique(why, whylen) bounds(why, whylen)
  ```
  The projector copies these into CXIF, and consumers are checked against them.
- **Why modules help:**
  1. Every cross-unit call goes through a finite, generated CXIF inventory. That gives a "pub functions without a contract" ratchet whose denominator is exact.
  2. No shared objects (CX3b) means cross-module aliasing happens only through parameters, which contracts describe.
  3. `static` functions are whole-module visible, so they can be inferred rather than annotated.
  4. The C boundary (`.c` checks, a future `cimport`) is an explicit list, which is spec §7's trusted inventory.
  5. The CXIF hash becomes the cache key for "interface unchanged means consumer not rechecked" (spec §3).

## 7. Tempting ideas that conflict with the constraints

| Idea | Conflict |
|---|---|
| `mod`/`use`/`pub` keywords or `::` paths | spec §2: no new syntax. Pragmas plus C `static` already express it |
| Auto-mangle private non-static names or prefix exports per module | changes ABI symbol names; the cc oracle and debuggers no longer see the source names |
| Keep `.cx`-includes-`.cx` and "dedupe" | it is textual include; non-static names double. The spec bans it |
| Export macros in CXIF | spec §2 bans exported macros. Enum constants carry the values |
| Derive CXIF with the full E3 parser now | model/δ change and product bytes for a v0 the text projector plus refusals already covers. The record form comes later, from a reproduction of the text |
| Search paths, a global lib dir, or "all .cx in the dir" | resolution starts depending on disk state and order, and visibility becomes implicit |
| Fix the zero-read with weak or common symbols, or treat undefined objects as 0 "by design" | hides exactly what CX3 must name |
| Rely on today's implicit declarations instead of CXIF | wrong types (implicit int), no visibility control, and removing `use` stays green **[measured gap]** |
| Commit generated CXIF headers | drift. Derive on every build; if ever committed, it must go through derivedcheck |
| Honour `#pragma unisa` in `.c`, or run the scanner on `.c` | breaks the `.c` byte-for-byte red line |
| Manifest and lock file first (spec §3) | big-bang. The v1 relative `use` is cross-checked by a manifest later (spec §9.1) |
| Shrink the implicit prelude to `std.core` now | breaks the ~77 tracked scripts. Only at an edition boundary |
| Prepend CXIF into the source buffer on line 1 | shifts line numbers, collides with shebang and directives, and `-E` has no line markers to repair it. The forced-include list already accounts for lines |
| Allow exported mutable globals because "use guarantees the definition" | defeats CX4 aliasing control. Revisit only for `const` tables, behind contracts |

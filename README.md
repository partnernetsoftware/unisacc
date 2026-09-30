# unisacc

A C99-subset compiler for six targets -- {Linux, macOS, Windows} x {x86-64,
arm64} -- that writes the executables itself (ELF, Mach-O with an ad-hoc
signature, PE; no assembler, no linker), compiles itself, and ships as one
file, `unisacc.com`. What is unusual is inside: every *table-shaped*
decision the compiler makes (lexing classes, preprocessor directives,
operator precedence, types, instruction selection, ABI facts, peephole
rewrites ...) is answered by a small integer neural network whose weights are
**constructed** from the decision table and **verified by enumeration** over
the table's whole domain, not trained.

## Model product

`make com` builds the model compiler from declarations, dynamic facts and
shared templates. Offline Python constructs and checks integer networks;
the packaged compiler runs without Python. The classic C and Python
implementations remain behavior references. `make classic-com` builds an
explicit fallback at `out/unisacc-classic.com`; a model refusal never silently
retries through it.

Published versions, their sizes, SHA-256 and acceptance receipts:
[GitHub releases](https://github.com/partnernetsoftware/unisacc/releases), with
the per-version rows and the current model byte ledger in
[prd §0](prd.md). Development state, candidate identities, evidence boundaries
and measurement history are specification material and live there too -- this
file is the user entry point, not the status page.

## Use it

`unisacc.com` is a single file that runs on all six targets (Windows reads it
as a PE; a Unix shell runs it as a script that picks the slice for the
machine). The C headers it needs travel inside it.

```bash
./unisacc.com hello.c                       # like cc: a.out for this machine
./unisacc.com -run hello.c [args]          # compile and run in memory, nothing on disk
./unisacc.com hello.c -b osx/arm64 -o hello # write an executable for a target
./unisacc.com -O2 a.c b.c -b lnx/x86_64 -o prog
./unisacc.com -E file.c                     # preprocess only
```

- **Flags:** `-O0`/`-O1`/`-O2`, `-o`, `-b os/arch`, `-run`, `-I`, `-D`,
  `-include`, `-E`, `-MD`/`-MF`/`-MT`/`-MP`, `-nostdinc`, `-ftrim-libc`/`-fno-trim-libc`, `--version`; `-Wall`,
  `-Wextra`, `-g`, `-std=c99` are accepted. A `#!` first line is skipped.
- **Headers:** twenty standard/compatibility headers are bundled (`assert
  ctype dirent errno float inttypes iso646 limits math memory signal stdarg stdbool
  stddef stdint stdio stdlib string time wchar`), plus `unisacc_ffi.h`. Ordinary
  library calls use the bundled C implementations, compiled on demand;
  `-ftrim-libc` (**on by default since 0.0.11**; `-fno-trim-libc` keeps every library body; `-libneed` is a compatible alias) conservatively selects library bodies using identifier and dependency closures. On
  macOS, the explicit dl/libffi bridge can call system APIs; it is not automatic
  forwarding of all libc calls. System FILE/va_list and allocator families must
  not be mixed with the bundled implementations.
- **Several files, one program:** `unisacc a.c b.c ...` compiles all the units
  of a program in one invocation. Each unit is preprocessed on its own, and a
  later unit's file-scope `static` names are renamed, so two units may each
  keep their own `helper`.
- **Not a `cc` drop-in:** `-c` writes the compiler's intermediate *tape*, not
  an object file; there is no `.o`, no link step, and no `-l`/`-L`. Build
  systems that compile each file with `-c` and link later cannot use it
  directly. C11 features and GCC extensions (statement expressions, `_Generic`,
  empty structs, inline asm) are outside the subset.

## Build it

```bash
./tests/build_ref.sh            # cc builds a private flat export (-> /tmp/ua_ref)
make export-ref                 # independent full source: out/unisacc-flat.c
/tmp/ua_ref -O2 out/unisacc-flat.c -b osx/arm64 -o ua1  # unisacc builds itself
make com                        # construct the model compiler in two build slots
make classic-com                # explicit classic fallback in out/
```

`unisacc.c` is the classic reference's ordered include entry (`kernel/` +
`src/`), with no embedded weight literals. The generated
`kernel/weight.<stage>.inc` and `kernel/dense.<stage>.inc` keep the 18 classic
stages' data separate. `tests/export_ref.sh OUTPUT` expands the entry into one
independent C file without Python; transfer and self-hosting tests use this
full export rather than the short entry. `build_ref.sh` never rewrites the
root source. The default product's shared networks remain in its separate P3
package; these classic fragments are not another copy of that package. The model route uses Python offline to
bind declarations, construct and verify networks, package them, and assemble
the APE container; the packaged compiler needs no Python to compile programs.
Its fixed-package driver bootstrap (`N1 = N2 = N3`) has a narrower scope than
reconstructing the package. `tests/release.sh` performs bounded local acceptance of an explicit, already-built
model candidate (see [model build instructions](exec/c/BUILDING.md)); it does not
build a replacement or assert cross-platform release readiness. CI only re-runs tests.

## How the decisions are made

> **Shell = inferencer + executor + model data.**

In the classic seed/reference, the walker, symbol table, relocation arithmetic
and image writers are ordinary code. Its fact stages below use networks, with one kernel for all of them
(`embed -> gemv -> ReLU -> gemv -> argmax`, integers only):

<!-- stages:begin -->
| stage | key | out | keys | units |
|---|---|---|---:|---:|
| `pp` | dir(9) x defined(2) | 4 classes | 18 | 6 |
| `lex` | c(12) x peek(12) | 10 classes | 144 | 11 |
| `parse` | nt(5) x tok(68) | 36 classes | 340 | 34 |
| `type` | t1(15) x op(19) x t2(15) | 16 classes | 4,275 | 62 |
| `scope` | ctx(6) x kind(5) | 7 classes | 30 | 9 |
| `irsel` | family(6) x flavor(70) | 70 classes | 420 | 75 |
| `enc` | op(74) x os(3) x arch(2) | 5 classes | 444 | 5 |
| `reloc` | kind(3) x arch(2) | 3 classes | 6 | 3 |
| `regmap` | treg(8) x arch(2) | 16 classes | 16 | 10 |
| `tyinfo` | t(16) | 3 heads | 16 | 6 |
| `pfconv` | conv(9) | 7 classes | 9 | 7 |
| `peep` | a(8) x b(17) x rel(12) | 10 classes | 1,632 | 18 |
| `opinfo` | op(71) | 3 heads | 71 | 16 |
| `prec` | op(19) | 11 classes | 19 | 11 |
| `binsel` | op(16) x sign(2) | 23 classes | 32 | 18 |
| `isel` | op(74) x arch(2) | 2 heads | 148 | 74 |
| `abi` | op(74) x os(3) x arch(2) | 13 heads | 444 | 52 |
| `combo` | op(74) x os(3) x arch(2) | 15 heads | 444 | 100 |
| **total** | | | **8,508** | **517** |

*Generated by `python3 -m unisa docs` from `unisa/gold.py` and the constructed weights; `tests/docs.sh` fails when this differs.*
<!-- stages:end -->

The last two are a control arm: the lowering asks neither. Units are the
constructed weights' hidden units -- what ships -- not trained parameters.

**Exactness is constructed, not searched.** The weights are derived from the
gold decision tables (`weights/gold/*.tsv` is the tables as data): `W1` in
{0,1}, `b1` recoverable from `W1`, `W2` small powers of two. The hidden
activation is always 0 or 1, so inference has no multiply and no float.

**Verification is exhaustive, not statistical.** Each stage is a total
function on a finite domain, so "the net equals the table" is decided by
enumerating every key (the total is in the table above). That proves the net
equals the *table*; whether the table is right about C is checked separately,
against the system compiler (`tests/gold_audit.py`, `difftest`).

## Develop it (the Python seed)

The Python package `unisa/` is the seed: it holds the gold tables, constructs
and verifies the weights, generates `kernel/`, and is a second, independent
front and back end that the C compiler is checked against byte for byte.
Python 3.11+, standard library only; no training is on the shipping path.

```bash
python3 -m unisa build-weights          # construct the weights from the tables
python3 -m unisa acc                    # every stage 1.000, by enumeration
python3 -m unisa run examples/fact.c --fold   # six lowerings in the interpreter
python3 -m unisa emit-kernel            # regenerate kernel/
```

`iterate/` holds development tools written in C (a weight constructor and a
kernel-data generator) that reproduce parts of the seed's output; they are
not part of the product.

## Tests

`tests/all.sh` is the entry point; each suite's verdict is its exit status, and
each gets a watchdog so no suite can hang the run. **Testing happens on this
desk, not on GitHub**: macOS natively, Linux in a local Lima VM
(`tests/linux.sh`), Windows 11 in a local UTM machine (`tests/crossnative.sh`).
CI is a second opinion, not the test loop: the local suites run first, and
`.github/workflows/ci.yml` re-checks them on a machine nobody has been
editing. Training is **not** in it:
the weights we ship are constructed, `unisa acc` verifies those by enumeration
in a twentieth of a second, and the SGD control arm lives in
`tests/baseline.sh` for when someone wants the comparison numbers.

| suite | what it checks |
|---|---|
| `acceptance` | the spec's own checklist |
| `vm` | the tape interpreter, on hand-written fixtures |
| `difftest` | unisa vs the system compiler — the only instrument that can see a defect in the reference tables |
| `native` | emitted images actually executed on this host |
| `crossnative` | the *other* targets executed on real machines — local Linux VMs and the Windows 11 machine in UTM; the blind spot where three codegen bugs lived |
| `linux` | the whole suite again, inside the local Lima VM, because glibc and BSD libc disagree about things C only calls unspecified |
| `fat` | one file, both macOS architectures, both slices actually executed |
| `artifacts` | the shipped kit is *usable*: weight blob round-trips to the same decisions, every image is recognised by the platform's own tools, shipping twice gives the same bytes |
| `ccrun` | `unisacc` compiles C and the reference VM runs it |
| `selfhost` | `unisacc` built two ways agrees with the Python front end |
| `stages` | every decision stage the Python front end asks a net about, the C front end asks too — the only suite that can see a hand-written rule that happens to agree with the table |
| `selfgap` | how much C `unisacc`'s own front end still refuses that the Python one accepts — a ratchet, because that gap was growing unmeasured |
| `multi` | two translation units compiled into one program, against `cc a.c b.c` |
| `bootstrap` | `B = C = U`, the self-hosting fixed point |
| `closure` | the image `unisacc -b` writes is byte-identical to the Python back end's, for every probe on all six targets (this is a check of the C back end against the seed, not a proof of C semantics) |
| `nativeboot` | `unisacc` builds itself and the result rebuilds itself to the same bytes — no Python anywhere |
| `ablate` | each stage's answer is rotated to a wrong one: an image must change, or the compile must be refused — asking a net is not the same as obeying it |
| `run` | `unisacc -run` compiles a file and runs it in memory, against the system `cc` |
| `cli` | the compiler as a tool, from a scratch directory: built-in headers, `-I`, `-D`, shebang, exit status |
| `ccparity` | unisacc next to `cc -std=c99` as a user sees it: a.out, `-o`, exit status, stderr, files left after a failed compile, `-E`/`-D`/`-U`/`-I`, `-Werror`, and the documented `-c` difference |
| `ape` | `unisacc.com` is built and run on this host: one file, a PE for Windows and a script for Unix |
| `layout` | the data layout **enumerated**, not sampled: every tape of up to three data definitions, each also with one symbol defined twice — 762 tapes, 4,572 image comparisons across all six targets, in twelve seconds |
| `datashape` | generated programs whose globals are declared in one order and allocated in another; the suite asserts the shape is present before it asserts the bytes match |
| `bigclosure` | the closure on the compiler itself — the largest input there is, and the only one ever big enough to break the back end while every probe stayed green |
| `corpus` | [c-testsuite](https://github.com/c-testsuite/c-testsuite) — 220 programs written by other people, for other compilers |
| `tools` | real library code by other people — [crypto-algorithms](https://github.com/B-Con/crypto-algorithms), [tiny-AES-c](https://github.com/kokke/tiny-AES-c), [tiny-regex-c](https://github.com/kokke/tiny-regex-c); eleven entries, several files each, with their own known-answer tests |

## Layout

```
exec/             production model declarations, offline constructors, driver and generic execution cores
src/*.c           classic reference compiler, written in the C subset it compiles
kernel/           generated classic fact data + integer inference kernel, as C
include/          the bundled C99 headers
unisacc.c         ordered include entry: classic reference, no inline weights
out/unisacc-flat.c make export-ref: independent full source for transfer/self-hosting
unisa/            the Python seed: gold tables, weight construction, reference front/back end, packaging
weights/          the constructed weights (built.uns2) and the tables as data (gold/*.tsv)
iterate/          development tools in C (weight constructor, kernel-data generator); not the product
tests/            the suites (differential vs cc, native runs, self-hosting, closure, corpora)
prd.md            the specification and the measured log
```

Current design and status are in [prd §0.3](prd.md#pipeline-design); older
measured findings and failed predictions remain in [the archive](archive/s17-migration-log-20260928.md).

## Historical corpus and library observations

The results below describe earlier classic/reference measurements, retained
for the defects they exposed. They are not the current candidate's acceptance
snapshot; `tests/corpus.baseline` now requires 216 passes. Current candidate
receipts are linked above.

`tests/corpus.sh` runs c-testsuite, 220 single-file C programs this project had
no hand in writing:

```
corpus 220   pass 214   wrong 0   unsupported 0   knownfail 6   slow 0
```

The programs are compiled to a real image for this host and **executed**, not
interpreted: the reference VM is a Python loop, and an eight-queens search a
real CPU finishes instantly takes a quarter of an hour there. `wrong` is the
only failure — a program that compiled and then disagreed.
`unsupported` is the honest coverage gap — the front end refuses the
program — and was **zero** in this snapshot, so the next one to appear was an alarm
rather than another entry in an old backlog. `knownfail` is the list we are
deliberately not chasing: floating point, GCC extensions, `_Generic`, and one
place where this subset evaluates `int` arithmetic at 64 bits on purpose.
`tests/corpus.baseline` is a ratchet, so `pass` may only go up.

But a corpus of single files written *for compilers* is not the same as
library code written *to be used*. `tests/tools.sh` runs the other kind —
crypto-algorithms, tiny-AES-c and tiny-regex-c: eleven entries, every one of
them several files, with their own known-answer tests, none of it floating
point:

```
tools 11   pass 11   wrong 0   unsupported 0   skip 0
```

It read `pass 2` the first time. The ten defects behind that are the most
useful thing this project has measured in a while — **seven of them produced a
wrong answer rather than a crash**, and two were invisible to all six `--fold`
targets because the interpreter does not model addressing modes:

- comments were being removed by the *lexer*, not before the directives, so
  `#define N 32  // note` commented out the rest of every line that used N;
- macro parameters were substituted one at a time instead of simultaneously,
  so MD5's `FF(d,a,b,c,…)` rounds wrote the wrong variable;
- arm64's `[fp, #-off]` immediate is a *signed* 9-bit field and we were masking
  it, so every function with more than 256 bytes of frame read the wrong local;
- `uint8_t` was a typedef for signed `char`, so AES printed a screenful of `f`s;
- parentheses destroyed an lvalue, so `(*p)++` was refused outright;
- and a struct passed by value was copied through the registers the *later*
  arguments were still sitting in.

`prd.md` §6 E-45 and E-46 retain the full list. The lesson is not that real code
is harder — it is that real code leans on the type system, the ABI, the
encoder and the library *at the same time*, and a probe we write ourselves
leans on one of them at a time.

## Prior art

`research/prior-art.md` is a survey of the neighbouring literature, and it is
deliberately unflattering. The short version: constructing weights rather than
training them is 1996 (Omlin & Giles) and 2023 (Tracr); replacing a lookup
table with a small network is ACAS Xu, 2016 — which is also where the
counter-example lives (Bak & Tran 2022 showed the compression unsafe, and
Boniol et al. 2026 then compressed the same tables *exactly* with BDDs). Not
new here: the existence theorem, integer/power-of-two weights, exhaustive
verification, or size. What we have not found a precedent for is the
combination — every table-shaped decision point of a real self-hosting C99
compiler behind one kernel, with **no implicit fallback on model rejection**, verified by
enumeration over the whole domain.

## UJS（平行 JS 产品）

开箱：**浏览器 / Node / Bun** `wasm_run`；Python 仅在 `ujs/construct/` 做出货。

```js
import { bootRuntime, wasm_run } from "./ujs/core/wasm_run.js";
await bootRuntime(new URL("./ujs/core/ujs_full.wasm", import.meta.url));
console.log((await wasm_run("return 1+2;", {}, {})).ok);
```

| | |
|---|---|
| 规格 | [`ujs/prd.md`](ujs/prd.md)（思维树 · 记忆宫殿） |
| 开箱 | [`ujs/README.md`](ujs/README.md) · Host [`ujs/uxe/HOST_ABI.md`](ujs/uxe/HOST_ABI.md) |
| 论文 B | [`research/ujs-paper-outline.md`](research/ujs-paper-outline.md) · [`research/README.md`](research/README.md) |
| 演示 | `cd ujs && npm run demo` |
| 验收 | `./tests/ujs.sh` · `cd ujs && npm run test:uxe:all` |
| 本地产物 | `python3 -m ujs web-build` |
| 发版附件 | `./ujs/scripts/release-artifacts.sh` → `dist/`（上传 GitHub Release） |

与 C 线共享构造代数；**不**把 Web 并入 `unisa` 交付范围。

## Status

The published model baseline is v0.0.8; the smaller P3/one-pass-memory
candidate is local and has not been released. Candidate-specific verification,
platform gaps and signing work are recorded above and in [prd](prd.md).

The generated table above describes classic finite fact decisions; current
whole-stage networks and their limits are documented in
[the pipeline design](prd.md#pipeline-design). Exhaustive network/table equality
is distinct from C semantics, stage compatibility, platform execution and
package/container self-construction. Classical no-Python self-hosting and
fixed-package model-driver self-hosting retain their separate evidence scopes.

## Known limitations

What this compiler does not do, so a reader does not have to find out by
experiment.  Every line was measured against the shipped `unisacc.com`, and the
reproduction is one short program.  The table exists because an external trial
of 0.0.12 reported time lost on exactly these, documented nowhere.

| Limitation | What happens | Reproduce |
|---|---|---|
| No `__LINE__` / `__FILE__` | `error: unknown identifier` at the use site.  `#include` is spliced inline without line markers, so a naive expansion would be wrong after the first header -- but the diagnostic engine does track the user's line, so "the information is unavailable" is not the reason | `int main(void){return __LINE__;}` |
| `long double` is `double` | `sizeof(long double) == 8`; no extended precision | `printf("%d", (int)sizeof(long double))` |
| No `offsetof` | `error: this is not the start of an expression` -- the macro is absent and the built-in form is not accepted | `#include <stddef.h>` then `offsetof(struct S,b)` |
| `stdin` is `((FILE *)0)` | `stdin == 0` is 1, so `FILE *in = argv[1] ? fopen(argv[1],"r") : stdin; if (!in) ...` reads the stdin path as a failure; fd 0 gets no sentinel | `printf("%d", stdin == 0)` |
| Private calling convention between generated code | Not SysV / AAPCS64: arguments are pushed on the tape stack left to right, `r9` is the frame pointer, the result is in `rax`, the caller pops.  **Unstable** -- it follows the regmap and abi tables and external code must not rely on it.  Interop with outside code goes through libunisacc's carriers, not through this convention.  [prd W-16](prd.md) | `examples/apps/xgui.c` writes a raw syscall stub by hand and must save `r9`, `rcx` and `r11`, because `r9` doubles as syscall argument 6 and `syscall` clobbers `rcx`/`r11` |
| libffi interop is macOS-only | `include/unisacc_ffi.h` opens `/usr/lib/libffi.dylib` with no platform branch, so the FFI path is unavailable on Linux and Windows.  The bridge exists; the hard-coded path is what is missing | any FFI call on Linux |
| Missing C library surface | No `setjmp.h`, `regex.h`, `termios.h`, `locale.h`, `strings.h`, `unistd.h`, `fcntl.h`, `sys/stat.h`.  Missing functions include `memchr` `strspn` `strpbrk` `strcspn` `strcoll`, `strtoll` `strtoull` `atoll`, `feof` `ferror` `ungetc` `clearerr` `setvbuf` `tmpfile` `vprintf`.  `errno.h` and `time.h` exist but are partial: several `errno` codes and `time()` itself are absent.  `sys/types.h` exists and is **types only** -- `ssize_t`, `off_t`, `pid_t`, `mode_t`, `dev_t` and the rest; `sys/stat.h` is deliberately absent because `struct stat` would describe an interface `stat` does not yet implement, and a header of constants with no functions behind them would be inventing one | plan item N14 carries the list |
| Identifiers: 63 significant characters | C99 6.4.2.1 requires that many, and both front ends keep them.  **The C reference refuses a 64-character name** with `error: identifier too long (max 63 characters)` at its position, rather than truncating it -- truncation merged two distinct names that shared a prefix, and miniz's `tinfl_`/`tdefl_` families are exactly that shape.  The **product accepts arbitrary lengths** (E3 interns names by source position), so a program the reference rejects still compiles on the product; the asymmetry is deliberate, not a bug in either | a 64-character identifier: the reference exits 1 with that message at the name, the product compiles it |
| Struct member count: 256 per struct | A limit separate from the table capacities below: one struct may hold at most 256 members.  Both front ends refuse a 257th, worded differently -- the reference prints `too many members` with no position, the product prints `error: not covered: structure member capacity` at the member | a struct with 257 members, then `s.m0 = 1` |
| Shared table capacities are not deduplicated | The typedef, struct-tag and member tables are shared across translation units and **not deduplicated**: every unit re-registers the headers it includes, so N units of the same headers consume N times the entries.  Counted rather than guessed -- one unit of the miniz headers contributes ~91 typedefs, and three units crossed the old limit of 256.  The limits are now typedefs 1024, struct tags 512, members 4096 | compile the same header from three translation units and watch the typedef count climb |
| Indirect calls with six or more register arguments | A call through a function POINTER with six or more arguments is refused: `not covered: six indirect register arguments leave no callee register`.  Five is fine, and a direct call with six is fine, so this is not a general calling-convention limit.  Both sides read the same table (`tape` has r0..r7: r0..r5 are the argument registers on this path, r6 is the frame pointer every prologue sets with `mov r6, r7`, r7 is the stack pointer), so there is no register left for the callee's address.  Routing it through the stack instead fights the callee's own prologue, which reads arguments from registers until more than six of them.  Fixing it properly needs a new tape operation -- a call through memory, or a scratch register the lowerer picks per target -- which is 0.0.14's calling-convention work.  stb_image.h's JPEG decoder uses this shape | a function pointer called with six arguments: `z.fn(&r,1,2,3,4,5)` |

Two things that look like limitations and are not.  `-ftrim-libc` is on by
default and keeps library bodies only when a program reaches them by identifier
closure, so a symbol reached solely through a bundled header's macro expansion
can be reported undefined ([R13-0 #03](tests/difftest.com.knownfail)).  And the
model compiler refuses to compile rather than falling back to the classic path,
by design.

# exec/ — model-driven compiler development route

The current source-to-image route is `pipeline/elf.sh`, using `c/run.c` and
six constructed threshold networks. `c/run.c --chain` also connects explicit
models in one process (see below). `c/buildcompiler.sh OUTPUT_DIR` now builds
`unisacc-next.com`: a development compiler with all six target routes, shared
networks and two carried assembly cores. It runs without loose model/core files
or Python; offline construction still uses the seed tools. See
[the assembly binding and tests](c/asm/README.md#carried-kernel-and-compiler-container).
The shipped compiler has not adopted this route: CLI/source/error parity is
still incomplete. `-Wall`, `-Wextra` and `-Werror` use the located warning
models for one or several source files. `c/warningcheck.sh` and
`c/multiwarningcheck.sh` check this contract. Ordinary compilation also retains
source locations. `parse2/errors.py` generates mapped syntax diagnostics and
top-level recovery; unmapped prototype limitations remain explicit rejections.
`parse2/errorcheck.py` checks complete results and error limits. The current
model container passes diag 14/14, CLI 64/64 and ccparity 53/53 (plus its
existing known -c difference). `pp/macrocheck.py` compares 20 macro inputs in
ordinary and located formats, including the C99 expansion examples. Broader
source and failure compatibility remains incomplete; these are suite results.
For current declaration sources and remaining work, see [rules.md](rules.md).
The sections below retain milestone evidence; statements that packaging was
unfinished describe those earlier milestones, not the current container above.
The following E0 machine and toy are retained as their original experiment.

## E0 toy experiment

Milestone E0 of prd.md S-17. `exec.c` runs the machine of
research/delta-framework.md §2 with option A primitives (§3); it knows no
language. `toy/` is the §4 LL(1) expression grammar as a postfix translator.

    ./exec/check.sh    # build with cc and unisacc, T1 table check, 29 cases vs toy/ref.py
    ./exec/ledger.sh   # sizes and speed (after check.sh)

## Machine

C = (q, r, i, s, W, o, h). One step: obs = (q, r, b, t) with b = x[i] or 256
(EOF), t = stack top or NG (⊥); (q', acts) = δ(obs); q := q'; r := 0; run the
actions in order, stopping at the first halt. Accept: o on stdout, exit 0.
Reject(k): `reject k at i` on stderr, no stdout, exit 1. Bad table: exit 2.
Storage is static and bounded (`MAX*` in exec.c); running out is exit 3
`exhausted`, never a reject.

## Actions (code, params)

| code | name | params | effect |
|---|---|---|---|
| 0 | ACC | – | h := accept |
| 1 | REJ | k | h := reject(k) |
| 2 | ADV | – | i := i+1 if i < \|x\| |
| 3 | PUSH | g | push g (0 ≤ g < NG) |
| 4 | POP | – | pop; empty stack: reject(254) |

> The E0 table above is the 19-action toy machine of exec.c. The machine E1-E3 run on has 56 actions; its authority is the header of `exec/pp/sim.py`, and `exec/c/run.c` implements the same. There, a POP on an empty stack is a bad-table error on both sides, not a reject (2026-09-26, bdy review).
| 5 | EMIT | c | append byte c |
| 6 | COPY | – | append x[i]; at EOF: reject(253) |
| 7 | SETR | v | r := v (0 ≤ v < NR) |
| 8 | LDI | k v | W[k] := v |
| 9 | BYTE | k | W[k] := x[i] or 256 |
| 10 | ALU | op d a b | W[d] := W[a] op W[b], u32 wrap; op 0 add 1 sub 2 mul 3 divu (÷0 → 0) 4 remu (÷0 → a) 5 and 6 or 7 xor 8 shl 9 shr (shift by b&31) |
| 11 | CMP | a b | r := 0/1/2 for W[a] <,=,> W[b] (unsigned) |
| 12 | LOAD | d k | W[d] := W[W[k]] |
| 13 | STORE | k v | W[W[k]] := W[v] |
| 14 | OUTW | k | append W[k] & 255 |
| 15 | GETI | k | W[k] := i |
| 16 | SETI | k | i := min(W[k], \|x\|) |
| 17 | SPAN | a b | append x[W[a] .. min(W[b], \|x\|)) (nothing if W[a] ≥ that) |
| 18 | SETRW | k | r := W[k]; W[k] ≥ NR: reject(252) |

ALU op 10 is ltu: W[a] < W[b] (unsigned) as 0/1.  Codes 15-18 and ALU op
10 were added for E1 (exec/lex/tbl.py lowers the E1 lexer delta's actions to
these); like the rest they name positions, bytes and integers, no language.

W is a dictionary u32 → u32, missing keys read 0. NR ≥ 3 (CMP's codes).

## Table format (text, decimal integers, `#` to end of line is a comment)

    NQ NR NG q0
    <default entry>
    nrows
    q r b t <entry>        x nrows; -1 in q/r/b/t = any value
    entry := q' n a1 p.. a2 p.. ... an p..   (n actions, each code + its params)

Every observation starts at the default entry; rows are applied in order,
later rows overriding earlier ones. The loader expands this to the dense map
over the whole domain NQ×NR×257×(NG+1), stored factored: state q is indexed
only by the components (r, b, t) that some row for q names -- a component no
row names cannot change q's entries.  `exec -fdump` prints the factored cells
(-1 = unread); `exec -dump` prints the whole domain, and
`toy/gen.py check` compares it line for line with `delta_ref` (T1 for the toy).

## Toy

`toy/gen.py` derives Q (entry states + one item per grammar position, 34),
Γ (the return points after non-tail calls, 6) and δ from the grammar
table in it; `toy/toy.tbl` is its output (107 rows). Reject codes: 1 no F
alternative, 2 `)` expected, 3 trailing input. `toy/ref.py` is the
hand-written recursive-descent reference.

Not done in E0: the table is not built as an integer net through the
`unisa build-weights` route; T1 here is the enumeration check only.

## E1: the lexer delta on this executor

    exec/lex/run.sh gen; exec/lex/run.sh xbuild          # table, cc + unisacc builds, T1
    E1EXEC=/tmp/e1x/exec_ua:/tmp/e1x/e1.tbl exec/lex/run.sh lexdiff   # (probes, self, corpus.*)
    exec/lex/ledger.sh                                   # __text, table bytes, speed

## Source-to-image development route

`TARGET=win/arm64 ./exec/pipeline/elf.sh OUTPUT_DIR unisacc.c` now produces
`OUTPUT_DIR/unisacc.exe` through six generated deltas on the generic C executor.
The same entry supports Windows x86_64 and the four POSIX targets (`.elf` / `.macho`). Python is
still used to generate transition tables, not to process source between these
six stages. The default constructs and exhaustively checks integer threshold
networks, then evaluates them at runtime; `NETWORK=0` selects lookup tables (see [c/NETWORK.md](c/NETWORK.md)). The generated
compiler is the existing C compiler; this is not E7 adoption in the product.

`TARGET=win/arm64 ./exec/pipeline/selfcheck.sh` checks all five Windows target
macros against the native front end, optimized tape and whole PE bytes against
the current reference. It does not claim native execution on a macOS host.
Windows `__LP64__` matches this compiler's current predefined macros and
64-bit `long`; it does not describe the Windows system's LLP64 C ABI.
For an already-running Windows ARM64 UTM VM, the separate opt-in command is
`perl -e 'alarm 60; exec @ARGV' python3 exec/pipeline/winbootstrap.py OUTPUT_DIR unisacc.c`.
It copies and verifies inputs, compiles N2 and N3 in the guest, checks explicit
per-run exit receipts and byte equality. Guest timeout cleanup uses taskkill
on the process tree; VM lifecycle remains the caller's responsibility.
The outer 60 s budget covers both generations and transfers: it can expire
before the two separate 30 s guest watchdogs. Such a run fails; it is not a
successful bootstrap. Missing exit-receipt files are retried while polling.

Windows x86 setup and WinAPI gates are encoded by `exec/enc/x86win.py`: winsave,
winrest (result in rax), winstdh and winargs. Fixed sizes are measured before
branch relaxation; bytes are regenerated afterwards with final RIP addresses,
and the final sizes must match. `x86wincheck.sh` compares both executors with
the reference, including a shortening branch before these instructions.
WINARGS_BODY remains an explicit machine-code template. Eleven WinAPI bodies
and their ABI-selected return conversions use the same deferred output path.
`TARGET=win/x86_64 ./exec/pipeline/elf.sh OUTPUT_DIR unisacc.c` produces the
sixth target's complete PE. `TARGET=win/x86_64` also selects that target for
`winbootstrap.py`; the PE machine field must match. Tested on Windows 11 ARM64
via x86_64 emulation, not on x86_64 hardware: N1=N2=N3, 694,272 B, SHA256
`19bfcf3d809ac42e6120a4cbc033b9002de3d556c1634c6f59ad23647b312fc8`.
Source: f360ba0's unisacc.c, SHA256
`11ef59801d8c18f637e67b2a5b0e988ad31baec1859fabdd06dc879935574307`.
This closes the measured self-source route for six targets. Frontend coverage
is still partial. The current default evaluates constructed threshold networks;
Python still generates the models, and the shipped .com has not switched to
this executor.

E3 sizeof scalar expressions now reuse expression parsing and discard emitted
instructions, including nested sizeof and unevaluated static-initializer
operands. General non-scalar sizeof expressions still reject; named-array
sizes retain the existing dimension path. The independent host-cc probe also
corrected the product's sizeof result descriptor to unsigned 64-bit size_t.
The authoritative fixed regression sets are `parse2/keep-e3.txt`,
`c/keep-chain.txt` and `pipeline/keep-elf.txt`; they do not claim complete
C99 coverage.


## One-process byte-stream chain

After `pipeline/elf.sh` constructs and checks a target's six models, its
runtime can run them in one process without intermediate files:

```sh
perl -e 'alarm 60; exec @ARGV' env UNISA_MAXSTEPS=400000000000 \
  OUT/run --chain examples/hello.c examples/hello.c "$PWD/include" \
  OUT/e2.net OUT/e1.net OUT/e3.net OUT/e4.net OUT/lower.net OUT/elf.net > hello.image
```

`--chain INPUT SRCPATH INCLUDE_DIR MODEL...` is a development interface.
The list is arbitrary and ordered; no C stage names, target selection or
compiler decisions are built into the driver. Each model receives only the
preceding accepted bytes, like the file boundary of the existing pipeline.
Output attributes do not cross this boundary. Source path and include directory
are explicit configuration shared by the stages.

Every stage starts with fresh registers, indexed memory, stack, input frames,
blobs, intern table and file cache. Stage allocations and the previous input
are released; only accepted output survives into the next input. The final
bytes go to stdout once all stages accept. Reject/bad-model/step-limit status
stops the chain, preserves its diagnostic and publishes no partial image.
This does not change the existing single-model or `--check-net` interfaces.

`c/netcheck.py` checks repeated-stage reset, empty streams and failure
propagation. `c/nativecheck.sh` compares the chain against the six-process
route using cc-, unisacc- and network-built runtimes, including runtime
self-reconstruction. This is preparation for product integration, not a new
`.com` CLI, a single embedded model package, or completion of E7.


## A shared model package

Network construction now also writes `OUT/models.pkg`, using the ordered
format declarations in `pipeline/image-stages.tsv`. Run it without Python:

```sh
perl -e 'alarm 60; exec @ARGV' env UNISA_MAXSTEPS=400000000000 \
  OUT/run --bundle OUT/models.pkg osx/arm64 examples/hello.c examples/hello.c "$PWD/include" > hello.image
```

Choose the route that was constructed; the runtime does not infer a target
from the host. `python3 exec/c/pack.py -o all.pkg OUT1/route.tsv OUT2/route.tsv`
combines explicit routes and stores equal network bodies once. See
[c/PACKAGE.md](c/PACKAGE.md) for the directory and execution contract. The
package is an external development input; embedding it in `.com`, preserving
the product CLI and isolating the small execution core remain unfinished.


The constructed package now also carries `include/` as named byte resources.
The packaged route can therefore omit the final include-directory argument
and run from an empty directory containing only its source and package.
These 19 header files remain explicit source resources, separate from the
network weights. `--mount` is a construction-time generic byte-name mapping;
the runtime serves it through SBFIND. External project headers still resolve
through the filesystem. See the v2 section of [c/PACKAGE.md](c/PACKAGE.md).

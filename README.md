# UNISA SH

A C99 compiler whose entire **decision layer is a set of exact integer neural
networks**, targeting six platforms, and small enough to carry inside the
compiler it compiles.

```
$ python3 -m unisa run examples/hello.c --fold
  lnx/x86_64     ok    'hello from C99\n'  exit=0
  lnx/arm64      ok    'hello from C99\n'  exit=0
  osx/x86_64     ok    'hello from C99\n'  exit=0
  osx/arm64      ok    'hello from C99\n'  exit=0
  win/x86_64     ok    'hello from C99\n'  exit=0
  win/arm64      ok    'hello from C99\n'  exit=0
6/6 match
```

## The claim

> **Shell = inferencer + executor + model data.**

The walker, symbol table, relocation arithmetic and object-file headers are
classic code — algebra, not tables. Every *table-shaped* decision is a network:

| stage | key space | rows | θ |
|---|---|---|---|
| `pp` | dir × defined | 18 | 274 |
| `lex` | charclass × peek | 121 | 418 |
| `parse` | NT × TOK | 340 | 1,451 |
| `type` | t1 × op × t2 | 3,211 | 1,622 |
| `scope` | ctx × kind | 30 | 479 |
| `irsel` | family × flavor | 190 | 1,194 |
| `enc` | op × os × arch | 276 | 893 |
| `reloc` | jmpkind × arch | 6 | 161 |
| `isel` | op × arch | 92 | 2,628 |
| `abi` | op × os × arch | 276 | 4,457 |

Ten decision points, one kernel: `embed → gemv → ReLU → gemv → argmax`.
Swap the weights, change the capability. The kernel never changes.

## What makes it unusual

**Exactness is constructed, not searched.** The weights are *derived* from the
gold decision tables, not trained: `W1 ∈ {0,1}`, `b1 ∈ {0,−1,−2}` (and not
stored — it is recoverable), `W2 ∈ {1,2,4,8,16}`. The hidden activation is
always 0 or 1, so there is **no multiply, no shift, no float anywhere**, and an
**int8 accumulator suffices** (largest logit 40).

**Verification is exhaustive, not statistical.** Each stage is a total function
on a finite closed domain, so `∀k ∈ K_s : argmax(N_s(k)) = G_s(k)` is decided
by enumeration — 4,560 keys, zero disagreements. Not a test: a decision
procedure.

**Four of the six targets have really run.** `--fold` compares six lowerings
inside one interpreter, which catches ABI and syscall-number mistakes but not
encoder bugs: the interpreter models no page protection, no real `rsp` and no
two-operand ALU. `osx/arm64`, `osx/x86_64` (Rosetta), `lnx/x86_64` and
`lnx/arm64` are executed on real kernels, every probe, every time
(`tests/crossnative.sh`). `win/*` is still only verified structurally: the PE
now has aligned sections, a kernel32 import table, base relocations and a load
config, and Windows still refuses the image. The three rules arm64 Windows does
enforce — and the dozen that turned out not to matter — are written up in
`prd.md` §6 E-35.

**One file, two instruction sets.** `unisa fat hello.c -o hello` emits a
Mach-O universal binary; the arm64 slice runs natively and the x86_64 slice
runs under Rosetta, and `tests/fat.sh` executes both on every probe. That is
multi-ISA within one OS — a cosmopolitan-style file that is simultaneously an
ELF, a Mach-O and a PE is a different problem and is not what this emits.

**It self-hosts.** `unisacc.c` carries the model (a 12,554-byte blob) and the
integer kernel, and drives its own lexer, preprocessor and parser through the
same tables. The bootstrap fixed point holds:

```
A = cc(unisacc.c)      B = A(unisacc.c)      C = B(unisacc.c)      B == C
```

## Try it

```bash
python3 -m unisa build-weights          # construct exact integer weights
python3 -m unisa acc                    # every stage 1.000, by enumeration
python3 -m unisa run examples/fact.c --fold
python3 -m unisa compile examples/fib.c -o fib --target osx/arm64 --drive built
python3 -m unisa ship --out kit.zip     # weights + manifest + kernel + images
./tests/all.sh                          # every suite, one summary
```

Python 3.11+, **standard library only**. No numpy, no torch, no build step.

## Tests

`tests/all.sh` is the entry point; each suite's verdict is its exit status, and
each gets a watchdog so no suite can hang the run. Training is **not** in it:
the weights we ship are constructed, `unisa acc` verifies those by enumeration
in a twentieth of a second, and the SGD control arm lives in
`tests/baseline.sh` for when someone wants the comparison numbers.

| suite | what it checks |
|---|---|
| `acceptance` | the spec's own checklist |
| `vm` | the tape interpreter, on hand-written fixtures |
| `difftest` | unisa vs the system compiler — the only instrument that can see a defect in the reference tables |
| `native` | emitted images actually executed on this host |
| `crossnative` | the *other* targets executed in local Linux VMs — the blind spot where three codegen bugs lived |
| `fat` | one file, both macOS architectures, both slices actually executed |
| `artifacts` | the shipped kit is *usable*: weight blob round-trips to the same decisions, every image is recognised by the platform's own tools, shipping twice gives the same bytes |
| `ccrun` | `unisacc` compiles C and the reference VM runs it |
| `selfhost` | `unisacc` built two ways agrees with the Python front end |
| `bootstrap` | `B = C = U`, the self-hosting fixed point |
| `corpus` | [c-testsuite](https://github.com/c-testsuite/c-testsuite) — 220 programs written by other people, for other compilers |

## Layout

```
prd.md            the specification, with numbered clauses [T-*] [D-*] [P-*] ...
prd.tree.md       tree + DAG views, for working
prd.map.md        a memory palace, for the whole picture
unisa/            the Python driver: nets, gold, construction, front end, lowering
src/unisacc_main.c  the compiler, written in the C subset it compiles
kernel/           generated: the model blob + the integer kernel, as C
tests/            acceptance, differential (vs cc), native, self-hosting, bootstrap
```

`prd.md` §6 is a running log of measured findings — including the ones that
refuted our own predictions.

## Where it stands on someone else's code

`tests/corpus.sh` runs c-testsuite, 220 single-file C programs this project had
no hand in writing:

```
corpus 220   pass 193   wrong 0   unsupported 22   knownfail 5   slow 0
```

The programs are compiled to a real image for this host and **executed**, not
interpreted: the reference VM is a Python loop, and an eight-queens search a
real CPU finishes instantly takes a quarter of an hour there. `wrong` is the
only failure — a program that compiled and then disagreed.
`unsupported` is the honest coverage gap (the front end refuses the program);
`tests/corpus.baseline` is a ratchet, so that number may only go down.

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
compiler behind one kernel, with **no fallback path anywhere**, verified by
enumeration over the whole domain.

## Status

Research artifact. The C99 subset is real but partial; `tests/corpus.sh` says
how partial, and `tests/difftest.sh` says how much of it agrees with the system
compiler.

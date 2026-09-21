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

**All six targets have really run.** `--fold` compares six lowerings
inside one interpreter, which catches ABI and syscall-number mistakes but not
encoder bugs: the interpreter models no page protection, no real `rsp` and no
two-operand ALU. `osx/arm64`, `osx/x86_64` (Rosetta), `lnx/x86_64` and
`lnx/arm64` are executed on real kernels, every probe, every time
`win/arm64` and `win/x86_64` are executed on a real Windows 11 machine
(`tests/crossnative.sh`, which skips them when the VM is not up). What kept
Windows out for so long was one header field: declaring
`MajorSubsystemVersion` 10.0 puts the loader on a strict path that demands
relocations of a DYNAMIC_BASE image; at 4.0 a position-independent image with
no `.reloc` and no load config loads fine. The second bug was that the tape
stack pointer is a volatile register under Win64, so the calls that fetch the
standard handles had to move ahead of stack setup. Both, and the sixty rounds
of dissection that went the wrong way first, are in `prd.md` §6 E-35.

**Several files, one program, no linker.** `unisa compile a.c b.c -o x`
walks both units with one walker, so a call in the first reaches a definition
in the last the same way it reaches one further down its own file — calls were
already resolved at the end of the walk, not at the call site. There is no
object format and no link step. File-scope `static` gets a per-file name so
two units may keep their own `helper`; a single-file compile keeps the empty
suffix, so its tape is byte-for-byte what it always was. What the units do not
yet get is separate scope: a typedef from an earlier file is still visible in a
later one.

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

That is a fixed point, not a coverage claim: `unisacc.c` only has to accept the
subset `unisacc.c` is written in, and it trails the Python front end badly —
**31 of our 73 probes, 111 of the corpus's 220**, against 209 for the Python
one. It also stops at the tape; lowering and the images are still Python. So
the honest reading is that the C compiler reproduces itself, not that it could
replace the driver. `tests/selfgap.sh` ratchets those two numbers so the gap
can only shrink.

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
| `selfgap` | how much C `unisacc`'s own front end still refuses that the Python one accepts — a ratchet, because that gap was growing unmeasured |
| `multi` | two translation units compiled into one program, against `cc a.c b.c` |
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
corpus 220   pass 209   wrong 0   unsupported 0   knownfail 11   slow 0
```

The programs are compiled to a real image for this host and **executed**, not
interpreted: the reference VM is a Python loop, and an eight-queens search a
real CPU finishes instantly takes a quarter of an hour there. `wrong` is the
only failure — a program that compiled and then disagreed.
`unsupported` is the honest coverage gap — the front end refuses the
program — and it is now **zero**, so the next one to appear is a real alarm
rather than another entry in an old backlog. `knownfail` is the list we are
deliberately not chasing: floating point, GCC extensions, `_Generic`, and one
place where this subset evaluates `int` arithmetic at 64 bits on purpose.
`tests/corpus.baseline` is a ratchet, so `pass` may only go up.

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

Research artifact. Four layers, because the hard part and the big part are not
the same part — `prd.md` §5.5 keeps the full audit.

| | done when | today |
|---|---|---|
| **the claim** | every table-shaped decision is a net, `acc = 1.000` by enumeration | **there** — 11 stages, 4,560 keys, no fallback path |
| **the targets** | six images, real machines, identical behaviour | **there** — `fat` is multi-arch within one OS; a tri-format single file is not started |
| **the language** | someone else's C compiles, or is refused for a written reason | **there for this corpus** — 209/220, `unsupported 0`; floating point is a whole missing axis |
| **the product** | compiles ordinary C99 tools; `unisacc` builds its own executable | **half** — see the self-hosting gap above |

Nothing here is blocked on a question we cannot answer: `[P-8]` proves the
weights exist for any finite table, `[F-5]` says accuracy below 1.000 is a
key-encoding bug, and `[D-7]` says a net/gold disagreement is a defect rather
than variance. There is no randomness at inference, so **no shortfall anywhere
in this project has a statistical excuse**. What is left is work, and each
piece of it lands as a ratchet that may only go up.

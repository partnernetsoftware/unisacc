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
| `parse` | NT × TOK | 295 | 1,288 |
| `type` | t1 × op × t2 | 1,216 | 801 |
| `scope` | ctx × kind | 30 | 479 |
| `irsel` | family × flavor | 165 | 953 |
| `enc` | op × os × arch | 258 | 829 |
| `reloc` | jmpkind × arch | 6 | 161 |
| `isel` | op × arch | 86 | 2,328 |
| `abi` | op × os × arch | 258 | 4,361 |

Ten decision points, one kernel: `embed → gemv → ReLU → gemv → argmax`.
Swap the weights, change the capability. The kernel never changes.

## What makes it unusual

**Exactness is constructed, not searched.** The weights are *derived* from the
gold decision tables, not trained: `W1 ∈ {0,1}`, `b1 ∈ {0,−1,−2}` (and not
stored — it is recoverable), `W2 ∈ {1,2,4,8,16}`. The hidden activation is
always 0 or 1, so there is **no multiply, no shift, no float anywhere**, and an
**int8 accumulator suffices** (max logit 19).

**Verification is exhaustive, not statistical.** Each stage is a total function
on a finite closed domain, so `∀k ∈ K_s : argmax(N_s(k)) = G_s(k)` is decided
by enumeration — 2,300 keys, zero disagreements. Not a test: a decision
procedure.

**It self-hosts.** `unisacc.c` carries the model (a 10,514-byte blob) and the
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

`tests/all.sh` is the entry point; each suite's verdict is its exit status.

| suite | what it checks |
|---|---|
| `acceptance` | the spec's own checklist |
| `vm` | the tape interpreter, on hand-written fixtures |
| `difftest` | unisa vs the system compiler — the only instrument that can see a defect in the reference tables |
| `native` | emitted images actually executed on this host |
| `artifacts` | the shipped kit is *usable*: weight blob round-trips to the same decisions, every image is recognised by the platform's own tools, shipping twice gives the same bytes |
| `ccrun` | `unisacc` compiles C and the reference VM runs it |
| `selfhost` | `unisacc` built two ways agrees with the Python front end |
| `bootstrap` | `B = C = U`, the self-hosting fixed point |

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

## Status

Research artifact. The C99 subset is real but partial; see `tests/difftest.sh`
for exactly how much of it agrees with the system compiler.

# iterate/construct: the weight constructor in C (J10 step 2, first slice)

`construct.c` reads one gold table, `weights/gold/<stage>.tsv`, and prints the
net that `unisa/construct.py` `build_net` builds from it. It uses only the C
subset unisacc compiles, and it is standalone. It is not part of the compiler.

    construct [-d] weights/gold/prec.tsv
    python3 iterate/construct/tools/netdump.py [-d] weights/gold/prec.tsv

Both print the same canonical text. It holds the fields of `intnet.to_dict` in
that function's order: `stage`, `H`, `offs`, `b1`, the head with its classes,
`feeds` (one `f c:` line per coordinate), `w2` (one `r j: class,weight ...` line
per unit), `maxlogit`, and then `exact <keys>`. With `-d`, both sides first
print the intermediate steps, so a divergence shows up at the first step where
it occurs:

1. `groups i:`: the quotient groups of field i (value indices).
2. `rule r: cube => class rank l`: the decision list after REDUCE, with its
   winner-vs-loser ranks.
3. `kind` and `unit j: cube`: the chosen representation and its units, in order.

`check.sh [ua]` runs the whole comparison. Run it from the repo root. Every step
has an alarm.

## What is proven (checked byte for byte, 2026-09-25, at 8d4011c)

| stage | cc -O2 build | unisacc -O2 build (osx/arm64) |
|---|---|---|
| prec  | identical to netdump.py, with `-d` (855 B) | identical (855 B) |
| reloc | identical to netdump.py, with `-d` (348 B) | identical (348 B) |

- Semantics: `construct` runs the integer kernel over every key of the full
  original domain (19 keys for prec, 6 for reloc). For each key it requires a
  unique argmax equal to the TSV label, and it exits 1 otherwise.
  `exact <keys>` is printed only when that check passes.
- The reader enforces the contract of `tsvgold.load_stage`. It checks
  schema-before-header, the exact header, column counts, keys inside their
  fields, labels inside their head's classes, no repeated key, full cover of
  the product, and unique names, values and classes. `check.sh` damages
  prec.tsv in five ways (missing row, duplicate row, bad label, extra column,
  bad key). Both builds reject every copy with exit 1.

## What is not proven

- The other 16 stages. They are not attempted. Multi-head stages need the T4
  cross-head sharing and the multi-head `pick`, and neither is ported: the
  constructor refuses any stage with `nh != 1`. The capacity is at most 3
  fields and at most 62 quotient keys, because each key set is one `long`
  bitmask.
- The UNS2 bytes. The canonical dump lists everything `uns2.dump` reads, but
  no UNS2 writer exists in C yet, and this slice does not compare against
  `weights/built.uns2`.
- The factored path (T5, `rep_factored`). It is ported and it runs for reloc,
  but neither stage selects a factored representation (both are `dlist`).
  The dumps show only the chosen representation, so the factored candidates
  themselves are not compared.
- The semantic check alone is weak evidence. While the unisacc build produced
  a different `b1` (see below; since fixed), that net still passed
  it; only the byte comparison with Python caught the difference.  Three
  things are recorded separately: the nets' outputs agree on the whole
  domain; the constructed representation is identical byte for byte; and
  the verifier is trusted only as far as the compiler that built it.
- Deployment semantics.  The C verifier computes `hv * W2`, while the
  deployed IntNet adds a unit's W2 once when its activation is > 0.  They
  agree only if every hidden activation is 0 or 1 and `b1 = 1 - (number of
  constrained fields)`.  These construction invariants are NOT yet checked
  explicitly; before a UNS2 writer, check them over the whole domain and
  check the encode/decode round trip against the deployed arithmetic.
- Capacity.  A key set is one `long`, so a stage may have at most 62
  quotient keys.  This limits which stages can be ported independently of
  the multi-head work; the list of stages over the limit is not made yet.
- Beyond the contract: Python rejects a file that is not valid UTF-8, and the
  C reader does not check the encoding.

## unisacc problems met on the way

1. **Confirmed and fixed (2976a43).** A 3-D array indexed by variables
   crashed (SIGSEGV at -O0 and -O2): `long a[4][5][3]; int i=2,j=3;
   a[i][j][1]=7;`.  The index expression reset the inner dimension.
   Regression probe: `tests/c/a_arr3v.c`.  The candidate store here is
   still flat 2-D (`row = ci * MAXU + u`); that is harmless and stays.
2. **Confirmed and fixed.**  `b1[H] = -(nlits(ucube[H]) - 1);` built by
   unisacc gave the wrong b1 (7 where Python gives 0 on prec).  The first
   minimal attempt missed it because its argument was not a pointer: a
   call's result kept the pointer-ness the LAST ARGUMENT left behind
   (`ucube[H]` is a row, so a pointer), and `- 1` was scaled by 8.
   Pointer-returning calls were wrong the other way (`*(ip() + 2)` stepped
   by bytes).  Fixed in the C front end (`callptr`); probe
   `tests/c/a_callres.c`; construct.c is back to the single expression and
   still byte-identical to Python on prec and reloc.

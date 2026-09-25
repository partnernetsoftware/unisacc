# iterate/construct: the weight constructor in C (J10 step 2, first slice)

`construct.c` reads one gold table, `weights/gold/<stage>.tsv`, and prints the
net that `unisa/construct.py` `build_net` builds from it. It uses only the C
subset unisacc compiles, and it is standalone. It is not part of the compiler.

    construct [-d] weights/gold/prec.tsv
    python3 iterate/construct/tools/netdump.py [-d] weights/gold/prec.tsv
    construct -u out.uns2 weights/gold/prec.tsv weights/gold/reloc.tsv
    python3 iterate/construct/tools/uns2slice.py ref.uns2 weights/gold/prec.tsv weights/gold/reloc.tsv
    python3 iterate/construct/tools/uns2slice.py --shipped out.uns2 weights/built.uns2
    python3 iterate/construct/tools/uns2round.py out.uns2 weights/gold/prec.tsv weights/gold/reloc.tsv

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

## What is proven (2026-09-25; `check.sh`, cc -O2 and unisacc -O2 osx/arm64)

Four separate ledgers.  One passing does not imply another.

1. **Canonical-dump identity.**  `construct -d` equals `netdump.py -d`,
   byte for byte: prec 855 B, reloc 348 B, for both builds.
2. **UNS2 byte identity.**  `construct -u out.uns2 prec.tsv reloc.tsv`
   writes the UNS2 blob of just those stages (16 B header, sections in
   sorted name order, as `uns2.dump` does).  Two comparisons:
   - the whole blob (146 B) against `tools/uns2slice.py`, which writes
     `uns2.dump` of the Python nets built by
     `intnet.build(stages=tsvgold.load_all(...))`: raw bytes identical, both
     builds;
   - each stage section against the section of the same name in the shipped
     `weights/built.uns2`, located by walking the format's length fields:
     prec 83 B and reloc 47 B identical, both builds.  The 16 B header is
     not compared with the shipped one (nStages and nUnits differ by
     construction).
3. **Deployed-semantics round trip.**  `tools/uns2round.py` decodes the
   C-written blob with the deployment loader, `uns2.load` (which derives b1
   from W1 rather than reading it), and runs `IntNet.predict`, the deployed
   arithmetic (a unit adds its W2 once when `hit + b1 > 0`), on every key of
   the original domain from the TSV (19 prec, 6 reloc) and every head.  Each
   answer equals the TSV label, and the logits (summed by predict's rule)
   have a unique maximum.  Both builds.
   Before any of that, `construct` itself checks the two **deployment
   invariants** over the full original domain: every hidden activation is
   0 or 1, and `b1 = 1 - (fields the unit's W1 touches)`.  A violation exits
   3 with `deployment invariant broken`.  (No input in this slice violates
   them, so that exit path has not been exercised.)
4. **Trust caveat.**  Ledgers 1 and 2 compare against Python and do not
   depend on the C verifier.  The C verifier and invariant checks in the
   unisacc build are trusted only as far as unisacc is: an earlier unisacc
   miscompilation of `b1` passed the C semantic check and was caught only by
   the byte comparison.  Ledger 3 runs in Python, so it does not share that
   caveat.

The reader enforces the contract of `tsvgold.load_stage`: schema before
header, the exact header, column counts, keys inside their fields, labels
inside their head's classes, no repeated key, full cover of the product, and
unique names, values and classes.  `check.sh` damages prec.tsv in five ways
(missing row, duplicate row, bad label, extra column, bad key), and both
builds reject every copy with exit 1 and the matching diagnostic.

## What is not proven

- The other 16 stages. They are not attempted. Multi-head stages need the T4
  cross-head sharing and the multi-head `pick`, and neither is ported: the
  constructor refuses any stage with `nh != 1`. The capacity is at most 3
  fields and at most 62 quotient keys, because each key set is one `long`
  bitmask.
- UNS2 for any stage other than prec and reloc, and the full
  `built.uns2` (header over all 18 stages).
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

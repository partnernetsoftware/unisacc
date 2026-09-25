# iterate/construct: the weight constructor in C (J10 step 2)

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
   byte for byte: prec 855 B, reloc 348 B, tyinfo 2657 B, regmap 1097 B, for both builds.
2. **UNS2 byte identity.**  `construct -u out.uns2 prec.tsv reloc.tsv`
   writes the UNS2 blob of just those stages (16 B header, sections in
   sorted name order, as `uns2.dump` does).  Two comparisons:
   - the whole blob (146 B) against `tools/uns2slice.py`, which writes
     `uns2.dump` of the Python nets built by
     `intnet.build(stages=tsvgold.load_all(...))`: raw bytes identical, both
     builds;
   - each stage section against the section of the same name in the shipped
     `weights/built.uns2`, located by walking the format's length fields:
     prec 83 B and reloc 47 B identical, both builds.
   tyinfo is checked the same way in a blob of its own: `construct -u
   out.uns2 tyinfo.tsv` is 104 B, raw-byte identical to `uns2slice.py`,
   and its 88 B section is identical to the shipped one, both builds.
   regmap likewise: `construct -u out.uns2 regmap.tsv` is 99 B, raw-byte
   identical, and its 83 B section equals the shipped one, both builds.  The 16 B header is
     not compared with the shipped one (nStages and nUnits differ by
     construction).
3. **Deployed-semantics round trip.**  `tools/uns2round.py` decodes the
   C-written blob with the deployment loader, `uns2.load` (which derives b1
   from W1 rather than reading it), and runs `IntNet.predict`, the deployed
   arithmetic (a unit adds its W2 once when `hit + b1 > 0`), on every key of
   the original domain from the TSV (19 prec, 6 reloc, 16 tyinfo x 3
   heads, 16 regmap) and every head.  Each
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

## Acceptance batch: pp, lex, scope, pfconv, binsel (2026-09-25)

Existing constructor, no change to construct.c.  `check.sh` covers them;
each is a single-stage UNS2 pack (MAXS is 8, so no 9-stage pack is claimed).
Both builds, all five items pass:

| stage | -d dump | UNS2 blob | shipped section | round trip | trace |
|---|---|---|---|---|---|
| pp | 589 B | 74 B | 58 B | 18 keys | dlist 6 units; T5 1 partition, 12 calls, **12 None** |
| lex | 1086 B | 109 B | 93 B | 144 keys | dlist 11 units; T5 1 partition, 18 calls, **18 None** |
| scope | 942 B | 84 B | 68 B | 30 keys | dlist 9 units; T5 1 partition, 18 calls, **18 None** |
| pfconv | 555 B | 73 B | 57 B | 9 keys | dlist 7 units; one field, T5 not run |
| binsel | 1755 B | 157 B | 141 B | 32 keys | factored cand 1, 18 units; 18 calls, 0 None, all 18 kept, **18-way tie at 18 units** (first index wins) |

New branches versus prec/reloc/tyinfo/regmap: `rep_factored` returning None
(pp, lex, scope), and a tie among many equal-length factored candidates
(binsel; regmap tied only two).  Still not exercised: a candidate dropped as
not shorter, more than one partition, 3 fields, anything multi-head beyond
tyinfo.  Patch units are not visible in the trace.  Invariant negatives
(`-T bias`, `-T act`) fire on all five, both builds.

## What is not proven

- The other 14 stages.  They are not attempted.  The capacity is at most 3
  fields, 4 heads and 62 quotient keys, because each key set is one `long`
  bitmask.
- UNS2 for any stage other than prec, reloc, tyinfo and regmap, and the full
  `built.uns2` (header over all 18 stages).
- The multi-head algorithm in general.  It is ported whole, but tyinfo (one
  field, 16 values, 10 quotient keys, 3 heads) reaches only part of it; see
  the next section.
- The factored path (T5, `rep_factored`) beyond what regmap reaches (next
  section but one).  The -d dump shows only the chosen representation, so
  the candidates that were not chosen are compared with Python only by
  their lengths, once, by hand (a wrapper around `construct.rep_factored`
  gave the same 18 lengths as the C trace).
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

## Multi-head: what tyinfo exercises, and what it does not

`construct -t weights/gold/tyinfo.tsv` prints the branch trace (check.sh
prints it for both builds and requires them to agree).  The -d dump shows
the same path step by step: every decision list T4 rebuilds (`dl <head>
pool <n>`; a rule that is a pool cube ends in ` pool`) and every selection
(`select`, `cands`, `pick <start>`, `chosen`).  netdump.py prints the Python
side of that by wrapping `construct.decision_list` and the module's `min`
for one build_net call; construct.py itself is unchanged.  The counts of
REDUCE alignments and pool tie-breaks come from C only; they were checked
once against an instrumented copy of the Python decision_list (4 and 0).

Exercised by tyinfo:

- The per-head decision lists (size 4 rules, uns 2, narrow 2) and their
  winner-vs-loser ranks.
- Every head picks candidate 0, the decision list.  No head has a factored
  candidate: tyinfo has one field, `partitions_of(1)` is empty, so T5 never
  runs.
- `pick` runs from all 4 starts (all-dlist, and one start per head, which
  equals all-dlist because each head has one candidate) and moves nothing.
- T4 runs all 3 rounds.  All 9 rebuilt lists are accepted (none is longer);
  3 of them differ from the old list (size, each round).  REDUCE is aligned
  to a pool cube 4 times; the pool tie-break flag never changes which rule
  is chosen.  The selection's unit count oscillates 8 -> 6 -> 8 -> 6, and
  the build ends on 6.
- Merging: the chosen candidates have 8 units in total; the net has H 6,
  so 2 cubes are shared across heads (both narrow units are size cubes).
- One W1/b1 layer shared by all heads, W2 per head, UNS2 with 3 head blocks.

NOT exercised by tyinfo (ported, not checked by any stage):

- Factored candidates of a multi-head stage (T5 with more than one field),
  and so: pick choosing a factored candidate, pick moving any head, a
  per-head shortest start differing from all-dlist, a head of kind
  `factored`, T4 appending factored candidates after an accepted list.
- A T4 list rejected because it is longer than the old one.
- T4 ending early because no list was accepted (`if not improved: break`).
- The pool flag in the decision-list score breaking a coverage tie.
- A tie between two starts in the final selection (first minimum wins).
- More than one field, and 4 heads (the declared maximum).

The multi-head algorithm is therefore verified only on this one path.

## Factored (T5): what regmap exercises, and what it does not

regmap (2 fields of 8 and 2 values, 1 head, 16 classes, 16 keys, quotient
16) is the first stage with more than one field.  `construct -t
weights/gold/regmap.tsv`, identical for both builds:

    trace head y: 19 candidates, chose 1 (factored), 10 units
    trace head y: candidate units 16 10 11 11 12 12 13 13 14 14 10 11 11 12 12 13 13 14 14
    trace T5 partitions 1, rep_factored calls 18, None 0, not shorter than dlist 0, kept 18
    trace selections 1, starts whose pick moved a head 1
    trace T4 rounds 0, ...

Exercised by regmap:

- `partitions_of(2)`: one partition, {0} | {1}.
- 18 `rep_factored` calls (1 partition x merge 0/1 x k = 0..8); every one
  returns a representation, and every one (10..14 units) is shorter than
  the 16-unit dlist, so all 18 are kept.  Exceptions k = 1..8 (the
  high-rank dlist rules kept before factoring) are therefore built.
- Selection chooses a factored candidate: candidate 1 (merge 0, k 0, 10
  units: 8 treg units + 2 arch units).  `pick` moves the head from the
  dlist start to it; candidate 10 (merge 1, k 0) also has 10 units and
  loses the tie to the earlier index.
- A net of kind `factored`: b1 all 0 (each unit constrains one field),
  maxlogit 2 (the gold class is the only one reachable from both parts).

NOT exercised by regmap:

- `rep_factored` returning None (no call did), and a candidate dropped as
  not shorter than the dlist (none was).
- Residual collision patch units: the dump's units are only the 10
  partition units, so the chosen candidate has none; whether any
  unchosen candidate built patch units is not shown by the trace.
- Anything multi-head: T4 does not run (one head), no cross-head merging.
- More than one partition, and 3 fields.

## Deployment invariants: negative coverage

`construct -T bias <tsv>` breaks b1 of unit 0 and must exit 3 on invariant 2
(`b1 = 1 - constrained fields`), which is checked BEFORE the semantic
enumeration so a bias fault is not reported as "not exact".  `-T act` breaks
b1 the same way but skips invariant 2, and must exit 3 on invariant 1
(activation in {0,1}).  The INPUT is one-hot per field and W1 is 0/1, so
each field contributes at most 1 to a unit (a unit may accept several values
of a field; only one of them is present in a key).  Hence activation = b1 +
fields hit <= 1 - t + t = 1: invariant 1 follows from invariant 2, and only a
broken b1 can reach it -- which is why the second case has to bypass the
first.  That bypass only shows the activation check CAN fire; the normal
construction path does not produce such a state.
check.sh runs both on prec, tyinfo and regmap, on the cc and the unisacc build, and counts only exit 3
with that invariant's diagnostic.  `-T` is a test entry only.

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

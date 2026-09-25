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

## enc: raw value capacity lifted (2026-09-25)

enc has 3 fields of 73, 3 and 2 raw values, 1 head, 5 classes, 438 keys and
8 quotient keys.  Only MAXV (raw values per field, was 62) blocked it.

Audit, before the change: every MAXV use and every bit/shift operation, by
the index that drives it.

| where | what | driven by |
|---|---|---|
| `val[MAXF][MAXV]` | value names | raw value |
| reader `np - 2 > MAXV` | the bound on the above | raw value count |
| `grp[MAXF][MAXV]` | value -> group (an int, not a mask) | raw value |
| `gfirst[MAXF][MAXV]` | group -> first value (int) | group (sized by MAXV, ng <= nv) |
| `qmask[MAXF][MAXV]` | group -> keys, `long` | row: group; bits: quotient key |
| `ALL = bit(nq)-1`, `qmask |= bit(j)`, `labmask`, `rem >> j`, `resid >> j`, `m[u] >> j`, `cubemask` result | key sets | quotient key |
| `FULL = bit(ng)-1`, cube `c[i] >> g`, `bit(seed)`, `bit(v)` in expand, `sup |= bit(g)`, `sets = bit(qk)`, `bit(codedigit)`, `w1`: `ucube >> grp[i][v]` | cubes | quotient group (w1 maps a raw value to its group first) |
| `gcls |= bit(qlab)`, `rankset`, `s >> c`, `wmask >> i`, `bit(crank)`, `cmpset` | class sets | class / class rank |
| `bit(lv)`, `bit(k+1-i)`, `bit(k+2)` (`lv <= 60` checked) | W2 weights | rank |
| `popc`, `bitlen` | counts | value being counted |
| `out16/out32 >>`, `tput`, `bacc <<` | UNS2 bytes/bits | byte/bit position; W1 bits are written one per raw value (`tput(w1(i,v,j), 1)`), not as a mask |

No path shifts by a raw value index; no raw-indexed 64-bit mask exists.
Every group bitset is bounded by the field-group check in `domain()`:
ng[i] <= 62 for every field, checked before any `bit(ng)` or `bit(g)`
(exit 4).  (It used to follow from ng[i] <= nq <= MAXQ = 62; since MAXQ is
1024 that no longer holds, see "UB audit" below.)

Change: `MAXV 62 -> 128` (storage only).  MAXQ, MAXR, MAXC, MAXH, MAXS,
rank bound and every other stage untouched.  The quotient check now exits
**4** (new code, capacity) with `capacity: <n> quotient keys so far, more
than 62 (one long bitmask)`, instead of the generic exit 1.

Results, both builds (cc -O2, unisacc -O2 osx/arm64): -d 1510 B identical;
UNS2 115 B raw-identical to uns2slice.py; section 99 B identical to the
shipped one; round trip 438 keys, unique argmax = TSV label; invariants
hold; -T bias and -T act exit 3.

Trace (`-t`, identical on both builds): 1 candidate, chose 0 (dlist), 5
units; T5 **4 partitions** (first stage with more than one), 40
rep_factored calls, **40 None**, 0 not shorter, 0 kept.  First time: 3
fields, more than one partition.  Still not exercised: a candidate dropped
as not shorter.

Capacity negative: check.sh builds a table from a temp copy of regmap.tsv
(fields of 9 and 7 values, label class[(7a+b) % 16], no two slices equal,
so 63 quotient keys).  Both builds exit exactly 4 with the capacity
diagnostic; check.sh accepts nothing else.

## What is not proven

- The other 6 stages (type, parse, irsel, isel, abi, combo).  They are
  not attempted; each is over some limit (prd.md J10 limit matrix).
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
- Capacity.  A quotient-key set is QW = 17 longs of 62 bits, so a
  stage may have at most MAXQ = 1024 quotient keys; MAXR (62 rules per
  decision list), MAXC, MAXH, MAXOK and MAXS are unchanged.  prd.md J10
  keeps the limit matrix.
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

## opinfo (acceptance, no code change)

With MAXV at 128, opinfo (1 field of 67 values, 3 heads, 16 classes, 18
quotient keys) is within every capacity.  The existing constructor, unchanged,
gives on both the cc and the unisacc build: canonical dump 6,428 B identical;
single-stage UNS2 273 B identical to uns2.dump, section 257 B identical to
weights/built.uns2; 67 keys x 3 heads deployed round trip, unique argmax equal
to the TSV; invariants hold, -T bias / -T act fire (exit 3).
Branches: every head has one candidate (dlist; one field, no T5); pick never
moves a head (4 selections); T4 runs 3 rounds, accepts 9 lists of which 6
change, rejects none; REDUCE aligns to a pool cube 12 times, the pool flag
never changes a choice; 26 candidate units share down to H = 16 (10 merged).
New against tyinfo: more changed lists and more sharing; still no T4
rejection, early stop, or factored multi-head candidate.

## peep: quotient-key sets become multi-word bitsets (2026-09-25)

peep: 3 fields of 8, 17 and 12 values, 1 head, 10 classes, 1632 original
keys, 6 x 8 x 11 = 528 quotient keys.  Only the quotient-key width (MAXQ 62,
one `long`) blocked it.

### Pre-audit (before any code change)

(a) The Python reference on peep (`build_net`, observed from outside by
wrapping `decision_list`, `rep_from_dl`, `rep_factored` and
`_head_failures`; construct.py unchanged):

| quantity | peep | C limit | fits |
|---|---|---|---|
| decision-list rules | 18 (one list; one head, no T4) | MAXR 62 | yes |
| maximum rank | 3 | 60 (`a rank does not fit a long`) | yes |
| candidates | 1 (dlist); 72 rep_factored calls (4 partitions x merge 2 x k 0..8), all None | MAXCAND 96 (a slot is only consumed when kept) | yes |
| chosen units / candidate units | 18 / 18 | MAXU 128 | yes |
| transient units inside rep_factored | `_head_failures` is never reached (every call returns None before it); the bound before the cap check is k + group values of the parts <= 8 + 6 + 8 x 11 = 102 | MAXU 128 | yes |
| partitions | 4 (`partitions_of(3)`) | 4 slots | yes |
| fields / heads / classes | 3 / 1 / 10 | MAXF 3 / MAXH 4 / MAXC 32 | yes |
| raw values per field | 8 / 17 / 12 | MAXV 128 | yes |
| original keys | 1632 | MAXOK 4096 | yes |
| TSV size | 42,191 B, 1638 lines | BUFSZ 262,144 | yes |
| quotient keys | 528 | MAXQ 62 | **no -- the only blocker** |

Group sets per field (6, 8, 11 groups), class sets (10) and rank weights
(2^3) all stay within one `long`.

(b) Every quotient-key set (converted):

| where | what |
|---|---|
| `qmask[i][g]` | field i, group g -> keys having it (now row `i * MAXV + g` of `long [MAXF*MAXV][QW]`: flat, no 3-D array) |
| `ALL` | the domain, `bit(nq) - 1` before |
| `cubemask` result | keys in a cube; now written into a caller buffer; `cmo` scratch |
| `expand`: `cur[]`, `others`, `nm`, `badmask`, return | now globals `xcur`, `xoth`, `xnm`; result into a buffer |
| `decision_list`: `labmask[MAXC]`, `rem`, `bad`, `cm`, `bcm`, `newly`; `popc(cm & rem)`; `qmask & newly` in REDUCE | now globals of QW words |
| `ranks`: `m` (the rule's keys, to fill `fire`) | `rkm` |
| `headfail`: `m[MAXU]` | global `hfm[MAXU][QW]` |
| `rep_factored`: `covered`, `resid` | `fcov`, `fres`, `ftmp` |

MAXQ-sized arrays that are NOT key sets (indexed by a key, a group value
or a bucket, stored as ints or class sets; resized, not converted):
`qk`, `qlab`, `fire`, `nfire`, `gcode`, `gcls` and `bcls` (class sets per
group value / bucket), `bn`, `bmem` (group-value codes per bucket),
`border`, `badj`.

Not converted (other meanings): cubes `c[i]`, `FULL`, `bit(seed)`,
`sup`, `sets`, `pr`, `bit(codedigit)`, `w1` (group sets within a field,
at most 11 groups here); `gcls`, `bcls`, `rankset`, `wmask`, `cmpset`,
`crank` (class sets); `bit(lv)`, `bit(k+1-i)`, `bit(k+2)` (rank weights);
`tput`/`out*` (UNS2 bits).

(c) Static memory, bytes (long 8, int 4), MAXQ 62 -> 1024, QW 16:

| array | before | after |
|---|---|---|
| `bmem[MAXQ][MAXQ]` int | 15,376 | 4,194,304 |
| `fire[MAXQ][MAXR]` int | 15,376 | 253,952 |
| `qmask` | 3,072 | 49,152 |
| `qk[MAXQ][MAXF]`, `qlab[MAXH][MAXQ]` | 744 + 992 | 12,288 + 16,384 |
| `nfire`, `gcode`, `bn`, `border` (int) | 4 x 248 | 4 x 4,096 |
| `gcls`, `bcls` (long) | 2 x 496 | 2 x 8,192 |
| `ALL` | 8 | 128 |
| `labmask` (was 256 B on the stack), `hfm` (was `m`, 1,024 B on the stack) | -- | 4,096 + 16,384 |
| set scratch (`xcur`, `rem`..., `rkm`, `fcov`..., `cmo`, self-test) | -- | 2,304 |
| `badj` (stack) | 248 | 4,096 |

Measured on the cc -O2 build: `__common` (zerofill) 4,659,224 ->
9,181,848 B, +4,522,624 B; `bmem` is 93 % of it.  All of it is bss.

### Width

MAXQ = 1024, QB = 62 key bits per word, QW = ceil(1024 / 62) = 17 words
(1,054 bits).  (It was 16 words of 64 bits; see "UB audit" below.)  It
covers 528 with a 1.9x margin.  Every in-matrix stage except type (3,150
quotient keys, also over MAXOK) is <= 414 quotient keys.  Only
`nqw = (nq + QB - 1) / QB` words are read or written, so the small stages
loop over one word.

Operations: `qzero qcopy qset qtest qand qor qandnot qempty qeq qmeets
qmeets3 qpopc qpopcand qnext`.  `qandnot` cuts the result with `ALL`
(`qtail`), and ALL has exactly nq bits, so no complement or difference
yields a key >= nq.  Iteration (`qnext`) is ascending, the order of Python's
`for j in range(D.n)` with a bit test; every loop that iterated `j < nq`
with `(x >> j) & 1` now iterates `qnext` or tests `qtest` in the same
order.

`construct -Q` is the word-boundary self-test: for nq 1, 61, 62, 63, 123,
124, 125, 528, 1023 and 1024 it checks ALL (no bit >= nq, bits 62 and 63
clear in every word), set/test, popcount, ascending iteration, complement
(disjoint, covering, tail cut even from an all-ones operand, bits 62/63
clear afterwards) and equality at keys 0, QB-1 = 61, QB = 62,
2QB-1 = 123, 2QB = 124 and nq - 1.  check.sh
runs it on both builds (exit 5 on a failure).

### Results (both builds: cc -O2, unisacc -O2 osx/arm64)

- `-d` dump identical to `netdump.py -d`: 2,727 B.
- single-stage UNS2 178 B raw-identical to `uns2slice.py`; section 162 B
  identical to peep's in `weights/built.uns2`.
- deployed round trip: 1632 keys, unique argmax = TSV label.
- invariants hold; `-T bias` (unit 0 has b1 0, expected -1) and `-T act`
  (unit 0 activation 2 on key 204) exit 3.
- all 11 earlier stages still identical (dumps, blobs, sections, round
  trips, traces unchanged); reader, capacity and invariant negatives green.
- timings: construct on peep 0.01 s (cc), 0.03 s (unisacc); whole
  check.sh 5 s.

Trace (`-t`, identical on both builds): 1 candidate, chose 0 (dlist), 18
units; T5 4 partitions, 72 rep_factored calls, **72 None**, 0 not shorter,
0 kept; no T4 (one head).  No branch fires for the first time: peep's
path is enc's path with a larger domain.  Still not exercised: a candidate
dropped as not shorter, patch units.

### Capacity gate

- **Positive** (was the negative): the synthetic 9 x 7 table with 63
  quotient keys now builds and verifies over its full domain.  Its label
  had to change: `class[(7a+b) % 16]` needs 63 decision-list rules, over
  MAXR 62 -- a separate limit, found by this positive and not widened.
  The new label `class[a]` when b = 0, else `class[8 + b]`, still keeps
  every slice distinct (9 + 7 singleton groups, 63 quotient keys) and needs
  15 rules.  Both builds: `-d` identical to netdump.py on the same TSV
  (1,215 B), UNS2 111 B identical to uns2slice.py, deployed round trip 63
  keys unique argmax.
- **Negative**: 11 x 11 x 9 values, label `class[(a + 3b + 5c) % 16]` (no
  shift d <= 10 of one field is 0 mod 16, so nothing merges): 1089 quotient
  keys.  Both builds exit exactly 4 with `capacity: 1089 quotient keys so
  far, more than 1024 (a 16-word bitset)`, from `domain()`, before any
  set is built.

## UB audit: 62-bit quotient words, field-group bound (2026-09-25)

A review of the peep port found two defects; both are fixed, nothing else
changed.

1. `qset` did `((long)1) << 63` (undefined; a UBSan build aborted in
   `-Q`), `popc` does `m - 1` (overflows for LONG_MIN), and `qtest`/`qnext`
   right-shifted words that could be negative (implementation-defined).
2. The field-group bound was lost: group sets are one `long` per field and
   `FULL[i] = bit(ng[i]) - 1`, which was safe while ng <= nq <= 62; with
   MAXQ 1024 and MAXV 128 a field could have up to 128 groups.

**Choice: option (a), words that never touch the sign bit.**  Key j is bit
`j % QB` of word `j / QB`, QB = 62, so every stored word is in [0, 2^62).
That keeps `long` and the existing unisacc code paths (signed shifts,
compares, `/` and `%` of non-negative ints, all already exercised by the
byte-identical stages) instead of depending on unisacc's unsigned 64-bit
shifts and compares, which nothing in this slice tests.  62 rather than 63
so that the quotient words and the field-group sets obey one invariant:
no value ever has bit 62 or 63 set, and the largest shift is `bit(62)`,
only inside `FULL = bit(62) - 1`.

Field groups: `domain()` checks ng[i] <= 62 for every field right after
grouping, before the quotient count and before any shift, and exits 4
with `capacity: field F has N value groups, more than 62`.  The bound is
exact: group bits are 0..ng-1 <= 61, `bit(ng) <= bit(62) = 2^62` is
representable, `FULL = 2^62 - 1 >= 0`; ng = 63 would need `bit(63)`.
Group sets stay one word (not migrated).

| operation | where | operand range | why defined |
|---|---|---|---|
| `1 << (j % QB)` | `qset` | shift 0..61 | < 63, result < 2^62 |
| `a[j / QB] >> (j % QB)` | `qtest`, `qnext` | word in [0, 2^62), shift 0..61 | right shift of a non-negative value |
| `j / QB`, `j % QB`, `(j / QB + 1) * QB` | `qset`, `qtest`, `qnext` | 0 <= j < nq <= 1024 | non-negative int division; <= 1054 |
| `a & b`, `a \| b` | `qand`, `qor`, `qmeets*`, `qtail` | words in [0, 2^62) | bitwise; result in [0, 2^62) |
| `a & ~b` then `& ALL` | `qandnot` (+`qtail`) | `~b` is negative; `a & ~b` with a >= 0 is >= 0 | bitwise ops only; nothing shifts, subtracts or counts it before qtail cuts it to ALL |
| `m - 1`, `m & (m-1)` | `popc` via `qpopc`, `qpopcand` | m in [0, 2^62) (m != 0 in the loop) | m - 1 >= 0, no overflow |
| `nqw = (nq + QB - 1) / QB` | `qsetall` | nq <= 1024 | int, <= 17 = QW |
| self-test `sc[w] = -1` | `-Q` only | all-ones operand to `qandnot` | only `&`, `~`; the result is tail-cut before popc, then checked for bits 62/63 |
| self-test `ALL[w] >> j`, `>> QB` | `-Q` only | ALL[w] in [0, 2^62), shift <= 62 | non-negative right shift |
| `bit(g)`, `bit(v)`, `bit(seed)` | `expand`, REDUCE `sup`, `sets[i]` | g < ng <= 62, so g <= 61 | < 63, value < 2^62 |
| `bit(ng[i]) - 1` | `FULL` in `domain()` | ng <= 62 (checked first) | 2^62 - 1, no overflow |
| `c >> g`, `ucube >> grp` | `cubemask`, `prcube`, `w1`, `expand` | group set in [0, 2^62), g <= 61 | non-negative right shift |
| `d >> p`, `a >> p`, `b >> p` | `cmpset` | group sets in [0, 2^62), p <= 61 (lowest bit of d != 0) | non-negative right shift |
| `bit(codedigit)`, `popc(pr)` | T5 `rep_factored` | digit < ng <= 62 | as `bit(g)` |

Other shifts (`bit(lv)`, `bit(k+2)`, `bit(qlab)`, `bit(crank)`, the UNS2
writers) have their own bounds, listed in the table further up; they were
not changed.  The UBSan run below executes every stage with them.

Gates (check.sh):

- a third build, `cc -O1 -fsanitize=undefined -fno-sanitize-recover=undefined`:
  `-Q` passes all 10 sizes, and all 12 stages' `-d` dump and UNS2 run with
  exit 0, no report, the dump identical to Python and the blob identical to
  the cc build.
- field-group negative: fields a (70 values) and b (2), label
  `class[b ? 8 + a/16 : a%16]` (16 classes; `(label(a,0), label(a,1))` is
  distinct for every a), so 70 groups in a, 140 quotient keys.  cc,
  unisacc and UBSan builds all exit 4 with `capacity: field a has 70 value
  groups, more than 62`.
- unchanged and green: 63-key positive, 1089-key negative (now "a 17-word
  bitset"), reader and invariant negatives, all 12 stages byte-identical on
  both builds (dumps, UNS2, shipped sections, round trips, traces).

## parse: the class-set audit and MAXC 32 -> 62

parse: 2 fields of 5 and 68 raw values, 1 head, 36 classes, 340 keys.
The only limit it crossed was MAXC 32.  Audit, done BEFORE the change:
every class set and every shift driven by a class or a rank.

| operation | where | index source | max bit |
|---|---|---|---|
| `gcls[x] \|= bit(qlab[ch][j])` | rep_factored | class id | ncl - 1 |
| `bcls[x] = gcls[t]`, `bcls[b] == gcls[t]` | rep_factored | copies/compares a class set | ncl - 1 |
| `rankset`: `(s >> c) & 1` | rankset | class id, c < ncl | shift ncl - 1 |
| `rankset`: `r \|= bit(crank[ch][c])` | rankset | class name rank, a permutation of 0..ncl-1 | ncl - 1 |
| `cmpset(rankset(..), rankset(..))`: `d >> p`, `a >> p`, `b >> p` | bucket sort | lowest differing bit of two rank sets | ncl - 1 |
| `wmask = bcls[b]`, `(wmask >> i) & 1` | rep_factored | class id, i < ncl | shift ncl - 1 |
| `labmask[qlab]`, `cw[..][c]`, `W2[..][c]`, `z[c]`, `zs[c]`, `cls[h][c]`, `crank[h][c]` | everywhere | class id | storage (arrays of MAXC), never a bit index |
| `tput(c, cb)`, `cb = bitlen(ncl - 1)` | UNS2 | class id as a value | 6 bits at 62 classes |
| `bit(lv)` | rep_from_dl, ranks | rank, `lv <= 60` checked | 60 |
| `bit(k + 1 - i)`, `bit(k + 2)` | rep_factored | k < min(nr, 9) | 10 |

The rank-indexed shifts do not depend on the class count: `lv` is bounded by
`ranks()` (`lv > 60` dies), `k` by `factored()` (`k < 9`).  Rank sets are a
separate thing.  cmpset is only ever called on group sets (cubes, ng <= 62)
and rank sets of classes (bits < ncl).

**Bound.**  Every class set is one `long`, and its members are bits c or
crank[c] with 0 <= c, crank[c] < ncl.  With ncl <= 62 the highest bit is
61: no shift touches bit 62 or the sign bit 63, every class set is >= 0, and
cmpset's right shifts see values >= 0.  At ncl = 63 `bit(62)` would still be
defined but the value-bits <= 61 rule would break; at 64 `bit(63)` is
signed-overflow UB.  So MAXC = 62, the same bound and argument as the field
groups; class sets stay single `long`s (not raised to 128).  The reader
checks it on the `#head` line, before `cls[]` is stored and before any class
shift: exit 4, `capacity: head H has N classes, more than 62`.

Other limits for parse against the Python reference (netdump -d, -t):
145 quotient keys <= MAXQ 1024; groups 5 and 29 <= 62; 35 decision-list
rules <= MAXR 62; 34 net units (35/34/34 per candidate) <= MAXU 128;
3 candidates <= MAXCAND 96 (18 rep_factored calls); ranks <= 60 (no die);
1 head <= MAXH 4; 1 section <= MAXS 8.  Nothing else blocks.

Result (cc, unisacc, UBSan builds): -d identical to netdump.py (4305 B);
UNS2 identical to uns2slice.py (559 B), section parse 543 B identical to
weights/built.uns2; deployed round trip 340 keys, unique argmax = TSV; -T
bias and -T act fire; -Q clean under UBSan.  Trace:

    trace head y: 3 candidates, chose 1 (factored), 34 units
    trace head y: candidate units 35 34 34
    trace T5 partitions 1, rep_factored calls 18, None 16, not shorter than dlist 0, kept 2

New negative `capc`: 2 x 2 keys, 63 classes (labels c0/c1 only), every
other limit met; exit 4 with the class diagnostic on cc, unisacc, UBSan.

## type, step 1: bmem flattened at the current capacity (2026-09-25)

`bmem[MAXQ][MAXQ]` (group-value codes per bucket, 4,194,304 B) is replaced
by a member pool `bpool[MAXQ]` with per-bucket `boff`/`bn` (plus `bfill`,
`bof`: 4 x 4,096 B).  Per part, rep_factored counts the members of each
bucket, takes prefix sums, then fills the pool in the original first-seen
order.  `boff`/`bn` are indexed by bucket IDENTITY; `border` only permutes
bucket numbers, so the sort never moves an offset.  Every group value is in
exactly one bucket: total members = ngv <= nq is asserted, and each bucket
is checked to be filled to its count (both `die`, never reached -- uncovered).
In-bucket sort and bucket order are unchanged.

Verified: all 13 stages' `-d`, `-u` and `-t` outputs from cc, unisacc and
UBSan builds are byte-identical to the pre-change build; `check.sh` ok.
Static memory (`size -m`, cc -O2, `__common` zerofill): 12,294,848 B ->
8,100,544 B.  This is static memory, not RSS.

## type, step 2: early rejection inside rep_factored (2026-09-25)

Before each `newunit` of a factor block (base path) and of a patch unit
(patch path), rep_factored returns 0 (None) once the candidate already has
cap = nr units.  Why this is equivalent: Python only ever appends units and
returns None at the first `len(units) >= cap` check; every exit after the
count reaches cap is None, and the only success exit sits behind that check.
So the RESULT is preserved -- None, or the identical valid candidate.  What
is NOT preserved is a rejected candidate's intermediate unit trajectory: C
stops at cap, Python keeps appending first.  The C build no longer reaches
counts such as the 245 / 2,969 units measured in the Python reference on type.

Preconditions nr > 0 and nr <= MAXU are checked on entry (exit 4 with a
capacity diagnostic), so a future capacity change cannot let a newunit hit the
physical MAXU die first.  That branch is never hit (uncovered): nr >= 1
whenever nq >= 1, and MAXR 62 < MAXU 128.  The k-prefix newunit needs no guard
(it runs with cn = i < k <= nr).

`-t` adds `trace T5 early rejections ...: base path B, patch path P`.  On the
13 stages: pp 6/3, reloc 6/0, lex 18/0, scope 18/0, enc 38/0, peep 72/0,
parse 14/0, the rest 0/0.  The patch-path guard is reached (pp).  Every other
output is byte-identical to step 1, including the kept factored candidates of
regmap (18), binsel (18) and parse (2).

## type, step 3: capacity MAXOK 4352, MAXQ 3200 (2026-09-25)

`MAXOK 4096 -> 4352`, `MAXQ 1024 -> 3200`, `QW` is now `(MAXQ + QB - 1) / QB`
(= 52), not a literal.  The raw-key limit is a capacity check at the header,
before `oseen`/`olab` are indexed: exit 4, `capacity: N raw keys so far, more
than 4352` (it was a reader `die`, exit 1).  `-Q` now also covers the end of
the capacity: nq 3161, 3162, 3163, 3199, 3200 and keys at the last word's
boundary ((nqw-1)QB - 1, (nqw-1)QB) -- 13 sizes.  Negatives: quotient keys
15^3 = 3375 > 3200 (raw 3375 <= 4352, groups 15 <= 62, so domain()'s check is
the one reached; exit 4); raw keys 67 x 66 = 4422 > 4352 (exit 4); the
field-group, class-count negatives and the 63-key positive are unchanged.
All pass on cc, unisacc and UBSan.  13 stages' `-d`/`-u`/`-t` are
byte-identical to step 2.  Static memory (`size -m`, `__common`):
8,100,544 B -> 8,953,912 B (static, not RSS).

## type, step 4: acceptance (2026-09-25)

type is in every stage loop of `check.sh`.  `-d` dump 8,933 B, identical to
`netdump.py -d` on cc, unisacc and UBSan; single-stage UNS2 593 B,
raw-byte identical to `uns2slice.py`; its section (577 B) identical to the
one in `weights/built.uns2`; deployed round trip: 4,275 raw keys, unique
argmax equal to the TSV label; invariants hold and `-T bias` / `-T act` fire.
Trace: 4 partitions, 72 rep_factored calls, all None (early rejections: base
path 54, patch path 18); one candidate, the decision list, chosen; 62 rules,
H = 62.  Timings: `netdump.py -d` 7.1 s, `uns2slice.py` 2.6 s, construct -d
cc 0.19 s, unisacc 2.25 s, UBSan 1.39 s; full `check.sh` 31 s.  MAXR 62 is
now exactly used by type (62 rules).

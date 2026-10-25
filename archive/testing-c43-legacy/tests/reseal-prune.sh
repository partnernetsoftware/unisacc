#!/bin/sh
# reseal-prune.sh: regenerate tests/seals/exec-prune (measured leaf seal of the prune cell, stage prune = gen.py prune;
# README in that folder).  Ungated tool (reseal).  Run from a clean repository root (export).  Mirrors
# tests/reseal-opt-e4.sh and tests/reseal-elf.sh: each step is bounded (<= 58 s) and writes one TSV row; nothing in
# the repository is written, OUTDIR/capture is the new seal.
#   usage: tests/reseal-prune.sh PRODUCT OUTDIR      (OUTDIR must not exist; PRODUCT runs tests/streamseal.cx)
#   env:   SHARED_PRUNE=PATH   a product build's shared/prune.net; the sealed network must be byte-equal (fatal)
# Everything upstream of the cell is an INPUT: fixtures are the reference's `-O2 -S -o -` tape (tape.text) of fixed
# corpus sources, recorded as bytes.  The reference output (SELF-RULED choice, recorded in the README) is the Python
# referee unisa.prune.prune_text -- the one exec/prune/check.py qualifies the cell against; the C reference's -S tape
# is pre-prune (src/back_image.c), so no C-side post-prune stream exists.  The referee runs HERE, once, untraced, to
# freeze the expected bytes; the gate (tests/streamseal.cx) replays only the product's prune net against those frozen
# bytes and never runs the referee.  The closure (dependencies.tsv) is the strace read-set of the cell's own chain; any
# pp path or sealed header in it refuses the reseal.
set -u
P=${1:?product}; O=${2:?outdir}
[ -e "$O" ] && { echo "reseal: $O exists" >&2; exit 2; }
mkdir -p "$O/work/trace" "$O/work/cases" "$O/work/corpus" "$O/fixtures" || exit 2
ROOT=$(pwd)
SEAL=tests/seals/exec-prune
TSV="$O/reseal.tsv"
printf 'step\targv\trc\tmonotonic_s\tproduct_sha\tsource_hash\tartifact_sha\n' > "$TSV"
sha() { if [ -d "$1" ]; then (cd "$1" && find . -type f | sort | xargs sha256sum | sha256sum | cut -c1-64); else sha256sum "$1" | cut -c1-64; fi; }
run() { name=$1; art=$2; srcf=$3; shift 3
  t0=$(python3 -c 'import time;print(time.monotonic())')
  "$@" > "$O/$name.log" 2>&1; rc=$?
  t1=$(python3 -c 'import time;print(time.monotonic())')
  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "$name" "$*" "$rc" "$(python3 -c "print(round($t1-$t0,2))")" \
    "$(sha "$P")" "$(sha "$srcf")" "$( [ -e "$art" ] && sha "$art" || echo ABSENT)" >> "$TSV"
  return $rc
}
B="tests/bound 58"
W="$O/work"
# The referee, one untraced python process over a directory: every NAME.SUFFIX in DIR -> NAME.ref.stdout (prune_text of
# the bytes) and NAME.ref.rc (0).  It is the reference, not a dependency: it runs only here, never in the gate.
# One line, so the reseal.tsv argv column stays one row.
REFEREE='import sys, pathlib; sys.dont_write_bytecode = True; root, folder, suffix = sys.argv[1:4]; sys.path.insert(0, root); from unisa.prune import prune_text; ps = sorted(pathlib.Path(folder).glob("*" + suffix)); assert ps, "no referee input"; w = lambda p, out: ((p.parent / (p.name[:-len(suffix)] + ".ref.stdout")).write_bytes(out), (p.parent / (p.name[:-len(suffix)] + ".ref.rc")).write_text("0\n")); [w(p, prune_text(p.read_bytes()) or sys.exit("empty referee output: %s" % p)) for p in ps]; print("referee:", len(ps))'
# The sealed corpus: the same 21 sources as exec-opt-e4 and exec-elf (the 8 examples/ plus every 7th of the rest of
# exec/pipeline/keep-elf.txt, from the 9th); all 21 are accepted by the cell.  The whole 96-source corpus is replayed by
# the `corpus` step but not sealed.
SOURCES="examples/do.c examples/fact.c examples/fib.c examples/hello.c examples/host.c examples/ptr.c examples/struct.c examples/switch.c tests/c/a_args5.c tests/c/a_enum.c tests/c/a_ptr2.c tests/c/a_typedef.c tests/c/b_constkind.c tests/c/b_goto.c tests/c/b_macrofn.c tests/c/b_printfret.c tests/c/b_switch3.c tests/c/b_shift.c exec/parse2/probes/s52.c exec/parse2/probes/s58.c tests/c/b_init6.c"
# 1. the reference compiler (tests/build_ref.sh export), the same reference exec/pipeline/run.py uses; its `-O2 -S` tape
#    is the cell's input (exec/pipeline/image-stages.tsv: prune follows e4)
run build-ref "$W/ua_ref" tests/build_ref.sh $B sh tests/build_ref.sh "$W/ua_ref.c" "$W/ua_ref" || exit 1
# 2. fixtures: tape.text = the reference's -O2 tape of each sealed source
run fixtures "$O/fixtures" tests/reseal-prune.sh $B sh -c 'for s in $1; do n=$(basename "$s" .c); "$2" -O2 -S -o - "$s" > "$3/$n.tape.text" || exit 1; done' fixtures "$SOURCES" "$W/ua_ref" "$O/fixtures" || exit 1
# 3. the cell under strace (the measured chain): executor, delta, table, network, net check.  cwd is ROOT.
ST="strace -ff -yy -s 4096 -e trace=%file -o $W/trace/t"
run cc-run "$W/run" exec/c/run.c $B $ST cc -O2 -w exec/c/run.c -o "$W/run" || exit 1
run gen "$W/prune.json" exec/build/gen.py $B $ST python3 exec/build/gen.py prune "$W/prune.json" || exit 1
run tbl "$W/prune.tbl" exec/c/tbl.py $B $ST python3 exec/c/tbl.py "$W/prune.json" "$W/prune.tbl" || exit 1
run net "$W/prune.net" exec/c/net.py $B $ST python3 exec/c/net.py "$W/prune.tbl" "$W/prune.net" || exit 1
run check-net "$W/prune.net" exec/c/run.c $B $ST "$W/run" --check-net "$W/prune.tbl" "$W/prune.net" || exit 1
# 4. every fixture through the cell (each run traced; stdout/stderr/rc recorded), then the referee's expected bytes per
#    case (UNTRACED: the referee is the reference, not a dependency)
run cases "$W/cases" tests/reseal-prune.sh $B sh -c ': > "$1/cases.tsv"; for f in "$2"/*.tape.text; do n=$(basename "$f" .tape.text); c="$1/cases/$n"; cp "$f" "$c.input"; echo "$n" >> "$1/cases.tsv"; strace -ff -yy -s 4096 -e trace=%file -o "$1/trace/r" "$1/run" "$1/prune.net" "$f" > "$c.stdout" 2> "$c.stderr"; echo $? > "$c.rc"; done' cases "$W" "$O/fixtures" || exit 1
run reference "$W/cases" unisa/prune.py $B python3 -c "$REFEREE" "$ROOT" "$W/cases" .input || exit 1
# 5. the whole keep-elf corpus (96 sources) through the cell, untraced, against the referee: a measurement row per source
#    in OUTDIR/corpus.tsv (name, rc, equal); any inequality or refusal is fatal.  Not sealed.
run corpus-tapes "$W/corpus" exec/pipeline/keep-elf.txt $B sh -c 'for s in $(cat exec/pipeline/keep-elf.txt); do n=$(basename "$s" .c); "$1/ua_ref" -O2 -S -o - "$s" > "$1/corpus/$n.o2" 2>/dev/null || exit 1; done' corpus-tapes "$W" || exit 1
run corpus-referee "$W/corpus" unisa/prune.py $B python3 -c "$REFEREE" "$ROOT" "$W/corpus" .o2 || exit 1
run corpus "$O/corpus.tsv" exec/pipeline/keep-elf.txt $B sh -c ': > "$2"; bad=0; for f in "$1"/corpus/*.o2; do n=$(basename "$f" .o2); "$1/run" "$1/prune.net" "$f" > "$1/corpus/$n.out" 2>/dev/null; rc=$?; cmp -s "$1/corpus/$n.out" "$1/corpus/$n.ref.stdout" && eq=1 || eq=0; printf "%s\t%s\t%s\n" "$n" "$rc" "$eq" >> "$2"; [ "$rc" = 0 ] && [ "$eq" = 1 ] || bad=1; done; [ "$(grep -c . "$2")" = "$(grep -c . exec/pipeline/keep-elf.txt)" ] || bad=1; exit $bad' corpus "$W" "$O/corpus.tsv" || exit 1
# 6. capture: trace files + case records -> the sealed data (model, cases, streams, dependencies, closure.expected); refuses pp paths
mkdir -p "$O/capture" && cp "$ROOT/$SEAL/cell.tsv" "$ROOT/$SEAL/measurement.tsv" "$O/capture/" || exit 1
run capture "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --capture "$W" "$ROOT" "$O/capture" || exit 1
# 7. writer: the repo checker's --write-seal (same checks and replay, then identity.tsv is written; measurement.tsv from SEAL)
run writer "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --write-seal "$O/capture" "$ROOT" || exit 1
# 8. the gate's own checks on the new data
run seal-check "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --check-core "$O/capture" "$ROOT" || exit 1
run negative "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --negative "$O/capture" "$ROOT" || exit 1
# 9. the sealed network against a product build's shared/prune.net (SHARED_PRUNE), byte for byte
if [ -n "${SHARED_PRUNE:-}" ]; then
  run product-prune "$O/capture" "$SHARED_PRUNE" $B cmp "$SHARED_PRUNE" "$O"/capture/model-*.net || exit 1
fi
cat > "$O/capture/README.md" <<'EOF'
# exec-prune: measured leaf seal of the prune cell (stage prune)

Scope: the prune cell alone -- stream prune, input **tape.text** (the -O2 tape), output
**tape.text** (the pruned -O2 tape) (exec/pipeline/image-stages.tsv row prune; formats in
exec/pipeline/formats.md); the delta is `gen.py prune`, the product's shared/prune.net.

**Everything upstream is an input, not a dependency.** Every `NAME.input` is a recorded tape.text
fixture, captured once as the reference's `-O2 -S -o -` output on fixed corpus sources (21 of
exec/pipeline/keep-elf.txt, the same subset as exec-opt-e4 and exec-elf; the list is in
tests/reseal-prune.sh) and frozen here as bytes.  Nothing of the pp cell (exec/pp/,
exec/facts/pp-*.tsv, pp-autoinc, src/front_pp.c, tests/seals/exec-pp-literals) and no sealed
header under include/ (only include/unisa_sha256.h, run.c's hash) is read by this cell:
`dependencies.tsv` is the strace read-set of the cell's own chain, and both the capture and the
checker refuse any `exec/pp/`, `exec/facts/pp-`, `autoinc`, `src/front_pp.c`, `tests/seals/` or
other `include/` path in it.  The exec/facts/parse-*.tsv files in the closure are the tape
token/word/constant facts the prune delta is built from, not pp facts.

Cell (the same tools as exec-opt-e4): `exec/build/gen.py prune DELTA` -> `exec/c/tbl.py` ->
`exec/c/net.py` -> `run --check-net`; executor `exec/c/run.c` built with the host cc;
`run prune.net FIXTURE` (the executor's default step budget suffices for every sealed case).

**Reference (self-ruled choice, recorded here):** the frozen expected output of every case is the
bytes the Python referee `unisa.prune.prune_text` (unisa/prune.py -- the same referee
exec/prune/check.py qualifies the cell against) produced for the fixture at reseal time.  The C
reference compiler has no post-prune stream to compare with: its `-S` tape is the pre-prune tape
(src/back_image.c prunes only on the way to an image), so a C-side `-S` post-prune dump would be
a new product feature, not a seal.  Hence the referee is frozen as bytes: tests/reseal-prune.sh
runs it once, untraced (it is the reference, not a dependency; it appears in no closure row), to
write each case's `ref.stdout`; the gate (tests/streamseal.cx) replays ONLY the product's prune
net on the frozen inputs and requires byte-equality with the frozen `NAME.stdout`, and never
imports or runs the referee.  Every sealed case is accepted and byte-equal to the referee
(`ref-equal=1`); the reseal's `corpus` step replays all 96 keep-elf sources the same way
(recorded in the reseal's corpus.tsv, not sealed: 96 accepted / 0 refused / 96 byte-equal at
the 2026-10-21 reseal).

Sealed subset (recorded choice): 21 of 96 (the 8 examples/ plus every 7th of the rest).  A replay
costs ~0.02 s per case, so the gate bound is not the limit; the subset keeps the sealed bytes at
~3.7 MB (each -O2 tape fixture and its pruned output are ~85 KB) and mirrors exec-opt-e4 and
exec-elf so the three cells freeze the same sources.

Gate (tests/gate.sh job prune-seal, tests/streamseal.cx --gate): frozen bytes (dependencies against
the checkout, closure.expected.tsv inventory, case input/stdout/stderr/model bytes,
identity.tsv), then a **replay**: the executor built from exec/c/run.c runs the sealed
`model-<sha>.net` on every frozen input and must give the frozen stdout, stderr and rc; then
the negatives (omitted rule, changed rule/executor source, changed fixture/record/model each
FAIL; every negative baseline re-check replays too).  Verification only: no stamp, no cache or
reuse grant.

Files: `streams.tsv` (one row per case: verdict, rc, input/stdout/stderr/model with length and
sha256, `ref-rc`, `ref-equal`), `dependencies.tsv` (role, path, length, sha256),
`closure.expected.tsv` (role, path), `measurement.tsv`, `identity.tsv` (core = sha256 of the
sorted dependency+case rows; proof = sha256 over verdict+stdout+stderr shas).

NOT covered: refusal cases (the corpus is all-accept; the checker's refusal path is exercised by
no sealed case; exec/prune/check.py's hand controls -- CR, wide numbers, host slots, oversize --
are not replayed here); -O0/-O1 tapes as inputs (the pipeline prunes the -O2 tape only); the
pp/lex/parse/opt/lower/enc cells and their closures; the driver/package route (`run --bundle`);
the seed-C construction route (seed/gen.c via exec/prune/gen-delta.sh, seed/tbl.c, seed/net.c)
-- the sealed network was compared byte-for-byte with a product build's shared/prune.net at
reseal time (tests/reseal-prune.sh SHARED_PRUNE), not re-derived by the gate; the referee's own
correctness (it is the frozen oracle, qualified separately by exec/prune/check.py).
EOF
echo "reseal-prune: $O (seal: $O/capture)"

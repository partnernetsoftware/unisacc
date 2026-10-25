#!/bin/sh
# reseal-opt-e4.sh: regenerate tests/seals/exec-opt-e4 (measured leaf seal of the opt cell, stage e4 = gen.py opt --o2; README in that folder).
# Ungated tool (reseal).  Run from a clean repository root (export).  Mirrors tests/reseal-lex-e1.sh: each step is
# bounded (<= 58 s) and writes one TSV row; nothing in the repository is written, OUTDIR/capture is the new seal.
#   usage: tests/reseal-opt-e4.sh PRODUCT OUTDIR      (OUTDIR must not exist; PRODUCT runs tests/streamseal.cx)
#   env:   SHARED_E4=PATH   a product build's shared/e4.net; the sealed network must be byte-equal (fatal)
# The front end (pp, lex, parse) is an INPUT: fixtures are the reference's -O0 -S tape on fixed corpus sources, recorded
# once as bytes; the reference output is the same reference's -O2 -S tape.  The closure (dependencies.tsv) is the
# strace read-set of the whole chain; any pp path or sealed header in it refuses the reseal.
set -u
P=${1:?product}; O=${2:?outdir}
[ -e "$O" ] && { echo "reseal: $O exists" >&2; exit 2; }
mkdir -p "$O/work/trace" "$O/work/cases" "$O/fixtures" || exit 2
ROOT=$(pwd)
SEAL=tests/seals/exec-opt-e4
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
# The sealed corpus: 21 sources of exec/pipeline/keep-elf.txt (the 8 examples/ plus every 7th of the rest, from the 9th);
# all 21 are accepted by the cell.  The whole 96-source corpus is replayed by the `corpus` step but not sealed (gate time).
SOURCES="examples/do.c examples/fact.c examples/fib.c examples/hello.c examples/host.c examples/ptr.c examples/struct.c examples/switch.c tests/c/a_args5.c tests/c/a_enum.c tests/c/a_ptr2.c tests/c/a_typedef.c tests/c/b_constkind.c tests/c/b_goto.c tests/c/b_macrofn.c tests/c/b_printfret.c tests/c/b_switch3.c tests/c/b_shift.c exec/parse2/probes/s52.c exec/parse2/probes/s58.c tests/c/b_init6.c"
# 1. the reference (tests/build_ref.sh export), the same reference exec/pipeline/run.py uses for E3 (-S) and the -O2 tape
run build-ref "$W/ua_ref" tests/build_ref.sh $B sh tests/build_ref.sh "$W/ua_ref.c" "$W/ua_ref" || exit 1
# 2. fixtures: tape.text = the reference's -O0 -S tape (exec/pipeline/image-stages.tsv row e4 input)
run fixtures "$O/fixtures" tests/reseal-opt-e4.sh $B sh -c 'for s in $1; do n=$(basename "$s" .c); "$2" -S -o - "$s" > "$3/$n.tape.text" || exit 1; done' fixtures "$SOURCES" "$W/ua_ref" "$O/fixtures" || exit 1
# 3. the cell under strace (the measured chain): executor, delta, table, network, net check.  cwd is ROOT.
ST="strace -ff -yy -s 4096 -e trace=%file -o $W/trace/t"
run cc-run "$W/run" exec/c/run.c $B $ST cc -O2 -w exec/c/run.c -o "$W/run" || exit 1
run gen "$W/e4.json" exec/build/gen.py $B $ST python3 exec/build/gen.py opt "$W/e4.json" --o2 || exit 1
run tbl "$W/e4.tbl" exec/c/tbl.py $B $ST python3 exec/c/tbl.py "$W/e4.json" "$W/e4.tbl" || exit 1
run net "$W/e4.net" exec/c/net.py $B $ST python3 exec/c/net.py "$W/e4.tbl" "$W/e4.net" || exit 1
run check-net "$W/e4.net" exec/c/run.c $B $ST "$W/run" --check-net "$W/e4.tbl" "$W/e4.net" || exit 1
# 4. every fixture through the cell (each run traced; stdout/stderr/rc recorded), then the reference's -O2 tape per source
run cases "$W/cases" tests/reseal-opt-e4.sh $B sh -c ': > "$1/cases.tsv"; for f in "$2"/*.tape.text; do n=$(basename "$f" .tape.text); c="$1/cases/$n"; cp "$f" "$c.input"; echo "$n" >> "$1/cases.tsv"; strace -ff -yy -s 4096 -e trace=%file -o "$1/trace/r" "$1/run" "$1/e4.net" "$f" > "$c.stdout" 2> "$c.stderr"; echo $? > "$c.rc"; done' cases "$W" "$O/fixtures" || exit 1
run reference "$W/cases" tests/reseal-opt-e4.sh $B sh -c 'for s in $2; do n=$(basename "$s" .c); c="$1/cases/$n"; "$1/ua_ref" -O2 -S -o - "$s" > "$c.ref.stdout" 2> /dev/null; echo $? > "$c.ref.rc"; done' reference "$W" "$SOURCES" || exit 1
# 5. the whole keep-elf corpus (96 sources) through the cell, untraced, against the reference's -O2 tape: a measurement
#    row per source in OUTDIR/corpus.tsv (name, rc, equal); any inequality or refusal is fatal.  Not sealed.
run corpus "$O/corpus.tsv" exec/pipeline/keep-elf.txt $B sh -c ': > "$2"; bad=0; for s in $(cat exec/pipeline/keep-elf.txt); do n=$(basename "$s" .c); "$1/ua_ref" -S -o - "$s" > "$1/corpus.in" 2>/dev/null || exit 1; "$1/ua_ref" -O2 -S -o - "$s" > "$1/corpus.ref" 2>/dev/null || exit 1; "$1/run" "$1/e4.net" "$1/corpus.in" > "$1/corpus.out" 2>/dev/null; rc=$?; cmp -s "$1/corpus.out" "$1/corpus.ref" && eq=1 || eq=0; printf "%s\t%s\t%s\n" "$n" "$rc" "$eq" >> "$2"; [ "$rc" = 0 ] && [ "$eq" = 1 ] || bad=1; done; sh tests/cxbin tests/safedel.cx --purge -- "$1/corpus.in" "$1/corpus.ref" "$1/corpus.out" || bad=1; exit $bad' corpus "$W" "$O/corpus.tsv" || exit 1
# 6. capture: trace files + case records -> the sealed data (model, cases, streams, dependencies, closure.expected); refuses pp paths
mkdir -p "$O/capture" && cp "$ROOT/$SEAL/cell.tsv" "$ROOT/$SEAL/measurement.tsv" "$O/capture/" || exit 1
run capture "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --capture "$W" "$ROOT" "$O/capture" || exit 1
# 7. writer: the repo checker's --write-seal (same checks and replay, then identity.tsv is written; measurement.tsv from SEAL)
run writer "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --write-seal "$O/capture" "$ROOT" || exit 1
# 8. the gate's own checks on the new data
run seal-check "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --check-core "$O/capture" "$ROOT" || exit 1
run negative "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --negative "$O/capture" "$ROOT" || exit 1
# 9. the sealed network against a product build's shared/e4.net (SHARED_E4), byte for byte
if [ -n "${SHARED_E4:-}" ]; then
  run product-e4 "$O/capture" "$SHARED_E4" $B cmp "$SHARED_E4" "$O"/capture/model-*.net || exit 1
fi
cat > "$O/capture/README.md" <<'EOF'
# exec-opt-e4: measured leaf seal of the opt cell (stage e4, -O2)

Scope: the opt cell alone -- stream e4, input **tape.text** (the -O0 tape), output **tape.text**
(the -O2 tape) (exec/pipeline/image-stages.tsv row e4; formats in exec/pipeline/formats.md); the
delta is `gen.py opt --o2`, the product's shared/e4.net.

**The front end is an input, not a dependency.** Every `NAME.input` is a recorded tape.text
fixture, captured once as the reference's `-S -o -` output on fixed corpus sources (21 of
exec/pipeline/keep-elf.txt; the list is in tests/reseal-opt-e4.sh) and frozen here as bytes.
Nothing of the pp cell (exec/pp/, exec/facts/pp-*.tsv, pp-autoinc, src/front_pp.c,
tests/seals/exec-pp-literals) and no sealed header under include/ (only include/unisa_sha256.h,
run.c's hash) is read by this cell: `dependencies.tsv` is the strace read-set of the whole
chain, and both the capture and the checker refuse any `exec/pp/`, `exec/facts/pp-`, `autoinc`,
`src/front_pp.c`, `tests/seals/` or other `include/` path in it.

Cell (the same tools as exec-lex-e1): `exec/build/gen.py opt DELTA --o2` -> `exec/c/tbl.py` ->
`exec/c/net.py` -> `run --check-net`; executor `exec/c/run.c` built with the host cc;
`run e4.net FIXTURE` (the executor's default step budget suffices for every sealed case).

Reference: `ua_ref -O2 -S -o - SOURCE` (tests/build_ref.sh export), the reference stream
exec/pipeline/run.py compares the -O2 tape with.  Every sealed case is accepted and byte-equal
to it (`ref-equal=1`); the reseal's `corpus` step replays all 96 keep-elf sources the same way
(recorded in the reseal's corpus.tsv, not sealed: 96 accepted / 0 refused / 96 byte-equal at
the 2026-10-21 reseal).

Gate (tests/gate.sh job opt-e4-seal, tests/streamseal.cx --gate): frozen bytes (dependencies
against the checkout, closure.expected.tsv inventory, case input/stdout/stderr/model bytes,
identity.tsv), then a **replay**: the executor built from exec/c/run.c runs the sealed
`model-<sha>.net` on every frozen input and must give the frozen stdout, stderr and rc; then
the negatives (omitted rule, changed rule/executor source, changed fixture/record/model each
FAIL).  Verification only: no stamp, no cache or reuse grant.

Files: `streams.tsv` (one row per case: verdict, rc, input/stdout/stderr/model with length and
sha256, `ref-rc`, `ref-equal`), `dependencies.tsv` (role, path, length, sha256),
`closure.expected.tsv` (role, path), `measurement.tsv`, `identity.tsv` (core = sha256 of the
sorted dependency+case rows; proof = sha256 over verdict+stdout+stderr shas).

NOT covered: the -O1 delta (`gen.py opt` without --o2, the product's shared/o1.net: a second
cell with the same closure, not sealed here); refusal cases (the corpus is all-accept; the
checker's refusal path is exercised by no sealed case); the pp/lex/parse cells and their
closures; the prune, lower and enc stages; targets other than the host; the driver/package
route (`run --bundle`); the seed-C construction route (seed/gen.c, seed/tbl.c, seed/net.c)
-- the sealed network was compared byte-for-byte with a product build's shared/e4.net at reseal
time (tests/reseal-opt-e4.sh SHARED_E4), not re-derived by the gate.
EOF
echo "reseal-opt-e4: $O (seal: $O/capture)"

#!/bin/sh
# reseal-e3.sh: regenerate tests/seals/exec-e3 (measured leaf seal of the e3 cell: stage e3 = gen.py parse2,
# tokens.typed -> tape.text; README in that folder).  Ungated tool (reseal).  Run from a clean repository root (export).
# Mirrors tests/reseal-lower.sh: each step is bounded (<= 58 s) and writes one TSV row; nothing in the repository is
# written, OUTDIR/capture is the new seal.
#   usage: tests/reseal-e3.sh PRODUCT OUTDIR      (OUTDIR must not exist; PRODUCT runs tests/streamseal.cx)
# The pp and lex cells are INPUTS: fixtures are the sealed lex-e1 accept stdout (tests/seals/exec-lex-e1/<n>.stdout) of
# the four sealed sources, the same bytes as the sealed *.input; the reference output is the reference's -S tape of the
# matching source (tests/seals/exec-e3/measurement.tsv).  The closure (dependencies.tsv) is the strace read-set of the
# traced chain; any pp path or sealed header in it refuses the reseal.
set -u
P=${1:?product}; O=${2:?outdir}
[ -e "$O" ] && { echo "reseal: $O exists" >&2; exit 2; }
mkdir -p "$O/work/trace" "$O/work/cases" "$O/fixtures" || exit 2
ROOT=$(pwd)
SEAL=tests/seals/exec-e3
LEX=tests/seals/exec-lex-e1
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
# the sealed sources (tests/seals/exec-e3/README.md): hello, fib, fact, a_char
SOURCES="examples/hello.c examples/fib.c examples/fact.c tests/c/a_char.c"
# 1. the reference (tests/build_ref.sh export): the -S tape of each source is the cell's reference output
run build-ref "$W/ua_ref" tests/build_ref.sh $B sh tests/build_ref.sh "$W/ua_ref.c" "$W/ua_ref" || exit 1
# 2. the cell under strace (the measured chain): executor, delta, table, network, net check.  cwd is ROOT.
ST="strace -ff -yy -s 4096 -e trace=%file -o $W/trace/t"
run cc-run "$W/run" exec/c/run.c $B $ST cc -O2 -w exec/c/run.c -o "$W/run" || exit 1
run gen "$W/e3.json" exec/build/gen.py $B $ST python3 exec/build/gen.py parse2 "$W/e3.json" || exit 1
run tbl "$W/e3.tbl" exec/c/tbl.py $B $ST python3 exec/c/tbl.py "$W/e3.json" "$W/e3.tbl" || exit 1
run net "$W/e3.net" exec/c/net.py $B $ST python3 exec/c/net.py "$W/e3.tbl" "$W/e3.net" || exit 1
run check-net "$W/e3.net" exec/c/run.c $B $ST "$W/run" --check-net "$W/e3.tbl" "$W/e3.net" || exit 1
# 3. fixtures: tokens.typed = the sealed lex-e1 accept stdout (exec/pipeline/image-stages.tsv row e3 input)
run fixtures "$O/fixtures" "$LEX" $B sh -c 'for s in $1; do n=$(basename "$s" .c); cp "$2/$n.stdout" "$3/$n.tokens.typed" || exit 1; done' fixtures "$SOURCES" "$ROOT/$LEX" "$O/fixtures" || exit 1
# 4. every fixture through the cell (each run traced; stdout/stderr/rc recorded); the reference is the reference's -S tape
run cases "$W/cases" tests/reseal-e3.sh $B sh -c ': > "$1/cases.tsv"; for s in $3; do n=$(basename "$s" .c); f="$2/$n.tokens.typed"; c="$1/cases/$n"; cp "$f" "$c.input"; echo "$n" >> "$1/cases.tsv"; strace -ff -yy -s 4096 -e trace=%file -o "$1/trace/r" "$1/run" "$1/e3.net" "$f" > "$c.stdout" 2> "$c.stderr"; echo $? > "$c.rc"; "$1/ua_ref" -S -o - "$s" > "$c.ref.stdout" 2>/dev/null; echo $? > "$c.ref.rc"; cmp -s "$c.stdout" "$c.ref.stdout" || { echo "cell differs from reference: $n"; exit 1; }; done' cases "$W" "$O/fixtures" "$SOURCES" || exit 1
# 5. the frozen inputs must be the same bytes as before (the fixtures are inputs; a reseal changes the model, not them)
run same-inputs "$W/cases" "$SEAL" $B sh -c 'bad=0; for f in "$1"/*.input; do n=$(basename "$f"); cmp -s "$f" "$2/cases/$n" || { echo "input changed: $n"; bad=1; }; done; exit $bad' same-inputs "$ROOT/$SEAL" "$W" || exit 1
# 6. capture: trace files + case records -> the sealed data (model, cases, streams, dependencies, closure.expected); refuses pp paths
mkdir -p "$O/capture" && cp "$ROOT/$SEAL/cell.tsv" "$ROOT/$SEAL/measurement.tsv" "$ROOT/$SEAL/README.md" "$O/capture/" || exit 1
run capture "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --capture "$W" "$ROOT" "$O/capture" || exit 1
# 7. writer: the repo checker's --write-seal (same checks and replay, then identity.tsv is written; measurement.tsv from SEAL)
run writer "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --write-seal "$O/capture" "$ROOT" || exit 1
# 8. the gate's own checks on the new data
run seal-check "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --check-core "$O/capture" "$ROOT" || exit 1
run negative "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --negative "$O/capture" "$ROOT" || exit 1
echo "reseal-e3: OUTDIR/capture is the new tests/seals/exec-e3 ($(sha "$O/capture"))"

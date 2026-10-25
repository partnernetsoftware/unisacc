#!/bin/sh
# reseal-lower.sh: regenerate tests/seals/exec-lower (measured leaf seal of the lower cell: stage lower = gen.py lower --full,
# tape.text -> target.text; README in that folder).  Ungated tool (reseal).  Run from a clean repository root (export).
# Mirrors tests/reseal-elf.sh: each step is bounded (<= 58 s) and writes one TSV row; nothing in the repository is
# written, OUTDIR/capture is the new seal.
#   usage: tests/reseal-lower.sh PRODUCT OUTDIR      (OUTDIR must not exist; PRODUCT runs tests/streamseal.cx)
# The front end and the opt/prune cells are INPUTS: fixtures are prune.net(the reference's -O2 -S tape) on the four
# sealed sources, recorded once as bytes; the reference output is the cell's own target.text, frozen at capture
# (tests/seals/exec-lower/measurement.tsv: "ref.stdout frozen at capture").  The closure (dependencies.tsv) is the strace
# read-set of the traced chain; any pp path or sealed header in it refuses the reseal.
set -u
P=${1:?product}; O=${2:?outdir}
[ -e "$O" ] && { echo "reseal: $O exists" >&2; exit 2; }
mkdir -p "$O/work/trace" "$O/work/cases" "$O/work/producer" "$O/fixtures" || exit 2
ROOT=$(pwd)
SEAL=tests/seals/exec-lower
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
PR="$W/producer"
# the sealed sources (tests/seals/exec-lower/README.md): hello, fib, fact, a_char
SOURCES="examples/hello.c examples/fib.c examples/fact.c tests/c/a_char.c"
# 1. the reference (tests/build_ref.sh export): the -O2 -S tape producer
run build-ref "$W/ua_ref" tests/build_ref.sh $B sh tests/build_ref.sh "$W/ua_ref.c" "$W/ua_ref" || exit 1
# 2. the cell under strace (the measured chain): executor, delta, table, network, net check.  cwd is ROOT.
ST="strace -ff -yy -s 4096 -e trace=%file -o $W/trace/t"
run cc-run "$W/run" exec/c/run.c $B $ST cc -O2 -w exec/c/run.c -o "$W/run" || exit 1
run gen "$W/lower.json" exec/build/gen.py $B $ST python3 exec/build/gen.py lower "$W/lower.json" --full || exit 1
run tbl "$W/lower.tbl" exec/c/tbl.py $B $ST python3 exec/c/tbl.py "$W/lower.json" "$W/lower.tbl" || exit 1
run net "$W/lower.net" exec/c/net.py $B $ST python3 exec/c/net.py "$W/lower.tbl" "$W/lower.net" || exit 1
run check-net "$W/lower.net" exec/c/run.c $B $ST "$W/run" --check-net "$W/lower.tbl" "$W/lower.net" || exit 1
# 3. fixture producer, UNTRACED (an input of the cell, not a dependency): prune.net
run producer-prune "$PR/prune.net" exec/prune/gen-manifest.tsv $B sh -c 'python3 exec/build/gen.py prune "$1/prune.json" && python3 exec/c/tbl.py "$1/prune.json" "$1/prune.tbl" && python3 exec/c/net.py "$1/prune.tbl" "$1/prune.net"' producer "$PR" || exit 1
# 4. fixtures: tape.text = prune.net(the reference's -O2 -S tape) (exec/pipeline/image-stages.tsv row lower input)
run fixtures "$O/fixtures" tests/reseal-lower.sh $B sh -c 'for s in $1; do n=$(basename "$s" .c); "$2" -O2 -S -o - "$s" > "$4/$n.o2" || exit 1; "$3/run" "$3/producer/prune.net" "$4/$n.o2" > "$5/$n.tape.text" || exit 1; done' fixtures "$SOURCES" "$W/ua_ref" "$W" "$PR" "$O/fixtures" || exit 1
# 5. every fixture through the cell (each run traced; stdout/stderr/rc recorded); the reference is the cell's own output,
#    frozen at capture (measurement.tsv), so ref.stdout/ref.rc mirror the case record
run cases "$W/cases" tests/reseal-lower.sh $B sh -c ': > "$1/cases.tsv"; for f in "$2"/*.tape.text; do n=$(basename "$f" .tape.text); c="$1/cases/$n"; cp "$f" "$c.input"; echo "$n" >> "$1/cases.tsv"; strace -ff -yy -s 4096 -e trace=%file -o "$1/trace/r" "$1/run" "$1/lower.net" "$f" > "$c.stdout" 2> "$c.stderr"; echo $? > "$c.rc"; cp "$c.stdout" "$c.ref.stdout"; cp "$c.rc" "$c.ref.rc"; done' cases "$W" "$O/fixtures" || exit 1
# 6. the frozen inputs must be the same bytes as before (the fixtures are inputs; a reseal changes the model, not them)
run same-inputs "$W/cases" "$SEAL" $B sh -c 'bad=0; for f in "$1"/*.input; do n=$(basename "$f"); cmp -s "$f" "$2/cases/$n" || { echo "input changed: $n"; bad=1; }; done; exit $bad' same-inputs "$ROOT/$SEAL" "$W" || exit 1
# 7. capture: trace files + case records -> the sealed data (model, cases, streams, dependencies, closure.expected); refuses pp paths
mkdir -p "$O/capture" && cp "$ROOT/$SEAL/cell.tsv" "$ROOT/$SEAL/measurement.tsv" "$ROOT/$SEAL/README.md" "$O/capture/" || exit 1
run capture "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --capture "$W" "$ROOT" "$O/capture" || exit 1
# 8. writer: the repo checker's --write-seal (same checks and replay, then identity.tsv is written; measurement.tsv from SEAL)
run writer "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --write-seal "$O/capture" "$ROOT" || exit 1
# 9. the gate's own checks on the new data
run seal-check "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --check-core "$O/capture" "$ROOT" || exit 1
run negative "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --negative "$O/capture" "$ROOT" || exit 1
echo "reseal-lower: OUTDIR/capture is the new tests/seals/exec-lower ($(sha "$O/capture"))"

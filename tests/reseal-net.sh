#!/bin/sh
# reseal-net.sh: regenerate tests/seals/exec-net-NET (measured leaf seal of one constructed net of the classic grid,
# run by the integer kernel over its whole gold domain; README in that folder).  Ungated tool (reseal).  Run from a
# clean repository root (export).  Mirrors tests/reseal-prune.sh: each step is bounded (<= 58 s) and writes one TSV
# row; nothing in the repository is written, OUTDIR/capture is the new seal.
#   usage: tests/reseal-net.sh PRODUCT OUTDIR NET      (OUTDIR must not exist; PRODUCT runs tests/netseal.cx; NET != pp)
# The cell's chain (tests/netseal.cx --produce) copies kernel/unisa_core.c and kernel/weight.NET.inc beside a one-net
# shim and builds the self-test with the host cc; the reference is weights/gold/NET.tsv, from which the cases are
# derived.  The closure (dependencies.tsv) is the strace read-set of that chain; any pp path, the shared model
# (kernel/unisa_model.inc, weights/built.json, ...) or a header under include/ in it refuses the reseal.
set -u
P=${1:?product}; O=${2:?outdir}; N=${3:?net}
[ -e "$O" ] && { echo "reseal: $O exists" >&2; exit 2; }
[ "$N" = pp ] && { echo "reseal: a net cell never seals pp" >&2; exit 2; }
mkdir -p "$O/work/trace" "$O/work/cases" || exit 2
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
D="$O/capture"
# 1. the cell's chain under strace (cwd is the root): kernel copy + shim + cc, then the one-net kernel over the cases
run produce "$W/cases" tests/netseal.cx $B strace -ff -yy -s 4096 -e trace=%file -o "$W/trace/t" sh "$P" -run tests/netseal.cx -- --produce "$W" . "$N" || exit 1
# 2. the read-set and the records -> the new seal; identity written; then the gate's own two checks on it
run capture "$D" tests/netseal.cx $B sh "$P" -run tests/netseal.cx -- --capture "$W" . "$D" || exit 1
run write-seal "$D/identity.tsv" tests/netseal.cx $B sh "$P" -run tests/netseal.cx -- --write-seal "$D" . || exit 1
run check-core "$D" tests/netseal.cx $B sh "$P" -run tests/netseal.cx -- --check-core "$D" . || exit 1
run negative "$D" tests/netseal.cx $B sh "$P" -run tests/netseal.cx -- --negative "$D" . || exit 1
echo "reseal: $D is the new tests/seals/exec-net-$N (README.md is written by hand; review, then copy)"

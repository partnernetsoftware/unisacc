#!/bin/sh
# reseal-pp-literals.sh: regenerate tests/seals/exec-pp-literals (measured leaf seal; README in that folder).
# Ungated tool (reseal).  Run from a clean repository root (export).  Rows match release/tools/model-reseal.py columns.  Each step is bounded (<= 58 s) and writes one TSV row.
#   usage: tests/reseal-pp-literals.sh PRODUCT OUTDIR      (OUTDIR must not exist)
set -u
P=${1:?product}; O=${2:?outdir}
[ -e "$O" ] && { echo "reseal: $O exists" >&2; exit 2; }
mkdir -p "$O/trace" || exit 2
ROOT=$(pwd)
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
# 1. measurement: literalcheck.py under strace (the trace-file prefix gives trace.PID files)
run strace-literalcheck "$O/trace" exec/pp/literalcheck.py \
  tests/bound 58 strace -ff -yy -s10485760 -e trace=%file,read,write,getdents64 -o "$O/trace/trace" python3 exec/pp/literalcheck.py || exit 1
# 2. capture: trace files -> the sealed data (streams, resources, dependencies, identity, measurement)
run capture "$O/capture" tests/ppleafcapture.cx \
  tests/bound 58 sh "$P" -run tests/ppleafcapture.cx -- "$O/trace" "$ROOT" "$O/capture" || exit 1
# 2b. observation files the README keeps out of the repo (not checked; not sealed)
#    (through the one deletion guard's argv CLI, commissar ruling 2026-10-23)
sh "$ROOT/tests/cxbin" "$ROOT/tests/safedel.cx" --purge -- "$O/capture/directories.tsv" "$O/capture/tools.tsv" "$O/capture/queries.tsv" "$O/capture/observation.tsv" || exit 1
# 3. writer: the repo checker's formal subcommand --write-seal (same checks, then measurement.tsv and identity.tsv
#    are written; the values come from the capture, not from a hand-typed number)
run writer "$O/capture" tests/ppleafseal.cx \
  tests/bound 58 sh "$P" -run tests/ppleafseal.cx -- --write-seal "$O/capture" "$ROOT" || exit 1
# 4. seal check with the repository checker on the new data (the same check the gate runs: --check-core DIR ROOT)
run seal-check "$O/capture" tests/ppleafseal.cx \
  tests/bound 58 sh "$P" -run tests/ppleafseal.cx -- --check-core "$O/capture" "$ROOT" || exit 1
echo "reseal-pp-literals: $O"

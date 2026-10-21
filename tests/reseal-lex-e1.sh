#!/bin/sh
# reseal-lex-e1.sh: regenerate tests/seals/exec-lex-e1 (measured leaf seal of the lex cell, stage e1; README in that folder).
# Ungated tool (reseal).  Run from a clean repository root (export).  Mirrors tests/reseal-pp-literals.sh: each step is
# bounded (<= 58 s) and writes one TSV row; nothing in the repository is written, OUTDIR/capture is the new seal.
#   usage: tests/reseal-lex-e1.sh PRODUCT OUTDIR      (OUTDIR must not exist; PRODUCT runs tests/streamseal.cx)
#   env:   SHARED_E1=PATH   a product build's shared/e1.net; the sealed network must be byte-equal (fatal)
# The pp stage is an INPUT: fixtures are the reference's -E output on fixed corpus sources, recorded once as bytes.
# The closure (dependencies.tsv) is the strace read-set of the whole chain; any pp path in it refuses the reseal.
set -u
P=${1:?product}; O=${2:?outdir}
[ -e "$O" ] && { echo "reseal: $O exists" >&2; exit 2; }
mkdir -p "$O/work/trace" "$O/work/cases" "$O/fixtures" || exit 2
ROOT=$(pwd)
SEAL=tests/seals/exec-lex-e1
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
# The fixture corpus: 18 sources (examples/, tests/c/, exec/lex/probes/); 11 accepted, 7 refused by the cell.
SOURCES="examples/hello.c examples/fact.c examples/fib.c tests/c/a_arith.c tests/c/a_bighex.c tests/c/a_char.c tests/c/a_enum.c tests/c/b_ppif.c exec/lex/probes/att_eof.c exec/lex/probes/numpunct.c exec/lex/probes/words.c exec/lex/probes/words2.c exec/lex/probes/ucn.c exec/lex/probes/rej_at.c exec/lex/probes/rej_bq.c exec/lex/probes/rej_dollar.c exec/lex/probes/unterm_ch.c exec/lex/probes/unterm_str.c"
# 1. the reference (tests/build_ref.sh export) and its token dumper (exec/parse/mkdump.sh), the same reference run.py uses for E1
run build-ref "$W/ua_ref" tests/build_ref.sh $B sh tests/build_ref.sh "$W/ua_ref.c" "$W/ua_ref" || exit 1
run mkdump "$W/ua_tdump" exec/parse/mkdump.sh $B sh exec/parse/mkdump.sh "$W/ua_ref.c" "$W/ua_tdump" || exit 1
# 2. fixtures: pp.text = the reference's -E output (pp.text's definition; exec/pipeline/stages.tsv row E2)
run fixtures "$O/fixtures" tests/reseal-lex-e1.sh $B sh -c 'for s in $1; do n=$(basename "$s" .c); "$2" -E "$s" > "$3/$n.pp.text" || exit 1; done' fixtures "$SOURCES" "$W/ua_ref" "$O/fixtures" || exit 1
# 3. the cell under strace (the prototype's measured chain): executor, delta, table, network, net check.  cwd is ROOT.
ST="strace -ff -yy -s 4096 -e trace=%file -o $W/trace/t"
run cc-run "$W/run" exec/c/run.c $B $ST cc -O2 -w exec/c/run.c -o "$W/run" || exit 1
run gen "$W/e1.json" exec/build/gen.py $B $ST python3 exec/build/gen.py lex "$W/e1.json" --typed || exit 1
run tbl "$W/e1.tbl" exec/c/tbl.py $B $ST python3 exec/c/tbl.py "$W/e1.json" "$W/e1.tbl" || exit 1
run net "$W/e1.net" exec/c/net.py $B $ST python3 exec/c/net.py "$W/e1.tbl" "$W/e1.net" || exit 1
run check-net "$W/e1.net" exec/c/run.c $B $ST "$W/run" --check-net "$W/e1.tbl" "$W/e1.net" || exit 1
# 4. every fixture through the cell (each run traced; stdout/stderr/rc recorded), then through the reference dumper
run cases "$W/cases" tests/reseal-lex-e1.sh $B sh -c ': > "$1/cases.tsv"; for f in "$2"/*.pp.text; do n=$(basename "$f" .pp.text); c="$1/cases/$n"; cp "$f" "$c.input"; echo "$n" >> "$1/cases.tsv"; strace -ff -yy -s 4096 -e trace=%file -o "$1/trace/r" "$1/run" "$1/e1.net" "$f" > "$c.stdout" 2> "$c.stderr"; echo $? > "$c.rc"; done' cases "$W" "$O/fixtures" || exit 1
run reference "$W/cases" exec/parse/mkdump.sh $B sh -c 'for f in "$2"/*.pp.text; do n=$(basename "$f" .pp.text); c="$1/cases/$n"; UA_TYPESPELL=1 "$1/ua_tdump" -dump-tokens "$f" -nostdinc > "$c.ref.stdout" 2> /dev/null; echo $? > "$c.ref.rc"; done' reference "$W" "$O/fixtures" || exit 1
# 5. capture: trace files + case records -> the sealed data (model, cases, streams, dependencies, closure.expected); refuses pp paths
mkdir -p "$O/capture" && cp "$ROOT/$SEAL/cell.tsv" "$ROOT/$SEAL/measurement.tsv" "$O/capture/" || exit 1
run capture "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --capture "$W" "$ROOT" "$O/capture" || exit 1
# 6. writer: the repo checker's --write-seal (same checks and replay, then identity.tsv is written; measurement.tsv from SEAL)
run writer "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --write-seal "$O/capture" "$ROOT" || exit 1
# 7. the gate's own checks on the new data
run seal-check "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --check-core "$O/capture" "$ROOT" || exit 1
run negative "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --negative "$O/capture" "$ROOT" || exit 1
# 8. the sealed network against a product build's shared/e1.net (SHARED_E1), byte for byte
if [ -n "${SHARED_E1:-}" ]; then
  run product-e1 "$O/capture" "$SHARED_E1" $B cmp "$SHARED_E1" "$O"/capture/model-*.net || exit 1
fi
cat > "$O/capture/README.md" <<'EOF'
# exec-lex-e1: measured leaf seal of the lex cell (stage e1)

Scope: the lex cell alone -- stream e1, input **pp.text**, output **tokens.typed**
(exec/pipeline/image-stages.tsv row e1; formats in exec/pipeline/formats.md).

**The pp stage is an input, not a dependency.** Every `NAME.input` is a recorded pp.text
fixture, captured once as the reference's `-E` output on fixed corpus sources (examples/,
tests/c/, exec/lex/probes/; the list is in tests/reseal-lex-e1.sh) and frozen here as bytes.
Nothing of the pp cell (exec/pp/, exec/facts/pp-*.tsv, pp-autoinc, src/front_pp.c,
tests/seals/exec-pp-literals, the sealed headers) is read by this cell: `dependencies.tsv`
is the strace read-set of the whole chain, and both the capture and the checker refuse any
`exec/pp/`, `exec/facts/pp-`, `autoinc`, `src/front_pp.c` or `tests/seals/` path in it.

Cell (the same tools as exec-pp-literals): `exec/build/gen.py lex DELTA --typed` ->
`exec/c/tbl.py` -> `exec/c/net.py` -> `run --check-net`; executor `exec/c/run.c` built with
the host cc; `run e1.net FIXTURE` (the USLATTR1 side-car is stripped by run itself).

Reference: `UA_TYPESPELL=1 ua_tdump -dump-tokens FIXTURE -nostdinc` (exec/parse/mkdump.sh on
the tests/build_ref.sh export), the reference exec/pipeline/run.py uses for E1.  Accepted
cases are byte-equal to it (`ref-equal=1`); refusals are `reject: ...` on stderr, empty
stdout, rc 1..125, with the reference also refusing (`ref-rc` nonzero).

Gate (tests/gate.sh job lex-e1-seal, tests/streamseal.cx --gate): frozen bytes (dependencies
against the checkout, closure.expected.tsv inventory, case input/stdout/stderr/model bytes,
identity.tsv), then a **replay**: the executor built from exec/c/run.c runs the sealed
`model-<sha>.net` on every frozen input and must give the frozen stdout, stderr and rc; then
the negatives (omitted rule, changed rule/executor source, changed fixture/record/model each
FAIL).  Verification only: no stamp, no cache or reuse grant.

Files: `streams.tsv` (one row per case: verdict, rc, input/stdout/stderr/model with length and
sha256, `ref-rc`, `ref-equal`), `dependencies.tsv` (role, path, length, sha256),
`closure.expected.tsv` (role, path), `measurement.tsv`, `identity.tsv` (core = sha256 of the
sorted dependency+case rows; proof = sha256 over verdict+stdout+stderr shas).

NOT covered: the pp cell and its closure; locations mode (`--locations`, pp.locations input);
the f1 side-car content (stripped, not compared); the driver/package route (`run --bundle`);
targets other than the host; the seed-C construction route (seed/gen.c, seed/tbl.c, seed/net.c)
-- the sealed network was compared byte-for-byte with a product build's shared/e1.net at reseal
time (tests/reseal-lex-e1.sh SHARED_E1), not re-derived by the gate.
EOF
echo "reseal-lex-e1: $O (seal: $O/capture)"

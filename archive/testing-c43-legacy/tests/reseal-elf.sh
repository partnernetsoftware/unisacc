#!/bin/sh
# reseal-elf.sh: regenerate tests/seals/exec-elf (measured leaf seal of the elf cell, stage elf = gen.py enc --elf, target
# lnx/x86_64; README in that folder).  Ungated tool (reseal).  Run from a clean repository root (export).  Mirrors
# tests/reseal-opt-e4.sh: each step is bounded (<= 58 s) and writes one TSV row; nothing in the repository is written,
# OUTDIR/capture is the new seal.
#   usage: tests/reseal-elf.sh PRODUCT OUTDIR      (OUTDIR must not exist; PRODUCT runs tests/streamseal.cx)
#   env:   SHARED_ELF=PATH   a product build's lnx-x86_64/elf.net; the sealed network must be byte-equal (fatal)
# Everything upstream of the cell is an INPUT: fixtures are target.text streams, produced once by the pipeline's own
# prune.net and lower.net (gen.py prune, gen.py lower --full; both UNTRACED fixture producers, not dependencies) on the
# reference's -O2 -S tape of fixed corpus sources, and recorded as bytes.  The reference output is the same reference's
# complete ELF image (`ua_ref -O2 -b lnx/x86_64 -o`).  The closure (dependencies.tsv) is the strace read-set of the cell's
# own chain; any pp path or sealed header in it refuses the reseal.
set -u
P=${1:?product}; O=${2:?outdir}
[ -e "$O" ] && { echo "reseal: $O exists" >&2; exit 2; }
mkdir -p "$O/work/trace" "$O/work/cases" "$O/work/producer" "$O/fixtures" || exit 2
ROOT=$(pwd)
SEAL=tests/seals/exec-elf
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
# The sealed corpus: the same 21 sources as exec-opt-e4 (the 8 examples/ plus every 7th of the rest of
# exec/pipeline/keep-elf.txt, from the 9th); all 21 are accepted by the cell.  The whole 96-source corpus is replayed by
# the `corpus` step but not sealed (seal size: each target.text fixture is ~100 KB).
SOURCES="examples/do.c examples/fact.c examples/fib.c examples/hello.c examples/host.c examples/ptr.c examples/struct.c examples/switch.c tests/c/a_args5.c tests/c/a_enum.c tests/c/a_ptr2.c tests/c/a_typedef.c tests/c/b_constkind.c tests/c/b_goto.c tests/c/b_macrofn.c tests/c/b_printfret.c tests/c/b_switch3.c tests/c/b_shift.c exec/parse2/probes/s52.c exec/parse2/probes/s58.c tests/c/b_init6.c"
# 1. the reference (tests/build_ref.sh export), the same reference exec/pipeline/run.py uses; `-O2 -S` feeds the fixture
#    producers, `-O2 -b lnx/x86_64 -o` is the image stream the cell is compared with
run build-ref "$W/ua_ref" tests/build_ref.sh $B sh tests/build_ref.sh "$W/ua_ref.c" "$W/ua_ref" || exit 1
# 2. the cell under strace (the measured chain): executor, delta, table, network, net check.  cwd is ROOT.
ST="strace -ff -yy -s 4096 -e trace=%file -o $W/trace/t"
run cc-run "$W/run" exec/c/run.c $B $ST cc -O2 -w exec/c/run.c -o "$W/run" || exit 1
run gen "$W/elf.json" exec/build/gen.py $B $ST python3 exec/build/gen.py enc "$W/elf.json" --elf || exit 1
run tbl "$W/elf.tbl" exec/c/tbl.py $B $ST python3 exec/c/tbl.py "$W/elf.json" "$W/elf.tbl" || exit 1
run net "$W/elf.net" exec/c/net.py $B $ST python3 exec/c/net.py "$W/elf.tbl" "$W/elf.net" || exit 1
run check-net "$W/elf.net" exec/c/run.c $B $ST "$W/run" --check-net "$W/elf.tbl" "$W/elf.net" || exit 1
# 3. fixture producers, UNTRACED (inputs of the cell, not dependencies): prune.net and lower.net (host target lnx/x86_64)
run producer-prune "$PR/prune.net" exec/prune/gen-manifest.tsv $B sh -c 'python3 exec/build/gen.py prune "$1/prune.json" && python3 exec/c/tbl.py "$1/prune.json" "$1/prune.tbl" && python3 exec/c/net.py "$1/prune.tbl" "$1/prune.net"' producer "$PR" || exit 1
run producer-lower "$PR/lower.net" exec/lower/gen-manifest.tsv $B sh -c 'python3 exec/build/gen.py lower "$1/lower.json" --full && python3 exec/c/tbl.py "$1/lower.json" "$1/lower.tbl" && python3 exec/c/net.py "$1/lower.tbl" "$1/lower.net"' producer "$PR" || exit 1
# 4. fixtures: target.text = lower.net(prune.net(the reference's -O2 -S tape)) (exec/pipeline/image-stages.tsv row elf input)
run fixtures "$O/fixtures" tests/reseal-elf.sh $B sh -c 'for s in $1; do n=$(basename "$s" .c); "$2" -O2 -S -o - "$s" > "$4/$n.o2" || exit 1; "$3/run" "$3/producer/prune.net" "$4/$n.o2" > "$4/$n.pruned" || exit 1; "$3/run" "$3/producer/lower.net" "$4/$n.pruned" > "$5/$n.target.text" || exit 1; done' fixtures "$SOURCES" "$W/ua_ref" "$W" "$PR" "$O/fixtures" || exit 1
# 5. every fixture through the cell (each run traced; stdout/stderr/rc recorded), then the reference's ELF image per source
run cases "$W/cases" tests/reseal-elf.sh $B sh -c ': > "$1/cases.tsv"; for f in "$2"/*.target.text; do n=$(basename "$f" .target.text); c="$1/cases/$n"; cp "$f" "$c.input"; echo "$n" >> "$1/cases.tsv"; strace -ff -yy -s 4096 -e trace=%file -o "$1/trace/r" "$1/run" "$1/elf.net" "$f" > "$c.stdout" 2> "$c.stderr"; echo $? > "$c.rc"; done' cases "$W" "$O/fixtures" || exit 1
run reference "$W/cases" tests/reseal-elf.sh $B sh -c 'for s in $2; do n=$(basename "$s" .c); c="$1/cases/$n"; sh tests/cxbin tests/safedel.cx --purge -- "$c.ref.stdout"; "$1/ua_ref" -O2 -b lnx/x86_64 -o "$c.ref.stdout" "$s" > /dev/null 2>&1; echo $? > "$c.ref.rc"; [ -e "$c.ref.stdout" ] || : > "$c.ref.stdout"; done' reference "$W" "$SOURCES" || exit 1
# 6. the whole keep-elf corpus (96 sources) through the producers and the cell, untraced, against the reference's ELF
#    image: a measurement row per source in OUTDIR/corpus.tsv (name, rc, equal); any inequality or refusal is fatal.
#    Not sealed.
run corpus "$O/corpus.tsv" exec/pipeline/keep-elf.txt $B sh -c ': > "$2"; bad=0; for s in $(cat exec/pipeline/keep-elf.txt); do n=$(basename "$s" .c); "$1/ua_ref" -O2 -S -o - "$s" > "$1/corpus.o2" 2>/dev/null || exit 1; "$1/run" "$1/producer/prune.net" "$1/corpus.o2" > "$1/corpus.pruned" 2>/dev/null || exit 1; "$1/run" "$1/producer/lower.net" "$1/corpus.pruned" > "$1/corpus.in" 2>/dev/null || exit 1; sh tests/cxbin tests/safedel.cx --purge -- "${1:?}/corpus.ref"; "$1/ua_ref" -O2 -b lnx/x86_64 -o "$1/corpus.ref" "$s" >/dev/null 2>&1 || exit 1; "$1/run" "$1/elf.net" "$1/corpus.in" > "$1/corpus.out" 2>/dev/null; rc=$?; cmp -s "$1/corpus.out" "$1/corpus.ref" && eq=1 || eq=0; printf "%s\t%s\t%s\n" "$n" "$rc" "$eq" >> "$2"; [ "$rc" = 0 ] && [ "$eq" = 1 ] || bad=1; done; sh tests/cxbin tests/safedel.cx --purge -- "${1:?}/corpus.o2" "${1:?}/corpus.pruned" "${1:?}/corpus.in" "${1:?}/corpus.ref" "${1:?}/corpus.out"; exit $bad' corpus "$W" "$O/corpus.tsv" || exit 1
# 7. capture: trace files + case records -> the sealed data (model, cases, streams, dependencies, closure.expected); refuses pp paths
mkdir -p "$O/capture" && cp "$ROOT/$SEAL/cell.tsv" "$ROOT/$SEAL/measurement.tsv" "$O/capture/" || exit 1
run capture "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --capture "$W" "$ROOT" "$O/capture" || exit 1
# 8. writer: the repo checker's --write-seal (same checks and replay, then identity.tsv is written; measurement.tsv from SEAL)
run writer "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --write-seal "$O/capture" "$ROOT" || exit 1
# 9. the gate's own checks on the new data
run seal-check "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --check-core "$O/capture" "$ROOT" || exit 1
run negative "$O/capture" tests/streamseal.cx $B sh "$P" -run tests/streamseal.cx -- --negative "$O/capture" "$ROOT" || exit 1
# 10. the sealed network against a product build's lnx-x86_64/elf.net (SHARED_ELF), byte for byte
if [ -n "${SHARED_ELF:-}" ]; then
  run product-elf "$O/capture" "$SHARED_ELF" $B cmp "$SHARED_ELF" "$O"/capture/model-*.net || exit 1
fi
cat > "$O/capture/README.md" <<'EOF'
# exec-elf: measured leaf seal of the elf cell (stage elf, target lnx/x86_64)

Scope: the elf cell alone -- stream elf, input **target.text**, output **image.bytes** (the
complete ELF executable) (exec/pipeline/image-stages.tsv row elf; formats in
exec/pipeline/formats.md); the delta is `gen.py enc --elf`, the product's lnx-x86_64/elf.net.

**Everything upstream is an input, not a dependency.** Every `NAME.input` is a recorded
target.text fixture: the reference has no target.text dump, so the fixture is the pipeline's own
`lower.net(prune.net(ua_ref -O2 -S -o - SOURCE))` (prune.net = `gen.py prune`, lower.net =
`gen.py lower --full`, both run UNTRACED as fixture producers by tests/reseal-elf.sh), captured
once on fixed corpus sources (21 of exec/pipeline/keep-elf.txt, the same subset as exec-opt-e4;
the list is in tests/reseal-elf.sh) and frozen here as bytes.  Nothing of the pp cell (exec/pp/,
exec/facts/pp-*.tsv, pp-autoinc, src/front_pp.c, tests/seals/exec-pp-literals) and no sealed
header under include/ (only include/unisa_sha256.h, run.c's hash) is read by this cell:
`dependencies.tsv` is the strace read-set of the cell's own chain, and both the capture and the
checker refuse any `exec/pp/`, `exec/facts/pp-`, `autoinc`, `src/front_pp.c`, `tests/seals/` or
other `include/` path in it.  exec/facts/enc-librarysymbols.tsv and enc-hostbridge-library.tsv
in the closure are library *symbol* facts of the encoder, not the library/header closure.

Cell (the same tools as exec-opt-e4): `exec/build/gen.py enc DELTA --elf` -> `exec/c/tbl.py` ->
`exec/c/net.py` -> `run --check-net`; executor `exec/c/run.c` built with the host cc;
`run elf.net FIXTURE` (the executor's default step budget suffices for every sealed case).

Reference: `ua_ref -O2 -b lnx/x86_64 -o IMAGE SOURCE` (tests/build_ref.sh export), the complete
ELF image the reference writes for the same source.  Every sealed case is accepted and byte-equal
to it (`ref-equal=1`); the reseal's `corpus` step replays all 96 keep-elf sources the same way
(recorded in the reseal's corpus.tsv, not sealed: 96 accepted / 0 refused / 96 byte-equal at
the 2026-10-21 reseal).

Sealed subset (recorded choice): 21 of 96 (the 8 examples/ plus every 7th of the rest).  A replay
costs ~0.01 s per case, so the gate bound is not the limit; the subset keeps the sealed bytes at
~2.6 MB (each target.text fixture is ~100 KB, each image ~25 KB) and mirrors exec-opt-e4's
corpus so the two cells freeze the same sources.

Gate (tests/gate.sh job elf-seal, tests/streamseal.cx --gate): frozen bytes (dependencies against
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
no sealed case; the encoder's exec/enc/neg-*.txt controls are not replayed here); the macho, pe
and arm64 targets (`gen.py enc --macho/--pe`, `gen.py enc/arm`: other deltas with other
closures, not sealed here); the pp/lex/parse/opt/prune/lower cells and their closures (the
producers prune.net and lower.net are inputs, re-derived by the reseal but not by the gate); the
driver/package route (`run --bundle`); the seed-C construction route (seed/gen.c, seed/tbl.c,
seed/net.c) -- the sealed network was compared byte-for-byte with a product build's
lnx-x86_64/elf.net at reseal time (tests/reseal-elf.sh SHARED_ELF), not re-derived by the gate.
EOF
echo "reseal-elf: $O (seal: $O/capture)"

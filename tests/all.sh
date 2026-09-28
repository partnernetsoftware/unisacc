#!/bin/bash
# Every suite, one summary.  Run from the repo root.
#
#   acceptance  the spec's own checklist [A-*]
#   vm          the tape interpreter on hand-written fixtures [TP-4]
#   difftest    unisa vs the system compiler -- the only instrument that can
#               see a gold defect [P-6] [A-17]
#   native      emitted images actually executed on this host [A-18]
#   fat         one file, both macOS architectures, both executed [A-28]
#   crossnative the OTHER targets, executed in local Linux VMs [A-27];
#               skipped when limactl or the VMs are absent
#   artifacts   the shipped kit is usable, not merely well-formed [A-24]
#   ccrun       unisacc compiles C and the answers are compared against
#               the Python front end's, on EVERY probe [A-21].  Being
#               refused is a gap; disagreeing is a defect.
#   selfhost    unisacc built two ways agrees with the Python front end [A-20]
#   multi       several translation units, one program -- against cc [A-30]
#   tools       real library code by other people, with its own test
#               vectors, several files each [A-31]
#   stages      every stage the Python front end asks on a probe, the C
#               front end asks too: no table-shaped decision in code [A-33]
#   selfgap     how much C the two front ends DISAGREE about, as a ratchet:
#               the one number that was missing while the debt grew [A-29]
#   nativeboot  unisacc -b builds unisacc, which rebuilds itself to the same
#               bytes -- the bootstrap with no Python in it [S-6]
#   run         `unisacc -run` compiles a file and RUNS it in memory (no
#               image, no temp file): same stdout and exit as the cc [S-9]
#   cli         the compiler as a TOOL, from a scratch dir: built-in headers,
#               -I, -D, a shebang, argv, exit status [S-11]
#   diag        what the compiler says when the program is wrong: the user's
#               file, the user's line, the column, the line itself [S-12]
#   ape         unisacc.com: one file that is a PE for Windows and a shell
#               script for Unix, with a slice per target inside [S-10]
#   closure     unisacc -b writes the same image bytes as the Python back
#               end, all six targets, and the host image runs right [S-7]
#   c99         one probe per C99 feature, the denominator written from the
#               standard's list of changes rather than from what we support
#               [A-44]
#   warn        the four -Wall warnings against cc -Wall: same lines, same
#               kinds on the probes, and none cc does not give on the corpus
#   fuzz        generated programs, against the system compiler: the
#               combinations nobody combined [A-46]
#   hostile     input the compiler was not expecting -- truncated files,
#               binary rubbish, a thousand nested parentheses, an include
#               cycle: it must exit, and must diagnose what C says is
#               invalid [A-47]
#   bench       what the compiler costs, as a ratchet: the self-compile, a
#               small probe, and the price of emitting unoptimised code
#               [A-45]
#   consts      the numbers lower.py derives and src/back_*.c hardcode
#               are the same numbers [A-43]
#   oracle      every question the model can be asked, through the cache,
#               equals the net's own answer -- both orders [A-49]
#   docs        the stage tables in prd.tree.md, prd.map.md and README.md
#               are the generated ones, not copies that can drift [A-48]
#   gold_audit  the type table against the system compiler, key by key:
#               enumeration proves the net equals the gold, this asks
#               whether the gold is C [A-50]
#   prec_audit  the prec table against the system compiler: every ordered
#               operator pair's grouping, by value (undetermined pairs listed)
#   tyinfo_audit  each scalar type's size, signedness and narrowness
#               against cc (representation-only rows listed as skipped)
#   abi_audit   the syscall numbers against this machine's <sys/syscall.h>;
#               each host checks its own column [A-51]
#   layout      the data layout ENUMERATED, not sampled: every tape of up to
#               three data definitions, with and without a symbol defined
#               twice, both back ends, all six targets [A-41] [P-3]
#   datashape   generated programs whose globals are declared in one order
#               and allocated in another -- shapes the corpus never asks
#               for, because it grew from things people wanted [A-41]
#   bigclosure  the closure on the compiler itself: 707 KB and 1874 data
#               symbols, the only input ever big enough to break the back
#               end while all 90 probes stayed green [A-42]
#   bootstrap   B = C = U, the self-hosting fixed point [A-23]
#   acc         net == gold over the FULL gold, on the CONSTRUCTED weights
#               we ship.  The SGD control arm is not here: `tests/baseline.sh`
#               runs it on purpose, because it is minutes of full-core work
#               and is not on the shipping path.
#   corpus      c-testsuite -- 220 programs we did not write [A-25];
#               skipped unless corpus/ is already present
set -u
# Explicit jobs keep CI and local dispatch on the same coverage plan.
LIST=0; SELECT=()
while [ "$#" -gt 0 ]; do
    case "$1" in
        --list) LIST=1; shift;;
        --suite) [ "$#" -ge 2 ] || exit 2; SELECT+=("$2"); shift 2;;
        *) echo "usage: all.sh [--list] [--suite NAME ...]" >&2; exit 2;;
    esac
done
# Validate all requested names before reference preparation or any dispatch.
if [ "$LIST" -eq 0 ] && [ "${#SELECT[@]}" -gt 0 ]; then
    AVAILABLE=$(bash "$0" --list) || exit 2
    for wanted in "${SELECT[@]}"; do
        found=0
        for n in $AVAILABLE; do [ "$wanted" = "$n" ] && found=1; done
        [ "$found" -eq 1 ] || { echo "unknown/unavailable suite: $wanted" >&2; exit 2; }
    done
fi
PROBES="examples/*.c tests/c/*.c"
# The verdict comes from each suite's EXIT STATUS, not from pattern-matching
# its last line -- a summary line that happens to end differently is not a
# failure, and a suite that dies silently must not read as green.
# No suite may hang the run.  macOS has no `timeout`, so each one gets a
# watchdog; 142 is the timeout and reads as a failure with a clear line.
LIMIT=${SUITE_LIMIT:-60}
case "$LIMIT" in *[!0-9]*|"") echo "SUITE_LIMIT must be 1..60" >&2; exit 2;; esac
[ "$LIMIT" -ge 1 ] && [ "$LIMIT" -le 60 ] || { echo "SUITE_LIMIT must be 1..60" >&2; exit 2; }
# Suites run CONCURRENTLY, JOBS at a time.  Each one writes its output to a
# file of its own; the summary is printed afterwards in the fixed order below.
# The default leaves cores free: this machine has overheated under full load.
JOBS=${JOBS:-2}
case "$JOBS" in *[!0-9]*|"") echo "JOBS must be positive" >&2; exit 2;; esac
[ "$JOBS" -ge 1 ] || { echo "JOBS must be positive" >&2; exit 2; }
W=$(mktemp -d)
cleanup() {
    local rc=$?
    if [ "$rc" -eq 0 ]; then rm -rf "$W";
    else echo "all.sh: preserved logs $W (rc=$rc)" >&2; fi
}
trap cleanup EXIT
APE_STATE=${APE_STATE:-$W/ape}; export APE_STATE
ACCEPT_STATE=${ACCEPT_STATE:-$W/accept}; export ACCEPT_STATE
ORDER=(); PLAN=()
run() {
    local n="$1"; shift
    PLAN+=("$n")
    if [ "$LIST" -eq 1 ]; then printf '%s\n' "$n"; return; fi
    if [ "${#SELECT[@]}" -gt 0 ]; then
        local found=0 wanted
        for wanted in "${SELECT[@]}"; do [ "$wanted" = "$n" ] && found=1; done
        [ "$found" -eq 1 ] || return
    fi
    ORDER+=("$n")
    while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do sleep 0.5; done
    (
        t0=$(date +%s)
        python3 tests/bound.py "$LIMIT" "$@" > "$W/$n.out" 2>&1
        rc=$?
        [ "$rc" -eq 142 ] && echo "TIMED OUT after ${LIMIT}s" >> "$W/$n.out"
        echo "$rc" > "$W/$n.rc"; echo $(( $(date +%s) - t0 )) > "$W/$n.sec"
        printf "finished %s rc=%s elapsed=%ss\n" "$n" "$rc" "$(cat "$W/$n.sec")"
    ) &
}
# Serial, and before anything else: a failing one says why in a known place.
serial() { wait; run "$@"; wait; }
T0=$(date +%s)

# One build of the self-hosted compiler for every suite that uses it.  ccrun
# never built it and ran BEFORE selfhost, so it could test a stale binary.
UA=${UA:-/tmp/ua_ref}; export UA
UA_RUN=${UA_RUN:-$UA}; export UA_RUN
prepare_reference() {
    python3 tests/bound.py 45 bash -c '
        R=$1
        . "$R/tests/lib.sh"
        ua_ready
    ' _ "$PWD" >/dev/null || { echo "build_ref failed"; exit 1; }
}
[ "$LIST" -eq 1 ] || prepare_reference

# acceptance rewrites weights/, which every other suite reads: it goes alone.
for part in $(seq 1 15); do
    # These two original sections contain no checks outside Darwin/arm64.
    # Name that platform exclusion instead of treating an empty shard as pass.
    if { [ "$part" -eq 10 ] || [ "$part" -eq 11 ]; } &&
       [ "$(uname -s)/$(uname -m)" != Darwin/arm64 ]; then
        echo "SKIP acceptance$part: requires Darwin/arm64 (not a pass)" >&2
        continue
    fi
    case "$part" in
        6)
            serial acceptance6-build env ACCEPT_CASE=6-build ./tests/acceptance.sh
            for k in 1 2 3 4; do
                serial "acceptance6-fold$k" env ACCEPT_CASE=6-fold SHARD="$k/4" ./tests/acceptance.sh
            done
            serial acceptance6-fault env ACCEPT_CASE=6-fault ./tests/acceptance.sh
            serial acceptance6-image env ACCEPT_CASE=6-image ./tests/acceptance.sh
            ;;
        10)
            for step in Btape Bimage Ctape Uimage compare; do
                serial "acceptance10-$step" env ACCEPT_CASE="10-$step" ./tests/acceptance.sh
            done
            ;;
        *) serial "acceptance$part" env PART="$part/15" ./tests/acceptance.sh;;
    esac
done
# The full run rechecks after acceptance. A selected job did not rerun
# acceptance and needs only its initial readiness check.
if [ "$LIST" -eq 0 ] && [ "${#SELECT[@]}" -eq 0 ]; then prepare_reference; fi
run vm         ./tests/vm.sh
for shard in 1 2 3 4; do
    run "difftest$shard" env SHARD="$shard/4" ./tests/difftest.sh
done
# File shards are a disjoint partition of the original wildcard inputs.
FILES=(examples/*.c tests/c/*.c)
for shard in 0 1 2 3 4 5 6 7; do
    CHUNK=()
    for ((i=shard; i<${#FILES[@]}; i+=8)); do CHUNK+=("${FILES[$i]}"); done
    [ "${#CHUNK[@]}" -gt 0 ] || { echo "empty probe shard" >&2; exit 2; }
    for suite in native fat ccrun selfhost closure stages; do
        if [ "$suite" = ccrun ]; then
            run "$suite$((shard+1))" env CCRUN_SHARD=1 "./tests/$suite.sh" "${CHUNK[@]}"
        else
            run "$suite$((shard+1))" "./tests/$suite.sh" "${CHUNK[@]}"
        fi
    done
done
run crossnative bash -c "./tests/crossnative.sh $PROBES"
run artifacts  ./tests/artifacts.sh
run run        ./tests/run.sh
run c99        ./tests/c99.sh
run fuzz       ./tests/fuzz.sh
run hostile    ./tests/hostile.sh
run bench      ./tests/bench.sh
run consts     bash -c 'python3 tests/consts_check.py'
run oracle     bash -c '"${UA:-/tmp/ua_ref}" --check-oracle'
run docs       ./tests/docs.sh
run kernel     ./tests/kernel.sh
run tsvbuild   ./tests/tsvbuild.sh
run gold_audit bash -c 'python3 tests/gold_audit.py'
run prec_audit bash -c 'python3 tests/prec_audit.py'
run tyinfo_audit bash -c 'python3 tests/tyinfo_audit.py'
run abi_audit  bash -c 'python3 tests/abi_audit.py'
run layout     ./tests/layout.sh
run datashape  ./tests/datashape.sh
k=0
for target in lnx/x86_64 lnx/arm64 osx/x86_64 osx/arm64 win/x86_64 win/arm64; do
    k=$((k+1)); run "bigclosure$k" ./tests/bigclosure.sh --target "$target"
done
run cli        ./tests/cli.sh
run diag       ./tests/diag.sh
run warn       ./tests/warn.sh
for k in 1 2 3 4 5 6 7 8; do
    run "opt-run$k" env OPT_PART=run SHARD="$k/8" ./tests/opt.sh
done
for k in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16; do
    run "opt-closure$k" env OPT_PART=closure SHARD="$k/16" ./tests/opt.sh
done
run opt-self env OPT_PART=self ./tests/opt.sh
for k in 1 2 3 4; do
    run "optpy-probes$k" env OPTPY_PART=probes SHARD="$k/4" ./tests/optpy.sh
done
run optpy-self env OPTPY_PART=self ./tests/optpy.sh
for shard in 1 2 3 4; do
    run "difftest_o$shard" env SHARD="$shard/4" ./tests/difftest_o.sh
done
run scale      ./tests/scale.sh
k=0
for target in lnx/x86_64 lnx/arm64 osx/x86_64 osx/arm64 win/x86_64; do
    k=$((k+1)); run "ape-prepare$k" ./tests/ape.sh --prepare "$target"
done
serial ape ./tests/ape.sh
run multi      ./tests/multi.sh
if [ -d corpus/crypto-algorithms ]; then
    for k in 1 2 3 4; do run "tools$k" env FETCH=0 SHARD="$k/4" ./tests/tools.sh; done
fi
run selfgap    ./tests/selfgap.sh
# bootstrap needs the host toolchain to turn a tape back into a binary
if [ "$(uname -s)" = "Darwin" ]; then
    run bootstrap ./tests/bootstrap.sh
fi
run nativeboot ./tests/nativeboot.sh
if [ -d corpus/c-testsuite ]; then
    # four shards: each run stays under the 60 s ceiling (AGENTS.md)
    run corpus1 env FETCH=0 SHARD=1/4 ./tests/corpus.sh
    run corpus2 env FETCH=0 SHARD=2/4 ./tests/corpus.sh
    run corpus3 env FETCH=0 SHARD=3/4 ./tests/corpus.sh
    run corpus4 env FETCH=0 SHARD=4/4 ./tests/corpus.sh
fi
run acc        bash -c "python3 -m unisa acc"   # the SHIPPED weights
[ "$LIST" -eq 0 ] || exit 0
if [ "${#SELECT[@]}" -gt 0 ]; then
    for wanted in "${SELECT[@]}"; do
        found=0
        for n in "${PLAN[@]}"; do [ "$wanted" = "$n" ] && found=1; done
        [ "$found" -eq 1 ] || { echo "unknown/unavailable suite: $wanted" >&2; wait; exit 2; }
    done
fi
wait

bad=0
for n in "${ORDER[@]}"; do
    rc=$(cat "$W/$n.rc" 2>/dev/null || echo 1)
    [ "$rc" -eq 0 ] && continue
    # A swallowed failure is useless -- show the suite when it fails.  The
    # tail is not enough: a per-probe suite prints one `ok` line each and the
    # ONE that failed is usually alphabetically early, so it scrolls off.
    printf '\n--- %s failed ---\n' "$n"
    grep -vE '^  ok ' "$W/$n.out" | head -40
    printf '%s\n--- end %s ---\n' "$(tail -20 "$W/$n.out")" "$n"
done
echo "================ summary ================"
for n in "${ORDER[@]}"; do
    rc=$(cat "$W/$n.rc" 2>/dev/null || echo 1)
    if [ "$rc" -eq 0 ]; then m=ok; else m=FAIL; bad=$((bad+1)); fi
    printf "  %-4s %-11s %4ss  %s\n" "$m" "$n" "$(cat "$W/$n.sec" 2>/dev/null)" \
        "$(tail -1 "$W/$n.out" 2>/dev/null)"
done
echo "========================================="
echo "wall $(( $(date +%s) - T0 ))s   (JOBS=$JOBS)"
[ "$bad" -eq 0 ] && echo "all suites green" || echo "$bad suite(s) failing"
[ "$bad" -eq 0 ]

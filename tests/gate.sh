#!/bin/sh
# gate.sh [--com] -- every suite a release needs, side by side, each bounded
# at 60 s (AGENTS.md), one line per suite with its time.  [S-16 T1]
#
#   JOBS=N   suites at once (default 4); each suite also runs PAR jobs inside
#   --com    also run the user-facing suites through the shipped unisacc.com
#
# On macOS the whole gate runs inside Terminal.app (tests/term.sh), where
# freshly written binaries are not held for the first-launch scan.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
if [ "$(uname -s)" = Darwin ] && [ -z "${TERM_SH_INSIDE:-}" ] && [ "${TERM_SH:-1}" != 0 ]; then
    TERM_SH_ALARM=${TERM_SH_ALARM:-900} exec ./tests/term.sh ./tests/gate.sh "$@"
fi
COM=0; [ "${1:-}" = --com ] && COM=1
JOBS=${JOBS:-4}
UA=${UA:-/tmp/ua_ref}; export UA
. "$R/tests/lib.sh"; ua_ready
O=$(mktemp -d); trap 'rm -rf "$O"' EXIT
TC=$(ls tests/c/*.c)
n=0
job() {   # job NAME ENV... -- CMD...: queued, JOBS at a time
    name=$1; shift
    while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do sleep 0.1; done
    n=$((n+1)); f="$O/$(printf %03d $n).$name"
    ( t0=$(date +%s)
      out=$(perl -e 'alarm 60; exec @ARGV' env "$@" 2>&1); rc=$?
      printf '%-14s rc=%-3s %3ss :: %s\n' "$name" "$rc" "$(( $(date +%s)-t0 ))" \
          "$(printf '%s' "$out" | grep -v '^ *$' | tail -1)" > "$f" ) &
}
T0=$(date +%s)
# longest first, so the tail of the run is short
job tools       ./tests/tools.sh
job bigclosure  ./tests/bigclosure.sh
job fat         ./tests/fat.sh examples/*.c tests/c/*.c
job c99         ./tests/c99.sh
for k in 1 2 3 4; do job corpus-$k   SHARD=$k/4 ./tests/corpus.sh; done
for k in 1 2 3 4; do job difftest-$k SHARD=$k/4 ./tests/difftest.sh; done
job closure-ex  ./tests/closure.sh examples/*.c
job closure-c1  ./tests/closure.sh $(echo "$TC" | awk 'NR%4==1')   # four shards: three ran 22 s alone
job closure-c2  ./tests/closure.sh $(echo "$TC" | awk 'NR%4==2')   # but 61 s beside three other suites
job closure-c3  ./tests/closure.sh $(echo "$TC" | awk 'NR%4==3')
job closure-c4  ./tests/closure.sh $(echo "$TC" | awk 'NR%4==0')
job stages      ./tests/stages.sh examples/*.c tests/c/*.c
job exec-chain  env CHAINKEEP=exec/c/keep-chain.txt ./exec/c/chain.sh $(cat exec/c/keep-chain.txt)   # S-17: one C executor, E2/E1/E3
job exec-e3self ./exec/parse2/selfcheck.sh                       # complete current compiler source -> tape
job exec-neg    ./exec/c/neg.sh   # the executors' error paths agree (bad table 2, reject 1)
job exec-e4     env E4STRICT=1 ./exec/opt/check.sh examples/*.c tests/c/*.c   # S-17 E4: -O1 and -O2 as deltas
job exec-e4self env E4STRICT=1 ./exec/opt/check.sh unisacc.c                    # the compiler's own 3.9 MB tape
job exec-e5     ./exec/enc/check.sh                               # S-17 E5, first slice: x86 encoder on a fixture
job exec-arm    ./exec/enc/armcheck.sh                            # ARM64 integer encoder on both runtimes
job exec-elf    ./exec/enc/imagecheck.sh                          # S-17: complete Linux x86 ELF from lowering payload
job exec-armelf ./exec/enc/armimagecheck.sh                      # shared ELF writer, ARM64 machine/byte labels
job exec-sha ./exec/enc/shacheck.sh                             # Mach-O page hashes through ordinary actions
job exec-macharm ./exec/enc/machocheck.sh                      # ARM Mach-O/signature, native execution
job exec-machx86 env ARCH=x86_64 ./exec/enc/machocheck.sh         # x86 Mach-O/signature, native/Rosetta
job exec-sparse ./exec/lower/sparsecheck.sh                      # large zero storage, independent layout assertions
job exec-lowdata ./exec/lower/check.sh                           # raw E4 tape data decoding/layout through delta
job exec-lower ./exec/lower/fullcheck.sh                        # typed full Linux lowering comparison
job exec-armlower ./exec/lower/armcheck.sh                     # ARM ABI, sext/immediate fusion and real tapes
job exec-winlower ./exec/lower/wincheck.sh                     # Windows typed setup, syscall preservation, both architectures
job exec-armwin ./exec/enc/armwincheck.sh                      # Windows ARM setup/gates, real text bytes
job exec-pearm ./exec/enc/pecheck.sh                           # PE sections, relocations, real ARM images
job exec-winself env TARGET=win/arm64 ./exec/pipeline/selfcheck.sh # full source PE, macros and reference bytes
job exec-selfelf ./exec/pipeline/selfcheck.sh                    # current compiler source through six deltas
job exec-armself env TARGET=lnx/arm64 ./exec/pipeline/selfcheck.sh # ARM source-to-ELF, includes target predefines
job exec-macself env TARGET=osx/arm64 ./exec/pipeline/selfcheck.sh # Mach-O source route and native bootstrap
job exec-macxself env TARGET=osx/x86_64 ./exec/pipeline/selfcheck.sh # x86/Rosetta bootstrap
job exec-srcelf ./exec/pipeline/check-elf.sh                     # fixed 68 complete source-to-ELF paths
job difftest_o  ./tests/difftest_o.sh
job warn        ./tests/warn.sh
job nativeboot  ./tests/nativeboot.sh
job staticinit  ./tests/staticinit.sh
job cli         ./tests/cli.sh
job ccparity    ./tests/ccparity.sh
job run         ./tests/run.sh
job multi       ./tests/multi.sh
job diag        ./tests/diag.sh
job hostile     ./tests/hostile.sh
job kernel      ./tests/kernel.sh
job malloc      ./tests/malloc.sh
job docs        ./tests/docs.sh
if [ "$COM" = 1 ]; then
    [ -x unisacc.com ] || { echo "gate: --com needs ./unisacc.com (make com)"; exit 1; }
    for s in cli ccparity run multi diag hostile staticinit; do job com-$s UA="$R/unisacc.com" ./tests/$s.sh; done
    job com-closure UA="$R/unisacc.com" ./tests/closure.sh examples/*.c
fi
wait
cat "$O"/*
bad=$(cat "$O"/* | grep -vc ' rc=0 ')
echo "gate  suites $(ls "$O" | wc -l | tr -d ' ')   failed $bad   $(( $(date +%s)-T0 ))s wall"
[ "$bad" -eq 0 ]

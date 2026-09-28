#!/bin/sh
# gate.sh [--com] --suite NAME [--suite NAME...] -- explicit bounded batches
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
    TERM_SH_ALARM=${TERM_SH_ALARM:-60} exec ./tests/term.sh ./tests/gate.sh "$@"
fi
# Also bound direct/non-macOS calls, including queueing and final collection.
if [ "${GATE_BOUND:-0}" != 1 ]; then
    exec python3 "$R/tests/bound.py" 60 env GATE_BOUND=1 "$0" "$@"
fi
COM=0; LIST=0; SELECT=""
while [ $# -gt 0 ]; do
    case $1 in
        --com) COM=1; shift;;
        --list) LIST=1; shift;;
        --plan) LIST=2; shift;;
        --suite) [ $# -ge 2 ] || { echo 'missing suite name' >&2; exit 2; }
            SELECT="$SELECT $2"; shift 2;;
        *) echo "unknown gate option: $1" >&2; exit 2;;
    esac
done
if [ "$LIST" = 0 ] && [ -z "$SELECT" ]; then
    echo 'gate: select a bounded batch with --suite NAME; use --list [--com] for names' >&2; exit 2
fi
if [ "$LIST" = 0 ]; then
    names=$(TERM_SH_INSIDE=1 "$0" --list $([ "$COM" = 1 ] && echo --com)) || exit 2
    seen=" "
    for selected in $SELECT; do
        case "$seen" in *" $selected "*) echo "duplicate suite: $selected" >&2; exit 2;; esac
        printf '%s\n' "$names" | grep -Fxq -- "$selected" || { echo "unknown suite: $selected" >&2; exit 2; }
        seen="$seen$selected "
    done
fi
if [ "$COM" = 1 ] && [ "$LIST" = 0 ]; then
    python3 "$R/exec/c/provenance.py" check "${MODEL_COM-$R/unisacc.com}" || exit 1
fi
JOBS=${JOBS:-4}
UA=${UA:-/tmp/ua_ref}; export UA
. "$R/tests/lib.sh"; [ "$LIST" != 0 ] || ua_ready
O=$(mktemp -d); trap 'rm -rf "$O"' EXIT
TC=$(ls tests/c/*.c)
n=0
job() {   # job NAME ENV... -- CMD...: queued, JOBS at a time
    name=$1; shift
    if [ "$LIST" = 1 ]; then echo "$name"; return; fi
    if [ "$LIST" = 2 ]; then printf '%s\0' "$name" "$#" "$@"; return; fi
    case " $SELECT " in *" $name "*) ;; *) return;; esac
    while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do sleep 0.1; done
    n=$((n+1)); f="$O/$(printf %03d $n).$name"
    ( t0=$(date +%s)
      out=$(python3 "$R/tests/bound.py" 60 env "$@" 2>&1); rc=$?
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
job exec-chain  env CHAINKEEP=exec/c/keep-chain.txt ./exec/c/chain.sh $(cat exec/c/keep-chain.txt)   # S-17: network inference, E2/E1/E3
job exec-macros python3 ./exec/pp/macrocheck.py
job exec-pp-literals python3 ./exec/pp/literalcheck.py
job exec-pp-pragmas python3 ./exec/pp/pragmacheck.py
job exec-pploc python3 ./exec/pp/locationcheck.py              # source-position metadata from actual preprocessing
job exec-lexpos python3 ./exec/lex/positioncheck.py             # token offsets against reference tpos
job exec-lexloc python3 ./exec/lex/locationcheck.py             # E2/E1 location envelope
job exec-parseloc python3 ./exec/parse2/locationcheck.py         # E3 retained maps and tape parity
job exec-diag python3 ./exec/parse2/diagnosticcheck.py           # actual reference diagnostic rendering
job exec-errors python3 ./exec/parse2/errorcheck.py
job exec-errors-warn python3 ./exec/parse2/errorcheck.py --warnings
job exec-returnwarn python3 ./exec/parse2/returnwarningcheck.py  # first warning kind and FP conditions
job exec-intwarn python3 ./exec/parse2/returnwarningcheck.py --int-conversion
job exec-unusedwarn python3 ./exec/parse2/returnwarningcheck.py --unused
job exec-formatwarn0 python3 ./exec/parse2/returnwarningcheck.py --format --shard 0/2
job exec-formatwarn1 python3 ./exec/parse2/returnwarningcheck.py --format --shard 1/2
job exec-e3self ./exec/parse2/selfcheck.sh                       # complete current compiler source -> tape
job exec-decimal ./exec/parse2/floatconstcheck.sh                # exact literal bits, table/net and host cc
job exec-neg    ./exec/c/neg.sh   # the executors' error paths agree (bad table 2, reject 1)
job exec-e4     env E4STRICT=1 ./exec/opt/check.sh examples/*.c tests/c/*.c   # S-17 E4: -O1 and -O2 as deltas
job exec-e4self env E4STRICT=1 ./exec/opt/check.sh unisacc.c                    # the compiler's own 3.9 MB tape
job exec-prune-0 env PRUNE_PART=0 PRUNE_UA="$UA" python3 ./exec/prune/check.py
job exec-prune-1 env PRUNE_PART=1 PRUNE_UA="$UA" python3 ./exec/prune/check.py
job exec-prune-2 env PRUNE_PART=2 PRUNE_UA="$UA" python3 ./exec/prune/check.py
job exec-prune-3 env PRUNE_PART=3 PRUNE_UA="$UA" python3 ./exec/prune/check.py
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
job exec-x86win ./exec/enc/x86wincheck.sh                      # Deferred Windows x86 setup after relaxation
job exec-armwin ./exec/enc/armwincheck.sh                      # Windows ARM setup/gates, real text bytes
job exec-pex86 env PE_ARCH=x86_64 ./exec/enc/pecheck.sh           # Shared PE writer, x86 text
job exec-winx86self env TARGET=win/x86_64 ./exec/pipeline/selfcheck.sh
job exec-pearm ./exec/enc/pecheck.sh                           # PE sections, relocations, real ARM images
job exec-winself env TARGET=win/arm64 ./exec/pipeline/selfcheck.sh # full source PE, macros and reference bytes
for part in stages chain resources; do job exec-native-$part env NATIVE_PART=$part ./exec/c/nativecheck.sh; done
job exec-net python3 ./exec/c/netcheck.py
job exec-codec ./exec/c/codeccheck.sh
job exec-package ./exec/c/packagecheck.sh
job modelbenchcheck python3 ./tests/modelbenchcheck.py       # performance evidence fails on functional errors
job bound python3 ./tests/boundcheck.py
job qprefix python3 ./exec/c/qprefixcheck.py --artifact "${MODEL_COM:-./unisacc.com}"
if [ "$(uname -s)" = Darwin ]; then
    job ffi-bridge python3 ./exec/ffi/bridgecheck.py
    job ffi-product python3 ./exec/ffi/productcheck.py
fi
job apps-structure python3 ./tests/appsstructurecheck.py
job strconvert-host python3 ./tests/strconvertcheck.py
job pptruth python3 ./tests/pptruthcheck.py
for route in seed classic model; do job strconvert-$route python3 ./tests/strconvertroute.py "$route"; done
job package-footer python3 ./tests/packagefootercheck.py
job ape-version python3 ./tests/apeversioncheck.py
job proc-enum python3 ./tests/procenumcheck.py
job libneed ./tests/libneed.sh
job apps-real python3 ./tests/appsrealcheck.py             # real snapshots, no fabricated application defaults
job exec-core ./exec/c/corecheck.sh     # isolated generic kernel, external linkage and ISA byte ledger
job exec-asm ./exec/c/asmcheck.sh       # complete assembly execution kernel
if [ "$(uname -s)" = Darwin ]; then
    job exec-asmx86 env CORE_ASM_ARCH=x86_64 ./exec/c/asmcheck.sh
    job exec-bindarm env CORE_ASM_ARCH=arm64 ./exec/c/asm/bindingcheck.sh
    job exec-bindx86 env CORE_ASM_ARCH=x86_64 ./exec/c/asm/bindingcheck.sh
    job exec-container ./exec/c/containercheck.sh
fi
job exec-embedded python3 ./exec/c/embeddedcheck.py
for part in modes contracts dependencies; do job exec-driver-core-$part ./exec/c/compilercheck.sh core-$part; done
job exec-driver-resources ./exec/c/compilercheck.sh resources
for shard in 1 2; do job exec-driver-language-$shard ./exec/c/compilercheck.sh language-$shard; done
for driver in cc asm; do job exec-warningdriver-$driver ./exec/c/warningcheck.sh "$driver"; done
for flag in Wall Wextra Werror; do job exec-warningdriver-ua-$flag ./exec/c/warningcheck.sh ua "$flag"; done
job exec-multiwarn ./exec/c/multiwarningcheck.sh
job exec-unitlocations python3 ./exec/parse2/unitlocationcheck.py
for kind in cc ua asm; do
    for pair in m n; do
        for order in forward reverse; do job exec-multi-$kind-$pair-$order env DRIVER_KIND=$kind MULTI_PART=tapes-$pair-$order ./exec/c/multicheck.sh; done
    done
    for part in isolation static; do job exec-multi-$kind-$part env DRIVER_KIND=$kind MULTI_PART=$part ./exec/c/multicheck.sh; done
done
job exec-multi-cc-frame env DRIVER_KIND=cc MULTI_PART=frame ./exec/c/multicheck.sh
for kind in cc ua asm; do for shard in 1 2 3; do job exec-memory-$kind-$shard env DRIVER_KIND=$kind MEMORY_SHARD=$shard/3 ./exec/c/memorycheck.sh; done; done
if [ "$(uname -s)/$(uname -m)" = Darwin/arm64 ]; then
    for kind in cc ua asm; do for shard in 1 2 3; do job exec-memx86-$kind-$shard env MEMORY_ARCH=x86_64 DRIVER_KIND=$kind MEMORY_SHARD=$shard/3 ./exec/c/memorycheck.sh; done; done
fi
job exec-memwinarm ./exec/c/winmemorycheck.sh arm64
job exec-memwinx86 ./exec/c/winmemorycheck.sh x86_64
job exec-tableself env NETWORK=0 TARGET=osx/arm64 ./exec/pipeline/selfcheck.sh
job exec-selfelf ./exec/pipeline/selfcheck.sh                    # current compiler source through seven deltas
job exec-armself env TARGET=lnx/arm64 ./exec/pipeline/selfcheck.sh # ARM source-to-ELF, includes target predefines
job exec-macself env TARGET=osx/arm64 ./exec/pipeline/selfcheck.sh # Mach-O source route and native bootstrap
job exec-macxself env TARGET=osx/x86_64 ./exec/pipeline/selfcheck.sh # x86/Rosetta bootstrap
job exec-srcelf ./exec/pipeline/check-elf.sh                     # fixed 68 complete source-to-ELF paths
for k in 1 2 3 4; do job difftest_o-$k SHARD=$k/4 ./tests/difftest_o.sh; done
job warn        ./tests/warn.sh
job diag-units  ./tests/diagunits.sh
job nativeboot  ./tests/nativeboot.sh
job staticinit  ./tests/staticinit.sh
job tagforward ./tests/tagforward.sh
job parserbounds ./tests/parserbounds.sh
job formatonce ./tests/formatonce.sh
job staticunits ./tests/staticunits.sh
job cli         ./tests/cli.sh
job ccparity    ./tests/ccparity.sh
job run         ./tests/run.sh
job multi       ./tests/multi.sh
job diag        ./tests/diag.sh
job hostile     ./tests/hostile.sh
job source-layout ./tests/source_layout.sh
job target-package python3 ./tests/targetpackagecheck.py "${MODEL_COM:-./unisacc.com}"
job lib-context python3 ./tests/libunisacccheck.py --package "${MODEL_COM:-./unisacc.com}"
job lib-format python3 ./tests/libunisaccformatcheck.py
job lib-stack-arm python3 ./exec/c/librarystackcheck.py "${MODEL_COM:-./unisacc.com}" arm64
job lib-stack-x86 python3 ./exec/c/librarystackcheck.py "${MODEL_COM:-./unisacc.com}" x86_64
job lib-stack-x86-hostabi python3 ./tests/r10stackx86check.py
job lib-sig2-host python3 ./tests/librarysig2hostcheck.py
job lib-typed-host python3 ./tests/librarynativehostcheck.py
job lib-variadic-host python3 ./tests/librarynativevariadiccheck.py
job lib-callplans-host python3 ./tests/librarycallplanscheck.py
job lib-variadic-resolver-host python3 ./tests/libraryvariadicresolvercheck.py
job lib-variadic-protocol ./tests/modelvariadicprotocolcheck.sh
job lib-callback-graph-host python3 ./tests/librarycallbackgraphcheck.py
job lib-callback-graph-protocol ./tests/modelcallbackgraphcheck.sh
job lib-callback-graph-equality ./tests/modelgraphequalitycheck.sh
job lib-callback-source-graph ./tests/modelcallbacksourcecheck.sh
job lib-callable-mechanism python3 ./tests/librarycallablescheck.py
job lib-carrier-model python3 ./tests/modelnativecarriercheck.py
job lib-bitfield-source python3 ./tests/modelbitfieldsourcecheck.py
job lib-carrier-mechanism python3 ./tests/librarycarriercheck.py
job lib-carrier-plan python3 ./tests/librarycarrierplancheck.py
job lib-carrier-native-plan python3 ./tests/librarycarriernativeplancheck.py
job lib-carrier-callback python3 ./tests/librarycarriercallbackcheck.py
job lib-carrier-import-model python3 ./tests/modelnativecarrierimportcheck.py
job lib-union-native python3 ./tests/libraryunionnativecheck.py --package "${MODEL_COM:-./unisacc.com}" --sanitize
job lib-carrier-factory python3 ./tests/libraryunionnativecheck.py --package "${MODEL_COM:-./unisacc.com}" --sanitize --factory
job lib-carrier-relay python3 ./tests/libraryunionnativecheck.py --package "${MODEL_COM:-./unisacc.com}" --sanitize --relay
job lib-callable-variadic-mechanism python3 ./tests/librarycallablevarcheck.py
job lib-callable-catalog python3 ./tests/librarycallablecatalogcheck.py
job lib-callable-variadic-model ./tests/modelcallablevarwirecheck.sh
job lib-callback-plan python3 ./tests/librarycallbackplancheck.py
job lib-callable-model ./tests/modelcallablewirecheck.sh
job lib-callback-native python3 ./tests/librarycallbacknativecheck.py --package "${MODEL_COM:-./unisacc.com}" --sanitize
job lib-callback-forward python3 ./tests/librarycallbackforwardcheck.py --package "${MODEL_COM:-./unisacc.com}" --sanitize
job lib-callable-variadic-native python3 ./tests/librarycallablevariadicnativecheck.py --package "${MODEL_COM:-./unisacc.com}" --sanitize
job lib-callback-outcome python3 ./tests/librarycallbackoutcomecheck.py --package "${MODEL_COM:-./unisacc.com}"
job lib-typed-native python3 ./tests/librarytypednativecheck.py --package "${MODEL_COM:-./unisacc.com}"
job lib-variadic-native python3 ./tests/libraryvariadicnativecheck.py --package "${MODEL_COM:-./unisacc.com}" --sanitize
job lib-sig2-native python3 ./tests/librarysig2nativecheck.py --package "${MODEL_COM:-./unisacc.com}"
if [ "$(uname -s)" = Darwin ] && [ "$(uname -m)" = arm64 ]; then
    job lib-callback-native-rosetta python3 ./tests/librarycallbacknativecheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --sanitize
    job lib-callback-forward-rosetta python3 ./tests/librarycallbackforwardcheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --sanitize
    job lib-callable-variadic-native-rosetta python3 ./tests/librarycallablevariadicnativecheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --sanitize
    job lib-sig2-rosetta python3 ./tests/librarysig2nativecheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64
    job lib-typed-rosetta python3 ./tests/librarytypednativecheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64
    job lib-variadic-rosetta python3 ./tests/librarynativevariadiccheck.py --arch x86_64
    job lib-callplans-rosetta python3 ./tests/librarycallplanscheck.py --arch x86_64
    job lib-callback-graph-rosetta python3 ./tests/librarycallbackgraphcheck.py --arch x86_64
    job lib-callback-outcome-rosetta python3 ./tests/librarycallbackoutcomecheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64
    job lib-callback-plan-rosetta python3 ./tests/librarycallbackplancheck.py --arch x86_64
    job lib-callable-catalog-rosetta python3 ./tests/librarycallablecatalogcheck.py --arch x86_64
    job lib-carrier-mechanism-rosetta python3 ./tests/librarycarriercheck.py --arch x86_64
    job lib-carrier-native-plan-rosetta python3 ./tests/librarycarriernativeplancheck.py --arch x86_64
    job lib-union-native-rosetta python3 ./tests/libraryunionnativecheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --sanitize
    job lib-carrier-callback-rosetta python3 ./tests/librarycarriercallbackcheck.py --arch x86_64
    job lib-carrier-factory-rosetta python3 ./tests/libraryunionnativecheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --sanitize --factory
    job lib-carrier-relay-rosetta python3 ./tests/libraryunionnativecheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --sanitize --relay
    job lib-variadic-resolver-rosetta python3 ./tests/libraryvariadicresolvercheck.py --arch x86_64
    job lib-variadic-native-rosetta python3 ./tests/libraryvariadicnativecheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --sanitize
fi
job lib-artifacts python3 ./tests/libraryartifactcheck.py --package "${MODEL_COM:-./unisacc.com}"
job lib-native-exports python3 ./tests/libunisaccexportcheck.py --package "${MODEL_COM:-./unisacc.com}"
job lib-module-exports python3 ./tests/libunisaccexportcheck.py --package "${MODEL_COM:-./unisacc.com}" --no-main
job lib-bindings python3 ./tests/libunisaccbindingscheck.py --package "${MODEL_COM:-./unisacc.com}"
job lib-data python3 ./tests/libunisaccdatacheck.py --package "${MODEL_COM:-./unisacc.com}"
job lib-resolver python3 ./tests/libunisaccresolvercheck.py --package "${MODEL_COM:-./unisacc.com}"
job lib-resolver-host python3 ./tests/libraryresolverhostcheck.py
job lib-word64 python3 ./tests/librarywordcheck.py --native-only
job exec-binaryio python3 ./tests/runtimeiocheck.py
job exec-crcllp64 python3 ./tests/crcllp64check.py --ua "${MODEL_COM:-./unisacc.com}" --native-only
job lib-windows-bridge python3 ./tests/windowslibrarybridgecheck.py
job lib-windows-imports python3 ./tests/librarywinimportscheck.py
job lib-windows-gp python3 ./exec/enc/windowshostbridgecheck.py
job lib-lifecycle python3 ./tests/libunisaccruncheck.py --package "${MODEL_COM:-./unisacc.com}" --iterations 1000
job shared-e2-plain ./tests/sharede2.sh plain
job shared-e2-located ./tests/sharede2.sh located
job shared-e2-tokens ./tests/sharede2.sh tokens
job kernel      ./tests/kernel.sh
job malloc      ./tests/malloc.sh
job docs        ./tests/docs.sh
job gate-infra python3 ./tests/queuecheck.py
if [ "$COM" = 1 ]; then
    PRODUCT=${MODEL_COM-$R/unisacc.com}
    case "$PRODUCT" in /*) ;; *) PRODUCT="$R/$PRODUCT";; esac
    if [ "$LIST" = 0 ]; then
        [ -f "$PRODUCT" ] && [ -x "$PRODUCT" ] || { echo "gate: --com needs an executable MODEL_COM file: $PRODUCT" >&2; exit 1; }
        product_hash() { python3 -c 'import hashlib,sys; print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$PRODUCT"; }
        PRODUCT_SHA=$(product_hash) || exit 1
        printf 'gate product: %s sha256 %s\n' "$PRODUCT" "$PRODUCT_SHA"
    fi
    for s in cli ccparity run multi diag diagunits hostile staticinit tagforward staticunits parserbounds formatonce c99; do job com-$s UA="$PRODUCT" UA_RUN="$PRODUCT" ./tests/$s.sh; done
    job com-full-signature python3 ./tests/fullsignaturecallcheck.py --package "$PRODUCT"
    job com-closure UA="$PRODUCT" UA_RUN="$PRODUCT" ./tests/closure.sh examples/*.c
    job com-tools UA="$PRODUCT" UA_RUN="$PRODUCT" TOOLS_UA="$PRODUCT" ./tests/tools.sh
    for shard in 1 2 3 4; do
        job com-corpus-$shard UA="$PRODUCT" UA_RUN="$PRODUCT" CORPUS_UA="$PRODUCT" SHARD=$shard/4 FETCH=0 ./tests/corpus.sh
        job com-difftest_o-$shard UA="$PRODUCT" UA_RUN="$PRODUCT" SHARD=$shard/4 ./tests/difftest_o.sh
    done
fi
[ "$LIST" = 0 ] || exit 0
[ "$n" -gt 0 ] || { echo "gate: no suites executed" >&2; exit 2; }
wait
count=$(ls "$O" | wc -l | tr -d ' ')
[ "$count" -eq "$n" ] || { echo "gate: missing results ($count of $n)" >&2; exit 1; }
cat "$O"/*
if [ "$COM" = 1 ]; then
    PRODUCT_AFTER=$(product_hash) || exit 1
    [ "$PRODUCT_AFTER" = "$PRODUCT_SHA" ] || { echo 'gate: product changed during acceptance' >&2; exit 1; }
fi
bad=$(cat "$O"/* | grep -vc ' rc=0 ')
echo "gate  suites $(ls "$O" | wc -l | tr -d ' ')   failed $bad   $(( $(date +%s)-T0 ))s wall"
[ "$bad" -eq 0 ]

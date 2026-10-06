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
# --list must be able to enumerate every suite without requiring the inputs a
# real run needs: a '${VAR:?}' expands during argument construction, so an unset
# variable truncated the list at the first suite that needed one and every suite
# below it became invisible.  In list mode the value is a placeholder; in a real
# run the hard requirement stands.
if [ "$LIST" != 0 ]; then
    UNISACC_FFI_X86_PROVIDER=${UNISACC_FFI_X86_PROVIDER:-LIST-MODE-PLACEHOLDER}
else
    : "${UNISACC_FFI_X86_PROVIDER:?set UNISACC_FFI_X86_PROVIDER to the x86_64 libffi provider}"
fi

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
job bigclosure-lnx-x86_64 ./tests/bigclosure.sh --target lnx/x86_64
job bigclosure-lnx-arm64 ./tests/bigclosure.sh --target lnx/arm64
job bigclosure-osx-x86_64 ./tests/bigclosure.sh --target osx/x86_64
job bigclosure-osx-arm64 ./tests/bigclosure.sh --target osx/arm64
job bigclosure-win-x86_64 ./tests/bigclosure.sh --target win/x86_64
job bigclosure-win-arm64 ./tests/bigclosure.sh --target win/arm64
# fat: 179 probes, each written, signed and executed twice (arm64 + Rosetta), was
# 50 s alone and 53 s+ beside another job.  Three shards grew to ~46 s each in the
# 0.0.31 queue (limit 53), so six now (cdx 10-06); the union is still every probe.
FATC=$(ls examples/*.c tests/c/*.c)
for k in 1 2 3 4 5 6; do job fat-$k ./tests/fat.sh $(echo "$FATC" | awk -v k=$k 'NR%6==k%6'); done
job c99         ./tests/c99.sh
for k in 1 2 3 4; do job corpus-$k   SHARD=$k/4 ./tests/corpus.sh; done
for k in 1 2 3 4; do job difftest-$k SHARD=$k/4 ./tests/difftest.sh; done
# The queue sets UA, so difftest-N runs the C reference route; the Python reference
# (python3 -m unisa) went ungated and a_externarr stayed red there from f265108e to 890546ab.
for k in 1 2 3 4; do job difftest-py-$k env -u UA -u UA_RUN SHARD=$k/4 ./tests/difftest.sh; done
for k in 1 2 3 4 5; do job declmatrix-$k SHARD=$k/5 python3 ./tests/declmatrix.py; done   # 0.0.22 TDD: declarator forms x positions vs cc (found four reference defects)
job finite-template python3 ./tests/finitetemplatecheck.py   # 0.0.23 K: parameterised templates (chains, nested loops, rename rewrites PUSH)
job decision-ledger python3 ./tests/decisionledger.py   # 0.0.23 I: build-time Python control (direct transition sites) may only fall
job manifest-entries-pp python3 ./tests/manifestentries.py --stage pp
job manifest-entries-lex python3 ./tests/manifestentries.py --stage lex
job manifest-entries-parse2 python3 ./tests/manifestentries.py --stage parse2
job manifest-entries-enc python3 ./tests/manifestentries.py --stage enc
job manifest-entries-enc-arm python3 ./tests/manifestentries.py --stage enc/arm
job manifest-entries-lower python3 ./tests/manifestentries.py --stage lower
job dsl-ops python3 ./tests/dslops.py   # T2: one independent contract and mutation for each manifest op
job declshape bash -c 'R=$PWD; export UA=${UA:-/tmp/ua_ref}; . tests/lib.sh && ua_ready && python3 ./tests/declshape.py'   # 0.0.22 (cdx): declarator metamorphisms -- equivalent spellings agree
job volatile-comma bash -c 'R=$PWD; export UA=${UA:-/tmp/ua_ref}; . tests/lib.sh && ua_ready && python3 ./tests/volatilecomma.py'
for k in $(seq 1 40); do job csmithdiff-$k env -u MODEL_COM SEEDS=1-500 SHARD=$k/40 python3 ./tests/csmithdiff.py; done   # 0.0.31: reference only; com-csmithdiff runs the product (both ran it: 677 s duplicated per queue)   # 0.0.22 TDD: fixed Csmith seeds 1-200, reference vs cc (found 7 reference defects)
# R17-6 (E2, first step): suites that only all.sh / CI / linux.sh ran.  Both
# regressions found after 0.0.16's local queue was green came from here
# (ccrun8: a Python-route bit-field value; opt-run1: c-testsuite 00204).
CCRUNP=$(ls examples/*.c tests/c/*.c | grep -v '/b_malloc\.c$')
for k in 1 2 3 4 5 6 7 8; do job ccrun-$k CCRUN_SHARD=1 ./tests/ccrun.sh $(echo "$CCRUNP" | awk -v k=$k 'NR%8==k%8'); done
job ccrun-malloc CCRUN_SHARD=1 ./tests/ccrun.sh tests/c/b_malloc.c
for k in 1 2 3 4 5 6 7 8; do job opt-run-$k OPT_PART=run SHARD=$k/8 ./tests/opt.sh; done
# The fixture directory suite (tests/fb12/, driver tests/fb12multi.sh) lives in
# the --com block only: it needs an executable compiler, and the default route
# here is `python3 -m unisa`, which is a command rather than a file.  The
# fixtures' expected values come from the host cc, and the defects they record
# are all on the product side.
job closure-ex  ./tests/closure.sh examples/*.c
job closure-c1  ./tests/closure.sh $(echo "$TC" | awk 'NR%4==1')   # four shards: three ran 22 s alone
job closure-c2  ./tests/closure.sh $(echo "$TC" | awk 'NR%4==2')   # but 61 s beside three other suites
job closure-c3  ./tests/closure.sh $(echo "$TC" | awk 'NR%4==3')
job closure-c4  ./tests/closure.sh $(echo "$TC" | awk 'NR%4==0')
for k in 1 2 3; do job stages-$k SHARD=$k/3 ./tests/stages.sh examples/*.c tests/c/*.c; done   # R4: 50.4/53 s whole
for k in 1 2 3 4 5; do job exec-chain-$k env CHAINSHARD=$k/5 CHAINKEEP=exec/c/keep-chain.txt ./exec/c/chain.sh $(cat exec/c/keep-chain.txt) examples/*.c tests/c/*.c; done   # S-17: all C probes, historical floor, named parity debt; five shards keep each job under 60 s with --jobs 4 (three hit 142 in the 0.0.23 queue)
job exec-macros python3 ./exec/pp/macrocheck.py
job exec-pp-literals python3 ./exec/pp/literalcheck.py
job exec-pp-pragmas python3 ./exec/pp/pragmacheck.py
job exec-pp-object python3 ./exec/pp/objectpredefinecheck.py
job exec-pp-directives python3 ./exec/pp/directivecheck.py   # R21: live #error, #include limit, stray #endif/#else, string prefixes = reference or named refusal
job exec-r21-e3 python3 ./exec/parse2/r21check.py   # R21: E3 on this week's reference fixes = reference bytes or named refusal (never different bytes)
job exec-pploc python3 ./exec/pp/locationcheck.py              # source-position metadata from actual preprocessing
job exec-lexpos python3 ./exec/lex/positioncheck.py             # token offsets against reference tpos
job exec-lexloc python3 ./exec/lex/locationcheck.py             # E2/E1 location envelope
job exec-f1-sidecar python3 ./exec/lex/f1sidecarcheck.py         # constructor/destructor mark framing
job exec-objectplan python3 ./tests/objectplancheck.py         # ordered ELF symbol plan
job exec-objectpack python3 ./tests/objectpackcheck.py         # target-scoped route declarations
job exec-modelobject python3 ./tests/modelobjectcheck.py       # two ELF architectures against C reference
job exec-objectfacts python3 ./tests/objectfactscheck.py       # object tape facts in delta
job exec-elfobject python3 ./tests/elfobjectdeltacheck.py        # independent ET_REL layout oracle
job exec-unitparse sh ./tests/unitparsecheck.sh              # separate-unit delta and network parity
job exec-parseloc python3 ./exec/parse2/locationcheck.py         # E3 retained maps and tape parity
job exec-f1-attributes python3 ./exec/parse2/f1attributescheck.py # marked definitions against reference tape
job exec-diag python3 ./exec/parse2/diagnosticcheck.py           # actual reference diagnostic rendering
# Cold parse2 models (30-45 s each) moved out of the seven warning/error checks below:
# warnprep fills the shared content-keyed cache (compilerpack.built_model, digest-checked
# on hit); the checks still self-build on a miss, so queue order does not matter.
for g in 0 1 2 3; do job exec-warnprep-$g python3 ./exec/parse2/warnprep.py $g; done
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
job exec-winx86self env SELF_PART=stages TARGET=win/x86_64 ./exec/pipeline/selfcheck.sh
job exec-winx86self-package env SELF_PART=package TARGET=win/x86_64 ./exec/pipeline/selfcheck.sh
job exec-pearm ./exec/enc/pecheck.sh                           # PE sections, relocations, real ARM images
job exec-winself env SELF_PART=stages TARGET=win/arm64 ./exec/pipeline/selfcheck.sh # full source PE, macros and reference bytes
job exec-winself-package env SELF_PART=package TARGET=win/arm64 ./exec/pipeline/selfcheck.sh
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
job ape-version python3 ./tests/apeversioncheck.py --ape "${MODEL_COM:-./unisacc.com}"
job proc-enum python3 ./tests/procenumcheck.py
job libneed ./tests/libneed.sh
job apps-real python3 ./tests/appsrealcheck.py             # real snapshots, no fabricated application defaults
job exec-core ./exec/c/corecheck.sh     # isolated generic kernel, external linkage and ISA byte ledger
job exec-asm ./exec/c/asmcheck.sh       # complete assembly execution kernel
if [ "$(uname -s)" = Darwin ]; then
    job exec-asmx86 env CORE_ASM_ARCH=x86_64 ./exec/c/asmcheck.sh
    # The binding check was one job per arch and measured 61 s warm, past the
    # 60 s watchdog -- it was killed with rc=142 before any assert ran.  A warm
    # trace split it prep 7 s / asserts 11 s, with the cold first run at 54 s
    # because the model cache (models.py:61, content-addressed) is empty then.
    # Two jobs per arch keep the warm path far inside the bound and let the
    # cold path finish; self-prepare on a missing artefact keeps the pair
    # order-independent when a queue runs them out of order.
    for a in arm64 x86_64; do
        job exec-bindprep-$a env CORE_ASM_ARCH=$a ./exec/c/asm/bindprep.sh
        job exec-bindverify-$a env CORE_ASM_ARCH=$a ./exec/c/asm/bindverify.sh
        job lib-bank-$a python3 ./tests/bankcheck.py --arch $a   # R12-1 BNK1 plan harness (was ungated; 7-10 s)
    done
    # Fresh construction is exercised by comboot; this job inspects and runs
    # the selected sealed container.  Building it again consumed the whole
    # 60-second job budget before the assertions could start.
    job exec-container env MODEL_COM="${MODEL_COM:-$R/unisacc.com}" ./exec/c/containercheck.sh
fi
job exec-embedded python3 ./exec/c/embeddedcheck.py
for part in build modes contracts dependencies; do job exec-driver-core-$part ./exec/c/compilercheck.sh core-$part; done
job exec-driver-resources ./exec/c/compilercheck.sh resources
for shard in 1 2; do job exec-driver-language-$shard ./exec/c/compilercheck.sh language-$shard; done
for driver in cc asm; do job exec-warningdriver-$driver ./exec/c/warningcheck.sh "$driver"; done
for flag in Wall Wextra Werror; do job exec-warningdriver-ua-$flag ./exec/c/warningcheck.sh ua "$flag"; done
job exec-multiwarn ./exec/c/multiwarningcheck.sh
job exec-unitlocations python3 ./exec/parse2/unitlocationcheck.py
for kind in cc ua asm; do
    for pair in m n pool; do
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
# models.py cold is ~40 s of the 58 s cold selfcheck; warm the cache in its own jobs
job exec-selfprep-table ./tests/selfprep.sh osx/arm64 0
job exec-selfprep-x86 ./tests/selfprep.sh lnx/x86_64
job exec-selfprep-arm ./tests/selfprep.sh lnx/arm64
job exec-tableself env SELF_PART=stages NETWORK=0 TARGET=osx/arm64 ./exec/pipeline/selfcheck.sh
job exec-selfelf env SELF_PART=stages ./exec/pipeline/selfcheck.sh                    # current compiler source through seven deltas
job exec-selfelf-package env SELF_PART=package ./exec/pipeline/selfcheck.sh
job exec-armself env SELF_PART=stages TARGET=lnx/arm64 ./exec/pipeline/selfcheck.sh # ARM source-to-ELF, includes target predefines
job exec-armself-package env SELF_PART=package TARGET=lnx/arm64 ./exec/pipeline/selfcheck.sh
job exec-macself env SELF_PART=stages TARGET=osx/arm64 ./exec/pipeline/selfcheck.sh # Mach-O source route and native bootstrap
job exec-macself-package env SELF_PART=package TARGET=osx/arm64 ./exec/pipeline/selfcheck.sh
job exec-macxself env SELF_PART=stages TARGET=osx/x86_64 ./exec/pipeline/selfcheck.sh # x86/Rosetta bootstrap
job exec-macxself-package env SELF_PART=package TARGET=osx/x86_64 ./exec/pipeline/selfcheck.sh
job exec-bootstrap-osxarm env SELF_PART=bootstrap TARGET=osx/arm64 ./exec/pipeline/selfcheck.sh
job exec-bootstrap-osxx86 env SELF_PART=bootstrap TARGET=osx/x86_64 ./exec/pipeline/selfcheck.sh
job exec-srcelf ./exec/pipeline/check-elf.sh                     # fixed 68 complete source-to-ELF paths
for k in 1 2 3 4; do job difftest_o-$k SHARD=$k/4 ./tests/difftest_o.sh; done
job warn        ./tests/warn.sh
job diag-units  ./tests/diagunits.sh
for part in self cross-a cross-b; do job nativeboot-$part env NB_PART=$part ./tests/nativeboot.sh; done  # 0.0.31: one window no longer holds all eight compiles
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
job lib-ordered-carrier-model python3 ./tests/modelnativebitfieldcheck.py
job referee-binsel python3 ./tests/binsel_audit.py        # external referee: host cc decides signedness-dependence
job referee-pfconv python3 ./tests/pfconv_audit.py        # external referee: host printf decides each conversion
job lib-ordered-carrier-host python3 ./tests/libraryorderedcarriercheck.py
if [ "$(uname -s)/$(uname -m)" = Darwin/arm64 ]; then
    job lib-ordered-carrier-native python3 ./tests/modelorderedbitfieldnativecheck.py --target osx/arm64
    job lib-packed-carrier-native python3 ./tests/modelpackednativecheck.py --target osx/arm64
    job lib-packed-carrier-refusal python3 ./tests/modelpackednativecheck.py --target osx/x86_64
fi
job lib-ffi-provider python3 ./tests/libraryffiprovidercheck.py
# parse2 JSON model (~32 s cold) shared by the five *-source checks; they self-build on a miss
job lib-source-prep python3 ./tests/parse2json.py
job lib-bitfield-source python3 ./tests/modelbitfieldsourcecheck.py
job lib-layout-source python3 ./tests/modellayoutfactscheck.py
job lib-sig3-host python3 ./tests/librarysignature3check.py
job lib-sig3-canonical ./tests/modelsignature3check.sh canonical
job lib-sig3-equality ./tests/modelsignature3check.sh equal
job lib-sig3-source python3 ./tests/modelsourcefacts3check.py
job lib-fp-rank-source python3 ./tests/modelfprankcheck.py
job lib-fp-value-rank-source python3 ./tests/modelfpvaluerankcheck.py
job lib-source-import-compat ./tests/modelsourceimportcompatcheck.sh
job lib-source-provenance ./tests/modelsourcelayoutprovenancecheck.sh plain
job lib-source-provenance-located ./tests/modelsourcelayoutprovenancecheck.sh located
if [ "$(uname -s)/$(uname -m)" = Darwin/arm64 ]; then
    job lib-source-bitfield-native python3 ./tests/librarysourcebitfieldcheck.py
    job lib-source-bitfield-import-native python3 ./tests/librarysourcebitfieldimportcheck.py
    job lib-source-callable-import-native python3 ./tests/librarysourcebitfieldimportcheck.py --mode callable
    job lib-source-callback-import-native python3 ./tests/librarysourcebitfieldimportcheck.py --mode callbacks
    job lib-source-variadic-import-native python3 ./tests/librarysourcebitfieldimportcheck.py --mode variadic
    job lib-source-pointee-import-native python3 ./tests/librarysourcebitfieldimportcheck.py --mode pointee
    job lib-source-longdouble-import-native python3 ./tests/librarysourcebitfieldimportcheck.py --mode longdouble
fi
job lib-carrier-mechanism python3 ./tests/librarycarriercheck.py
job lib-carrier-plan python3 ./tests/librarycarrierplancheck.py
job lib-carrier-native-plan python3 ./tests/librarycarriernativeplancheck.py
job lib-carrier-callback python3 ./tests/librarycarriercallbackcheck.py
job lib-carrier-import-model python3 ./tests/modelnativecarrierimportcheck.py
job lib-union-native python3 ./tests/libraryunionnativecheck.py --package "${MODEL_COM:-./unisacc.com}" --sanitize
job lib-carrier-factory python3 ./tests/libraryunionnativecheck.py --package "${MODEL_COM:-./unisacc.com}" --sanitize --factory
job lib-carrier-relay python3 ./tests/libraryunionnativecheck.py --package "${MODEL_COM:-./unisacc.com}" --sanitize --relay
job lib-union-scalar-1 python3 ./tests/libraryunionscalarcheck.py --package "${MODEL_COM:-./unisacc.com}" --cases int8,float4 --sanitize
job lib-union-scalar-2 python3 ./tests/libraryunionscalarcheck.py --package "${MODEL_COM:-./unisacc.com}" --cases double8,mixed2 --sanitize
job lib-union-scalar-3 python3 ./tests/libraryunionscalarcheck.py --package "${MODEL_COM:-./unisacc.com}" --cases mixed3,mixed-reordered --sanitize
job lib-union-composite-direct-1 python3 ./tests/libraryunioncompositecheck.py --package "${MODEL_COM:-./unisacc.com}" --cases mixed16,fp16 --sanitize
job lib-union-composite-direct-2 python3 ./tests/libraryunioncompositecheck.py --package "${MODEL_COM:-./unisacc.com}" --cases array2,array4 --sanitize
job lib-union-composite-direct-3 python3 ./tests/libraryunioncompositecheck.py --package "${MODEL_COM:-./unisacc.com}" --cases array5 --sanitize
job lib-union-composite-callback-1 python3 ./tests/libraryunioncompositecheck.py --package "${MODEL_COM:-./unisacc.com}" --cases mixed16,fp16 --sanitize --callback
job lib-union-composite-callback-2 python3 ./tests/libraryunioncompositecheck.py --package "${MODEL_COM:-./unisacc.com}" --cases array2,array4 --sanitize --callback
job lib-union-composite-callback-3 python3 ./tests/libraryunioncompositecheck.py --package "${MODEL_COM:-./unisacc.com}" --cases array5 --sanitize --callback
job lib-union16-direct-1 python3 ./tests/libraryunion16check.py --package "${MODEL_COM:-./unisacc.com}" --cases is8,si8 --sanitize
job lib-union16-callback-1 python3 ./tests/libraryunion16check.py --package "${MODEL_COM:-./unisacc.com}" --cases is8,si8 --sanitize --callback
job lib-union16-direct-2 python3 ./tests/libraryunion16check.py --package "${MODEL_COM:-./unisacc.com}" --cases ss8,ii8 --sanitize
job lib-union16-callback-2 python3 ./tests/libraryunion16check.py --package "${MODEL_COM:-./unisacc.com}" --cases ss8,ii8 --sanitize --callback
job lib-union16-direct-3 python3 ./tests/libraryunion16check.py --package "${MODEL_COM:-./unisacc.com}" --cases is4,si4 --sanitize
job lib-union16-callback-3 python3 ./tests/libraryunion16check.py --package "${MODEL_COM:-./unisacc.com}" --cases is4,si4 --sanitize --callback
job lib-union16-direct-4 python3 ./tests/libraryunion16check.py --package "${MODEL_COM:-./unisacc.com}" --cases ss4,ii4 --sanitize
job lib-union16-callback-4 python3 ./tests/libraryunion16check.py --package "${MODEL_COM:-./unisacc.com}" --cases ss4,ii4 --sanitize --callback
job lib-union16-direct-5 python3 ./tests/libraryunion16check.py --package "${MODEL_COM:-./unisacc.com}" --cases fpmix --sanitize
job lib-union16-callback-5 python3 ./tests/libraryunion16check.py --package "${MODEL_COM:-./unisacc.com}" --cases fpmix --sanitize --callback
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
    job lib-source-longdouble-import-rosetta python3 ./tests/librarysourcebitfieldimportcheck.py --mode longdouble --arch x86_64 --ffi-provider "$UNISACC_FFI_X86_PROVIDER"
    job lib-carrier-mechanism-rosetta python3 ./tests/librarycarriercheck.py --arch x86_64
    job lib-carrier-native-plan-rosetta python3 ./tests/librarycarriernativeplancheck.py --arch x86_64
    job lib-union-native-rosetta python3 ./tests/libraryunionnativecheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --sanitize
    job lib-carrier-callback-rosetta python3 ./tests/librarycarriercallbackcheck.py --arch x86_64
    job lib-carrier-factory-rosetta python3 ./tests/libraryunionnativecheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --sanitize --factory
    job lib-carrier-relay-rosetta python3 ./tests/libraryunionnativecheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --sanitize --relay
    job lib-union-scalar-1-rosetta python3 ./tests/libraryunionscalarcheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --cases int8,float4 --sanitize
    job lib-union-scalar-2-rosetta python3 ./tests/libraryunionscalarcheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --cases double8,mixed2 --sanitize
    job lib-union-scalar-3-rosetta python3 ./tests/libraryunionscalarcheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --cases mixed3,mixed-reordered --sanitize
    job lib-union-scalar-narrow-rosetta python3 ./tests/libraryunionscalarcheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --cases int1,int2,int4 --sanitize
    job lib-union-composite-direct-1-rosetta python3 ./tests/libraryunioncompositecheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --cases mixed16,fp16 --sanitize
    job lib-union-composite-direct-2-rosetta python3 ./tests/libraryunioncompositecheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --cases array2,array4 --sanitize
    job lib-union-composite-direct-3-rosetta python3 ./tests/libraryunioncompositecheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --cases array5 --sanitize
    job lib-union-composite-callback-1-rosetta python3 ./tests/libraryunioncompositecheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --cases mixed16,fp16 --sanitize --callback
    job lib-union-composite-callback-2-rosetta python3 ./tests/libraryunioncompositecheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --cases array2,array4 --sanitize --callback
    job lib-union-composite-callback-3-rosetta python3 ./tests/libraryunioncompositecheck.py --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --cases array5 --sanitize --callback
    job lib-union16-direct-1-rosetta python3 ./tests/libraryunion16check.py --package "${MODEL_COM:-./unisacc.com}" --cases is8,si8 --sanitize --arch x86_64
    job lib-union16-callback-1-rosetta python3 ./tests/libraryunion16check.py --package "${MODEL_COM:-./unisacc.com}" --cases is8,si8 --sanitize --callback --arch x86_64
    job lib-union16-direct-2-rosetta python3 ./tests/libraryunion16check.py --package "${MODEL_COM:-./unisacc.com}" --cases ss8,ii8 --sanitize --arch x86_64
    job lib-union16-callback-2-rosetta python3 ./tests/libraryunion16check.py --package "${MODEL_COM:-./unisacc.com}" --cases ss8,ii8 --sanitize --callback --arch x86_64
    job lib-union16-direct-3-rosetta python3 ./tests/libraryunion16check.py --package "${MODEL_COM:-./unisacc.com}" --cases is4,si4 --sanitize --arch x86_64
    job lib-union16-callback-3-rosetta python3 ./tests/libraryunion16check.py --package "${MODEL_COM:-./unisacc.com}" --cases is4,si4 --sanitize --callback --arch x86_64
    job lib-union16-direct-4-rosetta python3 ./tests/libraryunion16check.py --package "${MODEL_COM:-./unisacc.com}" --cases ss4,ii4 --sanitize --arch x86_64
    job lib-union16-callback-4-rosetta python3 ./tests/libraryunion16check.py --package "${MODEL_COM:-./unisacc.com}" --cases ss4,ii4 --sanitize --callback --arch x86_64
    job lib-union16-direct-5-rosetta python3 ./tests/libraryunion16check.py --package "${MODEL_COM:-./unisacc.com}" --cases fpmix --sanitize --arch x86_64
    job lib-union16-callback-5-rosetta python3 ./tests/libraryunion16check.py --package "${MODEL_COM:-./unisacc.com}" --cases fpmix --sanitize --callback --arch x86_64
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
job rowcov-pp python3 ./tests/rowcov.py pp all   # 0.0.25 T2: pp manifest rows reached by the probes; ratchet in tests/rowcov.baseline
job rowcov-pp-locations python3 ./tests/rowcov.py pp-locations all   # 0.0.32 T3': the --locations variant graph
job rowcov-pp-shared python3 ./tests/rowcov.py pp-shared all   # 0.0.32 T3': the --shared-predefines variant graph (predefines as resources)
job rowcov-lex python3 ./tests/rowcov.py lex all   # 0.0.26 T2: lex manifest rows reached (on the preprocessor output); ratchet in tests/rowcov.baseline
job rowcov-parse2-build python3 ./tests/rowcov.py parse2 build   # 0.0.31 R4: the shards reuse its cache
for k in $(seq 1 24); do job rowcov-parse2-$k python3 ./tests/rowcov.py parse2 gate$k/24; done   # 0.0.30 R2: 24 shards (16/12 hit the 55 s window three times in 0.0.29)   # 0.0.26 T2: each shard ratchets its own reached rows
for st in lower enc; do for k in $(seq 1 8); do job rowcov-$st-$k python3 ./tests/rowcov.py $st gate$k/8; done; done   # 0.0.26 T2: lower (--full) and enc (--elf) rows, on tapes from the reference -S
for st in lower enc; do job rowcov-$st-union python3 ./tests/rowcov.py $st union8; done   # 0.0.28 E22: the union may only rise, whatever the shards' redistribution
job rowcov-parse2-union python3 ./tests/rowcov.py parse2 union24   # 0.0.29 T4
job seedpy python3 ./tests/decisionledger.py --seedpy   # 0.0.25 X8: stage-specific seed .py and assemble.py size may only fall
job ledgercheck python3 ./tests/ledgercheck.py   # 0.0.25 P8: every plan row settled (完成/顺延/跨版进行/砍掉)
job freezecheck python3 ./tests/freezecheck.py   # 0.0.25 P1: inside the freeze window only fix: commits touch the product closure
job revivedscan python3 ./tests/revivedscan.py   # 0.0.25 P2: an excused probe that now agrees is reported in ~11 s, not at queue time
job facts-export python3 ./exec/facts/export.py --check   # 0.0.25 P3: a header change without regenerated facts (0.0.24 first seal) is red here
job malloc      ./tests/malloc.sh
job docs        ./tests/docs.sh
job tapebin-roundtrip python3 ./tests/tapebin.py
job tapebin-shape python3 -m unisa.tapebin_shape --check
job tape-reader python3 ./tests/tapereadercheck.py
job script-inventory python3 ./tests/inventory.py --check   # R15-1: no test/check script without a gate, a caller or a disposition
job subtract-safety python3 ./tests/subtractsafety.py   # R16-9: archived files have no users outside archive/; AGENTS.md/CLAUDE.md resolve
job subtract-safety-selftest python3 ./tests/subtractsafety.py --selftest   # R16-9: both 0.0.15 counterexamples are still caught
job realprog    ./tests/realprog.sh   # R16-12: pinned real programs (kilo, jsmn, cJSON, lua, sqlite) built by us and cc, same run, same bytes; ratchet list
job hosthdr     ./tests/hosthdr.sh   # R18-10: sys/stat, fcntl, poll, termios, sys/ioctl, isatty, dirent on macOS = cc (and gcc on Lima arm64)
job asmtext     ./tests/asmtext.sh   # R18-1/R18-2: -S -b lnx | unisacc as == -c on the corpus; system as accepts; hand-written .s
job combo       python3 ./tests/combo.py "$UA" "${MODEL_COM:-./unisacc.com}"   # R19-7: construct x type x position; product = reference tape or refused by name
job syscall6    ./tests/syscall6.sh   # R19-9 (3): generic system-call gate on osx/arm64, Rosetta x86_64, Lima lnx/arm64
job forward     ./tests/forward.sh   # R19-10/R21-4a': prototyped undefined functions forward to the system libc (-run and images, six targets)
job forward-multi ./tests/forwardmulti.sh   # 0.0.26 N5: forwarding across translation units (default run, -run, -o)
job winposix    ./tests/winposix.sh   # R21-4a': Windows POSIX layer over kernel32 = macOS cc (win/arm64, win/x86_64 in the UTM VM; skipped when it is down)
job ccinterop   ./tests/ccinterop.sh   # R21-1: -c objects call cc-compiled int/pointer functions (osx/arm64, lnx/arm64, lnx/x86_64); other signatures refused by name
job linkunits   ./tests/linkunits.sh   # R17-2: units compiled alone (-c -b HOST -funit) and joined by the unisacc linker = cc = one-step
job elfobj      ./tests/elfobj.sh   # R16-7: -c -b lnx/ARCH relocatable ELF: sections, relocations, image invariant, GNU ld/lld link, Lima run
job c99-ledger  python3 ./tests/c99ledger.py   # R14-6: the C99 clause ledger and README's coverage table
job publish-order python3 ./tests/publishordercheck.py   # wrong signed hash must leave the draft and assets untouched
job front-bounds python3 ./tests/frontboundscheck.py   # anonymous struct tags must not read token -1
# 0.0.28 R4: one job per part that seedconstructcheck.py --list names (C99 seed tools vs Python, byte for byte);
# a new part needs no edit here or in gatedeps.json (refresh_gatedeps.py declares it from seed-construct-parse2)
job seedfacts python3 ./tests/seedfactscheck.py   # 0.0.29 B4: seed/facts.h bindings and lets equal assemble.py on every manifest
job seedparse2-1 ./tests/seedparse2check.sh x locations warnings errors   # 0.0.29 B4: seed/gen.c parse2 = gen.py, eight variants in two jobs
job seedparse2-2 ./tests/seedparse2check.sh locations,warnings locations,errors warnings,errors locations,warnings,errors
# 0.0.32 B5: seed/gen.c = gen.py on every delta the build constructs; three shards stay under the 60 s ceiling
job seedgen ./tests/seedgencheck.sh e2 e1 e3 e4 o1 prune nativeabi enc-elf enc-macho enc-pe arm-elf arm-macho arm-pe
job seedgen-2 ./tests/seedgencheck.sh lower-lnx-x lower-lnx-a lower-osx-x lower-osx-a lower-win-x lower-win-a obj-lower-x obj-lower-a obj-enc-x obj-enc-a
job seedgen-3 ./tests/seedgencheck.sh tokenpp tokenlex warnlex warnparse warnunits errorparse warnpp
job com-memalign ./tests/memaligncheck.sh   # 0.0.32 D2: word-wise memcpy/memset only on shared alignment (UBSan + product)
job com-seedgen ./tests/seedgencomcheck.sh   # 0.0.32 D2: .com -O2 seed/gen.c parse2 == cc build, <= 10 s
job seedpack ./tests/seedpackcheck.sh   # 0.0.32 B5: seed/pack.c + compilerpack.c == pack.py + compilerpack.py
job seedape ./tests/seedapecheck.sh     # 0.0.32 B5: UA -b + seed/ape.c == unisa ape (product container)
scparts=$(python3 "$R/tests/seedconstructcheck.py" --list) || scparts=""
[ -n "$scparts" ] || job seed-construct-base sh -c 'echo "seedconstructcheck.py --list failed or listed no part"; exit 1'   # 0.0.28 E14: never zero jobs and green
for p in $scparts; do job "seed-construct-$p" python3 ./tests/seedconstructcheck.py --part "$p"; done
job seed-matrix-shared python3 ./tests/seedconstructmatrix.py shared
for k in 1 2 3; do job seed-matrix-features-$k python3 ./tests/seedconstructmatrix.py features-$k; done
job seed-matrix-lnx-arm64 python3 ./tests/seedconstructmatrix.py lnx/arm64
job seed-matrix-lnx-x86_64 python3 ./tests/seedconstructmatrix.py lnx/x86_64
job seed-matrix-osx-arm64 python3 ./tests/seedconstructmatrix.py osx/arm64
job seed-matrix-osx-x86_64 python3 ./tests/seedconstructmatrix.py osx/x86_64
job seed-matrix-win-arm64 python3 ./tests/seedconstructmatrix.py win/arm64
job seed-matrix-win-x86_64 python3 ./tests/seedconstructmatrix.py win/x86_64
job seed-matrix-object python3 ./tests/seedconstructmatrix.py object
job gate-infra python3 ./tests/queuecheck.py
job gate-layers python3 ./tests/gatelayers.py --check
# R12-0 ③b: checks that were in no gate at all (tests/ungated-checks.tsv).
# Each was run standalone with a measured rc=0 before being added; the four that
# print a Usage line and the one that fails on this HEAD are NOT here -- see the
# tsv.  all.sh is deliberately unchanged.
job exec-elfdata-bridge   python3 ./exec/enc/librarysymbolscheck.py
job exec-lex-sourcefacts  python3 ./exec/lex/sourcefactscheck.py
job exec-parse2-libimports python3 ./exec/parse2/libraryimportscheck.py
for m in 0 1 2 3 4 5 6 7 8; do job fresh-order-$m python3 ./tests/freshordercheck.py $m; done   # T1d: global fresh order = tests/freshorder.tsv
job lib-bindings-registry python3 ./tests/librarybindingscheck.py
job tsv-build-account     python3 ./tests/tsvbuild_check.py
# The five exec/pipeline/check_*_text.py format checkers have no caller in any
# gate; the only path that reaches them is exec/pipeline/run.py, which nothing
# invoked either.  This suite is that path, made bounded: run.py walks the seven
# stages over one small program and calls check_<fmt>.py on each produced dump,
# so a format checker that starts rejecting valid output fails here.
job exec-formats python3 ./exec/pipeline/run.py examples/hello.c
job pipeline-cache python3 ./tests/pipelinecache.py
# The last check that was in no gate AND failing on this HEAD: it cross-assembles
# librarycall_<arch>.S for a Linux target and an Apple target and requires the
# same instruction stream.  It failed for a year of commits' worth of reasons
# that were not drift -- see archive/research/r12/r12-linuxbridgecheck-diagnosis.md.
job exec-bridge-linux python3 ./exec/c/linuxbridgecheck.py
job windows-resolver-host python3 ./tests/windowsresolverhostcheck.py
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
    job com-forward UA="$PRODUCT" UA_RUN="$PRODUCT" ./tests/forward.sh   # 0.0.28 N1': posix2 (glob callback), var (variadic), host functions on the product
    job com-forward-multi UA="$PRODUCT" UA_RUN="$PRODUCT" ./tests/forwardmulti.sh   # 0.0.26 N5: the product failed here up to 0.0.25
    job com-volatile-comma MODEL_COM="$PRODUCT" bash -c 'R=$PWD; export UA=${UA:-/tmp/ua_ref}; . tests/lib.sh && ua_ready && python3 ./tests/volatilecomma.py --com'
    job com-full-signature python3 ./tests/fullsignaturecallcheck.py --package "$PRODUCT"
    for k in $(seq 1 40); do job com-csmithdiff-$k MODEL_COM="$PRODUCT" SEEDS=1-500 SHARD=$k/40 python3 ./tests/csmithdiff.py; done
    for k in 1 2 3 4 5; do job com-declmatrix-$k MODEL_COM="$PRODUCT" SHARD=$k/5 python3 ./tests/declmatrix.py; done   # the same matrix on the product (found functions named r0..r7)
    job com-auditnet MODEL_COM="$PRODUCT" python3 ./tests/auditnet.py   # 0.0.22 P1-C1: every packed network re-enumerated; reads model-audit/ beside the product
    for k in 1 2 3; do job com-tapebin-$k MODEL_COM="$PRODUCT" TAPEBIN_SHARD=$k/3 python3 ./tests/tapebinproduct.py; done   # one pass was 45-53 s
    job com-elfobj OBJ_PRODUCT="$PRODUCT" ./tests/elfobj.sh   # R17-1: the product writes the reference's Linux object bytes
    job com-linkunits LINK_PRODUCT="$PRODUCT" ./tests/linkunits.sh   # R17-2: the product links reference-built unit objects
    job com-closure UA="$PRODUCT" UA_RUN="$PRODUCT" ./tests/closure.sh examples/*.c
    job com-tools UA="$PRODUCT" UA_RUN="$PRODUCT" TOOLS_UA="$PRODUCT" ./tests/tools.sh
    for shard in 1 2 3 4; do
        job com-corpus-$shard UA="$PRODUCT" UA_RUN="$PRODUCT" CORPUS_UA="$PRODUCT" SHARD=$shard/4 FETCH=0 ./tests/corpus.sh
        # difftest against the product, not the reference: R13-0's
        # fb12-03/04/05 are wrong only here (the Python reference answers 0, 22
        # and 8).  Same suite, UA selects the route; the known list differs too
        # (tests/difftest.com.knownfail), because one list cannot be right for
        # both routes.
        job com-difftest-$shard UA="$PRODUCT" UA_RUN="$PRODUCT" SHARD=$shard/4 ./tests/difftest.sh
        job com-difftest_o-$shard UA="$PRODUCT" UA_RUN="$PRODUCT" SHARD=$shard/4 ./tests/difftest_o.sh
    done
    job com-fb12-multi MODEL_COM="$PRODUCT" ./tests/fb12multi.sh
    # N22: the shipped compiler builds itself.  Four shards, and the names do
    # not change: the plan and README list these four.  The fixed-point shard is
    # the one that matters -- stage 2 and stage 3 must be byte-equal, and a
    # mismatch is a defect to diagnose, not a warning.  SEED_DIR is private and
    # outside the tree, so running this never leaves an artifact behind.
    #
    # Each shard CHECKS its prerequisite instead of building it, which is what
    # the two earlier attempts each failed at separately:
    #   * running a whole stage in one call is ~90 s idle (53 s -> killed under
    #     --jobs 2), and a first run in an empty SEED_DIR cost 79 s, so
    #     `bound.py 55` cannot hold a shard that builds its own inputs;
    #   * gatequeue starts jobs in whatever order finishes first, so `stage2`
    #     may run before `seed` and must not fail on the missing seed.
    # The seed is therefore the release flow's job, before the queue:
    #     make seed-com SEED_DIR=/tmp/unisacc-seed-comb-gate
    # A shard that finds its prerequisite absent prints one `skipped:` line and
    # exits 0 -- an unrun pipeline is not a defect in the compiler.  STRICT=1 in
    # the environment turns those skips into failures for a caller that needs
    # the bootstrap actually proven.  Every `make` inside a shard still gets its
    # own `tests/bound.py 55`, and a mkdir lock keeps two runs off one artifact.
    COMB_SEED="${SEED_DIR:-/tmp/unisacc-seed-comb-gate}"
    job com-comboot-seed       COMBOOT_BUDGET=999 SEED_DIR="$COMB_SEED" sh ./exec/c/comboot.sh shard seed
    job com-comboot-stage2     COMBOOT_BUDGET=999 SEED_DIR="$COMB_SEED" UA="$PRODUCT" sh ./exec/c/comboot.sh shard stage2
    job com-comboot-stage3     COMBOOT_BUDGET=999 SEED_DIR="$COMB_SEED" UA="$PRODUCT" sh ./exec/c/comboot.sh shard stage3
    job com-comboot-fixedpoint COMBOOT_BUDGET=999 SEED_DIR="$COMB_SEED" UA="$PRODUCT" sh ./exec/c/comboot.sh shard fixedpoint
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

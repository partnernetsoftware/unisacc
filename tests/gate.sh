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
# H1 host recipe (0.0.43): a relative MODEL_COM names a file from the caller's directory, but jobs run from $R and
# some hand it to tools that run elsewhere (resourceflow), so it is made absolute once, before the cd.
case ${MODEL_COM:-} in
    ''|/*) ;;
    *) [ -e "$MODEL_COM" ] && MODEL_COM=$(cd "$(dirname "$MODEL_COM")" && pwd)/$(basename "$MODEL_COM") && export MODEL_COM;;
esac
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
# F4 made the product driver default to 8 unit workers; gate jobs run several at once,
# so each job gets 2 (R4 budget: outer jobs x UNISA_JOBS).  The csih F4 benchmark keeps 8.
export UNISA_JOBS=${UNISA_JOBS:-2}
if [ "$(uname -s)" = Darwin ] && [ -z "${TERM_SH_INSIDE:-}" ] && [ "${TERM_SH:-1}" != 0 ]; then
    TERM_SH_ALARM=${TERM_SH_ALARM:-60} exec ./tests/term.sh ./tests/gate.sh "$@"
fi
# Also bound direct/non-macOS calls, including queueing and final collection.
if [ "${GATE_BOUND:-0}" != 1 ]; then
    exec python3 "$R/tests/bound.py" 60 env GATE_BOUND=1 "$0" "$@"
fi
COM=0; LIST=0; SELECT=""; STAGES=""
while [ $# -gt 0 ]; do
    case $1 in
        --com) COM=1; shift;;
        --list) LIST=1; shift;;
        --plan) LIST=2; shift;;
        --host-plan) LIST=3; shift;;
        --suite) [ $# -ge 2 ] || { echo 'missing suite name' >&2; exit 2; }
            SELECT="$SELECT $2"; shift 2;;
        --stage) [ $# -ge 2 ] || { echo 'missing stage name' >&2; exit 2; }   # tests/stagemap.c
            STAGES="$STAGES $2"; shift 2;;
        *) echo "unknown gate option: $1" >&2; exit 2;;
    esac
done
# --list must be able to enumerate every suite without requiring the inputs a
# real run needs: a '${VAR:?}' expands during argument construction, so an unset
# variable truncated the list at the first suite that needed one and every suite
# below it became invisible.  In list mode the value is a placeholder; in a real
# run the hard requirement stands.
# 0.0.37: only the Rosetta suites (listed on Darwin/arm64 alone) read the provider, so a
# Linux host runs its whole list without one; on Darwin/arm64 the requirement is unchanged.
if [ "$LIST" != 0 ] || [ "$(uname -s)/$(uname -m)" != Darwin/arm64 ]; then
    UNISACC_FFI_X86_PROVIDER=${UNISACC_FFI_X86_PROVIDER:-LIST-MODE-PLACEHOLDER}
else
    : "${UNISACC_FFI_X86_PROVIDER:?set UNISACC_FFI_X86_PROVIDER to the x86_64 libffi provider}"
fi

# --stage S: the suites tests/stagemap.c assigns to compiler stage S (run by the product itself).
if [ -n "$STAGES" ] && [ "$LIST" = 0 ]; then
    c=""; [ "$COM" = 1 ] && c=--com
    picked=$(UNISACC_FFI_X86_PROVIDER=${UNISACC_FFI_X86_PROVIDER:-LIST} GATE_BOUND=1 "$0" --list $c |
             "${MODEL_COM:-$R/unisacc.com}" -run "$R/tests/stagemap.c" -- --stage $STAGES) || { echo 'gate: stagemap failed' >&2; exit 2; }
    [ -n "$picked" ] || { echo "gate: no suites in stage(s)$STAGES" >&2; exit 2; }
    SELECT="$SELECT $(echo $picked)"
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
# Timing sidecars live in $O.timing (outside $O, which holds exactly the job results, so gate counts stay right).
# Lifecycle: the gate never deletes $O.timing; it outlives the run, like $O.timing from an earlier run, and is
# removed by hand (rm -rf "$O.timing" for the run, or rm -rf /tmp/tmp.*.timing). Keep it only while diagnosing.
# Snapshots (<job>.snapshot: ps -eo pid,stat,etime,args + uptime) are diagnostics: they list other processes
# and their arguments.  Never upload them as CI artifacts and never send them outside the host.
TD="$O.timing"; mkdir -p "$TD"   # timing sidecars: outside $O, which holds exactly the job results
# monotonic seconds for every duration (CLOCK_MONOTONIC, not moved by a wall-clock step).
# The wall clock (date +%s) is only recorded beside it, for comparison.  A 440 s wall reading against 37 s monotonic is a step.
mono() { python3 -c 'import time;print("%.3f" % time.monotonic())'; }   # CLOCK_MONOTONIC, the same clock gatesummary.sh subtracts from
TC=$(ls tests/c/*.c)
n=0; PIDS=
job() {   # job NAME ENV... -- CMD...: queued, JOBS at a time
    name=$1; shift
    if [ "$LIST" = 1 ]; then echo "$name"; return; fi
    if [ "$LIST" = 2 ]; then printf '%s\0' "$name" "$#" "$@"; return; fi
    if [ "$LIST" = 3 ]; then
        # Executing host and emitted target are separate obligations.
        case $name in
          hostcheck|stagelog|exittable|h1source|seedmemory|attemptchain|c99precheck|rulingwait|namedrefuse|checkrun|commitgate|queuestart|queuestart-strict|queuestart-launch|queuetimeout|gatesummary|observation|gqalive|queuetimer|docedit|whyrerun|draftfetch|draftguard) req='any|host|python3|cdx|selfcheck';;
          seedparse2-1|seedparse2-2|seedgen|seedgen-2|seedgen-3|seedgen-4|com-seedgen) req="native-posix|host|seed-memory:$name|cdx|fresh-memory-admission";;
          csmithdiff-*|com-csmithdiff-*) req='any|host|csmith|cdx|csmith-fixed-seed-compile';;
          lib-stack-arm) req='darwin-arm64|osx/arm64|cc-arch-arm64|cdx|native-stack-bridge';;
          lib-stack-x86|lib-stack-x86-hostabi) req='darwin-x86_64|osx/x86_64|cc-arch-x86_64|cdx|native-stack-bridge';;
          lib-windows-gp) req='any|win/x86_64,win/arm64|clang-coff,llvm-objcopy|cdx|coff-declarations';;
          lib-windows-gp-native) req='darwin-x86_64|win/x86_64|clang-arch-x86_64|cdx|native-msabi-bridge';;
          exec-bootstrap-osxarm) req='darwin-arm64|osx/arm64|cc|cdx|native-bootstrap';;
          exec-bootstrap-osxx86) req='darwin-x86_64|osx/x86_64|cc|cdx|native-bootstrap';;
          exec-native-*) req='native-posix|host|cc|cdx|native-network-runtime';;
          seedgen-let-exit) req='native-posix|host|cc,python3|cdx2|seed-let-exit-byte-pair';;
          seedgen-opt) req='native-posix|host|cc,python3|cdx|seed-gen-opt-byte-pair';;
          unisa-bits) req='native-posix|host|cc,python3|cdx|unisa-bits-byte-pair';;
          unisa-hostabi) req='native-posix|host|cc,python3|cdx|unisa-hostabi-tsv-sample';;
          unisa-peversion) req='native-posix|host|cc,python3|cdx|unisa-peversion-point-sample';;
          unisa-padded) req='native-posix|host|cc,python3|cdx|unisa-padded-point-sample';;
          unisa-zdata) req='native-posix|host|cc,python3|cdx|unisa-zdata-script-sample';;
          unisa-linalg) req='native-posix|host|cc,python3|cdx|unisa-linalg-point-sample';;
          unisa-libneed) req='native-posix|host|cc,python3|cdx|unisa-libneed-point-sample';;
          unisa-uns1) req='native-posix|host|cc,python3|cdx|unisa-uns1-point-sample';;
          unisa-catalog) req='native-posix|host|cc,python3|cdx|unisa-catalog-point-sample';;
          unisa-fp) req='native-posix|host|cc,python3|cdx|unisa-fp-point-sample';;
          unisa-tape) req='native-posix|host|cc,python3|cdx|unisa-tape-point-sample';;
          unisa-tapebin) req='native-posix|host|cc,python3|cdx|unisa-tapebin-point-sample';;
          unisa-tapebin-shape) req='native-posix|host|cc,python3|cdx|unisa-tapebin-shape-point-sample';;
          unisa-image-fat) req='native-posix|host|cc,python3|cdx|unisa-image-fat-point-sample';;
          unisa-image-elf) req='native-posix|host|cc,python3|cdx|unisa-image-elf-point-sample';;
          unisa-image-macho) req='native-posix|host|cc,python3|cdx|unisa-image-macho-point-sample';;
          unisa-image-pe) req='native-posix|host|cc,python3|cdx|unisa-image-pe-point-sample';;
          unisa-ape) req='native-posix|host|cc,python3|cdx|unisa-ape-point-sample';;
          unisa-vm) req='native-posix|host|cc,python3|cdx|unisa-vm-point-sample';;
          unisa-gold-prec) req='native-posix|host|cc,python3|cdx|unisa-gold-prec-point-sample';;
          unisa-emit-arm-nwmem) req='native-posix|host|cc,python3|cdx|unisa-emit-arm-nwmem-point-sample';;
          unisa-gold-lex-label) req='native-posix|host|cc,python3|cdx|unisa-gold-lex-label-point-sample';;
          unisa-gold-type-label) req='native-posix|host|cc,python3|cdx|unisa-gold-type-label-point-sample';;
          unisa-gold-binsel-label) req='native-posix|host|cc,python3|cdx|unisa-gold-binsel-label-point-sample';;
          unisa-gold-opinfo-label) req='native-posix|host|cc,python3|cdx|unisa-gold-opinfo-label-point-sample';;
          unisa-gold-peep-label) req='native-posix|host|cc,python3|cdx|unisa-gold-peep-label-point-sample';;
          unisa-gold-scope-label) req='native-posix|host|cc,python3|cdx|unisa-gold-scope-label-point-sample';;
          unisa-gold-pp-label) req='native-posix|host|cc,python3|cdx|unisa-gold-pp-label-point-sample';;
          unisa-gold-reloc-label) req='native-posix|host|cc,python3|cdx|unisa-gold-reloc-label-point-sample';;
          unisa-gold-irsel-label) req='native-posix|host|cc,python3|cdx|unisa-gold-irsel-label-point-sample';;
          unisa-gold-parse-label) req='native-posix|host|cc,python3|cdx|unisa-gold-parse-label-point-sample';;
          seedgen-pp-1|seedgen-pp-2) req='native-posix|host|cc,python3|cdx|seed-gen-pp-byte-pair';;
          seedgen-ppconsumer-1-*|seedgen-ppconsumer-2-*) req='native-posix|host|cc,python3|cdx|seed-gen-pp-consumer';;
          seedgen-ppchain-1|seedgen-ppchain-1-*|seedgen-ppchain-2|seedgen-ppchain-3) req='native-posix|host|cc,python3|cdx|seed-gen-pp-chain';;
          seedgen-pprun) req='native-posix|host|cc,python3|cdx|seed-gen-pp-run';;
          seedgen-optconsumer-1|seedgen-optconsumer-2|seedgen-optconsumer-3) req='native-posix|host|cc,python3|cdx|seed-gen-opt-consumer';;
          seedgen-pruneconsumer-1|seedgen-pruneconsumer-2|seedgen-pruneconsumer-3) req='native-posix|host|cc,python3|cdx|seed-gen-prune-consumer';;
          seedgen-parse2consumer-1|seedgen-parse2consumer-2|seedgen-parse2consumer-3-prepare|seedgen-parse2consumer-3) req='native-posix|host|cc,python3|cdx|seed-gen-parse2-consumer';;
          lib-windows-imports) req='native-posix|win-export-synthetic|cc|cdx|native-export-probe';;
          lib-lifecycle|lib-callable-catalog) req='native-posix|host|cc,libffi|cdx|native-ffi-probe';;
          seedgen-construct-prep) req='native-posix|host|python3|cdx|seed-gen-shared-build';;
          elfobj|com-elfobj) req='native-posix|host,osx/*,win/*,lnx/arm64-lima|cc,python3|cdx|elf-macho-coff-link-and-lima-run-cells';;
          forward|com-forward) req='darwin-arm64|osx/arm64|cc|cdx|macos-run-host-forwarding';;
          ccinterop) req='native-posix|host,osx/arm64,lnx/arm64-lima|cc|cdx|cc-interop-link-cells';;
          syscall6) req='native-posix|host,lnx/arm64-lima|cc|cdx|syscall6-gate-cells';;
          hosthdr) req='native-posix|host,lnx/arm64-lima|cc|cdx|hosthdr-reference-and-lima-cells';;
          realprog) req='native-posix|host|cc|cdx|real-program-reference-build';;
          windows-resolver-host) req='any|win/x86_64,win/arm64|zig|cdx|windows-sdk-cross-build';;
          *) req='UNKNOWN|UNKNOWN|UNKNOWN|unassigned|UNKNOWN';;
        esac
        printf '%s\0%s\0' "$name" "$req"; return
    fi
    case " $SELECT " in *" $name "*) ;; *) return;; esac
    # 0.0.37: count live children by PID -- `jobs -rp` is bash-only and, inside $(...), dash
    # sees no jobs at all, so on a Linux /bin/sh every suite started at once (cloud host).
    while :; do
        live=; c=0
        for p in $PIDS; do kill -0 "$p" 2>/dev/null && { live="$live $p"; c=$((c+1)); }; done
        PIDS=$live; [ "$c" -lt "$JOBS" ] && break; sleep 0.1
    done
    n=$((n+1)); f="$O/$(printf %03d $n).$name"
    # Timing sidecar (research/c43-watchdog-402s.md section 4): start, bound return, the subshell's end, and a
    # snapshot only when the job is still alive at GATE_SNAPSHOT_SECS (default 70; the job bound is 60).  The
    # job file keeps its one-line contract (tests/gatesummary.sh reads it); the timing goes to $f.timing.
    ( t0=$(mono); w0=$(date +%s); tb_=$(basename "$f"); tmp="$TD/$tb_.run"
      python3 "$R/tests/bound.py" 60 env "$@" > "$tmp" 2>&1 & jp=$!
      ( sp=; trap '[ -n "$sp" ] && kill "$sp" 2>/dev/null; exit 0' TERM
        sleep "${GATE_SNAPSHOT_SECS:-70}" & sp=$!; wait "$sp"
        if kill -0 "$jp" 2>/dev/null; then { ps -eo pid,stat,etime,args; echo "uptime: $(uptime)"; } > "$TD/$tb_.snapshot" 2>&1; fi ) & wp=$!
      wait "$jp"; rc=$?; tb=$(mono); wb=$(date +%s)
      kill "$wp" 2>/dev/null; wait "$wp" 2>/dev/null
      out=$(cat "$tmp")
      printf '%-14s rc=%-3s %3ss :: %s\n' "$name" "$rc" "$(awk "BEGIN{printf \"%d\", $tb - $t0}")" \
          "$(printf '%s' "$out" | grep -v '^ *$' | tail -1)" > "$f"
      printf 'start=%s bound_return=%s bound_rc=%s subshell_end=%s wall_start=%s wall_return=%s\n' "$t0" "$tb" "$rc" "$(mono)" "$w0" "$wb" > "$TD/$tb_.timing"
      rm -f "$tmp" ) &
    PIDS="$PIDS $!"
}
T0=$(mono)
# longest first, so the tail of the run is short
job tools-1     env SHARD=1/3 ./tests/tools.sh   # 0.0.34 rc4: the single job ran 42.9-51.1 s against 51
job tools-2     env SHARD=2/3 TOOLS_STAGED=split ./tests/tools.sh   # 0.0.35 P10: 2/3 was 33 s; 0.0.43: 104.5 s at load 16 with regex2, which now runs staged (below)
# 0.0.43 timeout debt: a staged entry (tools.sh --staged) is three jobs with a content-checked handoff:
# cc reference (~20 s), build (~21 s), run (~39 s at load 8-10; exclusive in release.sh; after both, tests/gateorder.json)
for e in $(./tests/tools.sh --staged); do for step in ref build run; do job tools-$e-$step ./tests/tools.sh --stage $step $e; done; done
job tools-3     env SHARD=3/3 ./tests/tools.sh
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
for k in 1 2 3 4 5; do job declmatrix-$k SHARD=$k/5 sh ./tests/cxbin tests/declmatrix.cx; done   # 0.0.22 TDD: declarator forms x positions vs cc (found four reference defects) (.cx port, 2026-10-23)
job finite-template python3 ./tests/finitetemplatecheck.py   # 0.0.23 K: parameterised templates (chains, nested loops, rename rewrites PUSH)
job decision-ledger tests/bound 58 sh ./tests/cxbin tests/decisionledger.cx   # (.cx port, 2026-10-23) 0.0.23 I: build-time Python control (direct transition sites) may only fall
job zeropython tests/bound 58 sh ./tests/cxbin tests/zeropython.cx --check   # (2026-10-23) owner's hard goal, zero Python (prd.md §1): python call sites, unisa importers and .py files may only fall against tests/zeropython.tsv
job zeropython-selftest tests/bound 58 sh ./tests/cxbin tests/zeropython.cx --selftest   # the matcher on fixed snippets; seeded rises (a file, a total, a new .py, a new data line) must fail
job manifest-entries-pp tests/bound 58 sh ./tests/cxbin tests/manifestentries.cx --stage pp   # (.cx port, 2026-10-23)
job manifest-entries-lex tests/bound 58 sh ./tests/cxbin tests/manifestentries.cx --stage lex
job manifest-entries-parse2 tests/bound 58 sh ./tests/cxbin tests/manifestentries.cx --stage parse2
job manifest-entries-enc tests/bound 58 sh ./tests/cxbin tests/manifestentries.cx --stage enc
job manifest-entries-enc-arm tests/bound 58 sh ./tests/cxbin tests/manifestentries.cx --stage enc/arm
job manifest-entries-lower tests/bound 58 sh ./tests/cxbin tests/manifestentries.cx --stage lower
job dsl-ops python3 ./tests/dslops.py   # T2: one independent contract and mutation for each manifest op
job declshape bash -c 'R=$PWD; export UA=${UA:-/tmp/ua_ref}; . tests/lib.sh && ua_ready && python3 ./tests/declshape.py'   # 0.0.22 (cdx): declarator metamorphisms -- equivalent spellings agree
job volatile-comma bash -c 'R=$PWD; export UA=${UA:-/tmp/ua_ref}; . tests/lib.sh && ua_ready && tests/bound 58 sh ./tests/cxbin tests/volatilecomma.cx'   # (.cx port, 2026-10-23) V1 traced reads: six g_observed reads at -O0..-O2 in the reference tape VM (tests/cxlib/tapevm.cx) + system-cc load
job hostcheck python3 ./tests/hostcheckcheck.py
job seedmemory tests/bound 58 sh ./unisacc.com -run tests/seedmemory.cx -- --selftest   # (.cx port, 2026-10-23) resource admission negative cases (17 seedmemorycheck.py cases), never starts a generator; nested gate seedgen-3 must be rc=77
job seedgen-let-exit tests/bound 58 sh ./unisacc.com -run tests/seedletexitcheck.cx   # (.cx port, 2026-10-23) generic constructor exit before write, C/P byte parity
job copy-contract tests/bound 58 sh ./unisacc.com -run tests/copycontractcheck.cx   # (.cx port, 2026-10-23) copy/copy-state/rule:MODE C/P parity over tests/cpparity/cases.tsv families copy,mode (drop-edge live); P = exec/build/gen.py + exec/c/tbl.py, C = seed/gen.c
job ri-snapshot-overlay sh ./tests/risnapshotcheck.sh   # RI snapshot overlay: sealed E3 accept hit + fall-through visible in capture
job ri-snapshot-core sh ./tests/risnapshotcore.sh   # RI snapshot writer/loader: positive, refusals, capacity, write failure
# c43 watchdog scope tokens: orphaned and escaped children, and the caller EOF (macOS branch not verified yet)
job watchdog-orphan sh ./tests/watchdogregression.sh orphan tests/bound.py
job watchdog-eof sh ./tests/watchdogregression.sh eof tests/bound.py
job watchdog-success sh ./tests/watchdogregression.sh success tests/bound.py
job stagelog python3 ./tests/stagelogcheck.py
job exittable python3 ./tests/exittablecheck.py
job gatequeue-symlink python3 ./tests/gatequeuesymlinkcheck.py   # tracked-symlink input identity (120000, never followed)
job h1source python3 ./tests/h1sourcecheck.py   # 0.0.40 (机房主任 22:45 A/C): build_candidate settles its H1 table before any write or make
job attemptchain python3 ./tests/attemptchaincheck.py   # 0.0.39 WF5: offline attempt-chain replay
job c99precheck env LC_ALL=C python3 ./tests/c99precheckcheck.py   # 0.0.39 WF1: dynamic-route PRECHECK parsing and shard map (LC_ALL=C, H1: the bash glob order it compares against byte-sorted shards is the locale's collation)
job rulingwait python3 ./tests/rulingwaitcheck.py   # 0.0.39 WF4: ruling request ledger and waits
job namedrefuse ./tests/namedrefusecheck.sh   # 0.0.39 COV1: named refusals exact at every -O, REVIVED fails
job identity-product-negative tests/bound 58 sh ./tests/cxbin tests/identitynegativecheck.cx product --candidate "${MODEL_COM:-./unisacc.com}"   # (.cx port, 2026-10-23)
job identity-include-negative tests/bound 58 sh ./tests/cxbin tests/identitynegativecheck.cx include   # (.cx port, 2026-10-23)
job identity-include-consumer-negative tests/bound 58 sh ./unisacc.com -run tests/identityincludeconsumercheck.cx -- --package "${MODEL_COM:-./unisacc.com}"
job identity-prep-negative tests/bound 58 sh ./tests/cxbin tests/identitynegativecheck.cx prep --ua "${UA:-/tmp/ua_ref}"   # (.cx port, 2026-10-23)
job checkrun ./tests/checkruncheck.sh   # 0.0.39: check status never taken from a pipe
job commitgate ./tests/commitgatecheck.sh   # 0.0.39: red gate leaves no commit (scratch repo)
job k2b K2B_UA=${UA:-/tmp/ua_ref} python3 ./tests/k2bcheck.py   # 0.0.40 K2b: narrow whitelist, whole-tree fallbacks, missed-run RED (scratch copy)
job precommithook ./tests/precommithookcheck.sh   # 0.0.40-prep: local pre-commit routes shipping paths through commitgate (scratch repo)
job queuestart ./tests/queuestartcheck.sh   # 0.0.40: default fresh refuses backup auto-restore; resume is explicit+receipt-bound (fixture)
job queuestart-strict python3 ./tests/queuestartcheck.py   # 0.0.40: fresh refuses a live state, resume refuses malformed/foreign/unknown sources, no half copy
job envpin python3 ./tests/envpincheck.py   # c41: CURSOR_/AGENT_ shell drift does not change stamp identity when env-pin is loaded
job queuestart-launch ./tests/queuestartlaunchcheck.sh   # 0.0.40: the real queue.sh entry refuses a wrong start with zero window launches (fake launcher)
job queuetimeout ./tests/queuetimeoutcheck.sh   # 0.0.40: window timeouts (142/124) at the real queue.sh never turn green; state kept (fake launcher)
job gatesummary ./tests/gatesummarycheck.sh   # 0.0.40: the gate summary reads only the leading suite rc field (fixture result lines)
job observation ./tests/observationcheck.sh   # 0.0.40: RELEASE_SUITES observation scope refused (66) before scheduling; never acceptance
job gqalive ./tests/gqalivecheck.sh   # 0.0.40: queue.sh waits only on processes running gatequeue.py; unreadable table = refuse
job queuetimer ./tests/queuetimercheck.sh   # 0.0.40: a real term.sh/bound.py alarm ends a queue window with 142 (fixture window body)
job docedit tests/bound 58 sh ./unisacc.com -run tests/doceditcheck.cx   # (.cx port, 2026-10-22) 0.0.40: ledger appends keep the old text as an exact prefix; the 0586f01a truncating write is RED
job whyrerun python3 ./tests/whyreruncheck.py   # 0.0.39: invalidation explainer (reasons per suite)
job draftfetch python3 ./tests/draftfetchcheck.py   # 0.0.39 R3: draft court fetch identity checks
job draftguard tests/bound 58 sh ./unisacc.com -run tests/draftguardcheck.cx   # (.cx port, 2026-10-23) 0.0.39 R3: workflow draft-input guard executed
job warmup ./tests/warmupcheck.sh   # 0.0.38: warm-up markers only for a built cache; COLD named
job installpair ./tests/installpaircheck.sh   # 0.0.38: queue root pair = comboot's reinstall when same origin
for k in $(seq 1 40); do job csmithdiff-$k env -u MODEL_COM SEEDS=1-500 SHARD=$k/40 sh ./tests/cxbin tests/csmithdiff.cx; done   # (.cx port, 2026-10-23) 0.0.31: reference only; com-csmithdiff runs the product (both ran it: 677 s duplicated per queue)   # 0.0.22 TDD: fixed Csmith seeds 1-200, reference vs cc (found 7 reference defects)
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
# The former first lane is source work, not model preparation. Keep its ordered
# input union while dividing its source workload into three balanced contiguous
# shards. This leaves margin for the queue's shorter per-suite window.
CLOSURE_C1=$(printf '%s\n' "$TC" | awk 'NR%4==1')
CLOSURE_C1_N=$(printf '%s\n' "$CLOSURE_C1" | wc -l)
CLOSURE_C1_A=$(( (CLOSURE_C1_N + 2) / 3 ))
CLOSURE_C1_B=$(( (2 * CLOSURE_C1_N + 2) / 3 ))
job closure-c1a ./tests/closure.sh $(printf '%s\n' "$CLOSURE_C1" | awk -v a="$CLOSURE_C1_A" 'NR<=a')
job closure-c1b ./tests/closure.sh $(printf '%s\n' "$CLOSURE_C1" | awk -v a="$CLOSURE_C1_A" -v b="$CLOSURE_C1_B" 'NR>a && NR<=b')
job closure-c1c ./tests/closure.sh $(printf '%s\n' "$CLOSURE_C1" | awk -v b="$CLOSURE_C1_B" 'NR>b')
CLOSURE_C2=$(printf '%s\n' "$TC" | awk 'NR%4==2')
CLOSURE_C2_N=$(printf '%s\n' "$CLOSURE_C2" | wc -l)
job closure-c2a ./tests/closure.sh $(printf '%s\n' "$CLOSURE_C2" | awk -v n="$CLOSURE_C2_N" 'NR>int(n*0/3) && NR<=int(n*1/3)')
job closure-c2b ./tests/closure.sh $(printf '%s\n' "$CLOSURE_C2" | awk -v n="$CLOSURE_C2_N" 'NR>int(n*1/3) && NR<=int(n*2/3)')
job closure-c2c ./tests/closure.sh $(printf '%s\n' "$CLOSURE_C2" | awk -v n="$CLOSURE_C2_N" 'NR>int(n*2/3) && NR<=int(n*3/3)')
CLOSURE_C3=$(printf '%s\n' "$TC" | awk 'NR%4==3')
CLOSURE_C3_N=$(printf '%s\n' "$CLOSURE_C3" | wc -l)
job closure-c3a-prepare ./tests/closure.sh --prepare $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*0/3) && NR<=int(n*1/3)')
job closure-c3a-lnx-x86_64 ./tests/closure.sh --target lnx/x86_64 --require-cached $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*0/3) && NR<=int(n*1/3)')
job closure-c3a-lnx-arm64 ./tests/closure.sh --target lnx/arm64 --require-cached $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*0/3) && NR<=int(n*1/3)')
job closure-c3a-osx-x86_64 ./tests/closure.sh --target osx/x86_64 --require-cached $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*0/3) && NR<=int(n*1/3)')
job closure-c3a-osx-arm64 ./tests/closure.sh --target osx/arm64 --require-cached $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*0/3) && NR<=int(n*1/3)')
job closure-c3a-win-x86_64 ./tests/closure.sh --target win/x86_64 --require-cached $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*0/3) && NR<=int(n*1/3)')
job closure-c3a-win-arm64 ./tests/closure.sh --target win/arm64 --require-cached $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*0/3) && NR<=int(n*1/3)')
job closure-c3b-prepare ./tests/closure.sh --prepare $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*1/3) && NR<=int(n*2/3)')
job closure-c3b-lnx-x86_64 ./tests/closure.sh --target lnx/x86_64 --require-cached $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*1/3) && NR<=int(n*2/3)')
job closure-c3b-lnx-arm64 ./tests/closure.sh --target lnx/arm64 --require-cached $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*1/3) && NR<=int(n*2/3)')
job closure-c3b-osx-x86_64 ./tests/closure.sh --target osx/x86_64 --require-cached $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*1/3) && NR<=int(n*2/3)')
job closure-c3b-osx-arm64 ./tests/closure.sh --target osx/arm64 --require-cached $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*1/3) && NR<=int(n*2/3)')
job closure-c3b-win-x86_64 ./tests/closure.sh --target win/x86_64 --require-cached $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*1/3) && NR<=int(n*2/3)')
job closure-c3b-win-arm64 ./tests/closure.sh --target win/arm64 --require-cached $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*1/3) && NR<=int(n*2/3)')
job closure-c3c-prepare ./tests/closure.sh --prepare $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*2/3) && NR<=int(n*3/3)')
job closure-c3c-lnx-x86_64-p1 ./tests/closure.sh --target lnx/x86_64 --require-cached --source-part 1/3 --source-cuts 10,20 $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*2/3) && NR<=int(n*3/3)')
job closure-c3c-lnx-x86_64-p2 ./tests/closure.sh --target lnx/x86_64 --require-cached --source-part 2/3 --source-cuts 10,20 $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*2/3) && NR<=int(n*3/3)')
job closure-c3c-lnx-x86_64-p3 ./tests/closure.sh --target lnx/x86_64 --require-cached --source-part 3/3 --source-cuts 10,20 $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*2/3) && NR<=int(n*3/3)')
job closure-c3c-lnx-arm64 ./tests/closure.sh --target lnx/arm64 --require-cached $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*2/3) && NR<=int(n*3/3)')
job closure-c3c-osx-x86_64 ./tests/closure.sh --target osx/x86_64 --require-cached $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*2/3) && NR<=int(n*3/3)')
job closure-c3c-osx-arm64 ./tests/closure.sh --target osx/arm64 --require-cached $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*2/3) && NR<=int(n*3/3)')
job closure-c3c-win-x86_64 ./tests/closure.sh --target win/x86_64 --require-cached $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*2/3) && NR<=int(n*3/3)')
job closure-c3c-win-arm64 ./tests/closure.sh --target win/arm64 --require-cached $(printf '%s\n' "$CLOSURE_C3" | awk -v n="$CLOSURE_C3_N" 'NR>int(n*2/3) && NR<=int(n*3/3)')
CLOSURE_C4=$(printf '%s\n' "$TC" | awk 'NR%4==0')
CLOSURE_C4_N=$(printf '%s\n' "$CLOSURE_C4" | wc -l)
job closure-c4a-prepare ./tests/closure.sh --prepare $(printf '%s\n' "$CLOSURE_C4" | awk -v n="$CLOSURE_C4_N" 'NR>int(n*0/3) && NR<=int(n*1/3)')
job closure-c4a-lnx-x86_64 ./tests/closure.sh --target lnx/x86_64 --require-cached $(printf '%s\n' "$CLOSURE_C4" | awk -v n="$CLOSURE_C4_N" 'NR>int(n*0/3) && NR<=int(n*1/3)')
job closure-c4a-lnx-arm64 ./tests/closure.sh --target lnx/arm64 --require-cached $(printf '%s\n' "$CLOSURE_C4" | awk -v n="$CLOSURE_C4_N" 'NR>int(n*0/3) && NR<=int(n*1/3)')
job closure-c4a-osx-x86_64 ./tests/closure.sh --target osx/x86_64 --require-cached $(printf '%s\n' "$CLOSURE_C4" | awk -v n="$CLOSURE_C4_N" 'NR>int(n*0/3) && NR<=int(n*1/3)')
job closure-c4a-osx-arm64 ./tests/closure.sh --target osx/arm64 --require-cached $(printf '%s\n' "$CLOSURE_C4" | awk -v n="$CLOSURE_C4_N" 'NR>int(n*0/3) && NR<=int(n*1/3)')
job closure-c4a-win-x86_64 ./tests/closure.sh --target win/x86_64 --require-cached $(printf '%s\n' "$CLOSURE_C4" | awk -v n="$CLOSURE_C4_N" 'NR>int(n*0/3) && NR<=int(n*1/3)')
job closure-c4a-win-arm64 ./tests/closure.sh --target win/arm64 --require-cached $(printf '%s\n' "$CLOSURE_C4" | awk -v n="$CLOSURE_C4_N" 'NR>int(n*0/3) && NR<=int(n*1/3)')
job closure-c4b-prepare ./tests/closure.sh --prepare $(printf '%s\n' "$CLOSURE_C4" | awk -v n="$CLOSURE_C4_N" 'NR>int(n*1/3) && NR<=int(n*2/3)')
job closure-c4b-lnx-x86_64 ./tests/closure.sh --target lnx/x86_64 --require-cached $(printf '%s\n' "$CLOSURE_C4" | awk -v n="$CLOSURE_C4_N" 'NR>int(n*1/3) && NR<=int(n*2/3)')
job closure-c4b-lnx-arm64 ./tests/closure.sh --target lnx/arm64 --require-cached $(printf '%s\n' "$CLOSURE_C4" | awk -v n="$CLOSURE_C4_N" 'NR>int(n*1/3) && NR<=int(n*2/3)')
job closure-c4b-osx-x86_64 ./tests/closure.sh --target osx/x86_64 --require-cached $(printf '%s\n' "$CLOSURE_C4" | awk -v n="$CLOSURE_C4_N" 'NR>int(n*1/3) && NR<=int(n*2/3)')
job closure-c4b-osx-arm64 ./tests/closure.sh --target osx/arm64 --require-cached $(printf '%s\n' "$CLOSURE_C4" | awk -v n="$CLOSURE_C4_N" 'NR>int(n*1/3) && NR<=int(n*2/3)')
job closure-c4b-win-x86_64 ./tests/closure.sh --target win/x86_64 --require-cached $(printf '%s\n' "$CLOSURE_C4" | awk -v n="$CLOSURE_C4_N" 'NR>int(n*1/3) && NR<=int(n*2/3)')
job closure-c4b-win-arm64 ./tests/closure.sh --target win/arm64 --require-cached $(printf '%s\n' "$CLOSURE_C4" | awk -v n="$CLOSURE_C4_N" 'NR>int(n*1/3) && NR<=int(n*2/3)')
job closure-c4c-prepare ./tests/closure.sh --prepare $(printf '%s\n' "$CLOSURE_C4" | awk -v n="$CLOSURE_C4_N" 'NR>int(n*2/3) && NR<=int(n*3/3)')
job closure-c4c-lnx-x86_64 ./tests/closure.sh --target lnx/x86_64 --require-cached $(printf '%s\n' "$CLOSURE_C4" | awk -v n="$CLOSURE_C4_N" 'NR>int(n*2/3) && NR<=int(n*3/3)')
job closure-c4c-lnx-arm64 ./tests/closure.sh --target lnx/arm64 --require-cached $(printf '%s\n' "$CLOSURE_C4" | awk -v n="$CLOSURE_C4_N" 'NR>int(n*2/3) && NR<=int(n*3/3)')
job closure-c4c-osx-x86_64 ./tests/closure.sh --target osx/x86_64 --require-cached $(printf '%s\n' "$CLOSURE_C4" | awk -v n="$CLOSURE_C4_N" 'NR>int(n*2/3) && NR<=int(n*3/3)')
job closure-c4c-osx-arm64 ./tests/closure.sh --target osx/arm64 --require-cached $(printf '%s\n' "$CLOSURE_C4" | awk -v n="$CLOSURE_C4_N" 'NR>int(n*2/3) && NR<=int(n*3/3)')
job closure-c4c-win-x86_64 ./tests/closure.sh --target win/x86_64 --require-cached $(printf '%s\n' "$CLOSURE_C4" | awk -v n="$CLOSURE_C4_N" 'NR>int(n*2/3) && NR<=int(n*3/3)')
job closure-c4c-win-arm64 ./tests/closure.sh --target win/arm64 --require-cached $(printf '%s\n' "$CLOSURE_C4" | awk -v n="$CLOSURE_C4_N" 'NR>int(n*2/3) && NR<=int(n*3/3)')
STAGES_ALL=$(printf '%s\n' examples/*.c tests/c/*.c)
STAGES_1=$(printf '%s\n' "$STAGES_ALL" | awk '(NR-1)%3==0')
STAGES_1_N=$(printf '%s\n' "$STAGES_1" | wc -l)
job stages-1a env SHARD= ./tests/stages.sh $(printf '%s\n' "$STAGES_1" | awk -v n="$STAGES_1_N" 'NR>int(n*0/3) && NR<=int(n*1/3)')
job stages-1b env SHARD= ./tests/stages.sh $(printf '%s\n' "$STAGES_1" | awk -v n="$STAGES_1_N" 'NR>int(n*1/3) && NR<=int(n*2/3)')
job stages-1c env SHARD= ./tests/stages.sh $(printf '%s\n' "$STAGES_1" | awk -v n="$STAGES_1_N" 'NR>int(n*2/3) && NR<=int(n*3/3)')
STAGES_2=$(printf '%s\n' "$STAGES_ALL" | awk '(NR-1)%3==1')
STAGES_2_N=$(printf '%s\n' "$STAGES_2" | wc -l)
job stages-2a env SHARD= ./tests/stages.sh $(printf '%s\n' "$STAGES_2" | awk -v n="$STAGES_2_N" 'NR>int(n*0/3) && NR<=int(n*1/3)')
job stages-2b env SHARD= ./tests/stages.sh $(printf '%s\n' "$STAGES_2" | awk -v n="$STAGES_2_N" 'NR>int(n*1/3) && NR<=int(n*2/3)')
job stages-2c env SHARD= ./tests/stages.sh $(printf '%s\n' "$STAGES_2" | awk -v n="$STAGES_2_N" 'NR>int(n*2/3) && NR<=int(n*3/3)')
STAGES_3=$(printf '%s\n' "$STAGES_ALL" | awk '(NR-1)%3==2')
STAGES_3_N=$(printf '%s\n' "$STAGES_3" | wc -l)
job stages-3a env SHARD= ./tests/stages.sh $(printf '%s\n' "$STAGES_3" | awk -v n="$STAGES_3_N" 'NR>int(n*0/3) && NR<=int(n*1/3)')
job stages-3b env SHARD= ./tests/stages.sh $(printf '%s\n' "$STAGES_3" | awk -v n="$STAGES_3_N" 'NR>int(n*1/3) && NR<=int(n*2/3)')
job stages-3c env SHARD= ./tests/stages.sh $(printf '%s\n' "$STAGES_3" | awk -v n="$STAGES_3_N" 'NR>int(n*2/3) && NR<=int(n*3/3)')
job oracle-cache bash -c 'R=$PWD; export UA=${UA:-/tmp/ua_ref}; . tests/lib.sh && ua_ready && tests/bound 58 "$UA" --check-oracle'   # c43: the runtime ask funnel inf() (DENSE cache) answers every question of every stage identically to infer() over the weights, both directions (A-49); closes gold==weights (net-*-seal) == DENSE (this) for all 18 nets; 22694 questions, <1 s
job stages-back ./tests/backstages.sh   # c43: A-33 extended to the full route -- the six back-end nets (opinfo/peep via opt, abi/regmap/reloc via lower, enc) that only a full build asks: every net the Python full route (compile_file + optimise -O2 + lower) asks, the C full build (-O2 -o BIN -v, askedline after bk_build) must also ask; presence not counts (memoisation differs; answer equality is oracle-cache + difftest)
for k in 1 2 3 4 5; do job exec-chain-$k env CHAINSHARD=$k/5 CHAINKEEP=exec/c/keep-chain.txt ./exec/c/chain.sh $(cat exec/c/keep-chain.txt) examples/*.c tests/c/*.c; done   # S-17: all C probes, historical floor, named parity debt; five shards keep each job under 60 s with --jobs 4 (three hit 142 in the 0.0.23 queue)
job exec-macros tests/bound 58 sh ./tests/cxbin exec/pp/macrocheck.cx   # (.cx port, 2026-10-23) macro expansion: reference -E bytes, host cc -E -P tokens, network output = flat-table output, plain and --locations; E2 models from seed/gen.c + seed/tbl.c + seed/net.c (zero Python)
job exec-pp-literals tests/bound 58 sh ./unisacc.com -run exec/pp/literalcheck.cx   # (.cx port, 2026-10-23) literal/#if parity, plain and --locations (the pp-leaf seal and its reseal recipe still measure the .py, which stays)
job exec-pp-pragmas tests/bound 58 sh ./tests/cxbin exec/pp/pragmacheck.cx   # (.cx port, 2026-10-23) push_macro/pop_macro stacks: network tokens = host cc -E -P -nostdinc tokens, 2 named refusals, both formats
job exec-pp-object tests/bound 58 sh ./tests/cxbin exec/pp/objectpredefinecheck.cx   # (.cx port, 2026-10-23) object-mode and standard predefines, each case's resources handed to the flat table as a run --package; three variants check-net
job exec-pp-directives tests/bound 58 sh ./unisacc.com -run exec/pp/directivecheck.cx   # (.cx port, 2026-10-23) R21: live #error, #include limit, stray #endif/#else, string prefixes = reference or named refusal
job exec-r21-e3 python3 ./exec/parse2/r21check.py   # R21: E3 on this week's reference fixes = reference bytes or named refusal (never different bytes)
job exec-pploc tests/bound 58 sh ./unisacc.com -run exec/pp/locationcheck.cx   # (.cx port, 2026-10-23) source-position metadata from actual preprocessing
job exec-lexpos tests/bound 58 sh ./unisacc.com -run exec/lex/positioncheck.cx tests/cxlib/lexdelta.cx   # (.cx port, 2026-10-23) token offsets against reference tpos
job exec-lexloc tests/bound 58 sh ./unisacc.com -run exec/lex/locationcheck.cx tests/cxlib/lexdelta.cx   # (.cx port, 2026-10-23) E2/E1 location envelope
job exec-f1-sidecar tests/bound 58 sh ./unisacc.com -run exec/lex/f1sidecarcheck.cx tests/cxlib/lexdelta.cx   # (.cx port, 2026-10-23) constructor/destructor mark framing
job exec-objectplan python3 ./tests/objectplancheck.py         # ordered ELF symbol plan
job exec-objectpack python3 ./tests/objectpackcheck.py         # target-scoped route declarations
job exec-modelobject python3 ./tests/modelobjectcheck.py       # two ELF architectures against C reference
job exec-objectfacts python3 ./tests/objectfactscheck.py       # object tape facts in delta
job exec-elfobject python3 ./tests/elfobjectdeltacheck.py        # independent ET_REL layout oracle
job exec-unitparse sh ./tests/unitparsecheck.sh              # separate-unit delta and network parity
job exec-parseloc python3 ./exec/parse2/locationcheck.py         # E3 retained maps and tape parity
job exec-f1-attributes python3 ./exec/parse2/f1attributescheck.py --require-cached --ua "$UA" # marked definitions against reference tape
job exec-diag python3 ./exec/parse2/diagnosticcheck.py           # actual reference diagnostic rendering
# Cold parse2 models (30-45 s each) moved out of the seven warning/error checks below:
# warnprep fills the shared content-keyed cache (compilerpack.built_model, digest-checked
# on hit); the checks still self-build on a miss, so queue order does not matter.
for g in 0 1 2 3; do job exec-warnprep-$g python3 ./exec/parse2/warnprep.py $g; done
job exec-warnprep-tools python3 ./exec/parse2/warnprep.py --tools
for part in 1 2 3; do
  job exec-errors-$part python3 ./exec/parse2/errorcheck.py --require-cached --part $part
  job exec-errors-warn-$part python3 ./exec/parse2/errorcheck.py --warnings --require-cached --part $part
done
job exec-returnwarn tests/bound 58 sh ./tests/cxbin exec/parse2/returnwarningcheck.cx --require-cached  # (.cx port, 2026-10-23) first warning kind and FP conditions
job exec-intwarn tests/bound 58 sh ./tests/cxbin exec/parse2/returnwarningcheck.cx --int-conversion
job exec-unusedwarn tests/bound 58 sh ./tests/cxbin exec/parse2/returnwarningcheck.cx --unused
job exec-formatwarn0 tests/bound 58 sh ./tests/cxbin exec/parse2/returnwarningcheck.cx --format --shard 0/2
job exec-formatwarn1 tests/bound 58 sh ./tests/cxbin exec/parse2/returnwarningcheck.cx --format --shard 1/2
# 0.0.43: complete current compiler source -> tape, split at the tables (69 s whole: gen.py parse2 40 s);
# -1 constructs with seed/gen.c (= gen.py bytes), -2 checks the .ready proof by content and runs E1..E3
job exec-e3self-1 env E3_PART=construct ./exec/parse2/selfcheck.sh
job exec-e3self-2 env E3_PART=run ./exec/parse2/selfcheck.sh
job exec-decimal ./exec/parse2/floatconstcheck.sh                # exact literal bits, table/net and host cc
job exec-neg    ./exec/c/neg.sh   # the executors' error paths agree (bad table 2, reject 1)
# S-17 E4: -O1 and -O2 as deltas, four shards (the single job ran 48.8-49.1 s against 49)
job exec-e4-1   env E4STRICT=1 ./exec/opt/check.sh examples/*.c tests/c/a_*.c
job exec-e4-2   env E4STRICT=1 ./exec/opt/check.sh tests/c/b_[a-l]*.c
job exec-e4-4   env E4STRICT=1 ./exec/opt/check.sh tests/c/b_[!a-l]*.c
job exec-e4-3   env E4STRICT=1 ./exec/opt/check.sh tests/c/[!ab]*.c
job exec-e4self env E4STRICT=1 ./exec/opt/check.sh unisacc.c                    # the compiler's own 3.9 MB tape
job exec-prune-0 env PRUNE_PART=0 PRUNE_UA="$UA" tests/bound 58 sh ./tests/cxbin exec/prune/check.cx   # (.cx port, 2026-10-23)
job exec-prune-1 env PRUNE_PART=1 PRUNE_UA="$UA" tests/bound 58 sh ./tests/cxbin exec/prune/check.cx
job exec-prune-2 env PRUNE_PART=2 PRUNE_UA="$UA" tests/bound 58 sh ./tests/cxbin exec/prune/check.cx
job exec-prune-3 env PRUNE_PART=3 PRUNE_UA="$UA" tests/bound 58 sh ./tests/cxbin exec/prune/check.cx
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
job exec-native-resources-prep env NATIVE_PART=resources ./exec/c/nativecheck.sh --prepare
for part in stages chain; do job exec-native-$part env NATIVE_PART=$part ./exec/c/nativecheck.sh; done
job exec-native-resources env NATIVE_PART=resources ./exec/c/nativecheck.sh --require-cached
job exec-net python3 ./exec/c/netcheck.py
job exec-codec ./exec/c/codeccheck.sh
job exec-package ./exec/c/packagecheck.sh
job modelbenchcheck python3 ./tests/modelbenchcheck.py       # performance evidence fails on functional errors
job bound python3 ./tests/boundcheck.py
job boundreason sh -c './tests/bound 20 cc -std=c99 -O2 -Wall -Wextra -Itests tests/boundreason.c -o "${TMPDIR:-/tmp}/boundreason-$$" && ./tests/bound 20 "${TMPDIR:-/tmp}/boundreason-$$"'   # r7: pure composition matrix; each compile and each run is its own bound
job boundstall-r7g python3 ./tests/boundstall_r7g.py . --gate   # r7g: clean stall cases S1-S3 plus setsid, ledger identity harness, C entry
job boundnested-1 sh ./tests/cxbin tests/boundnested.cx . --gate --only 1,2   # (.cx port, 2026-10-23) r7: cases 1-2, C and py longer child (~16 s)
job boundnested-2 sh ./tests/cxbin tests/boundnested.cx . --gate --only 3,4   # (.cx port, 2026-10-23) r7: cases 3-4, py parent and py longer child (~16 s)
job boundnested-3 sh ./tests/cxbin tests/boundnested.cx . --gate --only 5,6   # (.cx port, 2026-10-23) r7: shorter child (case 5), shorter parent (case 6); theoretical 14.5 s
job boundnested-4 sh ./tests/cxbin tests/boundnested.cx . --gate --only 7   # (.cx port, 2026-10-23) r7: grandchild reaped (case 7) plus its pgrep; theoretical 5.25 s
job boundnested-5 sh ./tests/cxbin tests/boundnested.cx . --gate --only 8   # (.cx port, 2026-10-23) r7: independent deadline propagation call; parent N 8, theoretical 10.25 s
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
job staticimage tests/bound 58 sh ./unisacc.com -run tests/staticimagecheck.cx -- "${MODEL_COM:-./unisacc.com}"   # c42: the Linux compiler images in the product are static ELF (one per architecture, PT_LOAD, no PT_INTERP/PT_DYNAMIC); dynamic C, reads headers, never runs the product under test
job staticimage-selftest tests/bound 58 sh ./unisacc.com -run tests/staticimagecheck.cx -- --selftest
job abi-manifest tests/bound 58 sh ./unisacc.com -run tests/abimanifestcheck.cx -- --check   # c42: tests/abi.manifest (exports, host callbacks, gates, ABI tables) equals what the sources derive; dynamic C, reads sources, never runs the product under test
job abi-manifest-selftest tests/bound 58 sh ./unisacc.com -run tests/abimanifestcheck.cx -- --selftest
job early-stage-seal-selftest tests/bound 58 sh ./unisacc.com -run tests/earlystageseal.cx -- --selftest   # c43: data/byte-identity contract fixtures only; no stamp/cache reuse authority
job pp-leaf-seal tests/bound 58 sh ./unisacc.com -run tests/ppleafseal.cx -- --check-core tests/seals/exec-pp-literals .
job pp-leaf-seal-negative tests/bound 58 sh ./unisacc.com -run tests/ppleafseal.cx -- --negative tests/seals/exec-pp-literals .
job pp-leaf-capture-selftest tests/bound 58 sh ./unisacc.com -run tests/ppleafcapture.cx -- --selftest
job lex-e1-seal tests/bound 58 sh ./unisacc.com -run tests/streamseal.cx -- --gate tests/seals/exec-lex-e1 .   # c43: stream cell via streamseal.cx + cell.tsv (gate and reseal-* scripts share one checker)
job e3-seal tests/bound 58 sh ./unisacc.com -run tests/streamseal.cx -- --gate tests/seals/exec-e3 .   # c43: e3 measurement leaf tokens.typed->tape.text (streamseal + cell.tsv)
job lower-seal tests/bound 58 sh ./unisacc.com -run tests/streamseal.cx -- --gate tests/seals/exec-lower .   # c43: lower measurement leaf tape.text->target.text (streamseal + cell.tsv)
job opt-e4-seal tests/bound 58 sh ./unisacc.com -run tests/streamseal.cx -- --gate tests/seals/exec-opt-e4 .
job elf-seal tests/bound 58 sh ./unisacc.com -run tests/streamseal.cx -- --gate tests/seals/exec-elf .
job prune-seal tests/bound 58 sh ./unisacc.com -run tests/streamseal.cx -- --gate tests/seals/exec-prune .
job seal-paradigm-audit tests/bound 58 sh ./unisacc.com -run tests/sealparadigm.cx -- --audit .   # c43: stream/net paradigm registry (manifest+xy+edges); .cx only
job seal-paradigm-selftest tests/bound 58 sh ./unisacc.com -run tests/sealparadigm.cx -- --selftest
job seal-modules tests/bound 58 ./tests/seal-modules.sh --check-only   # c43: same audit via sealparadigm.cx; seal replay stays on lex-e1-seal + net-*-seal (no nested 21× bound)
job edge-e4-peep-seal tests/bound 58 sh ./unisacc.com -run tests/edgeseal.cx -- --gate tests/seals/edge-e4-peep .   # c43: edge key-map seal (edgeseal.cx); reference-route asks.tsv replay + pins
job edge-e2-pp-seal tests/bound 58 sh ./unisacc.com -run tests/edgeseal.cx -- --gate tests/seals/edge-e2-pp .
job edge-e1-lex-seal tests/bound 58 sh ./unisacc.com -run tests/edgeseal.cx -- --gate tests/seals/edge-e1-lex .
job edge-e3-parse-seal tests/bound 58 sh ./unisacc.com -run tests/edgeseal.cx -- --gate tests/seals/edge-e3-parse .
job edge-e2-pp-ablate tests/bound 58 sh ./unisacc.com -run tests/edgeablate.cx -- --gate tests/seals/edge-e2-pp .   # c43: sampled ablation witness (verified e2->pp + e3->parse only)
job regcheck tests/bound 58 sh ./unisacc.com -run tests/regcheck.cx -- --check .   # c43 R1: E3 save/restore frames pair up and partial saves of the value record are declared (tests/regs-e3.tsv)
job regcheck-negative tests/bound 58 sh ./unisacc.com -run tests/regcheck.cx -- --negative .   # c43 R1: dropping szcomma from its save, or a partial-save declaration, must be caught
job statecheck-tool tests/bound 58 sh ./unisacc.com -run tests/statecheck.cx -- --tool "${TMPDIR:-/tmp}/statecheck"   # c43 R0c: builds the C seed constructor (seed/gen.c) once per source digest for the statecheck-* jobs (tests/gateorder.json)
job statecheck-e3 tests/bound 58 sh ./unisacc.com -run tests/statecheck.cx -- --construct "${TMPDIR:-/tmp}/statecheck"   # c43 R0c: E3 frames pair up on the CONSTRUCTED parse2 delta (renames, partials and splits declared in tests/regs-e3.tsv)
job statecheck-e3-warn tests/bound 58 sh ./unisacc.com -run tests/statecheck.cx -- --construct "${TMPDIR:-/tmp}/statecheck" --warnings --errors   # c43 R0c: the same on the --warnings --errors delta (format/printf/if-else warning frames)
job statecheck-negative tests/bound 58 sh ./unisacc.com -run tests/statecheck.cx -- --construct "${TMPDIR:-/tmp}/statecheck" --negative --errors   # c43 R0c: dropped registers and removed declarations on the real frames must be caught
job tdescprobe-oracle tests/bound 58 sh ./unisacc.com -run tests/tdescprobe.cx -- --oracle cc "${TMPDIR:-/tmp}/tdescprobe-oracle"   # 0.0.43 R3: the TDESC size model equals the system cc on every probe
job tdescprobe-classic tests/bound 58 sh ./unisacc.com -run tests/tdescprobe.cx -- --check "$UA" tests/tdescprobe-classic.tsv "${TMPDIR:-/tmp}/tdescprobe-classic" 1/1   # 0.0.43 R3: the classic reference's verdicts, ratcheted (13 known WRONG)
job edge-e3-parse-ablate tests/bound 58 sh ./unisacc.com -run tests/edgeablate.cx -- --gate tests/seals/edge-e3-parse .
job edge-e3-type-seal tests/bound 58 sh ./unisacc.com -run tests/edgeseal.cx -- --gate tests/seals/edge-e3-type .
job edge-e3-scope-seal tests/bound 58 sh ./unisacc.com -run tests/edgeseal.cx -- --gate tests/seals/edge-e3-scope .
job edge-e3-irsel-seal tests/bound 58 sh ./unisacc.com -run tests/edgeseal.cx -- --gate tests/seals/edge-e3-irsel .
job edge-e3-tyinfo-seal tests/bound 58 sh ./unisacc.com -run tests/edgeseal.cx -- --gate tests/seals/edge-e3-tyinfo .
job edge-e3-binsel-seal tests/bound 58 sh ./unisacc.com -run tests/edgeseal.cx -- --gate tests/seals/edge-e3-binsel .
job edge-e3-prec-seal tests/bound 58 sh ./unisacc.com -run tests/edgeseal.cx -- --gate tests/seals/edge-e3-prec .
job edge-e4-opinfo-seal tests/bound 58 sh ./unisacc.com -run tests/edgeseal.cx -- --gate tests/seals/edge-e4-opinfo .
job edge-lower-abi-seal tests/bound 58 sh ./unisacc.com -run tests/edgeseal.cx -- --gate tests/seals/edge-lower-abi .
job edge-lower-regmap-seal tests/bound 58 sh ./unisacc.com -run tests/edgeseal.cx -- --gate tests/seals/edge-lower-regmap .
job edge-lower-reloc-seal tests/bound 58 sh ./unisacc.com -run tests/edgeseal.cx -- --gate tests/seals/edge-lower-reloc .
job edge-lower-enc-seal tests/bound 58 sh ./unisacc.com -run tests/edgeseal.cx -- --gate tests/seals/edge-lower-enc .
job edge-e3-pfconv-witness tests/bound 58 sh tests/edgewitness.sh --gate tests/seals/edge-e3-pfconv .   # c43: product-side edge witness (reference never asks pfconv; -nostdinc builtin printf path, 9/9 keys exercised): frozen asked count + program output bytes + pins
job edge-elf-enc-witness tests/bound 58 sh tests/edgewitness.sh --gate tests/seals/edge-elf-enc .   # c43: product-side edge witness (elf-flow enc asks are product-only; unisa/image never consults the oracle): frozen asked count + program output bytes + pins
job net-prec-seal tests/bound 58 sh ./unisacc.com -run tests/netseal.cx -- --gate tests/seals/exec-net-prec .   # c43: per-net kernel cell leaf seal (net prec alone: kernel/unisa_core.c + kernel/weight.prec.inc beside a one-net shim, host cc, the whole gold domain of weights/gold/prec.tsv re-derived as the cases): frozen bytes + replay, then the negatives; kernel/unisa_model.inc (the self-test's route to weight.pp.inc) is never opened, no pp path or header in its closure
job net-binsel-seal tests/bound 58 sh ./unisacc.com -run tests/netseal.cx -- --gate tests/seals/exec-net-binsel .   # c43 batch1: per-net kernel cell leaf seal (net binsel alone: kernel/unisa_core.c + kernel/weight.binsel.inc beside a one-net shim, host cc, the whole gold domain of weights/gold/binsel.tsv re-derived as the cases): frozen bytes + replay, then the negatives; same table-driven checker as net-prec-seal, no pp path or header in its closure
job net-tyinfo-seal tests/bound 58 sh ./unisacc.com -run tests/netseal.cx -- --gate tests/seals/exec-net-tyinfo .   # c43 batch1: per-net kernel cell leaf seal (net tyinfo alone: kernel/unisa_core.c + kernel/weight.tyinfo.inc beside a one-net shim, host cc, the whole gold domain of weights/gold/tyinfo.tsv re-derived as the cases): frozen bytes + replay, then the negatives; same table-driven checker as net-prec-seal, no pp path or header in its closure
job net-pfconv-seal tests/bound 58 sh ./unisacc.com -run tests/netseal.cx -- --gate tests/seals/exec-net-pfconv .   # c43 batch1: per-net kernel cell leaf seal (net pfconv alone: kernel/unisa_core.c + kernel/weight.pfconv.inc beside a one-net shim, host cc, the whole gold domain of weights/gold/pfconv.tsv re-derived as the cases): frozen bytes + replay, then the negatives; same table-driven checker as net-prec-seal, no pp path or header in its closure
job net-regmap-seal tests/bound 58 sh ./unisacc.com -run tests/netseal.cx -- --gate tests/seals/exec-net-regmap .   # c43 batch1: per-net kernel cell leaf seal (net regmap alone: kernel/unisa_core.c + kernel/weight.regmap.inc beside a one-net shim, host cc, the whole gold domain of weights/gold/regmap.tsv re-derived as the cases): frozen bytes + replay, then the negatives; same table-driven checker as net-prec-seal, no pp path or header in its closure
job net-lex-seal tests/bound 58 sh ./unisacc.com -run tests/netseal.cx -- --gate tests/seals/exec-net-lex .   # c43 batch2: per-net kernel cell leaf seal (net lex alone: kernel/unisa_core.c + kernel/weight.lex.inc beside a one-net shim, host cc, the whole gold domain of weights/gold/lex.tsv re-derived as the cases): frozen bytes + replay, then the negatives; same table-driven checker as net-prec-seal, no pp path or header in its closure
job net-parse-seal tests/bound 58 sh ./unisacc.com -run tests/netseal.cx -- --gate tests/seals/exec-net-parse .   # c43 batch2: per-net kernel cell leaf seal (net parse alone: kernel/unisa_core.c + kernel/weight.parse.inc beside a one-net shim, host cc, the whole gold domain of weights/gold/parse.tsv re-derived as the cases): frozen bytes + replay, then the negatives; same table-driven checker as net-prec-seal, no pp path or header in its closure
job net-type-seal tests/bound 58 sh ./unisacc.com -run tests/netseal.cx -- --gate tests/seals/exec-net-type .   # c43 batch2: per-net kernel cell leaf seal (net type alone: kernel/unisa_core.c + kernel/weight.type.inc beside a one-net shim, host cc, the whole gold domain of weights/gold/type.tsv re-derived as the cases): frozen bytes + replay, then the negatives; same table-driven checker as net-prec-seal, no pp path or header in its closure
job net-scope-seal tests/bound 58 sh ./unisacc.com -run tests/netseal.cx -- --gate tests/seals/exec-net-scope .   # c43 batch2: per-net kernel cell leaf seal (net scope alone: kernel/unisa_core.c + kernel/weight.scope.inc beside a one-net shim, host cc, the whole gold domain of weights/gold/scope.tsv re-derived as the cases): frozen bytes + replay, then the negatives; same table-driven checker as net-prec-seal, no pp path or header in its closure
job net-irsel-seal tests/bound 58 sh ./unisacc.com -run tests/netseal.cx -- --gate tests/seals/exec-net-irsel .   # c43 batch2: per-net kernel cell leaf seal (net irsel alone: kernel/unisa_core.c + kernel/weight.irsel.inc beside a one-net shim, host cc, the whole gold domain of weights/gold/irsel.tsv re-derived as the cases): frozen bytes + replay, then the negatives; same table-driven checker as net-prec-seal, no pp path or header in its closure
job net-isel-seal tests/bound 58 sh ./unisacc.com -run tests/netseal.cx -- --gate tests/seals/exec-net-isel .   # c43 batch2: per-net kernel cell leaf seal (net isel alone: kernel/unisa_core.c + kernel/weight.isel.inc beside a one-net shim, host cc, the whole gold domain of weights/gold/isel.tsv re-derived as the cases): frozen bytes + replay, then the negatives; same table-driven checker as net-prec-seal, no pp path or header in its closure
job net-enc-seal tests/bound 58 sh ./unisacc.com -run tests/netseal.cx -- --gate tests/seals/exec-net-enc .   # c43 batch2: per-net kernel cell leaf seal (net enc alone: kernel/unisa_core.c + kernel/weight.enc.inc beside a one-net shim, host cc, the whole gold domain of weights/gold/enc.tsv re-derived as the cases): frozen bytes + replay, then the negatives; same table-driven checker as net-prec-seal, no pp path or header in its closure
job net-reloc-seal tests/bound 58 sh ./unisacc.com -run tests/netseal.cx -- --gate tests/seals/exec-net-reloc .   # c43 batch2: per-net kernel cell leaf seal (net reloc alone: kernel/unisa_core.c + kernel/weight.reloc.inc beside a one-net shim, host cc, the whole gold domain of weights/gold/reloc.tsv re-derived as the cases): frozen bytes + replay, then the negatives; same table-driven checker as net-prec-seal, no pp path or header in its closure
job net-abi-seal tests/bound 58 sh ./unisacc.com -run tests/netseal.cx -- --gate tests/seals/exec-net-abi .   # c43 batch2: per-net kernel cell leaf seal (net abi alone: kernel/unisa_core.c + kernel/weight.abi.inc beside a one-net shim, host cc, the whole gold domain of weights/gold/abi.tsv re-derived as the cases): frozen bytes + replay, then the negatives; same table-driven checker as net-prec-seal, no pp path or header in its closure
job net-combo-seal tests/bound 58 sh ./unisacc.com -run tests/netseal.cx -- --gate tests/seals/exec-net-combo .   # c43 batch2: per-net kernel cell leaf seal (net combo alone: kernel/unisa_core.c + kernel/weight.combo.inc beside a one-net shim, host cc, the whole gold domain of weights/gold/combo.tsv re-derived as the cases): frozen bytes + replay, then the negatives; same table-driven checker as net-prec-seal, no pp path or header in its closure
job net-opinfo-seal tests/bound 58 sh ./unisacc.com -run tests/netseal.cx -- --gate tests/seals/exec-net-opinfo .   # c43 batch2: per-net kernel cell leaf seal (net opinfo alone: kernel/unisa_core.c + kernel/weight.opinfo.inc beside a one-net shim, host cc, the whole gold domain of weights/gold/opinfo.tsv re-derived as the cases): frozen bytes + replay, then the negatives; same table-driven checker as net-prec-seal, no pp path or header in its closure
job net-peep-seal tests/bound 58 sh ./unisacc.com -run tests/netseal.cx -- --gate tests/seals/exec-net-peep .   # c43 batch2: per-net kernel cell leaf seal (net peep alone: kernel/unisa_core.c + kernel/weight.peep.inc beside a one-net shim, host cc, the whole gold domain of weights/gold/peep.tsv re-derived as the cases): frozen bytes + replay, then the negatives; same table-driven checker as net-prec-seal, no pp path or header in its closure
job middle-flow-e3 tests/bound 58 sh ./unisacc.com -run tests/middleflowcheck.cx -- --sealed-e3 tests/seals/middleflow-step0
job middle-flow-lower-accept tests/bound 58 sh ./unisacc.com -run tests/middleflowcheck.cx -- --sealed-lower tests/seals/middleflow-step0
# S2 (0.0.43): middle-flow ledger assertion: the live resource ledger of the e3 and lower captures equals the sealed resources.tsv
job middle-flow-ledger sh -c 'sh "${MODEL_COM:-./unisacc.com}" -run tests/middleflowcheck.cx -- --ledger "$(dirname "${MODEL_COM:-./unisacc.com}")/compiler.pkg" .'
job stage-contract-e1e3 tests/bound 58 sh ./unisacc.com -run tests/stagecontractcheck.cx -- --fixture tests/seals/stagecontract-e1-e3 --fixture tests/seals/stagecontract-tape-lower --product "${MODEL_COM:-./unisacc.com}"   # c43 S step 0: E1->e3 replay, resource refusal, reset (measured scope)
for k in 1 2 3; do job tapecheck-model-$k ./exec/tapecheck/check.sh $k; done
job exec-hostsymbols-model ./exec/lower/hostsymbolscheck.sh
job exec-librarycontract-model ./exec/librarycontract/check.sh
job exec-librarypartition-model ./exec/librarypartition/check.sh
job exec-librarytypedpartition-model-1 ./exec/librarytypedpartition/check.sh 1
job exec-librarytypedpartition-model-2 ./exec/librarytypedpartition/check.sh 2
job exec-librarytypedpartition-model-3 ./exec/librarytypedpartition/check.sh 3
job exec-librarytypedpartition-model-4 ./exec/librarytypedpartition/check.sh 4
job exec-librarycallsitepartition-model-1 ./exec/librarycallsitepartition/check.sh 1
job exec-librarycallsitepartition-model-2 ./exec/librarycallsitepartition/check.sh 2
job exec-librarycallsitepartition-model-3 ./exec/librarycallsitepartition/check.sh 3
job exec-librarycallsitepartition-model-4 ./exec/librarycallsitepartition/check.sh 4
job exec-librarycallsitepartition-model-5 ./exec/librarycallsitepartition/check.sh 5
job exec-librarycallsitepartition-model-6 ./exec/librarycallsitepartition/check.sh 6
job exec-librarycallsitepartition-model-7 ./exec/librarycallsitepartition/check.sh 7
job exec-librarycallsitepartition-model-8 ./exec/librarycallsitepartition/check.sh 8
job exec-librarycallbackpartition-model-1 ./exec/librarycallbackpartition/check.sh 1
job exec-librarycallbackpartition-model-2 ./exec/librarycallbackpartition/check.sh 2
job exec-librarycallbackpartition-model-3 ./exec/librarycallbackpartition/check.sh 3
job exec-librarycallbackpartition-model-4 ./exec/librarycallbackpartition/check.sh 4
job exec-librarycallbackpartition-model-5 ./exec/librarycallbackpartition/check.sh 5
job exec-librarycallbackpartition-model-6 ./exec/librarycallbackpartition/check.sh 6
job exec-librarycallbackpartition-model-7 ./exec/librarycallbackpartition/check.sh 7
job exec-librarycallbackpartition-model-8 ./exec/librarycallbackpartition/check.sh 8
job exec-librarycallbackpartition-model-9 ./exec/librarycallbackpartition/check.sh 9
job exec-librarycallbackpartition-model-10 ./exec/librarycallbackpartition/check.sh 10
job exec-librarycallbackpartition-model-11 ./exec/librarycallbackpartition/check.sh 11
job exec-librarycallbackpartition-model-12 ./exec/librarycallbackpartition/check.sh 12
job exec-librarycallbackpartition-model-13 ./exec/librarycallbackpartition/check.sh 13
job exec-librarycallbackpartition-model-14 ./exec/librarycallbackpartition/check.sh 14
job exec-librarycallbackpartition-model-15 ./exec/librarycallbackpartition/check.sh 15
job exec-librarycallbackpartition-model-16 ./exec/librarycallbackpartition/check.sh 16
job exec-libraryfunctionimports-model ./exec/parse2/libraryfunctionimportscheck.sh
# Preparation declarations: compose original producers with the existing job executor.
PREP_JOBS=$(python3 - <<'PREP_DECL'
import json,re,shlex
p=json.load(open('tests/prepgroups.json'))
assert p['version']==1
seen=set()
for row in p['groups']:
    name=row['name']
    assert re.fullmatch(r'exec-prepare-[a-z0-9-]+',name) and name not in seen
    seen.add(name)
    command=['sh','-ec','\n'.join(shlex.join(m['argv']) for m in row['members'])]
    assert row['command']==command
    print(shlex.join(['job',name,*command]))
PREP_DECL
) || exit 2
eval "$PREP_JOBS"
unset PREP_JOBS
job exec-librarytypedreader-model ./exec/parse2/librarytypedreadercheck.sh --require-cached
job exec-librarycallback-reader-model sh tests/librarycallbacke3check.sh --require-cached check
job exec-librarycallbacklower-model-1 sh tests/librarycallbacklowercheck.sh --require-cached 1
job exec-librarycallbacklower-model-2 sh tests/librarycallbacklowercheck.sh --require-cached 2
job exec-librarycallbacklower-model-3 sh tests/librarycallbacklowercheck.sh --require-cached 3
job exec-librarycallbacklower-model-4 sh tests/librarycallbacklowercheck.sh --require-cached 4
job lib-callback-candidate-auth sh tests/librarycallbackauthcheck.sh --require-cached check
job exec-librarycallback-wiring-prepare sh tests/librarycallbackwiringcheck.sh --require-cached check
job exec-librarycallsite-reader-model ./exec/parse2/librarycallsitecheck.sh --require-cached
job exec-librarycallsitelower-model-1 ./exec/lower/librarycallsitecheck.sh --require-cached 1
job exec-librarycallsitelower-model-2 ./exec/lower/librarycallsitecheck.sh --require-cached 2
job exec-librarycallsitelower-model-3 ./exec/lower/librarycallsitecheck.sh --require-cached 3
job exec-librarycallsitelower-model-4 ./exec/lower/librarycallsitecheck.sh --require-cached 4
job exec-librarytypedlower-model-1 ./exec/lower/librarytypedcheck.sh --require-cached 1
job exec-librarytypedlower-model-2 ./exec/lower/librarytypedcheck.sh --require-cached 2
job exec-librarytypedlower-model-3 ./exec/lower/librarytypedcheck.sh --require-cached 3
job exec-librarytypedlower-model-4 ./exec/lower/librarytypedcheck.sh --require-cached 4
job exec-libraryfn-model ./exec/lower/libraryfncheck.sh
job exec-librarydata-model ./exec/lower/librarydatacheck.sh --require-cached
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
        # 0.0.32: three parts (qc11: the whole prep took 48.96 s of 49 alone under load 4)
        job exec-bindprep-first-$a env CORE_ASM_ARCH=$a ./exec/c/asm/bindprep.sh first
        job exec-bindprep-second-$a env CORE_ASM_ARCH=$a ./exec/c/asm/bindprep.sh second
        job exec-bindprep-$a env CORE_ASM_ARCH=$a ./exec/c/asm/bindprep.sh package
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
# S3 (0.0.43): driver model preparation as independent jobs (bindprep pattern): prep-0 = elf and o1 chain, prep-1..5 = parts K/5 of PREP_PARTS
# after prep-0; every other exec-driver-* job runs after prep-0..5 and never constructs (timeout debt: prep-3 was 109 s, core-build 116 s cold)
for k in 0 1 2 3 4 5; do job exec-driver-core-prep-$k ./exec/c/compilercheck.sh prep-$k; done
job exec-driver-core-prep-f1json ./exec/c/compilercheck.sh prep-f1json
job exec-driver-resources-macros ./exec/c/compilercheck.sh resources-macros
job exec-driver-resources-headers ./exec/c/compilercheck.sh resources-headers
job exec-driver-resources-printf ./exec/c/compilercheck.sh resources-printf
job exec-driver-resources-isolated ./exec/c/compilercheck.sh resources-isolated
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
# 0.0.33: the self-source stage check split after E3 (53 s whole, 49 s queue limit -> rc142 in cand36/37);
# stages2 verifies the stages1 handoff by content (source, run, models, E3 tape) and fails if stale
job exec-tableself-1 env SELF_PART=stages1 NETWORK=0 TARGET=osx/arm64 ./exec/pipeline/selfcheck.sh
job exec-tableself-2 env SELF_PART=stages2 NETWORK=0 TARGET=osx/arm64 ./exec/pipeline/selfcheck.sh
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
# the fixed source-to-ELF paths of exec/pipeline/keep-elf.txt (96 in 0.0.34), three shards
job exec-srcelf-1 env ELF_SHARD=1/3 ./exec/pipeline/check-elf.sh
job exec-srcelf-2 env ELF_SHARD=2/3 ./exec/pipeline/check-elf.sh
job exec-srcelf-3 env ELF_SHARD=3/3 ./exec/pipeline/check-elf.sh
for k in 1 2 3 4; do job difftest_o-$k SHARD=$k/4 ./tests/difftest_o.sh; done
job warn        ./tests/warn.sh
job diag-units  ./tests/diagunits.sh
job host-triple-u0-prepare ./tests/nativeboot.sh --host-triple-prepare  # preparation only, excluded from the four test gates
for generation in u0 n1 n2 n3; do job host-triple-$generation ./tests/nativeboot.sh --host-triple "$generation"; done
for part in cross-a cross-b; do job nativeboot-$part env NB_PART=$part ./tests/nativeboot.sh; done  # 0.0.31: one window no longer holds all eight compiles  # 0.0.31: one window no longer holds all eight compiles
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
job lib-sig2-host tests/bound 58 sh ./unisacc.com -run tests/librarysig2hostcheck.cx   # (.cx port, 2026-10-23) own synthetic V2 declarations + native libffi closure oracle, native + ASan/UBSan
job lib-typed-host sh ./unisacc.com -run tests/librarynativehostcheck.cx   # (.cx port, 2026-10-23)
job lib-variadic-host tests/bound 58 sh ./tests/cxbin tests/librarynativevariadiccheck.cx   # (.cx port, 2026-10-23)
job lib-callplans-host tests/bound 58 sh ./unisacc.com -run tests/librarycallplanscheck.cx   # (.cx port, 2026-10-23)
job lib-variadic-resolver-host tests/bound 58 sh ./unisacc.com -run tests/libraryvariadicresolvercheck.cx   # (.cx port, 2026-10-23)
job lib-variadic-protocol ./tests/modelvariadicprotocolcheck.sh
job lib-callback-graph-host tests/bound 58 sh ./unisacc.com -run tests/librarycallbackgraphcheck.cx   # (.cx port, 2026-10-23) exact USLSIG2 graph fixtures (tests/cxlib/libprobe.cx), native decoder oracle, normal + ASan/UBSan
job lib-callback-graph-protocol ./tests/modelcallbackgraphcheck.sh
job lib-callback-graph-equality ./tests/modelgraphequalitycheck.sh
job lib-callback-source-graph ./tests/modelcallbacksourcecheck.sh
job lib-callable-mechanism tests/bound 58 sh ./unisacc.com -run tests/librarycallablescheck.cx   # (.cx port, 2026-10-23) callable ABI mechanism: librarycallablescheck.c built with host cc + real libffi, plain and ASan/UBSan, each under tests/bound 20
job lib-carrier-model python3 ./tests/modelnativecarriercheck.py
job lib-ordered-carrier-model python3 ./tests/modelnativebitfieldcheck.py
job referee-binsel python3 ./tests/binsel_audit.py        # external referee: host cc decides signedness-dependence
job referee-pfconv python3 ./tests/pfconv_audit.py        # external referee: host printf decides each conversion
job lib-ordered-carrier-host tests/bound 58 sh ./unisacc.com -run tests/libraryorderedcarriercheck.cx   # (.cx port, 2026-10-23)
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
    job lib-source-bitfield-import-native tests/bound 58 sh ./unisacc.com -run tests/librarysourcebitfieldimportcheck.cx   # (.cx port, 2026-10-23)
    job lib-source-callable-import-native tests/bound 58 sh ./unisacc.com -run tests/librarysourcebitfieldimportcheck.cx -- --mode callable
    job lib-source-callback-import-native tests/bound 58 sh ./unisacc.com -run tests/librarysourcebitfieldimportcheck.cx -- --mode callbacks
    job lib-source-variadic-import-native tests/bound 58 sh ./unisacc.com -run tests/librarysourcebitfieldimportcheck.cx -- --mode variadic
    job lib-source-pointee-import-native tests/bound 58 sh ./unisacc.com -run tests/librarysourcebitfieldimportcheck.cx -- --mode pointee
    job lib-source-longdouble-import-native tests/bound 58 sh ./unisacc.com -run tests/librarysourcebitfieldimportcheck.cx -- --mode longdouble
fi
job lib-carrier-mechanism tests/bound 58 sh ./unisacc.com -run tests/librarycarriercheck.cx   # (.cx port, 2026-10-23)
job lib-carrier-plan python3 ./tests/librarycarrierplancheck.py
job lib-carrier-native-plan sh ./unisacc.com -run tests/librarycarriernativeplancheck.cx   # (.cx port, 2026-10-23)
job lib-carrier-callback tests/bound 58 sh ./unisacc.com -run tests/librarycarriercallbackcheck.cx   # (.cx port, 2026-10-23)
case "$(uname -s)" in
    Linux|Darwin)
        job lib-carrier-import-model-prepare sh ./tests/cxbin tests/carrierimportprepared.cx --prepare   # (.cx port, 2026-10-23)
        job lib-carrier-import-model-import sh ./tests/cxbin tests/carrierimportprepared.cx --require-cached --part import
        job lib-carrier-import-model-package sh ./tests/cxbin tests/carrierimportprepared.cx --require-cached --part package
        ;;
    *)
        job lib-carrier-import-model python3 ./tests/modelnativecarrierimportcheck.py
        ;;
esac
job lib-union-native tests/bound 58 sh ./unisacc.com -run tests/libraryunionnativecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --sanitize   # (.cx port, 2026-10-23)
job lib-carrier-factory tests/bound 58 sh ./unisacc.com -run tests/libraryunionnativecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --sanitize --factory
job lib-carrier-relay tests/bound 58 sh ./unisacc.com -run tests/libraryunionnativecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --sanitize --relay
if [ "$(uname -s)/$(uname -m)" = Linux/x86_64 ]; then
job lib-union-scalar-prepare sh ./tests/cxbin tests/libraryunionscalarcheck.cx --prepare --cases int8,float4,double8,mixed2,mixed3,mixed-reordered --pressures plain,gp9,fp9 --sanitize   # (.cx port, 2026-10-23)
job lib-union-scalar-int8-plain sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --cases int8 --pressures plain --sanitize --require-cached
job lib-union-scalar-int8-gp9 sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --cases int8 --pressures gp9 --sanitize --require-cached
job lib-union-scalar-int8-fp9 sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --cases int8 --pressures fp9 --sanitize --require-cached
job lib-union-scalar-float4-plain sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --cases float4 --pressures plain --sanitize --require-cached
job lib-union-scalar-float4-gp9 sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --cases float4 --pressures gp9 --sanitize --require-cached
job lib-union-scalar-float4-fp9 sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --cases float4 --pressures fp9 --sanitize --require-cached
job lib-union-scalar-double8-plain sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --cases double8 --pressures plain --sanitize --require-cached
job lib-union-scalar-double8-gp9 sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --cases double8 --pressures gp9 --sanitize --require-cached
job lib-union-scalar-double8-fp9 sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --cases double8 --pressures fp9 --sanitize --require-cached
job lib-union-scalar-mixed2-plain sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --cases mixed2 --pressures plain --sanitize --require-cached
job lib-union-scalar-mixed2-gp9 sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --cases mixed2 --pressures gp9 --sanitize --require-cached
job lib-union-scalar-mixed2-fp9 sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --cases mixed2 --pressures fp9 --sanitize --require-cached
job lib-union-scalar-mixed3-plain sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --cases mixed3 --pressures plain --sanitize --require-cached
job lib-union-scalar-mixed3-gp9 sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --cases mixed3 --pressures gp9 --sanitize --require-cached
job lib-union-scalar-mixed3-fp9 sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --cases mixed3 --pressures fp9 --sanitize --require-cached
job lib-union-scalar-mixed-reordered-plain sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --cases mixed-reordered --pressures plain --sanitize --require-cached
job lib-union-scalar-mixed-reordered-gp9 sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --cases mixed-reordered --pressures gp9 --sanitize --require-cached
job lib-union-scalar-mixed-reordered-fp9 sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --cases mixed-reordered --pressures fp9 --sanitize --require-cached
else
job lib-union-scalar-1 sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --cases int8,float4 --sanitize
job lib-union-scalar-2 sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --cases double8,mixed2 --sanitize
job lib-union-scalar-3 sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --cases mixed3,mixed-reordered --sanitize
fi
job lib-union-composite-direct-1 sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases mixed16,fp16 --sanitize   # (.cx port, 2026-10-23)
job lib-union-composite-direct-2 sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases array2,array4 --sanitize
job lib-union-composite-direct-3 sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases array5 --sanitize
job lib-union-composite-callback-1 sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases mixed16,fp16 --sanitize --callback
job lib-union-composite-callback-2 sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases array2,array4 --sanitize --callback
job lib-union-composite-callback-3 sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases array5 --sanitize --callback
# F3 (union-composite) split: one content-keyed runtime preparation, then require-cached variant shards by pressure; the original gates above stay
job lib-union-composite-prepare sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --sanitize --prepare
job lib-union-composite-direct-1-a sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --sanitize --require-cached --cases mixed16,fp16 --pressures plain,gp9
job lib-union-composite-direct-1-b sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --sanitize --require-cached --cases mixed16,fp16 --pressures fp9
job lib-union-composite-callback-1-plain sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --sanitize --require-cached --cases mixed16,fp16 --callback --pressures plain
job lib-union-composite-callback-1-gp9 sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --sanitize --require-cached --cases mixed16,fp16 --callback --pressures gp9
job lib-union-composite-callback-1-fp9 sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --sanitize --require-cached --cases mixed16,fp16 --callback --pressures fp9
job lib-union-composite-direct-2-a sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --sanitize --require-cached --cases array2,array4 --pressures plain,gp9
job lib-union-composite-direct-2-b sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --sanitize --require-cached --cases array2,array4 --pressures fp9
job lib-union-composite-callback-2-plain sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --sanitize --require-cached --cases array2,array4 --callback --pressures plain
job lib-union-composite-callback-2-gp9 sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --sanitize --require-cached --cases array2,array4 --callback --pressures gp9
job lib-union-composite-callback-2-fp9 sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --sanitize --require-cached --cases array2,array4 --callback --pressures fp9
job lib-union-composite-direct-3-req sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --sanitize --require-cached --cases array5
job lib-union-composite-callback-3-plain sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --sanitize --require-cached --cases array5 --callback --pressures plain
job lib-union-composite-callback-3-gp9 sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --sanitize --require-cached --cases array5 --callback --pressures gp9
job lib-union-composite-callback-3-fp9 sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --sanitize --require-cached --cases array5 --callback --pressures fp9
job lib-union16-direct-1 sh ./unisacc.com -run tests/libraryunion16check.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases is8,si8 --sanitize   # (.cx port, 2026-10-23)
job lib-union16-callback-1 sh ./unisacc.com -run tests/libraryunion16check.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases is8,si8 --sanitize --callback
job lib-union16-direct-2 sh ./unisacc.com -run tests/libraryunion16check.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases ss8,ii8 --sanitize
job lib-union16-callback-2 sh ./unisacc.com -run tests/libraryunion16check.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases ss8,ii8 --sanitize --callback
job lib-union16-direct-3 sh ./unisacc.com -run tests/libraryunion16check.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases is4,si4 --sanitize
job lib-union16-callback-3 sh ./unisacc.com -run tests/libraryunion16check.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases is4,si4 --sanitize --callback
job lib-union16-direct-4 sh ./unisacc.com -run tests/libraryunion16check.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases ss4,ii4 --sanitize
job lib-union16-callback-4 sh ./unisacc.com -run tests/libraryunion16check.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases ss4,ii4 --sanitize --callback
job lib-union16-direct-5 sh ./unisacc.com -run tests/libraryunion16check.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases fpmix --sanitize
job lib-union16-callback-5 sh ./unisacc.com -run tests/libraryunion16check.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases fpmix --sanitize --callback
job lib-callable-variadic-mechanism tests/bound 58 sh ./unisacc.com -run tests/librarycallablevarcheck.cx   # (.cx port, 2026-10-23) concrete variadic callable ABI, normal + ASan/UBSan
job lib-callable-catalog tests/bound 58 sh ./unisacc.com -run tests/librarycallablecatalogcheck.cx   # (.cx port, 2026-10-23) USLCALL1/2/3 wire + context-owned graph transactions, normal + ASan/UBSan
job lib-callable-variadic-model ./tests/modelcallablevarwirecheck.sh
job lib-callback-plan sh ./tests/cxbin tests/librarycallbackplancheck.cx   # (.cx port, 2026-10-23) callback declaration/plan probe, normal + ASan/UBSan
job lib-callable-model ./tests/modelcallablewirecheck.sh
job lib-callback-native sh ./unisacc.com -run tests/librarycallbacknativecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --sanitize   # (.cx port, 2026-10-23)
job lib-callback-forward sh ./unisacc.com -run tests/librarycallbackforwardcheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --sanitize   # (.cx port, 2026-10-23)
job lib-callable-variadic-native sh ./tests/cxbin tests/librarycallablevariadicnativecheck.cx --package "${MODEL_COM:-./unisacc.com}" --sanitize   # (.cx port, 2026-10-23)
job lib-callback-outcome sh ./tests/cxbin tests/librarycallbackoutcomecheck.cx --package "${MODEL_COM:-./unisacc.com}"   # (.cx port, 2026-10-23) public-API sticky outcome, normal + ASan/UBSan
job lib-typed-native tests/bound 58 sh ./unisacc.com -run tests/librarytypednativecheck.cx -- --package "${MODEL_COM:-./unisacc.com}"   # (.cx port, 2026-10-23) native->script->typed native fixed FP/GP17/Pair9
job lib-variadic-native tests/bound 58 sh ./tests/cxbin tests/libraryvariadicnativecheck.cx --package "${MODEL_COM:-./unisacc.com}" --sanitize   # (.cx port, 2026-10-23)
job lib-sig2-native tests/bound 58 sh ./unisacc.com -run tests/librarysig2nativecheck.cx -- --package "${MODEL_COM:-./unisacc.com}"   # (.cx port, 2026-10-23) native->script fixed FP/GP17/Pair9
if [ "$(uname -s)" = Darwin ] && [ "$(uname -m)" = arm64 ]; then
    job lib-callback-native-rosetta sh ./unisacc.com -run tests/librarycallbacknativecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --sanitize
    job lib-callback-forward-rosetta sh ./unisacc.com -run tests/librarycallbackforwardcheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --sanitize
    job lib-callable-variadic-native-rosetta sh ./tests/cxbin tests/librarycallablevariadicnativecheck.cx --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --sanitize
    job lib-sig2-rosetta tests/bound 58 sh ./unisacc.com -run tests/librarysig2nativecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --arch x86_64
    job lib-typed-rosetta tests/bound 58 sh ./unisacc.com -run tests/librarytypednativecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --arch x86_64
    job lib-variadic-rosetta tests/bound 58 sh ./tests/cxbin tests/librarynativevariadiccheck.cx --arch x86_64
    job lib-callplans-rosetta tests/bound 58 sh ./unisacc.com -run tests/librarycallplanscheck.cx -- --arch x86_64
    job lib-callback-graph-rosetta tests/bound 58 sh ./unisacc.com -run tests/librarycallbackgraphcheck.cx -- --arch x86_64
    job lib-callback-outcome-rosetta sh ./tests/cxbin tests/librarycallbackoutcomecheck.cx --package "${MODEL_COM:-./unisacc.com}" --arch x86_64
    job lib-callback-plan-rosetta sh ./tests/cxbin tests/librarycallbackplancheck.cx --arch x86_64
    job lib-callable-catalog-rosetta tests/bound 58 sh ./unisacc.com -run tests/librarycallablecatalogcheck.cx -- --arch x86_64
    job lib-source-longdouble-import-rosetta tests/bound 58 sh ./unisacc.com -run tests/librarysourcebitfieldimportcheck.cx -- --mode longdouble --arch x86_64 --ffi-provider "$UNISACC_FFI_X86_PROVIDER"
    job lib-carrier-mechanism-rosetta tests/bound 58 sh ./unisacc.com -run tests/librarycarriercheck.cx -- --arch x86_64
    job lib-carrier-native-plan-rosetta sh ./unisacc.com -run tests/librarycarriernativeplancheck.cx -- --arch x86_64
    job lib-union-native-rosetta tests/bound 58 sh ./unisacc.com -run tests/libraryunionnativecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --sanitize
    job lib-carrier-callback-rosetta tests/bound 58 sh ./unisacc.com -run tests/librarycarriercallbackcheck.cx -- --arch x86_64
    job lib-carrier-factory-rosetta tests/bound 58 sh ./unisacc.com -run tests/libraryunionnativecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --sanitize --factory
    job lib-carrier-relay-rosetta tests/bound 58 sh ./unisacc.com -run tests/libraryunionnativecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --sanitize --relay
    job lib-union-scalar-1-rosetta sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --cases int8,float4 --sanitize
    job lib-union-scalar-2-rosetta sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --cases double8,mixed2 --sanitize
    job lib-union-scalar-3-rosetta sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --cases mixed3,mixed-reordered --sanitize
    job lib-union-scalar-narrow-rosetta sh ./tests/cxbin tests/libraryunionscalarcheck.cx --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --cases int1,int2,int4 --sanitize
    job lib-union-composite-direct-1-rosetta sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --cases mixed16,fp16 --sanitize
    job lib-union-composite-direct-2-rosetta sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --cases array2,array4 --sanitize
    job lib-union-composite-direct-3-rosetta sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --cases array5 --sanitize
    job lib-union-composite-callback-1-rosetta sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --cases mixed16,fp16 --sanitize --callback
    job lib-union-composite-callback-2-rosetta sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --cases array2,array4 --sanitize --callback
    job lib-union-composite-callback-3-rosetta sh ./unisacc.com -run tests/libraryunioncompositecheck.cx -- --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --cases array5 --sanitize --callback
    job lib-union16-direct-1-rosetta sh ./unisacc.com -run tests/libraryunion16check.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases is8,si8 --sanitize --arch x86_64
    job lib-union16-callback-1-rosetta sh ./unisacc.com -run tests/libraryunion16check.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases is8,si8 --sanitize --callback --arch x86_64
    job lib-union16-direct-2-rosetta sh ./unisacc.com -run tests/libraryunion16check.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases ss8,ii8 --sanitize --arch x86_64
    job lib-union16-callback-2-rosetta sh ./unisacc.com -run tests/libraryunion16check.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases ss8,ii8 --sanitize --callback --arch x86_64
    job lib-union16-direct-3-rosetta sh ./unisacc.com -run tests/libraryunion16check.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases is4,si4 --sanitize --arch x86_64
    job lib-union16-callback-3-rosetta sh ./unisacc.com -run tests/libraryunion16check.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases is4,si4 --sanitize --callback --arch x86_64
    job lib-union16-direct-4-rosetta sh ./unisacc.com -run tests/libraryunion16check.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases ss4,ii4 --sanitize --arch x86_64
    job lib-union16-callback-4-rosetta sh ./unisacc.com -run tests/libraryunion16check.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases ss4,ii4 --sanitize --callback --arch x86_64
    job lib-union16-direct-5-rosetta sh ./unisacc.com -run tests/libraryunion16check.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases fpmix --sanitize --arch x86_64
    job lib-union16-callback-5-rosetta sh ./unisacc.com -run tests/libraryunion16check.cx -- --package "${MODEL_COM:-./unisacc.com}" --cases fpmix --sanitize --callback --arch x86_64
    job lib-variadic-resolver-rosetta tests/bound 58 sh ./unisacc.com -run tests/libraryvariadicresolvercheck.cx -- --arch x86_64
    job lib-variadic-native-rosetta tests/bound 58 sh ./tests/cxbin tests/libraryvariadicnativecheck.cx --package "${MODEL_COM:-./unisacc.com}" --arch x86_64 --sanitize
fi
job lib-artifacts python3 ./tests/libraryartifactcheck.py --package "${MODEL_COM:-./unisacc.com}"
job lib-native-exports tests/bound 58 sh ./tests/cxbin tests/libunisaccexportcheck.cx --package "${MODEL_COM:-./unisacc.com}"   # (.cx port, 2026-10-23)
job lib-module-exports tests/bound 58 sh ./tests/cxbin tests/libunisaccexportcheck.cx --package "${MODEL_COM:-./unisacc.com}" --no-main
job lib-bindings python3 ./tests/libunisaccbindingscheck.py --package "${MODEL_COM:-./unisacc.com}"
job lib-data python3 ./tests/libunisaccdatacheck.py --package "${MODEL_COM:-./unisacc.com}"
job lib-resolver python3 ./tests/libunisaccresolvercheck.py --package "${MODEL_COM:-./unisacc.com}"
job lib-latebind-W tests/bound 58 sh "${MODEL_COM:-./unisacc.com}" -run tests/liblatebindWcheck.cx -- "${MODEL_COM:-./unisacc.com}"   # W: private runtime + frozen r4 six snapshots; product is model/runner input
job latebind-p6 tests/bound 58 sh "${MODEL_COM:-./unisacc.com}" -run tests/latebindp6check.cx -- "${MODEL_COM:-./unisacc.com}" tests/seals/latebind-p6-baseline   # P6: untyped library-mode baseline (26 WBASE1 snapshots, public-API observers); lnx/x86_64 else UNVERIFIED 77
job resourceflow tests/bound 58 sh "${MODEL_COM:-$R/unisacc.com}" -run tests/resourceflowcheck.cx -- "${MODEL_COM:-$R/unisacc.com}"   # (H1: the product path must be absolute) S1: explicit-ledger strict resource flow on the product (C -> Tape); RF-LEGACY needs a base product and is not run here
job include-key tests/bound 58 sh ./tests/includekeycheck.sh "${MODEL_COM:-./unisacc.com}"   # S2 prerequisite: sim.py and the product request the same include keys (product capture vs reference)
job lib-resolver-host tests/bound 58 sh ./unisacc.com -run tests/libraryresolverhostcheck.cx   # (.cx port, 2026-10-23) R10: private native resolver candidate enumeration; owned handles close once, in reverse order
job lib-word64 python3 ./tests/librarywordcheck.py --native-only
job exec-binaryio python3 ./tests/runtimeiocheck.py
for part in host unisacc sanitised; do job exec-crcllp64-$part python3 ./tests/crcllp64check.py --ua "${MODEL_COM:-./unisacc.com}" --native-only --part "$part"; done
job lib-windows-bridge python3 ./tests/windowslibrarybridgecheck.py
job lib-windows-imports tests/bound 58 sh "${MODEL_COM:-./unisacc.com}" -run tests/librarywinimportscheck.cx   # (.cx port, 2026-10-23)
job lib-windows-gp python3 ./exec/enc/windowshostbridgecheck.py --part cross
job lib-windows-gp-native python3 ./exec/enc/windowshostbridgecheck.py --part native
job lib-lifecycle python3 ./tests/libunisaccruncheck.py --package "${MODEL_COM:-./unisacc.com}" --iterations 1000
job shared-e2-plain ./tests/sharede2.sh plain
job shared-e2-located ./tests/sharede2.sh located
job shared-e2-tokens ./tests/sharede2.sh tokens
job kernel      ./tests/kernel.sh
job rowcov-pp sh ./tests/cxbin tests/rowcov.cx pp all   # 0.0.25 T2: pp manifest rows reached by the probes; ratchet in tests/rowcov.baseline   # (.cx port, 2026-10-23)
job rowcov-pp-locations sh ./tests/cxbin tests/rowcov.cx pp-locations all   # 0.0.32 T3': the --locations variant graph
job rowcov-pp-shared sh ./tests/cxbin tests/rowcov.cx pp-shared all   # 0.0.32 T3': the --shared-predefines variant graph (predefines as resources)
job rowcov-lex sh ./tests/cxbin tests/rowcov.cx lex all   # 0.0.26 T2: lex manifest rows reached (on the preprocessor output); ratchet in tests/rowcov.baseline
job rowcov-parse2-build sh ./tests/cxbin tests/rowcov.cx parse2 build   # 0.0.31 R4: the shards reuse its cache
for k in $(seq 1 24); do job rowcov-parse2-$k sh ./tests/cxbin tests/rowcov.cx parse2 gate$k/24; done   # 0.0.30 R2: 24 shards (16/12 hit the 55 s window three times in 0.0.29)   # 0.0.26 T2: each shard ratchets its own reached rows
for st in lower enc; do job rowcov-$st-build sh ./tests/cxbin tests/rowcov.cx $st build; done   # rc3: the first enc shard paid the cold build and hit 49 s
for st in lower enc; do for k in $(seq 1 8); do job rowcov-$st-$k sh ./tests/cxbin tests/rowcov.cx $st gate$k/8; done; done   # 0.0.26 T2: lower (--full) and enc (--elf) rows, on tapes from the reference -S
for st in lower enc; do job rowcov-$st-union sh ./tests/cxbin tests/rowcov.cx $st union8; done   # 0.0.28 E22: the union may only rise, whatever the shards' redistribution
job rowcov-parse2-union sh ./tests/cxbin tests/rowcov.cx parse2 union24   # 0.0.29 T4
job seedpy tests/bound 58 sh ./tests/cxbin tests/decisionledger.cx --seedpy   # (.cx port, 2026-10-23) 0.0.25 X8: stage-specific seed .py and assemble.py size may only fall
job ledgercheck python3 ./tests/ledgercheck.py   # 0.0.25 P8: every plan row settled (完成/顺延/跨版进行/砍掉)
job freezecheck tests/bound 58 sh ./unisacc.com -run tests/freezecheck.cx   # (.cx port, 2026-10-22) 0.0.25 P1: inside the freeze window only fix:/fix(<scope>): commits touch the product closure; the window starts at the explicit freeze commit in release/freeze.tsv (0.0.43 M1, R10)
job freezecheck-selftest tests/bound 58 sh ./unisacc.com -run tests/freezecheck.cx -- --selftest   # 0.0.43 M1: scratch repo -- no freeze row rc 0, feat: touching exec/ in the window rc 1, fix(cx): ok, waiver, tag
job revivedscan tests/bound 58 sh ./unisacc.com -run tests/revivedscan.cx   # (.cx port, 2026-10-22) 0.0.25 P2: an excused probe that now agrees is reported in ~11 s, not at queue time
job reviewed-trees tests/bound 58 sh ./unisacc.com -run tests/reviewedtreescheck.cx   # (.cx port, 2026-10-22) 0.0.43: compilercheck reviewed_trees snapshots equal their trees (refreshed in the same commit)
job facts-export python3 ./exec/facts/export.py --check   # 0.0.25 P3: a header change without regenerated facts (0.0.24 first seal) is red here
job malloc      ./tests/malloc.sh
job docs        ./tests/docs.sh
for k in 1 2 3 4 5 6; do job tapebin-roundtrip-structure-$k python3 ./tests/tapebin.py structure-$k/6; done   # 0.0.43 timeout debt: /2 halves took 85 s and 68 s at load 15-19 (/4 quarters 31-42 s); six slices of ~44 probes (0.0.34: one job hit 49 s); keep Python: tests the Python codec unisa/tapebin.py itself
job tapebin-roundtrip-codec python3 ./tests/tapebin.py codec   # keep Python: C/Python byte-identical tapes from python -m unisa tape
job tapebin-shape python3 -m unisa.tapebin_shape --check
job tape-reader python3 ./tests/tapereadercheck.py
job script-inventory tests/bound 58 sh ./unisacc.com -run tests/inventory.cx -- --check   # (.cx port, 2026-10-22) R15-1: no test/check script without a gate, a caller or a disposition
job script-inventory-self python3 ./tests/inventorycheck.py   # matching and global reachability regressions
job inventory-derived tests/bound 58 sh ./unisacc.com -run tests/inventoryderivedcheck.cx   # (.cx port, 2026-10-23) exact generated inventory bytes
job subtract-safety tests/bound 58 sh ./unisacc.com -run tests/subtractsafety.cx   # R16-9: archived files have no users outside archive/; AGENTS.md/CLAUDE.md resolve (.cx port of subtractsafety.py, 2026-10-22)
job subtract-safety-selftest tests/bound 58 sh ./unisacc.com -run tests/subtractsafety.cx -- --selftest   # R16-9: both 0.0.15 counterexamples are still caught; --fix repoints an archived path
job cxinclude tests/bound 58 sh ./unisacc.com -run tests/cxinclude.cx -- --check   # CX0 (plans/cx-modules.md): .cx include lines only go down (tests/cxinclude.tsv); a .cx without a row must have none
job glossarycheck tests/bound 58 sh ./unisacc.com -run tests/glossarycheck.cx   # docs/glossary.md section 6: tracked .cx and new .md (not in tests/glossary-baseline.tsv) hold prd.md section 0 -- no bare stage, PHASES, STAGE_NETS, net name as flow, bare 4 streams; any new file: no Python invocation (G5); exceptions ratcheted in tests/glossary-whitelist.tsv
job glossarycheck-selftest tests/bound 58 sh ./unisacc.com -run tests/glossarycheck.cx -- --selftest   # seeded G1-G7 violations are caught, clean controls pass, whitelist above-count / no-reason / no-row are refused
job cxmodule-neg tests/bound 58 sh ./unisacc.com -run tests/cxmodcheck.cx -- --neg "${MODEL_COM:-./unisacc.com}"   # CX2a (research/cx2-module-design.md 4): #pragma unisa use refusals by name -- no file, cycle, bad name, unknown pragma, exported object, macro in an exported declaration, two exporters, main in a module, std.X, misplaced pragma, undefined prototype, export under #if, reserved prefix, a name both defined and imported, a type from two modules -- and a consumer without its use goes red (N11)
job cxmodule-pos tests/bound 58 sh ./unisacc.com -run tests/cxmodcheck.cx -- --pos "${MODEL_COM:-./unisacc.com}"   # CX2a: a module by name, a diamond (one copy), a module also on the command line, the derived interface deterministic and equal to tests/cx/p1mod.cxif, -o bytes independent of TMPDIR, self-contained interfaces, the system cc oracle (-cx-plan rows, cc -include include/cx.h -include CXIF), a .c unit never scanned, -ftrim-libc kept
job cxmodule-classic tests/bound 58 sh ./unisacc.com -run tests/cxmodcheck.cx -- "$UA"   # CX2a: the same refusals and positives against the classic reference (src/main.c, src/front_parse.c fe_load; both drivers share src/cxmod.c)
job flownet-w0-module tests/bound 58 sh ./unisacc.com -run tests/flownetw0.cx tests/flownetw0mod.cx -- .   # flownet W0 (plans/m4-flownet-cx-module.md): one .cx uses another, compiled as one program; external .tsv read with named errors
job flownet-w0-module-negative tests/bound 58 sh ./unisacc.com -run tests/flownetw0.cx tests/flownetw0mod.cx -- --negative .   # flownet W0: the consumer compiled without the module is refused by name (rc 127), never a silent pass
job flownet-baseline-file tests/bound 58 sh ./unisacc.com -run tests/flownetbaseline.cx -- --check-file .   # flownet W1 (plans/m4-flownet-cx-module.md): tests/flownet-baseline.tsv whole, no Python -- acc 10/10 exact, gold fallbacks 0, check same == outputs, selftest decides == EXPECT (re-read from m4/selftest.py), corpus sources unchanged
job flownet-baseline-acc tests/bound 58 sh ./unisacc.com -run tests/flownetbaseline.cx -- --check acc .   # flownet W1: python3 -m m4 acc (oracle) == the recorded acc section, per net (exact, total)
job flownet-baseline-out1 tests/bound 58 sh ./unisacc.com -run tests/flownetbaseline.cx -- --check out1 .   # flownet W1: shard 1/4 of corpus+probes through python3 -m m4 build (oracle): source sha256, output bytes + sha256, trace (phase, net, hits, gold) == recorded
job flownet-baseline-out2 tests/bound 58 sh ./unisacc.com -run tests/flownetbaseline.cx -- --check out2 .   # flownet W1: shard 2/4 of corpus+probes through python3 -m m4 build (oracle): source sha256, output bytes + sha256, trace (phase, net, hits, gold) == recorded
job flownet-baseline-out3 tests/bound 58 sh ./unisacc.com -run tests/flownetbaseline.cx -- --check out3 .   # flownet W1: shard 3/4 of corpus+probes through python3 -m m4 build (oracle): source sha256, output bytes + sha256, trace (phase, net, hits, gold) == recorded
job flownet-baseline-out4 tests/bound 58 sh ./unisacc.com -run tests/flownetbaseline.cx -- --check out4 .   # flownet W1: shard 4/4 of corpus+probes through python3 -m m4 build (oracle): source sha256, output bytes + sha256, trace (phase, net, hits, gold) == recorded
job flownet-baseline-check tests/bound 58 sh ./unisacc.com -run tests/flownetbaseline.cx -- --check check .   # flownet W1: python3 -m m4 check (oracle, m4 vs ujs gold path, negatives) == recorded
job flownet-baseline-selftest tests/bound 58 sh ./unisacc.com -run tests/flownetbaseline.cx -- --check selftest .   # flownet W1: python3 -m m4 selftest (oracle, ~40 s: isel choose proof + per-net decision kinds) == recorded
job flownet-w2-d1 tests/bound 58 sh ./unisacc.com -run adapter/ujswasm/ujswasm.cx -- d1 .   # flownet W2 (plans/m4-flownet-cx-module.md): D1 -- flownet/*.cx (fnx, contract, nets, flow, api) name neither ujs nor Python
job flownet-w2-parity tests/bound 58 sh ./unisacc.com -run adapter/ujswasm/ujswasm.cx -- check .   # flownet W2: D2 without Python -- acc 10/10 exact as recorded; the 33 recorded programs built through flownet byte-identical (bytes + sha256) with identical traces (phase, net, hits, gold 0); the 9 negatives accepted/refused as recorded
job flownet-w2-refuse tests/bound 58 sh ./unisacc.com -run adapter/ujswasm/ujswasm.cx -- refuse .   # flownet W2: D5 -- 22 uncovered inputs (emit gaps, out-of-domain token, non-ASCII outside strings, bad UTF-8, nested fn, missing/dir source, integer literals > 2^64-1, float literals rounding to inf, wat2wasm rejecting/missing) each rc 1, stdout empty, no output file, one stderr line with reason + position
job flownet-w2-negative tests/bound 58 sh ./unisacc.com -run adapter/ujswasm/ujswasm.cx -- negative .   # flownet W2: not decorative -- gold drive (fallbacks), a missing table, mutated isel/lex nets and a flipped gold label are each red
job flownet-w2-accepts tests/bound 58 sh ./unisacc.com -run adapter/ujswasm/ujswasm.cx -- accepts .   # flownet W2: the 5 negatives ujs accepts and 2 literal boundaries (2^64-1, DBL_MAX), flownet bytes == python3 -m m4 build (oracle child, outside flownet/)
job flownet-w2-tables-gold tests/bound 58 sh ./unisacc.com -run adapter/ujswasm/tables.cx -- --check-gold .   # flownet W2: adapter/ujswasm/tables contract + gold files == a fresh make from the .cx gold port (no Python)
job flownet-w2-tables tests/bound 58 sh ./unisacc.com -run adapter/ujswasm/tables.cx -- --check .   # flownet W2: adapter/ujswasm/tables (contract, gold, net) == a fresh make; nets from python3 -m ujs build-weights (oracle child)
job flownet-w3-d3-1 tests/bound 58 sh ./unisacc.com -run tests/flownetw3.cx -- d3 . 1/2   # flownet W3 (plans/m4-flownet-cx-module.md): D3 -- tests/tdescprobe.cx derives its rows through flownet (nets constructed from its own tables tests/tdescprobe, injected as the label source; rows handed over and judged by the emit hook); a scratch copy passes --oracle cc, and with the use of flownet.api removed, the fn_run removed or the fn_build removed its gate is red, each by name
job flownet-w3-d3-2 tests/bound 58 sh ./unisacc.com -run tests/flownetw3.cx -- d3 . 2/2   # flownet W3: D3 -- the copy driven from the gold table (the run's books refuse), with the adj answer discarded (the next ask leaves ap/sz's domain) with the ap asks replaced by a constant (the books pinned by fn_expect refuse), and (W6 B1) with the fn_label_source injection removed (flownet holds no domain data: the build refuses) is red
job flownet-w3-flow-1 tests/bound 58 sh ./unisacc.com -run tests/flownetw3.cx -- flow . 1/3   # flownet W3 fix 3: D3 flow-through per key -- tdescprobe (no labels in its derivation; the algebra is its tables tests/tdescprobe, injected into flownet) under FLOWNET_SWEEP=all: every one of the 46 keys of adj/ap/sz, each perturbation of one answer refused by flownet or by the emit hook's judge (--oracle cc and --check 1/106; books pinned and unpinned), 0 accepted; fix 4: each of the 150 perturbations leads where tests/flownet-tdesc-sweep.tsv records (the refused ask, the books, the changed row)
job flownet-w3-flow-2 tests/bound 58 sh ./unisacc.com -run tests/flownetw3.cx -- flow . 2/3   # flownet W3 fix 3: the sweep proves itself -- five copies whose own --oracle cc is green but which answer from elsewhere after asking (all re-derived locally, all but 1 adj/1 ap/9 sz, one sz key, sizes from the baseline inside the emit hook, and (W6 B1) all read from the tables tdescprobe itself injects) are each refused, naming exactly the keys they discarded
job flownet-w3-flow-3 tests/bound 58 sh ./unisacc.com -run tests/flownetw3.cx -- flow . 3/3   # flownet W3 fix 4: the record proves itself -- three copies that answer from their own algebra and, on a disagreement with flownet, make flownet refuse (an out-of-domain ask), refuse in their flows or write a stray row (the verifier's probe and its kin): each passes its own --oracle cc and the sweep alone (0 accepted), and differs from tests/flownet-tdesc-sweep.tsv on all 46 keys
job flownet-w3-d4-tdesc tests/bound 58 sh ./unisacc.com -run tests/flownetw3.cx -- d4 tdesc .   # flownet W3: D4 -- tdescprobe --oracle cc and --check 1/6 on the root m9 while /proc is listed every 2 ms: no python process under the run (the listing and the python rule prove themselves first)
job flownet-w3-d4-adapter tests/bound 58 sh ./unisacc.com -run tests/flownetw3.cx -- d4 adapter .   # flownet W3: D4 -- the ujs adapter's check (label source = the .cx gold port, emit hook = wat2wasm) on the root m9: no python process under the run
job flownet-w4-selftest-1 tests/bound 58 sh ./unisacc.com -run adapter/ujswasm/ujswasm.cx -- selftest . 1/4   # flownet W4 (plans/m4-flownet-cx-module.md; research/flownet-w4-decisions.md): the decision measure, m4 selftest ported (no Python) -- each net lying (shift; trap where it has one) as an injected label source the nets are built from (fn_build), over the 33 recorded programs; decides = some output changed or refused by name; share 1: honest constructed nets == the record 33/33, lex/parse/type == EXPECT_W4 (m4 EXPECT + type/shape/ic promoted)
job flownet-w4-selftest-2 tests/bound 58 sh ./unisacc.com -run adapter/ujswasm/ujswasm.cx -- selftest . 2/4   # flownet W4: scope decides nothing (stays RESERVED), shape decides shift (the adapter front end consumes its answer)
job flownet-w4-selftest-3 tests/bound 58 sh ./unisacc.com -run adapter/ujswasm/ujswasm.cx -- selftest . 3/4   # flownet W4: irsel nothing (RESERVED), ic shift (consumed), isel shift/trap
job flownet-w4-selftest-4 tests/bound 58 sh ./unisacc.com -run adapter/ujswasm/ujswasm.cx -- selftest . 4/4   # flownet W4: enc shift/trap, reloc nothing; the isel eq/ne choice (i64_cmp | f64_cmp: different bytes, both run 100 under node)
job flownet-w4-unconsumed tests/bound 58 sh ./unisacc.com -run tests/flownetw4.cx -- unconsumed .   # flownet W4: the measure is not decorative -- a scratch copy of the adapter with its one consumption point (the fn_gate of consume()) removed, the asks kept, is measured by its own selftest as type/shape/ic deciding nothing: red
job flownet-w6-b1 tests/bound 58 sh ./unisacc.com -run tests/flownetw6.cx -- b1 .   # flownet W6 B1 (plans/m4-flownet-cx-module.md section 9): mechanism and domain apart -- flownet/ holds only .cx code and .md docs (no table, directory or link); no entry name and no text (code, comment, doc) names a consumer (found: every .cx with `#pragma unisa use flownet.api`, every flownet contract's directory, the domains tdesc/ujs); no string literal is a consumer contract's name or a token of its gold tables; no doc, comment or literal (escapes decoded, a source line's literals together) holds a consumer's gold or contract row or a flownet table's shape (W6 B1 fix: the verifier's gold-as-.md, consumer-named .md, table-as-literal and rows-as-initialiser all red); the scan proves itself on seeded findings first, every gold table found smuggled three ways
job realprog    ./tests/realprog.sh   # R16-12: pinned real programs (kilo, jsmn, cJSON, lua, sqlite) built by us and cc, same run, same bytes; ratchet list
job hosthdr     ./tests/hosthdr.sh   # R18-10: sys/stat, fcntl, poll, termios, sys/ioctl, isatty, dirent on macOS = cc (and gcc on Lima arm64)
job asmtext     ./tests/asmtext.sh   # R18-1/R18-2: -S -b lnx | unisacc as == -c on the corpus; system as accepts; hand-written .s
job combo       python3 ./tests/combo.py "$UA" "${MODEL_COM:-./unisacc.com}"   # R19-7: construct x type x position; product = reference tape or refused by name
job syscall6    ./tests/syscall6.sh   # R19-9 (3): generic system-call gate on osx/arm64, Rosetta x86_64, Lima lnx/arm64
job lifecycle   tests/bound 58 sh ./unisacc.com -run tests/lifecycle.cx   # 0.0.34 X2 .cx pilot (was lifecycle.sh); 0.0.33 W3 (D2): exit statuses, atexit, abort/raise, a handled signal and a crash, against cc on this host
job luatests    ./tests/luatests.sh   # 0.0.34 L1: lua 5.4 testes run by the reference (and osx/x86_64 under Rosetta) = cc
job sqlspeed    ./tests/sqlspeed.sh ref   # 0.0.37 L2: speedtest1-shaped SQL workload, reference-built sqlite shell = cc (product route joins with the L2 delta)
job minicon     env NO_COM=1 tests/bound 58 sh tests/minicon.sh   # 0.0.36 M1: minicon loader built by cc and the reference; same exit codes, argv/signal pass-through, no extract dir left
job com-minicon tests/bound 58 sh tests/minicon.sh   # 0.0.36 M1: the same contract on the product (MODEL_COM or ./unisacc.com)
job prevheaders tests/bound 58 sh ./unisacc.com -run tests/prevheaders.cx   # 0.0.34 X2 .cx pilot (was prevheaders.sh); 0.0.33 D3′: the installed previous release still compiles the seed tools against this include/
job forward     ./tests/forward.sh   # R19-10/R21-4a': prototyped undefined functions forward to the system libc (-run and images, six targets)
job forward-multi ./tests/forwardmulti.sh   # 0.0.26 N5: forwarding across translation units (default run, -run, -o)
job winposix    ./tests/winposix.sh   # R21-4a': Windows POSIX layer over kernel32 = macOS cc (win/arm64, win/x86_64 in the UTM VM; skipped when it is down)
job ccinterop   ./tests/ccinterop.sh   # R21-1: -c objects call cc-compiled int/pointer functions (osx/arm64, lnx/arm64, lnx/x86_64); other signatures refused by name
job linkunits   ./tests/linkunits.sh   # R17-2: units compiled alone (-c -b HOST -funit) and joined by the unisacc linker = cc = one-step
job elfobj      ./tests/elfobj.sh   # R16-7: -c -b lnx/ARCH relocatable ELF: sections, relocations, image invariant, GNU ld/lld link, Lima run
job c99-ledger  python3 ./tests/c99ledger.py   # R14-6: the C99 clause ledger and README's coverage table
job publish-order python3 ./tests/publishordercheck.py   # wrong signed hash must leave the draft and assets untouched
job front-bounds tests/bound 58 sh ./unisacc.com -run tests/frontboundscheck.cx   # (.cx port, 2026-10-22) anonymous struct tags must not read token -1
# 0.0.28 R4: one job per part that seedconstructcheck.py --list names (C99 seed tools vs Python, byte for byte);
# a new part needs no edit here or in gatedeps.json (refreshgatedeps.cx declares it from seed-construct-parse2)
job seedfacts python3 ./tests/seedfactscheck.py   # 0.0.29 B4: seed/facts.h bindings and lets equal assemble.py on every manifest
job seedparse2-1 ./tests/seedparse2check.sh x locations warnings errors   # 0.0.29 B4: seed/gen.c parse2 = gen.py, eight variants in two jobs
job seedparse2-2 ./tests/seedparse2check.sh locations,warnings locations,errors warnings,errors locations,warnings,errors
job seedparse2-key ./tests/seedparse2check.sh --key-selftest   # 0.0.43: the parse2 Python-reference cache key is the measured read-set key (tests/seedmemorykey.cx); an edit to exec/finite_rules.py, exec/facts/load.py, exec/parse/*, exec/model*.tsv or a parse2 manifest misses the cache (private copies, no admission needed)
# 0.0.32 B5: seed/gen.c = gen.py on every delta the build constructs; three shards stay under the 60 s ceiling
job seedgen ./tests/seedgencheck.sh e2 e1 e3 e4 o1 prune nativeabi enc-elf enc-macho enc-pe arm-elf arm-macho arm-pe
job seedgen-2 ./tests/seedgencheck.sh lower-lnx-x lower-lnx-a lower-osx-x lower-osx-a lower-win-x lower-win-a obj-lower-x obj-lower-a obj-enc-x obj-enc-a
job seedgen-opt sh ./tests/seedoptcheck.sh   # 0.0.40 K5-1 (机房主任 23:36/23:41): opt seed/gen.c = gen.py, scratch tree only
job unisa-bits sh ./tests/unisabitscheck.sh   # 0.0.41 K6-1: unisa.bits C ref vs bits.py byte-pair; keep Python
job unisa-hostabi sh ./tests/unisahostabicheck.sh   # 0.0.41 K6-2: unisa.hostabi C ref vs hostabi.py on the TSV (point sample); keep Python
job unisa-peversion sh ./tests/unisapeversioncheck.sh   # 0.0.41 K6-3: unisa.peversion C ref vs peversion.py on sampled PE heads (point sample); keep Python
job unisa-padded sh ./tests/unisapaddedcheck.sh   # 0.0.41 K6-4: unisa.padded.Padded C ref vs padded.py observable values (point sample; ZData is K6-4b); keep Python
job unisa-zdata sh ./tests/unisazdatacheck.sh   # 0.0.41 K6-4b: unisa.padded.ZData C ref vs padded.py on scripted op sequences (point sample); keep Python
job unisa-linalg sh ./tests/unisalinalgcheck.sh   # 0.0.41 K6-5: unisa.linalg gemv/relu/argmax/margin C ref vs linalg.py (point sample); keep Python
job unisa-libneed sh ./tests/unisalibneedcheck.sh   # 0.0.41 K6-6: unisa.libneed consts/guard/bodies/table/roots C ref vs libneed.py on scratch micro incdir (point sample); keep Python
job unisa-uns1 sh ./tests/unisauns1check.sh   # 0.0.41 K6-7: unisa.control.uns1 dumps/loads/pad4 C ref vs uns1.py (point sample; i8+f32; q4/q2/f16 out of scope); keep Python
job unisa-catalog sh ./tests/unisacatalogcheck.sh   # 0.0.41 K6-8: unisa.catalog OS/ARCH/TARGETS + sampled sysno/form/symbol/gate/argregs C ref vs catalog.py (point sample; full table is K6-8b); keep Python
job unisa-fp sh ./tests/unisafpcheck.sh   # 0.0.41 K6-9: unisa.fp op3/op2 (binary64/32 arithmetic, compares, conversions, sqrt) C ref vs fp.py (point sample; dec_to_f32 out of C domain); keep Python
job unisa-tape sh ./tests/unisatapecheck.sh   # 0.0.41 K6-10: unisa.tape build/emit/string/to_text/to_canonical_text + parse round trips C ref vs tape.py (point sample; not gold/vm/layout); keep Python
job unisa-tapebin sh ./tests/unisatapebincheck.sh   # 0.0.41 K6-11: unisa.tapebin encode/decode (canonical LEB128, pools, sections, SHA-256 digest, target_id, check_origin) C ref vs tapebin.py (point sample; tapebin_shape out of scope); keep Python
job unisa-tapebin-shape sh ./tests/unisatapebinshapecheck.sh   # 0.0.41 K6-11b: unisa.tapebin_shape render/records/c (71 opcodes incl. fp) C ref vs tapebin_shape.py, read-only against docs/*.tsv and src/tapebin_shape.inc; no --write; keep Python
job unisa-image-fat sh ./tests/unisaimagefatcheck.sh   # 0.0.41 K6-12: unisa.image.fat write() (fat header, descriptors, page-aligned slices) C ref vs fat.py (point sample; elf/macho/pe/ape out of scope); keep Python
job unisa-image-elf sh ./tests/unisaimageelfcheck.sh   # 0.0.41 K6-13: unisa.image.elf static write() (ELF64 header, two PT_LOAD, page-aligned data, signed entry) C ref vs elf.py (point sample; dynamic=True not claimed; macho/pe/ape out of scope); keep Python
job unisa-image-macho sh ./tests/unisaimagemachocheck.sh   # 0.0.41 K6-14: unisa.image.macho write() (Mach-O 64 load commands, DYLD_INFO binds, ad-hoc SuperBlob with per-page SHA-256) C ref vs macho.py (point sample; not loadable/codesign closure; pe/ape out of scope); keep Python
job unisa-image-pe sh ./tests/unisaimagepecheck.sh   # 0.0.41 K6-15: unisa.image.pe write() static path (stub empty, dynamic False: PE32+ headers, import section, load config, .reloc) C ref vs pe.py (point sample; dynamic/stub/ape not claimed); keep Python
job unisa-ape sh ./tests/unisaapecheck.sh   # 0.0.41 K6-16: unisa.ape build() script/two-pass layout, gzip Unix members (zlib), PE stub split, UNIPKG1 payload trailer; fixture compile_target (not a product target; not loadable/multi-platform); keep Python
job unisa-vm sh ./tests/unisavmcheck.sh   # 0.0.41 K6-17: unisa.vm run() point programs (integer arithmetic and div/mod, shifts, loop/call/ret, store/load, print gate clobber, halts 124/136/139, narrow ld/st) C ref vs vm.py (point sample; not full opcode sweep; no syscall/file closure); keep Python
job unisa-gold-prec sh ./tests/unisagoldpreccheck.sh   # 0.0.41 K6-18: unisa.gold prec_label finite domain (PREC_OPS full table + boundary strings) C ref vs gold.py source (ast extraction; no build(); no regen; not gold closure); keep Python
job unisa-emit-arm-nwmem sh ./tests/unisaemitarmnwmemcheck.sh   # 0.0.41 K6-19: unisa.emit_arm N/w/movimm/mem legal domain (scaled, unscaled, sp, IP0 far path) + refusal points, C ref vs emit_arm.py (point sample; no encode/reloc/WinAPI/fp); keep Python
job unisa-gold-lex-label sh ./tests/unisagoldlexlabelcheck.sh   # 0.0.41 K6-20: unisa.gold lex_label finite domain (12 CHARC classes x peek; comment starts, dot rule) C ref vs gold.py source (ast extraction; no build(); no regen; not gold closure); keep Python
job unisa-gold-type-label sh ./tests/unisagoldtypelabelcheck.sh   # 0.0.41 K6-21: unisa.gold type_label full finite domain (TYS x TOPS x TYS = 4275 rows) + named points, C ref vs gold.py source (ast extraction; no build(); no regen; not gold closure); keep Python
job unisa-gold-binsel-label sh ./tests/unisagoldbinsellabelcheck.sh   # 0.0.41 K6-22: unisa.gold binsel_label stage domain BINSEL_OPS x (s,u) (32 rows) + named points, C ref vs gold.py source (ast extraction; no build(); no regen; not gold closure); keep Python
job unisa-gold-opinfo-label sh ./tests/unisagoldopinfolabelcheck.sh   # 0.0.41 K6-23: unisa.gold opinfo_label finite op domain OPINFO_OPS (tape.SHAPE 71 ops + other = 72 rows) + named points, C ref vs gold.py source (ast extraction; no build(); no regen; not peep/irsel/parse closure); keep Python
job unisa-gold-peep-label sh ./tests/unisagoldpeeplabelcheck.sh   # 0.0.41 K6-24: unisa.gold peep_label finite domain PEEP_A x PEEP_B x PEEP_REL (1632 rows) + named points incl. store64 fall-through, C ref vs gold.py source (ast extraction; no build(); no regen; not gold closure); keep Python
job unisa-gold-scope-label sh ./tests/unisagoldscopelabelcheck.sh   # 0.0.41 K6-25: unisa.gold scope_label finite domain CTX x KIND (30 rows) + named points, C ref vs gold.py source (ast extraction; no build(); no regen; not gold closure); keep Python
job unisa-gold-pp-label sh ./tests/unisagoldpplabelcheck.sh   # 0.0.41 K6-26: unisa.gold pp_label finite domain DIRS x DEFINED (18 rows) + named points, C ref vs gold.py source (ast extraction; no build(); no regen; not gold closure); keep Python
job unisa-gold-reloc-label sh ./tests/unisagoldreloclabelcheck.sh   # 0.0.41 K6-27: unisa.gold reloc_label finite domain JMPKIND x ARCH (6 rows) + named points, C ref vs gold.py/catalog.py source (ast extraction; no build(); no regen; not irsel/parse); keep Python
job unisa-gold-irsel-label sh ./tests/unisagoldirsellabelcheck.sh   # 0.0.41 K6-28: unisa.gold irsel_label IRSEL_MAP mapping (70 entries, not a Cartesian product) + named points, C ref vs gold.py source (ast extraction; no build(); no regen; not parse); keep Python
job unisa-gold-parse-label sh ./tests/unisagoldparselabelcheck.sh   # 0.0.41 K6-29: unisa.gold parse_label stage domain NT x TOKS (5 x 68 = 340) + named points, C ref vs gold.py source (ast extraction; labels checked against PRODS; no build(); not front-end parse); keep Python
job seedgen-pp-1 sh ./tests/seedppcheck.sh 1   # 0.0.40 K5-1b (机房主任 00:42): pp seed/gen.c = gen.py, none/--osx/--win + negatives + helper
job seedgen-pp-2 sh ./tests/seedppcheck.sh 2   # K5-1b: --arm64/--osx --arm64/--win --arm64 + default-route cache
job seedgen-ppconsumer-1-lnx-x86_64 sh ./tests/seedppconsumercheck.sh 1 lnx/x86_64   # 0.0.40 K5-1c (机房主任 01:23): prepare.sh pp step through gen-delta, controlled; 0.0.43 timeout debt: shard 1 (78 s) as four parts, each its own cold prepare
job seedgen-ppconsumer-1-lnx-arm64 sh ./tests/seedppconsumercheck.sh 1 lnx/arm64
job seedgen-ppconsumer-1-osx-x86_64 sh ./tests/seedppconsumercheck.sh 1 osx/x86_64
job seedgen-ppconsumer-1-negative sh ./tests/seedppconsumercheck.sh 1 negative
job seedgen-ppconsumer-2-prepare sh ./tests/cxbin tests/seedppconsumerprepared.cx --prepare   # (.cx port, 2026-10-23)
job seedgen-ppconsumer-2-osx-arm64 sh ./tests/cxbin tests/seedppconsumerprepared.cx --require-cached --target osx/arm64
job seedgen-ppconsumer-2-win-x86_64 sh ./tests/cxbin tests/seedppconsumerprepared.cx --require-cached --target win/x86_64
job seedgen-ppconsumer-2-win-arm64 sh ./tests/cxbin tests/seedppconsumerprepared.cx --require-cached --target win/arm64
job seedgen-ppchain-1-prepare sh ./tests/cxbin tests/seedppchainprepared.cx --prepare   # (.cx port, 2026-10-23)
job seedgen-ppchain-1-default sh ./tests/cxbin tests/seedppchainprepared.cx --require-cached --part default
job seedgen-ppchain-1-inherited sh ./tests/cxbin tests/seedppchainprepared.cx --require-cached --part inherited   # 0.0.40 K5-1d (机房主任 02:35): chain.sh pp through gen-delta, controlled; K5-1i (07:53): + parse2 helper, three slices
job seedgen-ppchain-2 sh ./tests/seedppchaincheck.sh 2
job seedgen-ppchain-3 sh ./tests/seedppchaincheck.sh 3
job seedgen-pprun sh ./tests/seedpprun.sh   # 0.0.40 K5-1e (机房主任 02:59): exec/pp/run.sh d.json through gen-delta, controlled
job seedgen-optconsumer-1 sh ./tests/seedoptconsumercheck.sh 1   # 0.0.40 K5-1f (机房主任 03:43): prepare.sh opt step through gen-delta, controlled; K5-1h (06:12): three slices
job seedgen-optconsumer-2 sh ./tests/seedoptconsumercheck.sh 2
job seedgen-optconsumer-3 sh ./tests/seedoptconsumercheck.sh 3
job seedgen-pruneconsumer-1 sh ./tests/seedpruneconsumercheck.sh 1   # 0.0.40 K5-1g (机房主任 04:09): prepare.sh prune step through gen-delta, controlled; K5-1h (06:12): three slices
job seedgen-pruneconsumer-2 sh ./tests/seedpruneconsumercheck.sh 2
job seedgen-pruneconsumer-3 sh ./tests/seedpruneconsumercheck.sh 3
job seedgen-parse2consumer-1 sh ./tests/seedparse2consumercheck.sh 1   # 0.0.40 K5-1h (机房主任 06:01): prepare.sh parse2 step through exec/parse2gen/gen-delta.sh, controlled
job seedgen-parse2consumer-2 sh ./tests/seedparse2consumercheck.sh 2
job seedgen-parse2consumer-3-prepare sh ./tests/seedparse2consumercheck.sh prepare   # 0.0.43 R11: the SEED_GEN=0 arm (python3 gen.py parse2, ~37-45 s) runs once per gen.py parse2 read-set key into a checked handoff (warm: ~2 s)
job seedgen-parse2consumer-3 sh ./tests/seedparse2consumercheck.sh 3   # SEED_GEN=0 route against the keyed handoff (rc 75 without it) and the C parse2 built alongside, byte for byte (~20 s; was 65 s)
job seedgen-3 ./tests/seedgencheck.sh tokenpp tokenlex warnlex
job seedgen-4 ./tests/seedgencheck.sh warnparse warnunits errorparse warnpp   # seedgen-3 hit 49 s in the rc3 queue
job com-memalign ./tests/memaligncheck.sh   # 0.0.32 D2: word-wise memcpy/memset only on shared alignment (UBSan + product)
job com-seedgen ./tests/seedgencomcheck.sh   # 0.0.32 D2: .com -O2 seed/gen.c parse2 == cc build, <= 10 s
job seedpack ./tests/seedpackcheck.sh   # 0.0.32 B5: seed/pack.c + compilerpack.c == pack.py + compilerpack.py
job seedape ./tests/seedapecheck.sh     # 0.0.32 B5: UA -b + seed/ape.c == unisa ape (product container)
scparts=$(sh "$R/tests/cxbin" "$R/tests/seedconstructcheck.cx" --list) || scparts=""   # (.cx port, 2026-10-23: --list and --prep; --part stays Python)
[ -n "$scparts" ] || job seed-construct-base sh -c 'echo "seedconstructcheck.cx --list failed or listed no part"; exit 1'   # 0.0.28 E14: never zero jobs and green
job seedgen-construct-prep tests/bound 58 sh ./tests/cxbin tests/seedconstructcheck.cx --prep   # one content-keyed build of seed/gen.c for every seed-construct shard (3 shards compiling it at once exceeded the 20 s compile limit under load); same cache, manifest and flock as the .py
for p in $scparts; do job "seed-construct-$p" python3 ./tests/seedconstructcheck.py --part "$p"; done   # the in-process Python E3 oracle; not ported
job seed-matrix-shared-prepare sh ./tests/cxbin tests/seedmatrixprepared.cx --prepare   # (.cx port, 2026-10-23)
job seed-matrix-shared-e2 sh ./tests/cxbin tests/seedmatrixprepared.cx --require-cached --unit e2
job seed-matrix-shared-e1 sh ./tests/cxbin tests/seedmatrixprepared.cx --require-cached --unit e1
job seed-matrix-shared-e3 sh ./tests/cxbin tests/seedmatrixprepared.cx --require-cached --unit e3
job seed-matrix-shared-e4 sh ./tests/cxbin tests/seedmatrixprepared.cx --require-cached --unit e4
job seed-matrix-shared-o1 sh ./tests/cxbin tests/seedmatrixprepared.cx --require-cached --unit o1
job seed-matrix-shared-prune sh ./tests/cxbin tests/seedmatrixprepared.cx --require-cached --unit prune
job seed-matrix-shared-nativeabi sh ./tests/cxbin tests/seedmatrixprepared.cx --require-cached --unit nativeabi
for k in 1 2 3; do job seed-matrix-features-$k sh ./tests/cxbin tests/seedconstructmatrix.cx features-$k; done   # (.cx port, 2026-10-23)
job seed-matrix-lnx-arm64 sh ./tests/cxbin tests/seedconstructmatrix.cx lnx/arm64
job seed-matrix-lnx-x86_64 sh ./tests/cxbin tests/seedconstructmatrix.cx lnx/x86_64
job seed-matrix-osx-arm64 sh ./tests/cxbin tests/seedconstructmatrix.cx osx/arm64
job seed-matrix-osx-x86_64 sh ./tests/cxbin tests/seedconstructmatrix.cx osx/x86_64
job seed-matrix-win-arm64 sh ./tests/cxbin tests/seedconstructmatrix.cx win/arm64
job seed-matrix-win-x86_64 sh ./tests/cxbin tests/seedconstructmatrix.cx win/x86_64
job seed-matrix-object sh ./tests/cxbin tests/seedconstructmatrix.cx object
job gate-infra python3 ./tests/queuecheck.py
job gate-infra-38 python3 ./tests/queuecheck38.py   # 0.0.38 scheduler controls (split to keep both under 60 s)
job gate-layers python3 ./tests/gatelayers.py --check
job gate-select python3 tests/gateselectcheck.py   # offline selector contracts; never executes gate jobs
# R12-0 ③b: checks that were in no gate at all (tests/ungated-checks.tsv).
# Each was run standalone with a measured rc=0 before being added; the four that
# print a Usage line and the one that fails on this HEAD are NOT here -- see the
# tsv.  all.sh is deliberately unchanged.
job exec-elfdata-bridge   python3 ./exec/enc/librarysymbolscheck.py
job exec-lex-sourcefacts tests/bound 58 sh ./unisacc.com -run exec/lex/sourcefactscheck.cx tests/cxlib/lexdelta.cx   # (.cx port, 2026-10-23) E1 provenance framing (USLFACT1)
job exec-parse2-libimports python3 ./exec/parse2/libraryimportscheck.py
for m in 0 1 2 3 4 5 6 7 8; do job fresh-order-$m tests/bound 58 sh ./unisacc.com -run tests/freshordercheck.cx -- $m; done   # T1d: global fresh order = tests/freshorder.tsv; (.cx port, 0.0.43) seed/gen.c + tests/freshorderhook.h, not gen.py (--errors modes ran 63-76 s)
job lib-bindings-registry python3 ./tests/librarybindingscheck.py
job tsv-build-account     python3 ./tests/tsvbuild_check.py
# The five exec/pipeline/check_*_text.py format checkers have no caller in any
# gate; the only path that reaches them is exec/pipeline/run.py, which nothing
# invoked either. These bounded suites walk the connected stages and call
# check_<fmt>.py on each dump. The F1 probe also checks that E1's attribute
# sidecar reaches E3 when a bundled header declares a constructor.
job exec-formats python3 ./exec/pipeline/run.py examples/hello.c
job exec-formats-f1 python3 ./exec/pipeline/run.py exec/pipeline/probes/f1_header_ctor.c
job pipeline-cache python3 ./tests/pipelinecache.py
# The last check that was in no gate AND failing on this HEAD: it cross-assembles
# librarycall_<arch>.S for a Linux target and an Apple target and requires the
# same instruction stream.  It failed for a year of commits' worth of reasons
# that were not drift -- see archive/research/r12/r12-linuxbridgecheck-diagnosis.md.
job exec-bridge-linux python3 ./exec/c/linuxbridgecheck.py
job windows-resolver-host sh ./unisacc.com -run tests/windowsresolverhostcheck.cx   # (.cx port, 2026-10-23; unbounded as before: a cold zig cache took 50 s)
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
    job com-lifecycle MODEL_COM="$PRODUCT" tests/bound 58 sh ./unisacc.com -run tests/lifecycle.cx   # 0.0.33 W3: the same matrix built by the product
    job com-luatests-build MODEL_COM="$PRODUCT" ./tests/luatests.sh com-build   # 0.0.34 L1: the product's cold lua build alone (~48 s), cached for com-luatests
    job com-luatests MODEL_COM="$PRODUCT" ./tests/luatests.sh com   # 0.0.34 L1: the same lua testes built by the product (build cached by product+source sha)
    job com-forward UA="$PRODUCT" UA_RUN="$PRODUCT" ./tests/forward.sh   # 0.0.28 N1': posix2 (glob callback), var (variadic), host functions on the product
    job com-forward-multi UA="$PRODUCT" UA_RUN="$PRODUCT" ./tests/forwardmulti.sh   # 0.0.26 N5: the product failed here up to 0.0.25
    job com-tapedomain tests/bound 58 sh "$PRODUCT" -run tests/tapedomaincheck.cx -- "$PRODUCT"   # 0.0.43 TP-7/TP-8/TP-9: a raw .tape outside the Tape domain is refused before anything is written
    job com-hostaddr-neg tests/bound 58 sh "$PRODUCT" -run tests/hostaddrnegcheck.cx -- "$PRODUCT"   # 0.0.43: .hostaddr index 0..3 refusal domain (control byte-equal to the reference; -1/4 named lower refusals both sides)
    job com-extent-neg tests/bound 58 sh "$PRODUCT" -run tests/extentnegcheck.cx -- "$PRODUCT"   # 0.0.43: tape data-extent refusal domain (sparse control byte-equal; 2^31 single/cumulative product-refused, reference acceptance registered)
    job com-bindings-neg tests/bound 58 sh "$PRODUCT" -run tests/bindingsnegcheck.cx -- "$PRODUCT"   # 0.0.43: .librarycall refusal ownership (missing bindings named by the binding layer, bad register/operands by the tape domain; plain and lnx .hostcall controls byte-equal)
    job com-tapebin-neg tests/bound 58 sh "$PRODUCT" -run tests/tapebinnegcheck.cx -- "$PRODUCT"   # 0.0.43: Tapebin carrier refusal domain (own round trip and origin carrier byte-equal to the text route; magic/trunc/digest/tail/origin-mismatch named refusals, nothing written)
    job com-cerr-hygiene tests/bound 58 sh "$PRODUCT" -run tests/cerrhygienecheck.cx -- "$PRODUCT"   # 0.0.43: C-route failure hygiene (pp/lex/parse/sema negatives x writing modes: rc1, empty stdout, output path untouched, reason named)
    job com-tdescprobe-1 tests/bound 58 sh "$PRODUCT" -run tests/tdescprobe.cx -- --check "$PRODUCT" tests/tdescprobe-product.tsv "${TMPDIR:-/tmp}/tdescprobe-com-1" 1/3   # 0.0.43 R3: declarator derivations x sizeof contexts against the TDESC model, ratcheted (tests/tdescprobe-product.tsv)
    job com-tdescprobe-2 tests/bound 58 sh "$PRODUCT" -run tests/tdescprobe.cx -- --check "$PRODUCT" tests/tdescprobe-product.tsv "${TMPDIR:-/tmp}/tdescprobe-com-2" 2/3   # 0.0.43 R3: declarator derivations x sizeof contexts against the TDESC model, ratcheted (tests/tdescprobe-product.tsv)
    job com-tdescprobe-3 tests/bound 58 sh "$PRODUCT" -run tests/tdescprobe.cx -- --check "$PRODUCT" tests/tdescprobe-product.tsv "${TMPDIR:-/tmp}/tdescprobe-com-3" 3/3   # 0.0.43 R3: declarator derivations x sizeof contexts against the TDESC model, ratcheted (tests/tdescprobe-product.tsv)
    job com-input-contract tests/bound 58 sh "$PRODUCT" -run tests/inputcontractcheck.cx -- "$PRODUCT"   # 0.0.43: driver input contract (tape route takes one source by name; C-route extra-input naming and the empty .c/.tape behaviours registered; empty-tape image byte-equal to the reference)
    job com-tape-struct tests/bound 58 sh "$PRODUCT" -run tests/tapestructcheck.cx -- "$PRODUCT"   # 0.0.43: program-structure refusals (undefined/duplicate label, encoder-slice width, named not-covered without a line) and the registered duplicate-data acceptance (byte-equal to the reference)
    job com-source-name tests/bound 58 sh "$PRODUCT" -run tests/sourcenamecheck.cx -- "$PRODUCT"   # 0.0.43 ruling (fixed logical source name): the core sees the argv spelling -- identical -S/__FILE__/diagnostics across physical directories, absolute spellings verbatim
    job com-hostenv tests/bound 58 sh "$PRODUCT" -run tests/hostenvcheck.cx -- "$PRODUCT"   # 0.0.43 plan step 1: no host environment variable changes a product result (UNISA_MAXSTEPS is a build constant in the shipped driver)
    job com-latebind tests/bound 58 sh "$PRODUCT" -run tests/latebindcheck.cx -- "$PRODUCT"   # 0.0.43 LB-1: host-symbol contract stream, binding refusal, in-memory -run of plain programs
    job lib-latebind tests/bound 58 sh "$PRODUCT" -run tests/liblatebindcheck.cx -- "$PRODUCT"   # 0.0.43 LB-2a: the library declaration contract carries no host address
    job com-forceinclude tests/bound 58 sh "$PRODUCT" -run tests/forceincludecheck.cx -- "$PRODUCT"   # 0.0.43: __FILE__, diagnostics and nested includes inside -include headers, forwarded and not
    job com-iquote tests/bound 58 sh "$PRODUCT" -run tests/iquotecheck.cx -- "$PRODUCT"   # 0.0.43: -iquote searched by quoted includes only, before -I
    job com-volatile-comma MODEL_COM="$PRODUCT" bash -c 'R=$PWD; export UA=${UA:-/tmp/ua_ref}; . tests/lib.sh && ua_ready && tests/bound 58 sh "$MODEL_COM" -run tests/volatilecomma.cx -- --com'   # (.cx port, 2026-10-23)
    job com-full-signature tests/bound 58 sh "$PRODUCT" -run tests/fullsignaturecallcheck.cx -- --package "$PRODUCT"   # (.cx port, 2026-10-23) 21 actual product calls (fp9/mixed17/returned/pick/nested probe x -O0..-O2) exit 0 with empty stdout and stderr
    for k in $(seq 1 40); do job com-csmithdiff-$k MODEL_COM="$PRODUCT" SEEDS=1-500 SHARD=$k/40 sh ./tests/cxbin tests/csmithdiff.cx; done   # (.cx port, 2026-10-23)
    for k in 1 2 3 4 5; do job com-declmatrix-$k MODEL_COM="$PRODUCT" SHARD=$k/5 sh ./tests/cxbin tests/declmatrix.cx; done   # the same matrix on the product (found functions named r0..r7) (.cx port, 2026-10-23)
    job com-auditnet MODEL_COM="$PRODUCT" sh "$PRODUCT" -run tests/auditnet.cx   # (.cx port, 2026-10-22) 0.0.22 P1-C1: every packed network re-enumerated; reads model-audit/ beside the product
    job com-tapebin-ref python3 ./tests/tapebinref.py prepare   # 0.0.43 T-1: the reference compiler, built once per content key; the shards only locate it
    job com-tapebin-six MODEL_COM="$PRODUCT" TAPEBIN_PART=six tests/bound 58 sh ./tests/cxbin tests/tapebinproduct.cx   # (.cx port, 2026-10-23) fixed cost: calls, six-target images, host run, origin guard
    for k in 1 2 3 4 5 6; do job com-tapebin-$k MODEL_COM="$PRODUCT" TAPEBIN_SHARD=$k/6 python3 ./tests/tapebinproduct.py; done   # 0.0.43: six slices of ~43 files, ~24 s quiet against the 42 s window; keep Python: compares against the Python tape codec
    job com-tapebin-cover MODEL_COM="$PRODUCT" TAPEBIN_PART=cover tests/bound 58 sh ./tests/cxbin tests/tapebinproduct.cx   # (.cx port, 2026-10-23) slices partition the file set; known upstream tape diffs checked globally
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
# 0.0.40 (机房主任 20:54): only the leading "suite rc=N" field counts (tests/gatesummary.sh); a suite whose own
# output mentions rc=0 or rc=77 can no longer be summarised as passed or unverified
bash "$R/tests/gatesummary.sh" "$O" "$T0"; exit $?

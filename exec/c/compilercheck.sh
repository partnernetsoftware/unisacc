#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# Fresh models and product-shaped driver checks. Every inner run <=60 s.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
T=$(mktemp -d); trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T"' EXIT
b() { "$_BOUND" 60 "$@"; }
case $(uname -s) in Darwin) OS=osx;; Linux) OS=lnx;; *) echo 'unsupported test host' >&2; exit 1;; esac
case $(uname -m) in arm64|aarch64) ARCH=arm64;; x86_64) ARCH=x86_64;; *) exit 1;; esac
TARGET=$OS/$ARCH; export TARGET
# F1 consumes raw JSON from the same model cache, independently of core prep.
case ${1:-all} in
prep-f1json)
    # 0.0.43: a missing m.json is written by seed/gen.c (gen.py parse2 --errors alone took ~65 s; the JSON of all three
    # models is byte-identical, proven once outside the gate), so the supplement is ~8 s; prep-2 usually kept it already
    mkdir -p "$T/seedbin"; b cc -std=c99 -O2 -w -Iseed seed/gen.c -o "$T/seedbin/gen"
    "$_BOUND" 55 python3 - "$TARGET" "$T/seedbin" <<'PYJSON'
import sys,tempfile
sys.path.insert(0,'exec/c')
import compilerpack as pack
names=('errorparse','warnlex','warnpp-shared')
jobs={n:(script,args) for n,script,args in pack.model_jobs({sys.argv[1]})}
with tempfile.TemporaryDirectory(prefix='f1-json-prepare-') as td:
    # This job only supplements prepared pairs; missing base never constructs.
    pack.REQUIRE_CACHED=True
    try:
        for name in names:
            pack.built_model(td,name,*jobs[name])
    except ValueError as error:
        print('NEED_PREPARE F1 JSON base: '+str(error)+'; run core-prep-0..5',file=sys.stderr)
        raise SystemExit(75)
    pack.REQUIRE_CACHED=False;pack.SEED_TOOLS=sys.argv[2]
    for name in names:
        pack.built_model(td,name,*jobs[name],want_json=True)
        print('prepared JSON',name,flush=True)
    pack.REQUIRE_CACHED=True
    for name in names:
        pack.built_model(td,name,*jobs[name],want_json=True)
        print('verified JSON',name,flush=True)
PYJSON
    exit $? ;;
esac
# Gate checks never construct: they need the prep-0..5 jobs (gateorder.json) to have cached the models first.  0.0.43
# (timeout debt): core-build/contracts/dependencies and language-1/2 built every model themselves (116 s cold); now
# only the manual aggregates (all, core, resources, language) and the prep jobs construct.  prep-1..5 also wait for
# prep-0 (gateorder.json), whose elf models they read.
case ${1:-all} in all|core|resources|language|prep-*) REQ=;; *) export UNISACC_REQUIRE_PREPARED=1; REQ=--require-cached;; esac
b ./exec/pipeline/elf.sh "$T" examples/hello.c > "$T/build.log" 2>&1 || { cat "$T/build.log"; exit 1; }
b python3 exec/build/gen.py opt "$T/o1.json"
b python3 exec/c/tbl.py "$T/o1.json" "$T/o1.tbl"
b python3 exec/c/net.py "$T/o1.tbl" "$T/o1.net"
b "$T/run" --check-net "$T/o1.tbl" "$T/o1.net"
# Preparation jobs (gate exec-driver-core-prep-K, bindprep pattern): prep-0 builds elf and the o1 chain into the content-addressed
# cache and asserts that a second elf.sh run is a cache hit; prep-K builds part K/5 of PREP_PARTS and then requires it cached
# (--require-cached, so the cache entries and their keys are checked, not rebuilt).  A missing product is rebuilt by the
# same bounded step, as bindprep self-prepares its segment.  Neither is a test verdict.
case ${1:-all} in
prep-0)
    mkdir -p "$T/again"
    b ./exec/pipeline/elf.sh "$T/again" examples/hello.c > "$T/again.log" 2>&1 || { cat "$T/again.log"; exit 1; }
    grep -q "model preparation: hit" "$T/again.log" || { echo "prep-0: second elf run was not a cache hit" >&2; exit 1; }
    echo "prep-0: elf and o1 chain cached; second elf run hit"; exit 0 ;;
prep-1|prep-2|prep-3|prep-4|prep-5)
    K=${1#prep-}
    # 0.0.43: parts 1-2 (warnparse, errorparse: gen.py parse2 --errors alone runs 63-76 s) construct with the C seed tools
    # (seed/gen.c, tbl.c, net.c by the host cc; byte-identical, as buildcompiler.sh's SEED_GEN=1), under the same cache key
    ST=; case $K in 1|2) mkdir -p "$T/seedbin"; for t in gen tbl net; do b cc -std=c99 -O2 -w -Iseed "seed/$t.c" -o "$T/seedbin/$t"; done; ST="--seed-tools $T/seedbin";; esac
    [ "$K" != 2 ] || ST="$ST --keep-json"   # errorparse: keep the seed-gen JSON for prep-f1json (F1), no gen.py rerun
    "$_BOUND" 55 python3 exec/c/compilerpack.py --prepare-only $ST --part "$K/5" --o1 "$T/o1.net" --include include -o /dev/null "$T/route.tsv" || exit 1
    "$_BOUND" 55 python3 exec/c/compilerpack.py --require-cached --prepare-only --part "$K/5" --o1 "$T/o1.net" --include include -o /dev/null "$T/route.tsv" || { echo "prep-$K: part $K not cached after preparation" >&2; exit 1; }
    echo "prep-$K: part $K constructed and cached"; exit 0 ;;
esac
# Model construction in the five PREP_PARTS groups (compilerpack.py --part K/5, gate prep-K), each bounded at 55 s
# on its own; the package step below then reads the cache.  One cold bounded step cannot hold all of them (1210-bank).
for k in 1 2 3 4 5; do "$_BOUND" 55 python3 exec/c/compilerpack.py $REQ --prepare-only --part "$k/5" --o1 "$T/o1.net" --include include -o /dev/null "$T/route.tsv" || { [ -z "$REQ" ] || echo "not prepared: run exec-driver-core-prep-$k first" >&2; exit 1; }; done
# The package and the drivers are independent: each is one bounded step started side by side (0.0.43 timeout debt,
# ~13 s of serial cc/asm builds in core-contracts), all are waited for, then their logs are printed in a fixed order and the first failing rc wins.
STARTED=
par() { n=$1; shift; ( b "$@" > "$T/$n.log" 2>&1 ) & eval "PID_$n=\$!"; STARTED="$STARTED $n"; }
par pack python3 exec/c/compilerpack.py $REQ --o1 "$T/o1.net" --include include -o "$T/compiler.pkg" "$T/route.tsv"
case ${1:-all} in all|core|core-modes|core-contracts|core-dependencies|resources|resources-macros|resources-headers|resources-printf|resources-isolated)
par cc cc -O2 -Wall -Wextra exec/c/compiler.c -o "$T/driver-cc"
par ua "$UA" -O2 exec/c/compiler.c -o "$T/driver-ua"
;; core-build)
par ua "$UA" -O2 exec/c/compiler.c -o "$T/driver-ua"
;; esac
case ${1:-all} in core-build) ;; *) par asm env CORE_ASM_ARCH="$ARCH" ./exec/c/asm/cc.sh -O2 exec/c/compiler.c -o "$T/driver-asm";; esac
for n in $STARTED; do eval "p=\$PID_$n"; r=0; wait "$p" || r=$?; eval "RC_$n=$r"; done
for n in $STARTED; do cat "$T/$n.log"; eval "r=\$RC_$n"; [ "$r" -eq 0 ] || exit "$r"; done
# Only the resources contract executes the packaged container. Core checks
# use the three raw drivers; rebuilding an unused APE costs their time budget.
case ${1:-all} in all|resources|resources-isolated)
b python3 exec/c/apecache.py "$T/driver.com" "$UA" "$T/compiler.pkg" > "$T/ape.log" 2>&1 || { cat "$T/ape.log"; exit 1; }   # APE by read-set key (apecache.py); hit re-verifies the artifact
;; esac
b python3 exec/c/compilercheck.py "$T" "$TARGET" "$UA" "$@"

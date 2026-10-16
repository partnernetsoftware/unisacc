#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# Fresh models and product-shaped driver checks. Every inner run <=60 s.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { "$_BOUND" 60 "$@"; }
case $(uname -s) in Darwin) OS=osx;; Linux) OS=lnx;; *) echo 'unsupported test host' >&2; exit 1;; esac
case $(uname -m) in arm64|aarch64) ARCH=arm64;; x86_64) ARCH=x86_64;; *) exit 1;; esac
TARGET=$OS/$ARCH; export TARGET
# resources-* never construct: they need the prep-0..3 jobs (gateorder.json) to have cached the models first
case ${1:-all} in resources-macros|resources-headers|resources-printf|resources-isolated) export UNISACC_REQUIRE_PREPARED=1; REQ=--require-cached;; *) REQ=;; esac
b ./exec/pipeline/elf.sh "$T" examples/hello.c > "$T/build.log" 2>&1 || { cat "$T/build.log"; exit 1; }
b python3 exec/build/gen.py opt "$T/o1.json"
b python3 exec/c/tbl.py "$T/o1.json" "$T/o1.tbl"
b python3 exec/c/net.py "$T/o1.tbl" "$T/o1.net"
b "$T/run" --check-net "$T/o1.tbl" "$T/o1.net"
# Preparation jobs (gate exec-driver-core-prep-K, bindprep pattern): prep-0 builds elf and the o1 chain into the content-addressed
# cache and asserts that a second elf.sh run is a cache hit; prep-K builds part K of PREP_PARTS and then requires it cached
# (--require-cached, so the cache entries and their keys are checked, not rebuilt).  A missing product is rebuilt by the
# same bounded step, as bindprep self-prepares its segment.  Neither is a test verdict.
case ${1:-all} in
prep-0)
    mkdir -p "$T/again"
    b ./exec/pipeline/elf.sh "$T/again" examples/hello.c > "$T/again.log" 2>&1 || { cat "$T/again.log"; exit 1; }
    grep -q "model preparation: hit" "$T/again.log" || { echo "prep-0: second elf run was not a cache hit" >&2; exit 1; }
    echo "prep-0: elf and o1 chain cached; second elf run hit"; exit 0 ;;
prep-1|prep-2|prep-3)
    K=${1#prep-}
    "$_BOUND" 55 python3 exec/c/compilerpack.py --prepare-only --part "$K/3" --o1 "$T/o1.net" --include include -o /dev/null "$T/route.tsv" || exit 1
    "$_BOUND" 55 python3 exec/c/compilerpack.py --require-cached --prepare-only --part "$K/3" --o1 "$T/o1.net" --include include -o /dev/null "$T/route.tsv" || { echo "prep-$K: part $K not cached after preparation" >&2; exit 1; }
    echo "prep-$K: part $K constructed and cached"; exit 0 ;;
esac
# Model construction in the three PREP_PARTS groups (compilerpack.py, buildcompiler pack-prep-K), each bounded at 55 s
# on its own; the package step below then reads the cache.  One cold bounded step cannot hold all of them (1210-bank).
for k in 1 2 3; do "$_BOUND" 55 python3 exec/c/compilerpack.py $REQ --prepare-only --part "$k/3" --o1 "$T/o1.net" --include include -o /dev/null "$T/route.tsv" || { [ -z "$REQ" ] || echo "not prepared: run exec-driver-core-prep-$k first" >&2; exit 1; }; done
b python3 exec/c/compilerpack.py $REQ --o1 "$T/o1.net" --include include -o "$T/compiler.pkg" "$T/route.tsv"
case ${1:-all} in all|core|core-modes|core-contracts|core-dependencies|resources|resources-macros|resources-headers|resources-printf|resources-isolated)
b cc -O2 -Wall -Wextra exec/c/compiler.c -o "$T/driver-cc"
b "$UA" -O2 exec/c/compiler.c -o "$T/driver-ua"
;; core-build)
b "$UA" -O2 exec/c/compiler.c -o "$T/driver-ua"
;; esac
case ${1:-all} in core-build) ;; *) b env CORE_ASM_ARCH="$ARCH" ./exec/c/asm/cc.sh -O2 exec/c/compiler.c -o "$T/driver-asm";; esac
# Only the resources contract executes the packaged container. Core checks
# use the three raw drivers; rebuilding an unused APE costs their time budget.
case ${1:-all} in all|resources|resources-isolated)
b python3 exec/c/apecache.py "$T/driver.com" "$UA" "$T/compiler.pkg" > "$T/ape.log" 2>&1 || { cat "$T/ape.log"; exit 1; }   # APE by read-set key (apecache.py); hit re-verifies the artifact
;; esac
b python3 exec/c/compilercheck.py "$T" "$TARGET" "$UA" "$@"

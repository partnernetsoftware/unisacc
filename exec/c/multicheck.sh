#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# Independent units -> model name isolation -> one program. No runtime Python.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
KIND=${DRIVER_KIND:-all}
case $KIND in all|cc|ua|asm) ;; *) echo 'unknown DRIVER_KIND' >&2; exit 2;; esac
PART=${MULTI_PART:-all}
case $PART in all|tapes-m-forward|tapes-m-reverse|tapes-n-forward|tapes-n-reverse|isolation|static|frame) ;; *) echo 'unknown MULTI_PART' >&2; exit 2;; esac
[ "$PART" != frame ] || [ "$KIND" = cc ] || [ "$KIND" = all ] || { echo 'frame requires cc driver kind' >&2; exit 2; }
export DRIVER_KIND MULTI_PART
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { "$_BOUND" 60 "$@"; }
case $(uname -s) in Darwin) OS=osx;; Linux) OS=lnx;; *) exit 1;; esac
case $(uname -m) in arm64|aarch64) ARCH=arm64;; x86_64) ARCH=x86_64;; *) exit 1;; esac
TARGET=$OS/$ARCH; export TARGET
b ./exec/pipeline/elf.sh "$T" examples/hello.c > "$T/build.log" 2>&1 || { cat "$T/build.log"; exit 1; }
if [ "$PART" != frame ]; then
    b python3 exec/build/gen.py opt "$T/o1.json"
    b python3 exec/c/tbl.py "$T/o1.json" "$T/o1.tbl"
    b python3 exec/c/net.py "$T/o1.tbl" "$T/o1.net"
    b python3 exec/c/compilerpack.py --o1 "$T/o1.net" --include include -o "$T/compiler.pkg" "$T/route.tsv"
fi
if { [ "$KIND" = all ] || [ "$KIND" = cc ]; } && { [ "$PART" = all ] || [ "$PART" = frame ]; }; then
    b python3 exec/parse2/units.py "$T/units.json"
    b python3 exec/c/tbl.py "$T/units.json" "$T/units.tbl"
    b python3 exec/c/net.py "$T/units.tbl" "$T/units.net"
    b "$T/run" --check-net "$T/units.tbl" "$T/units.net"
fi
if [ "$PART" != frame ]; then
    if [ "$KIND" = all ] || [ "$KIND" = cc ]; then b cc -O2 exec/c/compiler.c -o "$T/driver-cc"; fi
    if [ "$KIND" = all ] || [ "$KIND" = ua ]; then b "$UA" -O2 exec/c/compiler.c -o "$T/driver-ua"; fi
    if [ "$KIND" = all ] || [ "$KIND" = asm ]; then b env CORE_ASM_ARCH="$ARCH" ./exec/c/asm/cc.sh -O2 exec/c/compiler.c -o "$T/driver-asm"; fi
fi
b python3 exec/c/multicheck.py "$T" "$TARGET" "$UA"

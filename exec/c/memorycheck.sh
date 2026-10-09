#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# Native-memory compilation: model bytes against retained bk_run, then execute.
# POSIX host only here; cross-image gates retain all six file targets.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
KIND=${DRIVER_KIND:-all}
case $KIND in all|cc|ua|asm) ;; *) echo 'unknown DRIVER_KIND' >&2; exit 2;; esac
export DRIVER_KIND
case ${MEMORY_SHARD:-all} in all|1/3|2/3|3/3) ;; *) echo 'unknown MEMORY_SHARD' >&2; exit 2;; esac
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { "$_BOUND" 60 "$@"; }
b python3 tests/sourceflat.py "$T/unisacc-flat.c"
SELF="$T/unisacc-flat.c"
case $(uname -s) in Darwin) OS=osx;; Linux) OS=lnx;; *) echo 'unsupported memory-check host' >&2; exit 1;; esac
case $(uname -m) in arm64|aarch64) ARCH=arm64;; x86_64) ARCH=x86_64;; *) exit 1;; esac
HOST_ARCH=$ARCH
ARCH=${MEMORY_ARCH:-$ARCH}
case "$OS/$ARCH" in osx/arm64|osx/x86_64|lnx/arm64|lnx/x86_64) ;; *) echo 'unsupported memory-check target' >&2; exit 1;; esac
if [ "$OS" != osx ] && [ "$ARCH" != "$HOST_ARCH" ]; then
    echo 'memory execution requires a native host or Rosetta' >&2; exit 1
fi
hostcc() { if [ "$OS" = osx ]; then b cc -arch "$ARCH" "$@"; else b cc "$@"; fi; }
TARGET=$OS/$ARCH; export TARGET
# The package prep does not depend on DRIVER_KIND or MEMORY_SHARD: 18 gate jobs built it 18 times and
# one hit the 60 s window under load.  On a clean tree it is cached by the tracked inputs' blob ids.
prep() {
b ./exec/pipeline/elf.sh "$T" examples/hello.c > "$T/build.log" 2>&1 || { cat "$T/build.log"; exit 1; }
b python3 exec/build/gen.py opt "$T/o1.json"
b python3 exec/c/tbl.py "$T/o1.json" "$T/o1.tbl"
b python3 exec/c/net.py "$T/o1.tbl" "$T/o1.net"
b python3 exec/c/compilerpack.py --o1 "$T/o1.net" --include include -o "$T/compiler.pkg" "$T/route.tsv"
}
KEY=
if git rev-parse -q --verify HEAD >/dev/null 2>&1 && [ -z "$(git status --porcelain --untracked-files=no -- exec unisa include src kernel weights tests examples/hello.c 2>/dev/null)" ]; then
    KEY=$( (echo "$TARGET"; git ls-files -s -- exec unisa include src kernel weights tests examples/hello.c) | shasum -a 256 | cut -c1-16)
fi
C=${TMPDIR:-/tmp}/unisacc-memprep-$KEY
if [ -n "$KEY" ] && [ -f "$C/ok" ]; then
    cp -R "$C/." "$T/"; rm -f "$T/ok"
else
    prep
    if [ -n "$KEY" ] && mkdir "$C.lock" 2>/dev/null; then
        rm -rf "$C.tmp.$$"; cp -R "$T" "$C.tmp.$$" && rm -f "$C.tmp.$$/unisacc-flat.c" && touch "$C.tmp.$$/ok" \
            && { [ -d "$C" ] || mv "$C.tmp.$$" "$C"; }; rm -rf "$C.tmp.$$" "$C.lock"
    fi
fi
if [ "$KIND" = all ] || [ "$KIND" = cc ]; then hostcc -O2 exec/c/compiler.c -o "$T/driver-cc"; fi
if [ "$KIND" = all ] || [ "$KIND" = ua ]; then b "$UA" -b "$TARGET" -O2 exec/c/compiler.c -o "$T/driver-ua"; fi
if [ "$KIND" = all ] || [ "$KIND" = asm ]; then b env CORE_ASM_ARCH="$ARCH" ./exec/c/asm/cc.sh -O2 exec/c/compiler.c -o "$T/driver-asm"; fi
if [ "$KIND" = all ] || [ "$KIND" = cc ]; then
cat tests/refshim.h "$SELF" exec/c/memory-ref.c > "$T/ref.c"
hostcc -w -O1 "$T/ref.c" -o "$T/ref-memory"
fi
b python3 exec/c/memorycheck.py "$T" "$TARGET" "$UA"

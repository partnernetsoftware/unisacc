#!/bin/sh
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
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
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
b ./exec/pipeline/elf.sh "$T" examples/hello.c > "$T/build.log" 2>&1 || { cat "$T/build.log"; exit 1; }
b python3 exec/opt/gen.py "$T/o1.json" 1
b python3 exec/c/tbl.py "$T/o1.json" "$T/o1.tbl"
b python3 exec/c/net.py "$T/o1.tbl" "$T/o1.net"
b python3 exec/c/compilerpack.py --o1 "$T/o1.net" --include include -o "$T/compiler.pkg" "$T/route.tsv"
if [ "$KIND" = all ] || [ "$KIND" = cc ]; then hostcc -O2 exec/c/compiler.c -o "$T/driver-cc"; fi
if [ "$KIND" = all ] || [ "$KIND" = ua ]; then b "$UA" -b "$TARGET" -O2 exec/c/compiler.c -o "$T/driver-ua"; fi
if [ "$KIND" = all ] || [ "$KIND" = asm ]; then b env CORE_ASM_ARCH="$ARCH" ./exec/c/asm/cc.sh -O2 exec/c/compiler.c -o "$T/driver-asm"; fi
if [ "$KIND" = all ] || [ "$KIND" = cc ]; then
cat tests/refshim.h unisacc.c exec/c/memory-ref.c > "$T/ref.c"
hostcc -w -O1 "$T/ref.c" -o "$T/ref-memory"
fi
b python3 exec/c/memorycheck.py "$T" "$TARGET" "$UA"

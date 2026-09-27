#!/bin/sh
# Build real packaged warning routes, then compare the driver contract.
set -eu
KIND=${1:-all}; FLAG=${2:-all}
[ "$#" -le 2 ] || { echo 'usage: warningcheck.sh [cc|ua|asm|all] [Wall|Wextra|Werror|all]' >&2; exit 2; }
case $KIND in all|cc|ua|asm) ;; *) echo 'unknown warning driver' >&2; exit 2;; esac
case $FLAG in all|Wall|Wextra|Werror) ;; *) echo 'unknown warning flag shard' >&2; exit 2;; esac
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl "$R/tests/bound.pl" 45 "$@"; }
case $(uname -s) in Darwin) OS=osx;; Linux) OS=lnx;; *) exit 1;; esac
case $(uname -m) in arm64|aarch64) ARCH=arm64;; x86_64) ARCH=x86_64;; *) exit 1;; esac
TARGET=$OS/$ARCH; export TARGET
b ./exec/pipeline/elf.sh "$T" examples/hello.c > "$T/build.log" 2>&1 || { cat "$T/build.log"; exit 1; }
b python3 exec/opt/gen.py "$T/o1.json" 1
b python3 exec/c/tbl.py "$T/o1.json" "$T/o1.tbl"
b python3 exec/c/net.py "$T/o1.tbl" "$T/o1.net"
b python3 exec/c/compilerpack.py --o1 "$T/o1.net" --include include -o "$T/compiler.pkg" "$T/route.tsv"
if [ "$KIND" = all ] || [ "$KIND" = cc ]; then b cc -O2 -Wall -Wextra exec/c/compiler.c -o "$T/driver-cc"; fi
if [ "$KIND" = all ] || [ "$KIND" = ua ]; then b "$UA" -O2 exec/c/compiler.c -o "$T/driver-ua"; fi
if [ "$KIND" = all ] || [ "$KIND" = asm ]; then b env CORE_ASM_ARCH="$ARCH" ./exec/c/asm/cc.sh -O2 exec/c/compiler.c -o "$T/driver-asm"; fi
echo "warning driver ready: $KIND $FLAG"
b python3 exec/c/warningcheck.py "$T" "$TARGET" "$UA" "$KIND" "$FLAG"

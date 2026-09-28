#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# Check source maps and diagnostics across independently preprocessed units.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { "$_BOUND" 60 "$@"; }
case $(uname -s) in Darwin) OS=osx;; Linux) OS=lnx;; *) exit 1;; esac
case $(uname -m) in arm64|aarch64) ARCH=arm64;; x86_64) ARCH=x86_64;; *) exit 1;; esac
TARGET=$OS/$ARCH; export TARGET
b ./exec/pipeline/elf.sh "$T" examples/hello.c > "$T/build.log" 2>&1 || { cat "$T/build.log"; exit 1; }
b python3 exec/opt/gen.py "$T/o1.json" 1
b python3 exec/c/tbl.py "$T/o1.json" "$T/o1.tbl"
b python3 exec/c/net.py "$T/o1.tbl" "$T/o1.net"
b python3 exec/c/compilerpack.py --o1 "$T/o1.net" --include include -o "$T/compiler.pkg" "$T/route.tsv"
b cc -O2 -Wall -Wextra exec/c/compiler.c -o "$T/driver-cc"
b "$UA" -O2 exec/c/compiler.c -o "$T/driver-ua"
b env CORE_ASM_ARCH="$ARCH" ./exec/c/asm/cc.sh -O2 exec/c/compiler.c -o "$T/driver-asm"
b python3 exec/c/multiwarningcheck.py "$T" "$TARGET" "$UA"

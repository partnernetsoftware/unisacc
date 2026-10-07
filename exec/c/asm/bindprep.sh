#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# The preparation part of the assembly binding check.  bindingcheck.sh is one
# job in the gate and measured 61 s warm -- past the 60 s watchdog, killed with
# rc=142 before its asserts ran.  Traced warm, the cost is not where the plan
# guessed: blob construction 1 s, netcheck 1 s, the o1 pack chain 2 s and the
# driver compile 0 s, while `elf.sh` over four files took 31 s and the assert
# block 9 s.  So the split point is those two, and this file is everything
# before them.
#
# Artefacts land in $X (exec/stamp.sh), which is private to this checkout and
# rebuilds when its inputs change.  Three stale files in a shared /tmp have
# already cost this repository a wrong result, so the shared directory is not
# hand-rolled: it is the one that already carries the content hash.
set -eu
R=$(cd "$(dirname "$0")/../../.." && pwd); cd "$R"
PART=${1:-all}
case $PART in first|second|package|all) ;; *) echo 'usage: bindprep.sh [first|second|package]' >&2; exit 2;; esac
. ./tests/lib.sh; ua_ready
[ "$(uname -s)" = Darwin ] || { echo 'blob seed construction requires macOS' >&2; exit 2; }
ARCH=${CORE_ASM_ARCH:-$(uname -m)}
case $ARCH in arm64|x86_64) ;; *) exit 2;; esac
. ./exec/stamp.sh
D="$X/bind-$ARCH"
mkdir -p "$D"
E="$D/elf-$ARCH"
mkdir -p "$E"
# prep.done marks a run that reached the end; a prep killed by its watchdog
# leaves fresh route.tsv beside an old compiler.pkg, and verify must not take
# that mix for a prepared directory (0.0.24, cdx diagnosis).
b() { "$_BOUND" 60 "$@"; }

if [ "$PART" = first ] || [ "$PART" = all ]; then
rm -f "$D/first.done" "$D/images.done" "$D/prep.done"
# The seed blob and the carried kernels are indexed by ISA, so each arch gets
# its own pair under the same $X.  blobs are rebuilt by fresh(), not by hand.
b python3 exec/c/asm/blob.py "$ARCH" "$D/kernel.blob"
mkdir -p "$D/kernels"
for ISA in arm64 x86_64; do b python3 exec/c/asm/blob.py "$ISA" "$D/kernels/$ISA"; done

export CORE_ASM_ARCH=$ARCH UNISA_KERNEL="$D/kernel.blob"
export EXEC_CC="$R/exec/c/asm/blobcc.sh" TARGET="osx/$ARCH"
b python3 exec/c/netcheck.py

# elf.sh runs here, and it is not optional at this point: it reads its own
# OUTPUT_DIR back out.  elf.sh -> models.py -> prepare.sh writes route.tsv
# (prepare.sh:31) and every stage's .net into that directory, and the o1 pack
# chain below resolves each model as `manifest.parent / model`.  So route.tsv
# and the seven .net files are an output of this step, not a coincidence of
# sharing one $T -- moving elf.sh into a later third (the first attempt at
# this split) failed on exactly that, first with a missing elf.net and then
# with a route.tsv whose columns were wrong: compilerpack asserts five
# columns per line and the stage names in order.
b ./exec/pipeline/elf.sh "$E" examples/hello.c examples/fib.c > "$E/build.log" 2>&1 || { cat "$E/build.log"; exit 1; }
b python3 exec/build/gen.py opt "$D/o1.json"
b python3 exec/c/tbl.py "$D/o1.json" "$D/o1.tbl"
b python3 exec/c/net.py "$D/o1.tbl" "$D/o1.net"
: > "$D/first.done"
echo "bind prep $ARCH first: blob, kernels, netcheck, two images in $D"
fi

if [ "$PART" = second ] || [ "$PART" = all ]; then
if [ ! -f "$D/first.done" ]; then
    echo "bind prep $ARCH: first part is incomplete" >&2
    exit 1
fi
rm -f "$D/images.done" "$D/prep.done"
export CORE_ASM_ARCH=$ARCH UNISA_KERNEL="$D/kernel.blob"
export EXEC_CC="$R/exec/c/asm/blobcc.sh" TARGET="osx/$ARCH"
# A second elf.sh call reuses the same content-addressed models.  Restore its
# complete input ledger after the second call, which otherwise lists only its
# two inputs.  The four images and route.tsv remain in the same directory.
b ./exec/pipeline/elf.sh "$E" tests/c/b_strderef.c exec/c/run.c >> "$E/build.log" 2>&1 || { cat "$E/build.log"; exit 1; }
printf '%s\n' hello fib b_strderef run > "$E/inputs"
UNISACC_MODEL_CACHE="$D/model-cache" b python3 exec/c/compilerpack.py --prepare-only --part 2/3 --o1 "$D/o1.net" --include include -o /dev/null "$E/route.tsv"
: > "$D/images.done"
echo "bind prep $ARCH second: four images, pack models 2/3 in $D"
fi

if [ "$PART" = package ] || [ "$PART" = all ]; then
if [ ! -f "$D/images.done" ]; then
    echo "bind prep $ARCH: image part is incomplete" >&2
    exit 1
fi
rm -f "$D/prep.done"
export CORE_ASM_ARCH=$ARCH UNISA_KERNEL="$D/kernel.blob"
export EXEC_CC="$R/exec/c/asm/blobcc.sh" TARGET="osx/$ARCH"
UNISACC_MODEL_CACHE="$D/model-cache" b python3 exec/c/compilerpack.py --prepare-only --part 1/3 --o1 "$D/o1.net" --include include -o /dev/null "$E/route.tsv"
UNISACC_MODEL_CACHE="$D/model-cache" b python3 exec/c/compilerpack.py --prepare-only --part 3/3 --o1 "$D/o1.net" --include include -o /dev/null "$E/route.tsv"
UNISACC_MODEL_CACHE="$D/model-cache" b python3 exec/c/compilerpack.py --require-cached --o1 "$D/o1.net" --include include --kernels "$D/kernels" -o "$D/compiler.pkg" "$E/route.tsv"
b "$EXEC_CC" -O2 exec/c/compiler.c -o "$D/compiler"
: > "$D/prep.done"
echo "bind prep $ARCH package: package and driver in $D"
fi

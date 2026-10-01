#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# Current compiler source through seven deltas to the selected target image.
# The output compiles the existing C compiler; it is not E7 product adoption.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
TARGET=${TARGET:-lnx/x86_64}; export TARGET
SELF_PART=${SELF_PART:-all}
case $SELF_PART in all|stages|package|bootstrap) ;; *) echo "unknown selfcheck part: $SELF_PART" >&2; exit 2;; esac
[ "$SELF_PART" != package ] || [ "${NETWORK:-1}" = 1 ] || { echo 'package selfcheck requires NETWORK=1' >&2; exit 2; }
UA=${UA:-/tmp/ua_ref}; . ./tests/lib.sh; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { "$_BOUND" 60 "$@"; }
b python3 tests/sourceflat.py "$T/unisacc.c"
SELF="$T/unisacc.c"
if [ "$SELF_PART" = bootstrap ]; then
    case "$(uname -s)/$(uname -m):$TARGET" in
        Darwin/arm64:osx/*|Darwin/x86_64:osx/x86_64) ;;
        *) echo "bootstrap selfcheck needs a native macOS target: $TARGET" >&2; exit 2;;
    esac
    b "$UA" -O2 -b "$TARGET" "$SELF" -o "$T/n1"
    b "$T/n1" -O2 -b "$TARGET" "$SELF" -o "$T/n2"
    b "$T/n2" -O2 -b "$TARGET" "$SELF" -o "$T/n3"
    cmp "$T/n1" "$T/n2" && cmp "$T/n2" "$T/n3"
    echo "bootstrap $TARGET: reference N1=N2=N3 (stage selfcheck compares N1)"
    exit 0
fi
if [ "$SELF_PART" != package ]; then
b ./exec/pipeline/elf.sh "$T" "$SELF" > "$T/log" 2>&1 || { cat "$T/log"; exit 1; }
else
    b python3 exec/pipeline/models.py "$T" "$TARGET" "${NETWORK:-1}" "${EXEC_CC:-cc}" > "$T/log" 2>&1 || { cat "$T/log"; exit 1; }
fi
# All target predefines have value 1; other platform/architecture names
# must be absent. Exercise the generated E2 and the product -E independently.
case $TARGET in */arm64) arch=__aarch64__; other=__x86_64__;; *) arch=__x86_64__; other=__aarch64__;; esac
case $TARGET in
    win/*) osnames="_WIN32 _WIN64"; absent="__APPLE__ __MACH__ __unix__ __linux__ __ELF__"; IMAGE=exe;;
    osx/*) osnames='__APPLE__ __MACH__ __unix__'; absent='__linux__ __ELF__ _WIN32 _WIN64'; IMAGE=macho;;
    *) osnames='__linux__ __unix__ __ELF__'; absent='__APPLE__ __MACH__ _WIN32 _WIN64'; IMAGE=elf;;
esac
expected=''
for name in $osnames "$arch" __LP64__ __UNISA__; do expected=${expected}1; done
printf '%s\n' $osnames "$arch" __LP64__ __UNISA__ > "$T/macros.c"
for name in "$other" $absent; do
    printf '#ifdef %s\nWRONG_TARGET\n#endif\n' "$name" >> "$T/macros.c"
done
MODEL=tbl; [ "${NETWORK:-1}" != 1 ] || MODEL=net
if [ "$SELF_PART" != package ]; then
    b "$T/run" "$T/e2.$MODEL" "$T/macros.c" > "$T/macros.delta"
    b "$UA" -b "$TARGET" -E "$T/macros.c" > "$T/macros.ref"
    for file in "$T/macros.delta" "$T/macros.ref"; do
        [ "$(tr -d '[:space:]' < "$file")" = "$expected" ] || { echo 'target predefines differ' >&2; exit 1; }
    done
    b "$UA" -O2 -b "$TARGET" -S "$SELF" -o "$T/ref.tape"
    cmp "$T/ref.tape" "$T/unisacc.e4"
fi
b "$UA" -O2 -b "$TARGET" "$SELF" -o "$T/ref.elf"
if [ "$SELF_PART" != package ]; then
    cmp "$T/ref.elf" "$T/unisacc.$IMAGE"
fi
if [ "$MODEL" = net ] && [ "$SELF_PART" != stages ]; then
    b env UNISA_MAXSTEPS=400000000000 "$T/run" --bundle "$T/models.pkg" "$TARGET" "$SELF" "$SELF" "$R/include" > "$T/pack.image"
    cmp "$T/pack.image" "$T/ref.elf"
    if [ "$SELF_PART" = all ]; then cmp "$T/pack.image" "$T/unisacc.$IMAGE"; fi
    echo "package self-source $TARGET: same image as reference"
fi
if [ "$SELF_PART" = package ]; then
    echo "package self-source $TARGET: $(wc -c < "$T/pack.image" | tr -d ' ') bytes equal"
    exit 0
fi
# Execute the resulting compiler only on a host that can run this image.
case "$(uname -s)/$(uname -m):$TARGET" in
    Darwin/arm64:osx/*|Darwin/x86_64:osx/x86_64)
        b "$T/unisacc.$IMAGE" -O2 -b "$TARGET" "$SELF" -o "$T/n2"
        b "$T/n2" -O2 -b "$TARGET" "$SELF" -o "$T/n3"
        cmp "$T/unisacc.$IMAGE" "$T/n2" && cmp "$T/n2" "$T/n3"
        echo "$MODEL bootstrap $TARGET: N1=N2=N3" ;;
    *) echo "$MODEL bootstrap $TARGET: not executed on this host" ;;
esac
echo "$MODEL self-source $TARGET: $(wc -c < "$T/unisacc.$IMAGE" | tr -d ' ') bytes equal; bootstrap result above"

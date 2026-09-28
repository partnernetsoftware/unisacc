#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# The allocator under load: tests/malloc/*.c churn far past any fixed pool
# (100 MB+ total, peak a few MB).  Each program runs under unisacc -run and
# as a built host image, and must print what the system compiler's build
# prints.  Too much work for the reference VM, so these are not tests/c
# probes (tests/c/b_malloc.c is the small one closure runs).
set -u
R=$(cd "$(dirname "$0")/.." && pwd)
. "$R/tests/lib.sh"; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
HOSTT=$(host_target)
ok=0; bad=0
for f in "$R"/tests/malloc/*.c; do
    b=$(basename "$f" .c)
    cc -w -o "$T/$b.cc" "$f" || { echo "  $b: cc failed"; bad=$((bad+1)); continue; }
    "$_BOUND" 20 "$T/$b.cc" > "$T/$b.want" 2>&1
    "$_BOUND" 20 "$UA" "$f" -run > "$T/$b.run" 2>&1
    if cmp -s "$T/$b.want" "$T/$b.run"; then ok=$((ok+1)); else bad=$((bad+1)); echo "  $b: -run differs"; fi
    "$UA" "$f" -b "$HOSTT" > "$T/$b.img" 2>/dev/null && chmod +x "$T/$b.img"
    "$_BOUND" 20 "$T/$b.img" > "$T/$b.out" 2>&1
    if cmp -s "$T/$b.want" "$T/$b.out"; then ok=$((ok+1)); else bad=$((bad+1)); echo "  $b: $HOSTT image differs"; fi
    if [ "$HOSTT" = osx/arm64 ] && arch -x86_64 /usr/bin/true 2>/dev/null; then   # Rosetta
        "$UA" "$f" -b osx/x86_64 > "$T/$b.x86" 2>/dev/null && chmod +x "$T/$b.x86"
        "$_BOUND" 30 arch -x86_64 "$T/$b.x86" > "$T/$b.xout" 2>&1
        if cmp -s "$T/$b.want" "$T/$b.xout"; then ok=$((ok+1)); else bad=$((bad+1)); echo "  $b: osx/x86_64 image differs"; fi
    fi
done
echo "malloc  agree $ok  differ $bad"
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]

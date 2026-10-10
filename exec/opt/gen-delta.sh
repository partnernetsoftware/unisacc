#!/bin/sh
# K5-1: construct opt δ JSON. Prefer seed/gen.c (seed-gen); Python remains the
# independent byte reference (SEED_GEN=0). On seed-gen failure do not silently
# fall back to gen.py. exec/opt/check.sh still calls gen.py until a later cut.
# 0.0.40 (机房主任 23:41): SEED_GEN is 0 or 1 (anything else rc 2); at most one flag (--o2); the default
# seed-gen build is keyed by the content of seed/*.c seed/*.h, the compiler and the flags, not by mtime.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
usage() { echo "usage: exec/opt/gen-delta.sh OUT.json [--o2]" >&2; exit 2; }
[ $# -ge 1 ] && [ $# -le 2 ] || usage
OUT=$1; shift
case "${1:-}" in ""|--o2) ;; *) usage;; esac
B=$R/tests/bound
case "${SEED_GEN-1}" in
    0) exec "$B" 55 python3 exec/build/gen.py opt "$OUT" "$@";;
    1) ;;
    *) echo "exec/opt/gen-delta.sh: SEED_GEN must be 0 or 1, got '${SEED_GEN}'" >&2; exit 2;;
esac
if [ -n "${SEED_GEN_BIN:-}" ]; then
    # explicit binary: fail closed if missing/unusable (no rebuild, no Python)
    [ -x "$SEED_GEN_BIN" ] || { echo "exec/opt/gen-delta.sh: SEED_GEN_BIN not executable: $SEED_GEN_BIN" >&2; exit 2; }
    G=$SEED_GEN_BIN
else
    CC=${SEED_GEN_CC:-cc}; FLAGS="-std=c99 -O2 -w"
    ccpath=$(command -v "$CC") || { echo "exec/opt/gen-delta.sh: no compiler $CC" >&2; exit 2; }
    key=$({ printf '%s\n%s\n' "$ccpath" "$FLAGS"; "$CC" --version 2>&1 | head -1; cat seed/*.c seed/*.h; } | python3 -c 'import hashlib,sys;print(hashlib.sha256(sys.stdin.buffer.read()).hexdigest()[:16])')
    D=${SEED_GEN_DIR:-${TMPDIR:-/tmp}/unisacc-seedbin}
    mkdir -p "$D"
    G=$D/seed-gen-$key
    if [ ! -x "$G" ]; then
        "$B" 55 "$CC" $FLAGS -I"$R/seed" "$R/seed/gen.c" -o "$G.$$" \
            && mv -f "$G.$$" "$G" || { rm -f "$G.$$"; echo "exec/opt/gen-delta.sh: seed-gen build failed" >&2; exit 1; }
    fi
fi
# prefer seed-gen; failure is failure (no Python masquerade)
exec "$B" 55 "$G" opt "$OUT" "$@"

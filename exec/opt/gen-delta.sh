#!/bin/sh
# K5-1: construct opt δ JSON. Prefer seed/gen.c (seed-gen); Python remains the
# independent byte reference (SEED_GEN=0). On seed-gen failure do not silently
# fall back to gen.py. exec/opt/check.sh still calls gen.py until a later cut.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
[ $# -ge 1 ] || { echo "usage: exec/opt/gen-delta.sh OUT.json [--o2]" >&2; exit 2; }
OUT=$1; shift
case "${1:-}" in ""|--o2) ;; *) echo "usage: exec/opt/gen-delta.sh OUT.json [--o2]" >&2; exit 2;; esac
B=$R/tests/bound
if [ "${SEED_GEN:-1}" != 1 ]; then
    "$B" 55 python3 exec/build/gen.py opt "$OUT" "$@"
    exit $?
fi
if [ -n "${SEED_GEN_BIN:-}" ]; then
    # explicit binary: fail closed if missing/unusable (no rebuild, no Python)
    [ -x "$SEED_GEN_BIN" ] || { echo "exec/opt/gen-delta.sh: SEED_GEN_BIN not executable: $SEED_GEN_BIN" >&2; exit 2; }
    G=$SEED_GEN_BIN
else
    D=${SEED_GEN_DIR:-${TMPDIR:-/tmp}/unisacc-seedbin}
    mkdir -p "$D"
    G=$D/seed-gen
    if [ ! -x "$G" ] || [ seed/gen.c -nt "$G" ]; then
        "$B" 55 ${SEED_GEN_CC:-cc} -std=c99 -O2 -w -I"$R/seed" "$R/seed/gen.c" -o "$G.$$" \
            && mv -f "$G.$$" "$G" || { echo "exec/opt/gen-delta.sh: seed-gen build failed" >&2; exit 1; }
    fi
fi
# prefer seed-gen; failure is failure (no Python masquerade)
"$B" 55 "$G" opt "$OUT" "$@"

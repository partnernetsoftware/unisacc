#!/bin/bash
# Independent U (Python source frontend) = B (classic tape) = C (B tape).
# B->C and U are independent branches; run them concurrently, never reuse B
# as U's source. Every invocation uses its own scratch; no result cache.
set -eu
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"; ua_ready
T=$(scratch)
# 1417/1413 diagnostic copy, env-gated: unset = no change.  CI macOS suites sets
# UNISACC_DIAG_OUT so branch files are copied before lib.sh's EXIT trap removes the
# scratch; upload-artifact runs only on failure.  Never changes the verdict; copy
# failures are ignored.
diag_copy() {
    [ -n "${UNISACC_DIAG_OUT:-}" ] || return 0
    mkdir -p "$UNISACC_DIAG_OUT/bootstrap" 2>/dev/null || return 0
    for f in B.log U.log bs_B.tape bs_C.tape bs_UT.tape bs_B bs_U unisacc.flat.c; do
        [ -e "$T/$f" ] && cp -p "$T/$f" "$UNISACC_DIAG_OUT/bootstrap/$f" 2>/dev/null || :
    done
    { echo "host=$(uname -a)"; echo "date_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"; echo "brc=${brc:-na} urc=${urc:-na}"; } \
        >"$UNISACC_DIAG_OUT/bootstrap/meta.txt" 2>/dev/null || :
    return 0
}
SELF="$T/unisacc.flat.c"
bound 10 python3 "$R/tests/sourceflat.py" "$SELF" || exit 1
HOST_TARGET=${HOST_TARGET:-$(host_target)}
case "$HOST_TARGET" in osx/arm64|osx/x86_64|lnx/arm64|lnx/x86_64) ;;
    *) echo "bootstrap: unsupported native target [$HOST_TARGET]" >&2; exit 2;;
esac
[ "$HOST_TARGET" = "$(host_target)" ] || { echo 'bootstrap needs the native host target' >&2; exit 2; }
sign_image() {
    chmod +x "$1"
    if command -v codesign >/dev/null; then bound 10 codesign -f -s - "$1" >/dev/null 2>&1; fi
}
branch_b() {
    bound 30 "$UA" "$SELF" -t "$HOST_TARGET" -c > "$T/bs_B.tape"
    [ -s "$T/bs_B.tape" ] || { echo 'empty B tape'; return 1; }
    bound 45 python3 -m unisa compile "$T/bs_B.tape" --from-tape -o "$T/bs_B" \
        --target "$HOST_TARGET" --drive built
    [ -s "$T/bs_B" ] || { echo 'empty B image'; return 1; }
    sign_image "$T/bs_B"
    bound 30 "$T/bs_B" "$SELF" -t "$HOST_TARGET" -c > "$T/bs_C.tape"
    [ -s "$T/bs_C.tape" ] || { echo 'empty C tape'; return 1; }
}
branch_u() {
    bound 45 python3 -m unisa compile "$SELF" -o "$T/bs_U" \
        --target "$HOST_TARGET" --drive built
    [ -s "$T/bs_U" ] || { echo 'empty U image'; return 1; }
    sign_image "$T/bs_U"
    bound 30 "$T/bs_U" "$SELF" -t "$HOST_TARGET" -c > "$T/bs_UT.tape"
    [ -s "$T/bs_UT.tape" ] || { echo 'empty U tape'; return 1; }
}
# A background shell retains errexit; waiting preserves both branch failures.
branch_b >"$T/B.log" 2>&1 & bp=$!
branch_u >"$T/U.log" 2>&1 & up=$!
brc=0; wait "$bp" || brc=$?
urc=0; wait "$up" || urc=$?
diag_copy
if [ "$brc" -ne 0 ] || [ "$urc" -ne 0 ]; then
    echo "  FAIL bootstrap branches B=$brc U=$urc"
    cat "$T/B.log" "$T/U.log"; exit 1
fi
ok=1
cmp -s "$T/bs_B.tape" "$T/bs_C.tape" || { echo '  FAIL B != C'; ok=0; }
cmp -s "$T/bs_B.tape" "$T/bs_UT.tape" || { echo '  FAIL B != U'; ok=0; }
printf '  %s B=C=U %s (%s lines)\n' \
    "$([ "$ok" = 1 ] && echo ok || echo FAIL)" \
    "$(shasum < "$T/bs_B.tape" | cut -c1-16)" "$(wc -l < "$T/bs_B.tape")"
[ "$ok" = 1 ] && echo 'bootstrap fixed point reached' || echo 'bootstrap NOT reached'
[ "$ok" = 1 ]

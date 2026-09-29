#!/bin/bash
# unisacc.c built two ways must behave identically, and must agree with the
# Python front end.  This is the self-hosting ladder. [A-20]
#
# The verdict used to be lost: `lexdiff.sh | tail -1` reports the pipe's exit
# status, so a run that agreed on 0 of 44 files still scored `ok`.  CI showed
# `selfhost lexer agree 0 differ 44   ok`.
set -u
rc=0
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"; ua_ready
T=$(scratch)
SELF="$T/unisacc.flat.c"
bound 10 python3 "$R/tests/sourceflat.py" "$SELF" || exit 1
case "${1:-}" in
    --prepare-tape|--prepare-image)
        [ -n "${SELFHOST_STATE:-}" ] || { echo 'SELFHOST_STATE required'; exit 2; }
        bound 52 python3 tests/selfhost_prepare.py "${1#--prepare-}" "$SELFHOST_STATE" "$UA"
        exit $?;;
esac

run() {   # run() <banner> <env-assignment...>
    local banner=$1; shift
    echo "== $banner =="
    local out; out=$(bound 55 env "$@" ./tests/lexdiff.sh "${ARGS[@]}"); local r=$?
    printf '%s\n' "$out" | tail -1
    [ "$r" -eq 0 ] || rc=1
}

ARGS=()
for f in "$@"; do
    case "$f" in unisacc.c|"$R/unisacc.c") ARGS+=("$SELF");; *) ARGS+=("$f");; esac
done
run "unisacc built by cc" UA="$UA"
if [ "$(uname -s)" = "Darwin" ] && [ "$(uname -m)" = "arm64" ]; then
    if [ -n "${SELFHOST_STATE:-}" ]; then
        # A verify failure must name itself: CI once showed rc=1 with no message.
        self=$(bound 10 python3 tests/selfhost_prepare.py verify "$SELFHOST_STATE" "$UA" 2>"$T/verify.err") || {
            echo "selfhost verify failed (rc=$?): state=$SELFHOST_STATE"; cat "$T/verify.err"; ls -la "$SELFHOST_STATE" 2>&1; exit 1; }
    else
        bound 45 python3 -m unisa compile "$SELF" -o "$T/ua_self" --target osx/arm64 \
            --drive built >"$T/build.out" 2>"$T/build.err"; brc=$?
        if [ "$brc" -ne 0 ]; then echo "selfhost build rc=$brc"; cat "$T/build.out" "$T/build.err"; exit 1; fi
        self="$T/ua_self"
        chmod +x "$self"; bound 10 codesign -f -s - "$self" >/dev/null 2>&1 || exit 1
    fi
    run "unisacc built by unisa itself" UA="$self"
fi
exit $rc

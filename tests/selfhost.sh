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

run() {   # run() <banner> <env-assignment...>
    local banner=$1; shift
    echo "== $banner =="
    local out; out=$(bound 55 env "$@" ./tests/lexdiff.sh "${ARGS[@]}"); local r=$?
    printf '%s\n' "$out" | tail -1
    [ "$r" -eq 0 ] || rc=1
}

ARGS=("$@")
run "unisacc built by cc" UA="$UA"
if [ "$(uname -s)" = "Darwin" ] && [ "$(uname -m)" = "arm64" ]; then
    bound 45 python3 -m unisa compile unisacc.c -o "$T/ua_self" --target osx/arm64 \
        --drive built >/dev/null || exit 1
    chmod +x "$T/ua_self"; bound 10 codesign -f -s - "$T/ua_self" >/dev/null 2>&1 || exit 1
    run "unisacc built by unisa itself" UA="$T/ua_self"
fi
exit $rc

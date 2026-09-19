#!/bin/bash
# unisacc.c built two ways must behave identically, and must agree with the
# Python front end.  This is the self-hosting ladder. [A-20]
#
# The verdict used to be lost: `lexdiff.sh | tail -1` reports the pipe's exit
# status, so a run that agreed on 0 of 44 files still scored `ok`.  CI showed
# `selfhost lexer agree 0 differ 44   ok`.
set -u
rc=0
./tests/build_ref.sh >/dev/null

run() {   # run() <banner> <env-assignment...>
    local banner=$1; shift
    echo "== $banner =="
    local out; out=$(env "$@" ./tests/lexdiff.sh "${ARGS[@]}"); local r=$?
    printf '%s\n' "$out" | tail -1
    [ "$r" -eq 0 ] || rc=1
}

ARGS=("$@")
run "unisacc built by cc" UA=/tmp/ua_ref
if [ "$(uname -s)" = "Darwin" ] && [ "$(uname -m)" = "arm64" ]; then
    python3 -m unisa compile unisacc.c -o /tmp/ua_self --target osx/arm64 \
        --drive built >/dev/null
    chmod +x /tmp/ua_self; codesign -f -s - /tmp/ua_self >/dev/null 2>&1
    run "unisacc built by unisa itself" UA=/tmp/ua_self
fi
exit $rc

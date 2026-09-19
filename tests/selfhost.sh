#!/bin/bash
# unisacc.c built two ways must behave identically, and must agree with the
# Python front end.  This is the self-hosting ladder. [A-20]
set -u
./tests/build_ref.sh >/dev/null
echo "== unisacc built by cc =="
UA=/tmp/ua_ref ./tests/lexdiff.sh "$@" | tail -1
if [ "$(uname -s)" = "Darwin" ] && [ "$(uname -m)" = "arm64" ]; then
    python3 -m unisa compile unisacc.c -o /tmp/ua_self --target osx/arm64 \
        --drive built >/dev/null
    chmod +x /tmp/ua_self; codesign -f -s - /tmp/ua_self >/dev/null 2>&1
    echo "== unisacc built by unisa itself =="
    UA=/tmp/ua_self ./tests/lexdiff.sh "$@" | tail -1
fi

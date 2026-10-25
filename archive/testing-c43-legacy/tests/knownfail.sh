#!/bin/sh
# Source from a suite after R is set to the repository root.
# Parse once in Python; membership checks stay fixed-string shell operations.
knownfail_load() {
    KNOWNFAIL_KEYS=$2
    python3 "$R/tests/knownfail.py" keys "$1" > "$KNOWNFAIL_KEYS" || return 1
}

knownfail_load_many() {
    KNOWNFAIL_KEYS=$1
    shift
    python3 "$R/tests/knownfail.py" keys "$@" > "$KNOWNFAIL_KEYS" || return 1
}

knownfail_has() {
    grep -Fxqs "$1" "$KNOWNFAIL_KEYS"
}

knownfail_verdict() {
    # name, passed(0 or 1): pass/fail/known/revived
    if knownfail_has "$1"; then
        [ "$2" = 1 ] && printf 'revived\n' || printf 'known\n'
    else
        [ "$2" = 1 ] && printf 'pass\n' || printf 'fail\n'
    fi
}

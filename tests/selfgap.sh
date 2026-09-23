#!/bin/sh
# The self-hosting GAP, as a ratchet.  [A-29]
#
# `bootstrap.sh` proves B = C = U -- unisacc reproduces itself exactly.  That
# is a fixed point, not a coverage claim: unisacc.c only has to accept the
# subset unisacc.c is written in.  Meanwhile every front-end feature of the
# last thirty commits (bit-fields, VLAs, wide strings, the real declarator
# grammar) landed on the PYTHON side only, and nothing in the suite could see
# it: `selfhost.sh` compares the two LEXERS, and `ccrun.sh` prints `UNS` and
# moves on when unisacc refuses a program.  The debt was one-directional and
# unmeasured.
#
# This measures it.  A program counts when unisacc compiles it to a tape at
# all; the number may only go up.  It is deliberately a weaker check than
# ccrun (which also runs the tape) -- the point is direction, not depth.
set -u
REPO=$(cd "$(dirname "$0")/.." && pwd)
UA=${UA:-/tmp/ua_ref}
BASE=$REPO/tests/selfgap.baseline
[ -x "$UA" ] || "$REPO/tests/build_ref.sh" >/dev/null || exit 1
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT

# A front-end bug can loop, and macOS has no timeout(1).
try() { perl -e 'alarm 10; exec @ARGV' "$UA" "$1" -c >/dev/null 2>&1; }

count() {           # count <tag> <file...>  -> accepted, listed in $T/got
    tag=$1; shift
    n=0; tot=0
    for f in "$@"; do
        [ -f "$f" ] || continue
        tot=$((tot+1))
        if try "$f"; then
            n=$((n+1)); echo "$tag $(basename "$f")" >> "$T/got"
        elif [ "${VERBOSE:-0}" = "1" ]; then
            # stdout is this function's RESULT -- diagnostics go to stderr
            printf "  no   %-28s %s\n" "$(basename "$f")" \
                "$(perl -e 'alarm 10; exec @ARGV' "$UA" "$f" -c 2>&1 >/dev/null \
                   | head -1 | cut -c1-48)" >&2
        fi
    done
    echo "$n $tot"
}

: > "$T/got"
set -- "$REPO"/examples/*.c "$REPO"/tests/c/*.c
probes=$(count probe "$@")
CORP=${CORPUS:-$REPO/corpus/c-testsuite}/tests/single-exec
if [ -d "$CORP" ]; then
    corpus=$(count corpus "$CORP"/*.c)
else
    corpus="- -"
fi
sort "$T/got" > "$T/sorted"

# The GAP itself, which is what the name promises: a program the PYTHON
# front end compiles and unisacc refuses is debt on the C side.  When this
# was written the debt was one-directional and large; today the two front
# ends refuse exactly the same four programs, and those four are GNU
# statement expressions, __builtin_expect and C11 _Generic -- outside the
# C99 subset, refused by both, and so not a self-hosting gap at all.
# Counting only what unisacc accepts could never have said that.
gap=0; gapnames=""
if [ -d "$CORP" ]; then
    for f in "$CORP"/*.c "$REPO"/examples/*.c "$REPO"/tests/c/*.c; do
        [ -f "$f" ] || continue
        try "$f" && continue                    # unisacc took it: no debt
        if perl -e 'alarm 20; exec @ARGV' python3 -m unisa tape "$f" \
               >/dev/null 2>&1; then
            gap=$((gap+1)); gapnames="$gapnames $(basename "$f")"
        fi
    done
fi

echo
printf "selfgap  probes %s   corpus %s   (accepted / total)\n" \
    "$(echo "$probes" | tr ' ' '/')" "$(echo "$corpus" | tr ' ' '/')"
if [ "$gap" -eq 0 ]; then
    printf "  the gap itself: 0 -- no program the Python front end compiles is refused here\n"
else
    printf "  the gap itself: %d --%s\n" "$gap" "$gapnames"
fi

rc=0
[ "$gap" -eq 0 ] || { echo "  the C front end is behind the Python one on $gap program(s)"; rc=1; }
check() {           # check <dim> <accepted>
    dim=$1; got=$2
    [ "$got" = "-" ] && { echo "  $dim skipped (corpus absent)"; return; }
    prev=$(awk -v d="$dim" '$1==d{print $2}' "$BASE" 2>/dev/null)
    [ -n "$prev" ] || { echo "  $dim baseline recorded: $got"; new=1; return; }
    if [ "$got" -lt "$prev" ]; then
        echo "  REGRESSION $dim: $got < baseline $prev"
        comm -13 "$T/sorted" "$BASE.list" 2>/dev/null | grep "^$dim " \
            | head -20 | sed 's/^/    lost /'
        rc=1
    elif [ "$got" -gt "$prev" ]; then
        echo "  $dim $prev -> $got   (run with RATCHET=1 to record)"
        new=1
    fi
}
new=0
check probes "${probes%% *}"
check corpus "${corpus%% *}"
if [ "$new" = 1 ] && { [ "${RATCHET:-0}" = "1" ] || [ ! -f "$BASE" ]; }; then
    # A run without the corpus must not erase the corpus line, nor the
    # corpus half of the list.
    { echo "probes ${probes%% *}"
      if [ "${corpus%% *}" = "-" ]; then
          awk '$1=="corpus"' "$BASE" 2>/dev/null
      else
          echo "corpus ${corpus%% *}"
      fi; } > "$T/base"
    { cat "$T/sorted"
      [ "${corpus%% *}" = "-" ] && grep '^corpus ' "$BASE.list" 2>/dev/null
      :; } | sort > "$T/list"
    mv "$T/base" "$BASE"; mv "$T/list" "$BASE.list"
    echo "  recorded."
fi
exit $rc

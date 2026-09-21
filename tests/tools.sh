#!/bin/sh
# Real programs, by other people, in several files.  [A-31]
#
# `corpus.sh` runs c-testsuite: 220 single-file programs written to exercise a
# compiler.  This runs the other kind -- library code someone wrote to be
# used, with its own test vectors, spread across a .c and a .h and a driver.
# Brad Conte's crypto-algorithms is public domain, plain C99, integer only (no
# floating point anywhere), and every algorithm ships its own known-answer
# test that prints SUCCEEDED or FAILED.  That makes each one a real program
# with a real verdict, and it is multi-unit, which is the shape almost all
# working C has and c-testsuite does not.
#
# The reference is the system compiler on the SAME sources.  A program counts
# when both build and agree; the number is a ratchet.
set -u
R=$(cd "$(dirname "$0")/.." && pwd)
DIR=${TOOLS:-$R/corpus/crypto-algorithms}
BASE=$R/tests/tools.baseline
COMMIT=cfbde48414baacf51fc7c74f275190881f037d32
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT

if [ ! -d "$DIR" ]; then
    [ "${FETCH:-1}" = "1" ] || { echo "tools corpus absent (FETCH=0)"; exit 0; }
    git clone -q https://github.com/B-Con/crypto-algorithms.git "$DIR" \
        || { echo "clone failed -- tools skipped"; exit 0; }
    (cd "$DIR" && git checkout -q "$COMMIT" 2>/dev/null) || true
fi

case "$(uname -s)/$(uname -m)" in
    Darwin/arm64)  HOST=osx/arm64;;
    Darwin/x86_64) HOST=osx/x86_64;;
    Linux/x86_64)  HOST=lnx/x86_64;;
    Linux/aarch64) HOST=lnx/arm64;;
    *) echo "tools: no native target for this host -- skipped"; exit 0;;
esac
command -v cc >/dev/null || { echo "tools: no system compiler -- skipped"; exit 0; }

# The reference VM is a Python loop and these do 100,000 rounds, so the image
# is the only way to run them -- which is also the stronger check.
# Left out, with reasons, rather than silently:
#   base64  upstream's own known-answer test FAILS under cc here, and we
#           print PASSED -- we are not going to call our own answer the
#           reference until someone has read the C standard over it
#   aes     cc will not build aes_test.c at all on this host
ALGS="sha256 sha1 md5 md2 rot-13 arcfour des blowfish"
pass=0; wrong=0; unsup=0; skip=0
: > "$T/passing"
for a in $ALGS; do
    src="$DIR/$a.c $DIR/${a}_test.c"
    if ! cc -std=c99 -w -o "$T/ref" $src 2>/dev/null; then
        skip=$((skip+1))
        printf "  skip %-9s the system compiler will not build it either\n" "$a"
        continue
    fi
    want=$("$T/ref" 2>&1)
    if ! python3 -m unisa compile $src -o "$T/got" --target "$HOST" \
            >/dev/null 2>"$T/err"; then
        unsup=$((unsup+1))
        printf "  UNS  %-9s %s\n" "$a" "$(head -1 "$T/err" | cut -c1-58)"
        continue
    fi
    chmod +x "$T/got"
    command -v codesign >/dev/null && codesign -f -s - "$T/got" >/dev/null 2>&1
    got=$("$T/got" 2>&1)
    if [ "$got" = "$want" ]; then
        pass=$((pass+1)); echo "$a" >> "$T/passing"
        printf "  ok   %-9s %s\n" "$a" "$got"
    else
        wrong=$((wrong+1))
        printf "  WRONG %-9s got '%s'  want '%s'\n" "$a" "$got" "$want"
    fi
done

echo
echo "tools $((pass+wrong+unsup+skip))   pass $pass   wrong $wrong   unsupported $unsup   skip $skip"

rc=0
[ "$wrong" -eq 0 ] || rc=1
if [ -f "$BASE" ]; then
    prev=$(cat "$BASE")
    if [ "$pass" -lt "$prev" ]; then
        echo "REGRESSION: pass $pass < baseline $prev"
        sort "$T/passing" > "$T/s"
        comm -13 "$T/s" "$BASE.list" 2>/dev/null | sed 's/^/  lost /'
        rc=1
    elif [ "$pass" -gt "$prev" ]; then
        echo "baseline $prev -> $pass (run with RATCHET=1 to record)"
        [ "${RATCHET:-0}" = "1" ] && { echo "$pass" > "$BASE"; \
            sort "$T/passing" > "$BASE.list"; echo "recorded."; }
    fi
else
    echo "$pass" > "$BASE"; sort "$T/passing" > "$BASE.list"
    echo "baseline recorded: $pass"
fi
exit $rc

#!/bin/sh
# External corpus: c-testsuite (220 single-file C programs, not written for us).
#
# difftest.sh runs OUR probes -- programs we wrote, so it measures what we
# thought to try.  This runs someone else's, so it measures what we actually
# cover.  A program is `unsupported` when the front end refuses it and `wrong`
# when it compiles and then disagrees with the expected output; only `wrong` is
# a failure.  `pass` is a ratchet against tests/corpus.baseline -- it may rise,
# never fall.
set -u
U="python3 -m unisa"
DRIVE=${DRIVE:-built}
REPO=$(cd "$(dirname "$0")/.." && pwd)
DIR=${CORPUS:-$REPO/corpus/c-testsuite}
SRC=$DIR/tests/single-exec
BASE=$REPO/tests/corpus.baseline
KNOWN=$REPO/tests/corpus.knownfail

if [ ! -d "$SRC" ]; then
    [ "${FETCH:-1}" = "1" ] || { echo "corpus absent (FETCH=0)"; exit 0; }
    echo "fetching c-testsuite into $DIR ..."
    git clone -q --depth 1 https://github.com/c-testsuite/c-testsuite.git "$DIR" \
        || { echo "clone failed -- corpus skipped"; exit 0; }
fi

pass=0; wrong=0; unsup=0; known=0; revived=0; slow=0
isknown() { grep -qs "^$1[[:space:]]" "$KNOWN"; }

# macOS has no `timeout`, and one heavy program must not hang the suite: the
# reference VM is a Python interpreter, so an eight-queens search that a native
# compiler finishes instantly can take a quarter of an hour here.
LIMIT=${LIMIT:-10}
# Run the corpus NATIVELY when this host has a matching target: the reference
# VM is a Python interpreter, and an eight-queens search that a real CPU
# finishes instantly takes a quarter of an hour there.  It is also the stronger
# check -- the image is what we ship.
if [ -z "${HOST_TARGET:-}" ]; then
    case "$(uname -s)/$(uname -m)" in
        Darwin/arm64)  HOST_TARGET=osx/arm64;;
        Darwin/x86_64) HOST_TARGET=osx/x86_64;;
        Linux/x86_64)  HOST_TARGET=lnx/x86_64;;
        Linux/aarch64) HOST_TARGET=lnx/arm64;;
        *) HOST_TARGET=;;
    esac
fi
runlim() {
    "$@" & p=$!
    ( sleep "$LIMIT"; kill -9 $p 2>/dev/null ) >/dev/null 2>&1 & w=$!
    wait $p 2>/dev/null; rc=$?
    kill $w 2>/dev/null
    return $rc
}
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
: > "$T/passing"
for f in "$SRC"/*.c; do
    b=$(basename "$f" .c)
    want=$(cat "$f.expected" 2>/dev/null || echo "")
    : > "$T/err"
    if [ -n "$HOST_TARGET" ]; then
        if $U compile "$f" -o "$T/x" --target "$HOST_TARGET" --drive "$DRIVE" \
                >/dev/null 2>"$T/err" && [ ! -s "$T/err" ]; then
            chmod +x "$T/x"
            command -v codesign >/dev/null && \
                codesign -f -s - "$T/x" >/dev/null 2>&1
            got=$(runlim "$T/x" 2>/dev/null); code=$?
        else
            got=""; code=1
        fi
    else
        got=$(runlim $U run "$f" --drive "$DRIVE" 2>"$T/err"); code=$?
    fi
    if [ "$code" -ge 128 ] && [ ! -s "$T/err" ]; then
        slow=$((slow+1))
        [ "${VERBOSE:-0}" = "1" ] && printf "  SLOW %s  (over %ss in the reference VM)\n" "$b" "$LIMIT"
    elif [ -s "$T/err" ] && isknown "$b"; then
        known=$((known+1))          # refused, and we know why
    elif [ -s "$T/err" ]; then
        unsup=$((unsup+1))
        [ "${VERBOSE:-0}" = "1" ] && printf "  UNS  %s  %s\n" "$b" \
            "$(head -1 "$T/err" | cut -c1-70)"
    elif [ "$got" = "$want" ] && [ "$code" -eq 0 ]; then
        if isknown "$b"; then
            revived=$((revived+1))
            printf "  REVIVED %s  now passes -- drop it from corpus.knownfail\n" "$b"
        else
            pass=$((pass+1)); echo "$b" >> "$T/passing"
        fi
    elif isknown "$b"; then
        known=$((known+1))
    else
        wrong=$((wrong+1))
        printf "  WRONG %s  got '%s'(%d)  want '%s'(0)\n" "$b" "$got" "$code" "$want"
    fi
done

total=$((pass+wrong+unsup+known+revived+slow))
echo
echo "corpus $total   pass $pass   wrong $wrong   unsupported $unsup   knownfail $known   slow $slow"

rc=0
[ "$wrong" -eq 0 ] || rc=1
[ "$revived" -eq 0 ] || rc=1
if [ -f "$BASE" ]; then
    prev=$(cat "$BASE")
    if [ "$pass" -lt "$prev" ]; then
        echo "REGRESSION: pass $pass < baseline $prev"
        comm -13 "$T/passing" "$BASE.list" 2>/dev/null | head -20 | sed 's/^/  lost /'
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

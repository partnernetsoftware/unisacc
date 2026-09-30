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
CORPUS_UA=${CORPUS_UA:-}
DRIVE=${DRIVE:-built}
REPO=$(cd "$(dirname "$0")/.." && pwd)
. "$REPO/tests/lib.sh"
DIR=${CORPUS:-$REPO/corpus/c-testsuite}
SRC=$DIR/tests/single-exec
BASE=$REPO/tests/corpus.baseline
KNOWN=$REPO/tests/corpus.knownfail

if [ ! -d "$SRC" ]; then
    [ "${FETCH:-1}" = "1" ] || { echo "corpus absent (FETCH=0)"; exit 1; }
    echo "fetching c-testsuite into $DIR ..."
    bound 30 git clone -q --depth 1 https://github.com/c-testsuite/c-testsuite.git "$DIR" \
        || { echo "clone failed -- corpus not tested"; exit 1; }
fi

pass=0; wrong=0; unsup=0; known=0; revived=0; slow=0

# macOS has no `timeout`, and one heavy program must not hang the suite: the
# reference VM is a Python interpreter, so an eight-queens search that a native
# compiler finishes instantly can take a quarter of an hour here.
# Wall clock, and the first-launch scan of a new image QUEUES system-wide: with
# several suites launching at once a trivial program has waited past 8s before
# it ran a single instruction.  This is a hang detector, not a benchmark.
LIMIT=${LIMIT:-30}
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
case $LIMIT in ''|*[!0-9]*) echo 'invalid LIMIT' >&2; exit 2;; esac
[ "$LIMIT" -ge 1 ] && [ "$LIMIT" -le 60 ] || exit 2
runlim() { bound "$LIMIT" "$@"; }
compile_one() {
    if [ -n "$CORPUS_UA" ]; then
        runlim "$CORPUS_UA" -O2 -b "$HOST_TARGET" "$1" -o "$2"
    else
        runlim $U compile "$1" -o "$2" --target "$HOST_TARGET" --drive "$DRIVE"
    fi
}
if [ -n "$CORPUS_UA" ]; then
    [ -n "$HOST_TARGET" ] || { echo 'CORPUS_UA requires a native host target' >&2; exit 2; }
    echo "corpus driver: $CORPUS_UA"
else
    echo "corpus driver: Python --drive $DRIVE"
fi
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
R=$REPO
. "$R/tests/knownfail.sh"
knownfail_load "$KNOWN" "$T/known.keys" || exit 1
isknown() { knownfail_has "$1"; }
# SHARD=k/n runs every n-th program starting at the k-th.  The whole corpus
# is 220 first executions, each a 0.5-0.9 s XProtect scan, and one run may
# not take more than 60 s (AGENTS.md) -- so it goes in shards: 1/4 .. 4/4.
FILES=""; i=0
SHARD=${SHARD:-1/1}; SH_K=${SHARD%/*}; SH_N=${SHARD#*/}
case $SH_K:$SH_N in *[!0-9:]*|:*|*:) echo 'invalid SHARD' >&2; exit 2;; esac
[ ${#SH_K} -le 6 ] && [ ${#SH_N} -le 6 ] && [ "$SH_K" -ge 1 ] && [ "$SH_N" -ge "$SH_K" ] || exit 2
for f in "$SRC"/*.c; do
    [ -f "$f" ] || continue
    [ $((i % SH_N)) -eq $((SH_K - 1)) ] && FILES="$FILES $f"
    i=$((i + 1))
done
[ -n "$FILES" ] || { echo "empty corpus shard" >&2; exit 1; }
: > "$T/passing"
PAR_WAIT=4   # mostly the first-launch scan: waiting, not computing
. "$REPO/tests/par.sh"
# A: compile and run every program, PAR at a time, each in a directory of its
# own (some of these programs write files).
for f in $FILES; do
    b=$(basename "$f" .c)
    throttle
    (
    D="$T/$b.d"; mkdir -p "$D"; : > "$D/err"
    if [ -n "$HOST_TARGET" ]; then
        compile_one "$f" "$D/x" >/dev/null 2>"$D/err"; compiled=$?
        echo "$compiled" > "$D/compile_rc"
        if [ "$compiled" -eq 0 ] && [ -s "$D/x" ] && [ ! -s "$D/err" ]; then
            chmod +x "$D/x"
            command -v codesign >/dev/null && \
                codesign -f -s - "$D/x" >/dev/null 2>&1
            (cd "$D" && runlim ./x 2>/dev/null) > "$D/got"; echo $? > "$D/code"
        else
            : > "$D/got"; echo 1 > "$D/code"
        fi
    else
        (runlim $U run "$f" --drive "$DRIVE" 2>"$D/err") > "$D/got"; echo $? > "$D/code"
    fi
    ) &
done
wait
# B: the verdicts, in order.
for f in $FILES; do
    b=$(basename "$f" .c)
    want=$(cat "$f.expected" 2>/dev/null || echo "")
    D="$T/$b.d"
    cp "$D/err" "$T/err"
    got=$(cat "$D/got"); code=$(cat "$D/code")
    # 137 is our own watchdog's SIGKILL; any other signal is the program
    # crashing, which is a wrong answer, not a slow one
    compiled=$(cat "$D/compile_rc" 2>/dev/null || echo 0)
    if [ "$compiled" -gt 1 ] || { [ "$compiled" -eq 0 ] && [ -n "$HOST_TARGET" ] && [ ! -s "$D/x" ]; }; then
        wrong=$((wrong+1))
        printf "  TOOLFAIL %s compiler rc=%s or empty output\n" "$b" "$compiled"
    elif [ "$code" -eq 137 ] || [ "$code" -eq 142 ]; then
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
[ "$total" -gt 0 ] && [ "$slow" -eq 0 ] || rc=1
[ "$revived" -eq 0 ] || rc=1
if [ "$SH_N" -eq 1 ]; then
    ratchet "$BASE" "$pass" "$T/passing" || rc=1
else
    # a shard cannot move the baseline, but it must not lose anything in it
    for f in $FILES; do basename "$f" .c; done | sort > "$T/mine"
    sort "$T/passing" > "$T/got"
    lost=$(comm -12 "$T/mine" "$BASE.list" | comm -23 - "$T/got")
    [ -n "$lost" ] && { echo "$lost" | sed 's/^/  REGRESSION lost /'; rc=1; }
fi
exit $rc

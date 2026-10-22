#!/bin/sh
# Real programs, by other people, in several files.  [A-31]
#
# `corpus.sh` runs c-testsuite: 220 single-file programs written to exercise a
# compiler.  This runs the other kind -- library code someone wrote to be
# USED, with its own test vectors, spread across a .c and a .h and a driver.
# Everything here is public domain or MIT, plain C99, and integer only (no
# floating point anywhere), and every entry ships a known-answer test that
# prints its own verdict.  It is also all multi-unit, which is the shape
# almost all working C has and c-testsuite does not.
#
# The reference is the system compiler on the SAME sources.  A program counts
# when both build and agree; the number is a ratchet.  It read 2 of 8 the
# first time it ran -- see prd.md E-45 for what that bought.
set -u
R=$(cd "$(dirname "$0")/.." && pwd)
CACHE=${TOOLS:-$R/corpus}
BASE=$R/tests/tools.baseline

# name|url|commit
REPOS="crypto-algorithms|https://github.com/B-Con/crypto-algorithms.git|cfbde48414baacf51fc7c74f275190881f037d32
tiny-AES-c|https://github.com/kokke/tiny-AES-c.git|23856752fbd139da0b8ca6e471a13d5bcc99a08d
tiny-regex-c|https://github.com/kokke/tiny-regex-c.git|f2632c6d9ed25272987471cdb8b70395c2460bdb"

# name|repo|files, relative to the clone
#
# Left out, with reasons, rather than silently:
#   base64  upstream's own known-answer test FAILS under cc here, and we
#           print PASSED -- we are not calling our own answer the reference
#           until someone has read the standard over it
#   aes (crypto-algorithms)  cc will not build aes_test.c on this host
ENTRIES="sha256|crypto-algorithms|sha256.c sha256_test.c
sha1|crypto-algorithms|sha1.c sha1_test.c
md5|crypto-algorithms|md5.c md5_test.c
md2|crypto-algorithms|md2.c md2_test.c
rot-13|crypto-algorithms|rot-13.c rot-13_test.c
arcfour|crypto-algorithms|arcfour.c arcfour_test.c
des|crypto-algorithms|des.c des_test.c
blowfish|crypto-algorithms|blowfish.c blowfish_test.c
tiny-aes|tiny-AES-c|aes.c test.c
regex1|tiny-regex-c|re.c tests/test1.c
regex2|tiny-regex-c|re.c tests/test2.c"

# Staged entries: reference, build and run do not fit one queue window together (0.0.43
# timeout debt, regex2 at load 8-16: cc reference run 15-20 s, Python build 21 s, run 39 s).
# With TOOLS_STAGED=split a shard leaves them out and gate.sh runs each as three jobs,
# tools-NAME-ref / -build / -run (--stage STEP NAME).  ref and build publish into
# content-keyed directories (sources, this script, cc / compiler identity); run recomputes
# both keys, checks every handed-off byte, and refuses a missing or altered handoff (rc 75).
# Every staged entry must be in the baseline: any failure of one is a regression.
STAGED="regex2"
usage() { echo 'usage: tools.sh [--list | --staged | --stage ref|build|run NAME]' >&2; exit 2; }
MODE=shard; STAGE=; SNAME=
case $# in
    0) ;;
    1) case $1 in --list) MODE=list;; --staged) MODE=staged;; *) usage;; esac;;
    3) [ "$1" = --stage ] || usage; MODE=stage; STAGE=$2; SNAME=$3
       case $STAGE in ref|build|run) ;; *) usage;; esac
       case " $STAGED " in *" $SNAME "*) ;; *) echo "tools: $SNAME is not a staged entry ($STAGED)" >&2; exit 2;; esac;;
    *) usage;;
esac
if [ "$MODE" = staged ]; then printf '%s\n' $STAGED; exit 0; fi
case ${TOOLS_STAGED:-} in ''|split) ;; *) echo 'TOOLS_STAGED must be empty or split' >&2; exit 2;; esac
# Deterministic, disjoint entry shards. --list has no fixture/build side effects.
SHARD=${SHARD:-1/1}; SH_K=${SHARD%/*}; SH_N=${SHARD#*/}
case "$SHARD" in */*) ;; *) echo 'SHARD needs k/n' >&2; exit 2;; esac
case $SH_K:$SH_N in *[!0-9:]*|:*|*:) echo 'invalid SHARD' >&2; exit 2;; esac
[ "$SH_K" -ge 1 ] && [ "$SH_K" -le "$SH_N" ] && [ "$SH_N" -le 11 ] || { echo 'SHARD must be k/n with 1<=k<=n<=11' >&2; exit 2; }
SELECTED=$(printf '%s\n' "$ENTRIES" | awk -F'|' -v k="$SH_K" -v n="$SH_N" '((NR-1)%n)+1==k')
# split: the staged entries of this shard run as their own three jobs (above), not here
[ "${TOOLS_STAGED:-}" != split ] || SELECTED=$(printf '%s\n' "$SELECTED" | awk -F'|' -v s=" $STAGED " 'index(s, " " $1 " ")==0')
[ "$MODE" != stage ] || SELECTED=$(printf '%s\n' "$ENTRIES" | awk -F'|' -v n="$SNAME" '$1==n')
[ -n "$SELECTED" ] || { echo 'empty tools shard' >&2; exit 2; }
if [ "$MODE" = list ]; then printf '%s\n' "$SELECTED" | cut -d'|' -f1; exit 0; fi
. "$R/tests/lib.sh"
T=$(scratch)
printf '%s\n' "$SELECTED" | cut -d'|' -f1 | sort > "$T/selected"
# Project the original per-name ratchet onto this shard; never lower its count.
[ -s "$BASE.list" ] || { echo 'tools baseline list missing/empty' >&2; exit 1; }
comm -12 "$BASE.list" "$T/selected" > "$T/required"
for r in $REPOS; do
    name=$(echo "$r" | cut -d'|' -f1)
    url=$(echo "$r" | cut -d'|' -f2)
    commit=$(echo "$r" | cut -d'|' -f3)
    if [ -d "$CACHE/$name" ]; then
        # a cache at any other commit is refused, never used silently (c43 batch 05): remove it to re-fetch
        got=$(git -C "$CACHE/$name" rev-parse -q --verify HEAD 2>/dev/null) || got=
        [ "$got" = "$commit" ] || { echo "tools: $CACHE/$name is at ${got:-no commit}, not the pinned $commit; remove it to re-fetch" >&2; exit 1; }
        # a tracked file edited in the cache (HEAD unchanged) is refused too, never used silently
        [ -z "$(git -C "$CACHE/$name" status --porcelain --untracked-files=no)" ] || { echo "tools: $CACHE/$name has modified tracked files; remove it to re-fetch" >&2; exit 1; }
        continue
    fi
    [ "${FETCH:-1}" = "1" ] || { echo "tools corpus absent (FETCH=0)"; exit 1; }
    bound 30 git clone -q "$url" "$CACHE/$name" || { echo "clone failed"; exit 1; }
    (cd "$CACHE/$name" && bound 15 git checkout -q "$commit") || exit 1
done

case "$(uname -s)/$(uname -m)" in
    Darwin/arm64)  HOST=osx/arm64;;
    Darwin/x86_64) HOST=osx/x86_64;;
    Linux/x86_64)  HOST=lnx/x86_64;;
    Linux/aarch64) HOST=lnx/arm64;;
    *) echo "tools: no native target for this host"; exit 1;;
esac
command -v cc >/dev/null || { echo "tools: no system compiler"; exit 1; }

# The reference VM is a Python loop and these run 100,000 rounds, so a real
# image is the only way to run them -- which is also the stronger check.
# Explicit compiler selection: UA alone does not select this historically Python suite.
compile_tool() {
    if [ -n "${TOOLS_UA:-}" ]; then
        bound 60 "$TOOLS_UA" "$@" -b "$HOST"
    else
        bound 60 python3 -m unisa compile "$@" --target "$HOST"
    fi
}
echo "tools driver: ${TOOLS_UA:-python3 -m unisa compile}"
if [ "$MODE" = stage ]; then
    [ -s "$T/required" ] || { echo "tools: staged entry $SNAME is not in $BASE.list" >&2; exit 2; }
    SHA="shasum -a 256"; ! command -v sha256sum >/dev/null || SHA=sha256sum   # a command: find -exec runs it too
    sha() { $SHA "$@"; }
    repo=$(echo "$SELECTED" | cut -d'|' -f2); rel=$(echo "$SELECTED" | cut -d'|' -f3)
    dir=$CACHE/$repo; src=""; for f in $rel; do src="$src $dir/$f"; done
    # identity of what the step consumes; the clone is already pinned and clean (above)
    srcid=$({ echo "$SELECTED $HOST $(git -C "$dir" rev-parse HEAD)"; sha "$R/tests/tools.sh" $src | cut -c1-64; } | sha | cut -c1-64)
    ccbin=$(command -v cc)
    refid=$({ echo "$srcid"; echo "cc $ccbin $(cc --version 2>&1 | head -1)"; sha "$ccbin" | cut -c1-64; } | sha | cut -c1-64)
    buildid=$({ echo "$srcid"
        if [ -n "${TOOLS_UA:-}" ]; then echo "ua $(sha "$TOOLS_UA" | cut -c1-64)"
        else echo "py $(python3 -V 2>&1)"
             (cd "$R" && find unisa include weights -name __pycache__ -prune -o -type f -exec $SHA {} + | LC_ALL=C sort -k2)
        fi; } | sha | cut -c1-64)
    SB=${UNISACC_MODEL_CACHE:-${TMPDIR:-/tmp}/unisacc-model-cache}; mkdir -p "$SB"
    REFD=$SB/tools-ref-$SNAME-$refid; BLDD=$SB/tools-build-$SNAME-$buildid
    publish() {   # publish DEST ID FILE...: manifest, then one atomic rename into place
        tmp=$1.tmp.$$; rm -rf "$tmp"; mkdir -p "$tmp"; d=$1 id=$2; shift 2
        cp -p "$@" "$tmp/"; echo "$id" > "$tmp/key"
        (cd "$tmp" && for f in "$@"; do sha "$(basename "$f")"; done; sha key) > "$tmp/ready.tmp"
        mv "$tmp/ready.tmp" "$tmp/ready"; rm -rf "$d"; mv "$tmp" "$d"
    }
    need() {      # need DIR ID STEP: the handoff exists for exactly this content, byte for byte
        [ -s "$1/ready" ] && [ "$(cat "$1/key" 2>/dev/null)" = "$2" ] ||
            { echo "NEED_PREPARE tools $SNAME: no $3 handoff for this content ($1); run tools-$SNAME-$3 first" >&2; exit 75; }
        (cd "$1" && sha -c ready >/dev/null 2>&1) ||
            { echo "NEED_PREPARE tools $SNAME: $3 handoff altered ($1); run tools-$SNAME-$3 again" >&2; exit 75; }
    }
    case $STAGE in
    ref)
        bound 60 cc -std=c99 -w -I"$dir" -o "$T/ref" $src 2>"$T/err" ||
            { echo "  REGRESSION $SNAME the system compiler will not build it: $(head -1 "$T/err" | cut -c1-58)"; exit 1; }
        bound 60 "$T/ref" > "$T/want" 2>&1; wrc=$?
        [ "$wrc" -eq 0 ] || { echo "  WRONG $SNAME reference exited $wrc"; exit 1; }
        publish "$REFD" "$refid" "$T/want"
        echo "tools $SNAME ref: $(wc -c < "$T/want" | tr -d ' ') bytes expected, handoff $refid";;
    build)
        compile_tool $src -I "$dir" -o "$T/got" >/dev/null 2>"$T/err" ||
            { echo "  REGRESSION $SNAME unsupported: $(head -1 "$T/err" | cut -c1-58)"; exit 1; }
        chmod +x "$T/got"
        if command -v codesign >/dev/null && ! bound 10 codesign -f -s - "$T/got" >/dev/null 2>&1; then
            echo "  WRONG $SNAME signing failed"; exit 1; fi
        publish "$BLDD" "$buildid" "$T/got"
        echo "tools $SNAME build: image $(wc -c < "$T/got" | tr -d ' ') bytes, handoff $buildid";;
    run)
        need "$REFD" "$refid" ref; need "$BLDD" "$buildid" build
        bound 60 "$BLDD/got" > "$T/out" 2>&1; grc=$?
        if [ "$grc" -eq 0 ] && cmp -s "$T/out" "$REFD/want"; then
            printf "  ok   %-9s %s\n" "$SNAME" "$(tail -1 "$T/out" | cut -c1-48)"
            echo "tools $SNAME run: pass 1   wrong 0 (byte-identical to the cc reference)"; exit 0
        fi
        printf "  WRONG %-9s rc %s  got '%s'  want '%s'\n" "$SNAME" "$grc" \
            "$(grep -v '^ *$' "$T/out" | tail -1 | cut -c1-40)" "$(grep -v '^ *$' "$REFD/want" | tail -1 | cut -c1-40)"
        echo "tools $SNAME run: pass 0   wrong 1   REGRESSION missing baseline name $SNAME"; exit 1;;
    esac
    exit 0
fi
pass=0; wrong=0; unsup=0; skip=0
: > "$T/passing"
# a redirect, not a pipe: the loop must run in THIS shell or the
# counters it keeps are lost with the subshell
printf '%s\n' "$SELECTED" > "$T/entries"
while IFS= read -r e; do
    [ -n "$e" ] || continue
    name=$(echo "$e" | cut -d'|' -f1)
    repo=$(echo "$e" | cut -d'|' -f2)
    rel=$(echo "$e" | cut -d'|' -f3)
    dir=$CACHE/$repo
    src=""
    for f in $rel; do src="$src $dir/$f"; done
    if ! bound 60 cc -std=c99 -w -I"$dir" -o "$T/ref" $src 2>/dev/null; then
        skip=$((skip+1))
        printf "  skip %-9s the system compiler will not build it either\n" "$name"
        continue
    fi
    want=$(bound 60 "$T/ref" 2>&1); wrc=$?
    if [ "$wrc" -ne 0 ]; then
        wrong=$((wrong+1)); printf "  WRONG %-9s reference exited %s\n" "$name" "$wrc"
        continue
    fi
    if ! compile_tool $src -I "$dir" -o "$T/got" >/dev/null 2>"$T/err"; then
        unsup=$((unsup+1))
        printf "  UNS  %-9s %s\n" "$name" "$(head -1 "$T/err" | cut -c1-58)"
        continue
    fi
    chmod +x "$T/got"
    if command -v codesign >/dev/null && ! bound 10 codesign -f -s - "$T/got" >/dev/null 2>&1; then
        wrong=$((wrong+1)); echo "  WRONG $name signing failed"; continue
    fi
    got=$(bound 60 "$T/got" 2>&1); grc=$?
    if [ "$grc" -eq 0 ] && [ "$got" = "$want" ]; then
        pass=$((pass+1)); echo "$name" >> "$T/passing"
        printf "  ok   %-9s %s\n" "$name" "$(printf '%s' "$got" | tail -1 | cut -c1-48)"
    else
        wrong=$((wrong+1))
        printf "  WRONG %-9s got '%s'  want '%s'\n" "$name" \
            "$(printf '%s' "$got" | tail -1 | cut -c1-40)" \
            "$(printf '%s' "$want" | tail -1 | cut -c1-40)"
    fi
done < "$T/entries"

echo
echo "tools SHARD=$SHARD $((pass+wrong+unsup+skip))   pass $pass   wrong $wrong   unsupported $unsup   skip $skip"

rc=0
[ "$wrong" -eq 0 ] || rc=1
sort "$T/passing" > "$T/passing.sorted"
comm -23 "$T/required" "$T/passing.sorted" > "$T/lost"
if [ -s "$T/lost" ]; then echo '  REGRESSION missing baseline names:'; cat "$T/lost"; rc=1; fi
[ "$pass" -gt 0 ] || rc=1
# Whole-suite invocation retains the original ratchet update facility (never with staged entries left out).
if [ "$SH_N" -eq 1 ] && [ "${TOOLS_STAGED:-}" != split ]; then ratchet "$BASE" "$pass" "$T/passing" || rc=1; fi
exit $rc

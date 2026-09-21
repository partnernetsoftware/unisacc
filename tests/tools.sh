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
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT

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

for r in $REPOS; do
    name=$(echo "$r" | cut -d'|' -f1)
    url=$(echo "$r" | cut -d'|' -f2)
    commit=$(echo "$r" | cut -d'|' -f3)
    [ -d "$CACHE/$name" ] && continue
    [ "${FETCH:-1}" = "1" ] || { echo "tools corpus absent (FETCH=0)"; exit 0; }
    git clone -q "$url" "$CACHE/$name" || { echo "clone failed -- skipped"; exit 0; }
    (cd "$CACHE/$name" && git checkout -q "$commit") || true
done

case "$(uname -s)/$(uname -m)" in
    Darwin/arm64)  HOST=osx/arm64;;
    Darwin/x86_64) HOST=osx/x86_64;;
    Linux/x86_64)  HOST=lnx/x86_64;;
    Linux/aarch64) HOST=lnx/arm64;;
    *) echo "tools: no native target for this host -- skipped"; exit 0;;
esac
command -v cc >/dev/null || { echo "tools: no system compiler -- skipped"; exit 0; }

# The reference VM is a Python loop and these run 100,000 rounds, so a real
# image is the only way to run them -- which is also the stronger check.
pass=0; wrong=0; unsup=0; skip=0
: > "$T/passing"
# a redirect, not a pipe: the loop must run in THIS shell or the
# counters it keeps are lost with the subshell
printf '%s\n' "$ENTRIES" > "$T/entries"
while IFS= read -r e; do
    [ -n "$e" ] || continue
    name=$(echo "$e" | cut -d'|' -f1)
    repo=$(echo "$e" | cut -d'|' -f2)
    rel=$(echo "$e" | cut -d'|' -f3)
    dir=$CACHE/$repo
    src=""
    for f in $rel; do src="$src $dir/$f"; done
    if ! cc -std=c99 -w -I"$dir" -o "$T/ref" $src 2>/dev/null; then
        skip=$((skip+1))
        printf "  skip %-9s the system compiler will not build it either\n" "$name"
        continue
    fi
    want=$("$T/ref" 2>&1)
    if ! python3 -m unisa compile $src -I "$dir" -o "$T/got" \
            --target "$HOST" >/dev/null 2>"$T/err"; then
        unsup=$((unsup+1))
        printf "  UNS  %-9s %s\n" "$name" "$(head -1 "$T/err" | cut -c1-58)"
        continue
    fi
    chmod +x "$T/got"
    command -v codesign >/dev/null && codesign -f -s - "$T/got" >/dev/null 2>&1
    got=$("$T/got" 2>&1)
    if [ "$got" = "$want" ]; then
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

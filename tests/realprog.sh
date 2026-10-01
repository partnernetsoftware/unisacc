#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# Real PROGRAMS by other people, pinned, built by us and by the system
# compiler from the same sources, then run the same way; the outputs must
# agree byte for byte and so must the exit status.  (R16-12, 2026-10-01)
#
# tools.sh runs library known-answer tests; corpus.sh runs c-testsuite's
# compiler probes.  This runs the kind of program a user actually hands a
# compiler -- an editor, a JSON parser, an interpreter, a database -- and
# records, for each one that does not build, the FIRST reason, because that
# list decides which library surface 0.0.17 adds (R16-4): kilo wants
# termios.h, lua wants setjmp.h, sqlite's shell wants pwd.h and sqlite3.c is
# 9.2 MB against a 4 MB source buffer.
#
# Verdicts: ok (both build, agree), WRONG (both build, differ: a failure),
# UNS (we refuse it: printed with the reason, not a failure), skip (the
# system compiler will not build it here).  tests/realprog.baseline.list is
# the ratchet: every name in it must be ok, and each version adds at least
# one name (archive/plans/v0.0.16.md R16-12).
set -u
R=$(cd "$(dirname "$0")/.." && pwd)
CACHE=${REALPROG_CACHE:-$R/corpus}
BASE=$R/tests/realprog.baseline.list
# name|kind|url|pin        kind git: pin is a commit; kind zip: pin is the sha256 of the archive
REPOS="kilo|git|https://github.com/antirez/kilo.git|323d93b29bd89a2cb446de90c4ed4fea1764176e
jsmn|git|https://github.com/zserge/jsmn.git|fdcef3ebf886fa210d14956d3c068a653e76a24e
cJSON|git|https://github.com/DaveGamble/cJSON.git|acc76239bee01d8e9c858ae2cab296704e52d916
lua|git|https://github.com/lua/lua.git|1ab3208a1fceb12fca8f24ba57d6e13c5bff15e3
sqlite|zip|https://www.sqlite.org/2025/sqlite-amalgamation-3490100.zip|6cebd1d8403fc58c30e93939b246f3e6e58d0765a5cd50546f16c00fd805d2c3"
# name|repo|files (relative to the clone)|extra cflags|program arguments|stdin file (or -)
ENTRIES="jsmn-simple|jsmn|example/simple.c|-I.||-
cjson|cJSON|cJSON.c test.c|-I.||-
jsmn-dump|jsmn|example/jsondump.c|-I.||../cJSON/tests/inputs/test1
kilo|kilo|kilo.c||/dev/null|-
lua|lua|onelua.c|-DMAKE_LUA -I.|-e print(1+1)|-
sqlite|sqlite|shell.c sqlite3.c|-I.|:memory: select(1+1)|-"

[ "$#" -eq 0 ] || { [ "$#" -eq 1 ] && [ "$1" = --list ]; } || { echo 'usage: realprog.sh [--list]' >&2; exit 2; }
if [ "$#" -eq 1 ]; then printf '%s\n' "$ENTRIES" | cut -d'|' -f1; exit 0; fi
. "$R/tests/lib.sh"
T=$(scratch)
[ -s "$BASE" ] || { echo 'realprog baseline list missing/empty' >&2; exit 1; }

printf '%s\n' "$REPOS" > "$T/repos"
while IFS='|' read -r name kind url pin; do
    [ -n "$name" ] || continue
    [ -d "$CACHE/$name" ] && continue
    [ "${FETCH:-1}" = "1" ] || { echo "realprog corpus absent (FETCH=0): $name"; exit 1; }
    if [ "$kind" = git ]; then
        bound 40 git clone -q "$url" "$CACHE/$name" || { echo "clone failed: $name"; exit 1; }
        (cd "$CACHE/$name" && bound 15 git checkout -q "$pin") || exit 1
    else
        bound 50 curl -sfL -o "$T/$name.zip" "$url" || { echo "download failed: $name"; exit 1; }
        got=$(shasum -a 256 "$T/$name.zip" | cut -d' ' -f1)
        [ "$got" = "$pin" ] || { echo "sha256 differs for $name: $got"; exit 1; }
        mkdir -p "$T/unz" && bound 30 unzip -q -o "$T/$name.zip" -d "$T/unz" || exit 1
        mv "$T/unz"/* "$CACHE/$name" || exit 1
    fi
done < "$T/repos"

case "$(uname -s)/$(uname -m)" in
    Darwin/arm64)  HOST=osx/arm64;;
    Darwin/x86_64) HOST=osx/x86_64;;
    Linux/x86_64)  HOST=lnx/x86_64;;
    Linux/aarch64) HOST=lnx/arm64;;
    *) echo "realprog: no native target for this host"; exit 1;;
esac
command -v cc >/dev/null || { echo "realprog: no system compiler"; exit 1; }
if [ -n "${REALPROG_UA:-}" ]; then DRIVER=$REALPROG_UA; else ua_ready; DRIVER=$UA; fi
echo "realprog driver: $DRIVER   reference: cc"

pass=0; wrong=0; unsup=0; skip=0
: > "$T/passing"
printf '%s\n' "$ENTRIES" > "$T/entries"
while IFS='|' read -r name repo rel cflags args stdin; do
    [ -n "$name" ] || continue
    dir=$CACHE/$repo
    src=""; for f in $rel; do src="$src $dir/$f"; done
    in=/dev/null; [ "$stdin" = - ] || in=$dir/$stdin
    if ! (cd "$dir" && bound 60 cc -std=c99 -w $cflags -o "$T/ref" $src) 2>/dev/null; then
        skip=$((skip+1)); printf "  skip %-12s the system compiler will not build it here\n" "$name"; continue
    fi
    want=$(bound 30 "$T/ref" $args < "$in" 2>&1); wrc=$?
    if ! (cd "$dir" && bound 60 $DRIVER -O2 $cflags $src -b "$HOST" -o "$T/got") >/dev/null 2>"$T/err"; then   # $DRIVER may carry words (`sh unisacc.com`)
        unsup=$((unsup+1))
        printf "  UNS  %-12s %s\n" "$name" "$(head -2 "$T/err" | tr -s ' \n' ' ' | sed "s|$dir/||g" | cut -c1-96)"
        continue
    fi
    chmod +x "$T/got"
    if command -v codesign >/dev/null && ! bound 10 codesign -f -s - "$T/got" >/dev/null 2>&1; then
        wrong=$((wrong+1)); echo "  WRONG $name signing failed"; continue
    fi
    got=$(bound 30 "$T/got" $args < "$in" 2>&1); grc=$?
    if [ "$grc" -eq "$wrc" ] && [ "$got" = "$want" ]; then
        pass=$((pass+1)); echo "$name" >> "$T/passing"
        printf "  ok   %-12s rc %s  %s\n" "$name" "$grc" "$(printf '%s' "$got" | tail -1 | cut -c1-44)"
    else
        wrong=$((wrong+1))
        printf "  WRONG %-12s rc %s/%s  got '%s'  want '%s'\n" "$name" "$grc" "$wrc" \
            "$(printf '%s' "$got" | tail -1 | cut -c1-30)" "$(printf '%s' "$want" | tail -1 | cut -c1-30)"
    fi
done < "$T/entries"

echo
echo "realprog $((pass+wrong+unsup+skip))   ok $pass   wrong $wrong   unsupported $unsup   skip $skip"
rc=0
[ "$wrong" -eq 0 ] || rc=1
sort "$T/passing" > "$T/passing.sorted"
sort "$BASE" | comm -23 - "$T/passing.sorted" > "$T/lost"
if [ -s "$T/lost" ]; then echo '  REGRESSION missing baseline names:'; cat "$T/lost"; rc=1; fi
[ "$pass" -gt 0 ] || { echo '  nothing passed: the suite checked nothing'; rc=1; }
exit $rc

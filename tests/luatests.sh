#!/bin/bash
# 0.0.34 L1: lua 5.4's own test files (corpus/lua/testes, the pinned realprog checkout) run by a lua
# built by cc (the oracle) and by unisacc; stdout and exit status must be the same.
#   luatests.sh [ref]   the C reference ($UA) on this host, plus osx/x86_64 under Rosetta on Apple silicon
#   luatests.sh com-build   only the product's cached build (a gate job of its own: ~48 s cold)
#   luatests.sh com     the product ($MODEL_COM).  Its onelua.c build takes ~48 s, so the binary is
#                       cached under $TMPDIR keyed by the sha256 of the product and of the lua sources;
#                       the build is one bounded step and the runs another.  Run ref first: it warms
#                       the cached cc oracle.
# math, sort and constructs print random seeds, timings and a random branch: only their exit status
# is compared; literals also, because its decimal-point part needs the pt_BR locale, which setlocale
# here does not provide (it prints a skip note instead).  Tests needing the C test library (T, ltests.c) skip themselves, as under cc.
_BOUND=$(cd "$(dirname "$0")/.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. ./tests/lib.sh
L=${REALPROG_CACHE:-$R/corpus}/lua
[ -f "$L/onelua.c" ] && [ -d "$L/testes" ] || { echo "luatests: corpus/lua absent (run tests/realprog.sh once to fetch the pinned checkout)"; exit 1; }
mode=${1:-ref}
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
EXACT="strings bitwise calls code files errors closure events goto locals nextvar pm tpack utf8 vararg api coroutine attrib db gc cstack"
STATUS="math sort constructs literals"
# the cc oracle is cached too (key: lua sources + cc --version), so the product's cold build fits one run
SRCSHA=$(cat "$L"/*.c "$L"/*.h | shasum -a 256 | cut -c1-16)
CCK=${TMPDIR:-/tmp}/unisacc-luatests-cc-$( { echo "$SRCSHA"; cc --version 2>&1; } | shasum -a 256 | cut -c1-16)
if [ ! -x "$CCK" ]; then
    (cd "$L" && "$_BOUND" 58 cc -w -O1 -DMAKE_LUA -I. onelua.c -o "$CCK.part" -lm) || { rm -f "$CCK.part"; echo "luatests: cc build failed"; exit 1; }
    mv -f "$CCK.part" "$CCK"
fi
cp "$CCK" "$T/cc"
builds=""
case "$mode" in
ref)
    ua_ready
    (cd "$L" && "$_BOUND" 58 "$UA" -DMAKE_LUA -I. onelua.c -o "$T/ref") || { echo "luatests: reference build failed"; exit 1; }
    builds="ref"
    if [ "$(uname -s)/$(uname -m)" = Darwin/arm64 ] && arch -x86_64 /usr/bin/true 2>/dev/null; then
        (cd "$L" && "$_BOUND" 58 "$UA" -b osx/x86_64 -DMAKE_LUA -I. onelua.c -o "$T/x86") || { echo "luatests: osx/x86_64 build failed"; exit 1; }
        builds="ref x86"
    fi;;
com|com-build)
    [ -n "${MODEL_COM:-}" ] || { echo "luatests: com needs MODEL_COM"; exit 1; }
    case "$MODEL_COM" in /*) ;; *) MODEL_COM=$R/$MODEL_COM;; esac
    key=$( { shasum -a 256 < "$MODEL_COM"; echo "$SRCSHA"; } | shasum -a 256 | cut -c1-16)
    C=${TMPDIR:-/tmp}/unisacc-luatests-$key
    if [ ! -x "$C" ]; then
        (cd "$L" && "$_BOUND" 58 sh "$MODEL_COM" -DMAKE_LUA -I. onelua.c -o "$C.part") || { rm -f "$C.part"; echo "luatests: product build failed"; exit 1; }
        mv -f "$C.part" "$C"
    fi
    [ "$mode" = com-build ] && { echo "luatests  com-build  cached $C"; exit 0; }
    cp "$C" "$T/com"; builds="com";;
*) echo "usage: luatests.sh [ref|com-build|com]" >&2; exit 2;;
esac
ok=0; bad=0
run() { (cd "$L/testes" && "$_BOUND" 30 $1 "$T/$2" -e "_port=true" "$3.lua" 2>&1 | sed -e "s|$T/$2|lua|g" -e "s|^test done on .*|test done on <clock>|"; echo "status ${PIPESTATUS[0]}"); }
for t in $EXACT $STATUS; do
    want=$(run "" cc "$t")
    case " $STATUS " in *" $t "*) want=$(echo "$want" | tail -1);; esac
    for b in $builds; do
        pre=""; [ "$b" = x86 ] && pre="arch -x86_64"
        got=$(run "$pre" "$b" "$t")
        case " $STATUS " in *" $t "*) got=$(echo "$got" | tail -1);; esac
        if [ "$got" = "$want" ]; then ok=$((ok+1)); else bad=$((bad+1)); echo "  FAIL $b $t"; diff <(echo "$want") <(echo "$got") | head -6; fi
    done
done
echo "luatests  $mode  builds [$builds]  ok $ok   wrong $bad"
[ "$ok" -gt 0 ] && [ "$bad" -eq 0 ]

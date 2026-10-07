#!/bin/bash
# W3 (D2): the process lifecycle matrix on this host -- each case of
# tests/life/cases.c built by cc (the oracle), the C reference ($UA) and, when
# MODEL_COM is set, the product; the shell-visible status and stdout must be
# the same.  Linux runs through tests/linux.sh, Windows through the
# release-check winsuite.
_BOUND=$(cd "$(dirname "$0")/.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
CASES="ret0 ret7 ret255 ret256 exit3 atexit pending abort raise handled segv"
"$_BOUND" 30 cc -std=c99 -w -o "$T/cc" tests/life/cases.c || { echo "lifecycle  cc failed"; exit 1; }
"$_BOUND" 30 "$UA" -o "$T/ref" tests/life/cases.c || { echo "lifecycle  reference build failed"; exit 1; }
builds="ref"
if [ -n "${MODEL_COM:-}" ]; then
    "$_BOUND" 40 sh "$MODEL_COM" -o "$T/com" tests/life/cases.c || { echo "lifecycle  product build failed"; exit 1; }
    builds="ref com"
fi
ok=0; bad=0
for c in $CASES; do
    want=$(cd "$T" && "$_BOUND" 10 ./cc "$c" 2>/dev/null; echo "status $?")
    for b in $builds; do
        got=$(cd "$T" && "$_BOUND" 10 "./$b" "$c" 2>/dev/null; echo "status $?")
        if [ "$got" = "$want" ]; then ok=$((ok+1)); else bad=$((bad+1)); echo "  FAIL $b $c: got [$(echo $got)] want [$(echo $want)]"; fi
    done
done
echo; echo "lifecycle  $(uname -s)/$(uname -m)  builds [$builds]  ok $ok   wrong $bad"
[ $ok -gt 0 ] && [ $bad = 0 ]

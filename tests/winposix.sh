#!/bin/bash
# winposix.sh -- 0.0.21 Windows POSIX layer (batch 1): tests/hosthdr/winposix.c
# compiled by unisacc for win/arm64 and win/x86_64, run in the UTM Windows VM
# (tests/winrun.sh), against the same probe built with the system cc and run
# here.  Skips (exit 0, named; STRICT=1 makes it a failure) when the VM agent
# does not answer.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
_BOUND=$("$R/tests/bound" --helper) || exit 2
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
P=tests/hosthdr/winposix.c
UTM=/Applications/UTM.app/Contents/MacOS/utmctl
if [ -z "$("$UTM" ip-address "${WINVM:-minicon-win-arm-64}" 2>/dev/null | head -1)" ]; then
    echo "winposix  skipped (Windows VM not answering)"; [ "${STRICT:-0}" != 1 ]; exit; fi
"$_BOUND" 30 cc -std=c99 -w -o "$T/ref" "$P" || { echo "winposix: cc failed"; exit 1; }
mkdir -p "$T/c"; want=$(cd "$T/c" && "$_BOUND" 10 ../ref </dev/null 2>&1)
ok=0; bad=0; exes=""
for t in win/arm64 win/x86_64; do
    o="$T/wp_${t#win/}.exe"
    if "$_BOUND" 30 "$UA" "$P" -b "$t" -o "$o" 2>"$T/err"; then exes="$exes $o"; else bad=$((bad+1)); echo "  FAIL $t compile: $(head -1 "$T/err")"; fi
done
[ -n "$exes" ] && "$_BOUND" 58 bash tests/winrun.sh $exes > "$T/w.txt" 2>&1
for t in win/arm64 win/x86_64; do
    n="wp_${t#win/}.exe"
    got=$(awk -v n="=== $n" '$0==n{f=1;next} /^=== /{f=0} f&&!/^rc=/' "$T/w.txt")
    if [ -n "$got" ] && [ "$got" = "$want" ]; then ok=$((ok+1)); echo "  ok   winposix $t"
    else bad=$((bad+1)); echo "  FAIL winposix $t"; diff <(echo "$want") <(echo "$got") | head -6; fi
done
echo; echo "winposix  ok $ok   wrong $bad"
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]

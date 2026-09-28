#!/bin/bash
# Compiler-source image equality: all six targets, or one explicit target.
# A host-target slice also executes its self-compiled compiler.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
ALL_TARGETS="lnx/x86_64 lnx/arm64 osx/x86_64 osx/arm64 win/x86_64 win/arm64"
TARGETS="$ALL_TARGETS"; expected=6
case "$#" in
    0) ;;
    1) [ "$1" = --list-targets ] || { echo 'usage: bigclosure.sh [--target OS/ARCH | --list-targets]' >&2; exit 2; }
       printf '%s\n' $ALL_TARGETS; exit 0;;
    2) [ "$1" = --target ] || exit 2
       found=0; for t in $ALL_TARGETS; do [ "$t" = "$2" ] && found=1; done
       [ "$found" -eq 1 ] || { echo "unknown or empty target: $2" >&2; exit 2; }
       TARGETS="$2"; expected=1;;
    *) echo 'usage: bigclosure.sh [--target OS/ARCH | --list-targets]' >&2; exit 2;;
esac
. "$R/tests/lib.sh"; ua_ready
T=$(scratch); HOSTT=$(host_target)
same=0; diff=0; host_selected=0
for t in $TARGETS; do
    [ "$t" = "$HOSTT" ] && host_selected=1
    tt=$(echo "$t" | tr / _)
    if ! bound 30 "$UA" unisacc.c -t "$t" > "$T/tape.$tt" 2>"$T/tape.$tt.err"; then
        echo "  FAIL $t: front end failed"; cat "$T/tape.$tt.err"; diff=$((diff+1)); continue
    fi
    if [ ! -s "$T/tape.$tt" ]; then
        echo "  FAIL $t: empty tape"; diff=$((diff+1)); continue
    fi
    if ! bound 45 python3 -m unisa compile "$T/tape.$tt" --from-tape \
         -o "$T/py.$tt" --target "$t" --drive built >"$T/py.$tt.log" 2>&1; then
        echo "  FAIL $t: Python image build failed"; cat "$T/py.$tt.log"; diff=$((diff+1)); continue
    fi
    if ! bound 30 "$UA" unisacc.c -b "$t" > "$T/ua.$tt" 2>"$T/ua.$tt.err"; then
        echo "  FAIL $t: C image build failed"; cat "$T/ua.$tt.err"; diff=$((diff+1)); continue
    fi
    if [ ! -s "$T/py.$tt" ] || [ ! -s "$T/ua.$tt" ]; then
        echo "  FAIL $t: empty image"; diff=$((diff+1)); continue
    fi
    if cmp -s "$T/py.$tt" "$T/ua.$tt"; then
        same=$((same+1)); printf "  %-12s identical  %s B\n" "$t" "$(wc -c < "$T/ua.$tt")"
    else
        diff=$((diff+1)); printf "  %-12s DIFF  %s\n" "$t" "$(cmp "$T/py.$tt" "$T/ua.$tt" 2>&1 | head -1)"
    fi
done
ranok=0; ranwrong=0
if [ "$host_selected" -eq 1 ]; then
    tt=$(echo "$HOSTT" | tr / _)
    if [ -s "$T/ua.$tt" ]; then
        cp "$T/ua.$tt" "$T/self"; chmod +x "$T/self"
        signrc=0
        if command -v codesign >/dev/null; then bound 10 codesign -f -s - "$T/self" >"$T/sign.log" 2>&1 || signrc=$?; fi
        if [ "$signrc" -ne 0 ]; then
            ranwrong=1; echo "  FAIL self signing exited $signrc"; cat "$T/sign.log"
        else
            got=$(bound 20 "$T/self" -run examples/hello.c 2>&1); rc=$?
            if [ "$rc" -eq 0 ] && [ "$got" = "hello from C99" ]; then ranok=1
            else ranwrong=1; echo "  FAIL self compiler exited $rc and said [$got]"; fi
        fi
    else
        ranwrong=1; echo '  FAIL host image missing'
    fi
fi
echo
echo "bigclosure targets=$expected identical=$same differ=$diff host-selected=$host_selected self-ran=$ranok wrong=$ranwrong"
[ "$same" -eq "$expected" ] && [ "$diff" -eq 0 ] && [ "$ranwrong" -eq 0 ] && \
    { [ "$host_selected" -eq 0 ] || [ "$ranok" -eq 1 ]; }

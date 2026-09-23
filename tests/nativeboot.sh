#!/bin/bash
# The bootstrap with no Python anywhere. [S-6, S-7 item 7]
#
#   N1 = cc(unisacc.c) -b HOST      the foreign compiler's unisacc writes itself
#   N2 = N1 -b HOST                 the native image writes itself
#   N3 = N2 -b HOST
#
# N1 == N2 == N3 byte for byte, and N2 cross-writes every other target to the
# same bytes the foreign-built unisacc does.  bootstrap.sh proves the TAPE
# reaches a fixed point through the Python back end; this proves the IMAGE
# does through unisacc's own.  Images are compared unsigned: codesign
# rewrites the file it signs, so it only ever touches a copy that is run.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"; ua_ready
T=$(scratch)
case "$(uname -s)/$(uname -m)" in
    Darwin/arm64)  H=osx/arm64;;
    Darwin/x86_64) H=osx/x86_64;;
    Linux/x86_64)  H=lnx/x86_64;;
    Linux/aarch64) H=lnx/arm64;;
    *) echo "nativeboot: no host target"; exit 0;;
esac
runnable() { cp "$1" "$2"; chmod +x "$2"
    command -v codesign >/dev/null && codesign -f -s - "$2" >/dev/null 2>&1; true; }
ok=1
"$UA" unisacc.c -b "$H" > "$T/N1"
runnable "$T/N1" "$T/r1"
perl -e 'alarm 300; exec @ARGV' "$T/r1" unisacc.c -b "$H" > "$T/N2" || { echo "  FAIL N1 did not run"; ok=0; }
runnable "$T/N2" "$T/r2"
perl -e 'alarm 300; exec @ARGV' "$T/r2" unisacc.c -b "$H" > "$T/N3" || { echo "  FAIL N2 did not run"; ok=0; }
cmp -s "$T/N1" "$T/N2" || { echo "  FAIL N1 != N2"; ok=0; }
cmp -s "$T/N2" "$T/N3" || { echo "  FAIL N2 != N3"; ok=0; }
for t in lnx/x86_64 lnx/arm64 osx/x86_64 osx/arm64 win/x86_64 win/arm64; do
    [ "$t" = "$H" ] && continue
    cmp -s <("$UA" unisacc.c -b "$t") <("$T/r2" unisacc.c -b "$t") \
        || { echo "  FAIL cross $t"; ok=0; }
done
# Windows, when the UTM machine is up (see tests/crossnative.sh): unisacc's
# own PE, both ISAs, rebuilds itself ON Windows to the same bytes.
UTM=/Applications/UTM.app/Contents/MacOS/utmctl; VM=${WINVM:-minicon-win-arm-64}
if [ -x "$UTM" ] && "$UTM" status "$VM" 2>/dev/null | grep -q started; then
    n=$(date +%s)$RANDOM
    for t in win/arm64 win/x86_64; do
        tt=$(echo "$t" | tr / _)
        "$UA" unisacc.c -b "$t" > "$T/W.$tt"
        "$UTM" file push "$VM" 'C:\u\nb'"$n$tt"'.exe' < "$T/W.$tt" 2>/dev/null
    done
    "$UTM" file push "$VM" 'C:\u\nb'"$n"'.c' < unisacc.c 2>/dev/null
    printf '@echo off\r\ncd /d C:\\u\r\nnb%swin_arm64.exe nb%s.c -b win/arm64 > nb%sa.out\r\nnb%swin_x86_64.exe nb%s.c -b win/x86_64 > nb%sx.out\r\necho done > nb%s.txt\r\n' \
        "$n" "$n" "$n" "$n" "$n" "$n" "$n" | "$UTM" file push "$VM" 'C:\u\nb'"$n"'.bat' 2>/dev/null
    "$UTM" exec "$VM" --hide --cmd cmd.exe -- /c 'C:\u\nb'"$n"'.bat' >/dev/null 2>&1
    i=0; while [ $i -lt 150 ]; do
        case "$("$UTM" file pull "$VM" 'C:\u\nb'"$n"'.txt' 2>&1)" in *done*) break;; esac
        i=$((i+1)); sleep 3; done
    "$UTM" file pull "$VM" 'C:\u\nb'"$n"'a.out' > "$T/Wa" 2>/dev/null
    "$UTM" file pull "$VM" 'C:\u\nb'"$n"'x.out' > "$T/Wx" 2>/dev/null
    cmp -s "$T/W.win_arm64" "$T/Wa" || { echo "  FAIL win/arm64 self-build differs"; ok=0; }
    cmp -s "$T/W.win_x86_64" "$T/Wx" || { echo "  FAIL win/x86_64 self-build differs"; ok=0; }
    [ $ok = 1 ] && echo "  ok  on Windows: win/arm64 and win/x86_64 rebuild themselves"
else
    echo "  skip Windows ($VM not started)"
fi
printf "  %s  N1=N2=N3 on %s, cross 5/5  %s\n" "$([ $ok = 1 ] && echo ok || echo FAIL)" \
    "$H" "$(shasum < "$T/N1" | cut -c1-16)"
echo
[ $ok = 1 ] && echo "native bootstrap reached" || echo "native bootstrap NOT reached"
[ $ok = 1 ]

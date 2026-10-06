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
# Default: local proof only. Run the independent guest proofs explicitly:
#   tests/nativeboot.sh --windows win/arm64
#   tests/nativeboot.sh --windows win/x86_64
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
case "${1:-}" in
    "") mode=local;;
    --windows)
        case "${2:-}" in win/arm64|win/x86_64) ;; *) echo "usage: $0 [--windows win/arm64|win/x86_64]"; exit 2;; esac
        # One independently scheduled VM proof, including transfer/poll overhead.
        exec python3 "$R/tests/bound.py" 55 bash "$0" --windows-step "$2";;
    --windows-step) mode=windows;;
    *) echo "usage: $0 [--windows win/arm64|win/x86_64]"; exit 2;;
esac
. "$R/tests/lib.sh"
T=$(scratch)
SELF="$T/unisacc.flat.c"
bound 10 python3 "$R/tests/sourceflat.py" "$SELF" || exit 1
fail() { echo "  FAIL $*"; return 1; }
compile() { # output, compiler, target: a failed/empty compile is never comparable.
    local output=$1 compiler=$2 target=$3 rc
    bound 55 "$compiler" "$SELF" -b "$target" > "$output" 2> "$output.err"
    rc=$?
    if [ "$rc" -ne 0 ]; then cat "$output.err" >&2; fail "compile $target exited $rc"; return 1; fi
    [ -s "$output" ] || { fail "compile $target produced an empty image"; return 1; }
}
runnable() {
    cp "$1" "$2" && chmod +x "$2" || return 1
    if command -v codesign >/dev/null; then
        bound 10 codesign -f -s - "$2" >/dev/null 2>&1 || return 1
    fi
}
windows() {
    local target=$1 UTM=/Applications/UTM.app/Contents/MacOS/utmctl VM=${WINVM:-minicon-win-arm-64}
    if [ ! -x "$UTM" ] || ! bound 5 "$UTM" status "$VM" 2>/dev/null | grep -q started; then
        echo "  skip Windows $target ($VM not started or UTM unavailable)"
        return 77   # explicitly requested proof did not run; never a passing exit
    fi
    local n; n="$(date +%s)$RANDOM"
    local stem="C:\u\nb$n"
    compile "$T/W" "$UA" "$target" || return 1
    # The agent moves ~0.25 MB/s: the 1.1 MB compiler and the 1.3 MB flat source each blew a
    # 5 s watchdog. Both travel in one gzip tarball (~0.4 MB) that the guest's bsdtar unpacks.
    # The guest runs a cmd batch: its PowerShell only echoes the command text back (2026-09-29).
    mkdir -p "$T/g" && cp "$T/W" "$T/g/nb$n.exe" && cp "$SELF" "$T/g/nb$n.c" || return 1
    (cd "$T/g" && tar -czf "$T/nb.tgz" "nb$n.exe" "nb$n.c") || return 1
    bound 8 "$UTM" file push "$VM" "$stem.tgz" < "$T/nb.tgz" || return 1
    printf '@echo off\r\ntar -xzf %s.tgz -C C:\\u\r\n%s.exe %s.c -b %s > %s.out 2> %s.err\r\necho %%errorlevel%% > %s.rc\r\n' \
        "$stem" "$stem" "$stem" "$target" "$stem" "$stem" "$stem" > "$T/run.cmd"
    bound 5 "$UTM" file push "$VM" "$stem.cmd" < "$T/run.cmd" || return 1
    # `--hide` makes utmctl exec fail with OSStatus -10004; exec returns without waiting, and
    # `file pull` of a missing file exits 0 with empty output, so poll for a non-empty .rc.
    # The guest process is bounded from the host: taskkill on timeout.
    local deadline=$((SECONDS+35)) rc=''
    bound 10 "$UTM" exec "$VM" --cmd cmd.exe -- /c "$stem.cmd" >/dev/null 2>&1 || return 1
    while [ "$SECONDS" -lt "$deadline" ]; do
        if bound 3 "$UTM" file pull "$VM" "$stem.rc" > "$T/rc" 2>/dev/null && [ -s "$T/rc" ]; then
            rc=$(tr -d '\r\n ' < "$T/rc"); break
        fi
        sleep 1
    done
    [ -n "$rc" ] || bound 5 "$UTM" exec "$VM" --cmd taskkill.exe -- /F /IM "nb$n.exe" >/dev/null 2>&1
    [ "$rc" = 0 ] || { fail "$target guest compiler rc=${rc:-timeout}"; return 1; }
    bound 12 "$UTM" file pull "$VM" "$stem.out" > "$T/got" || return 1   # ~1.1 MB back at ~0.25 MB/s
    [ -s "$T/got" ] && cmp -s "$T/W" "$T/got" || { fail "$target guest self-build empty or different"; return 1; }
    echo "  ok on Windows: $target rebuilt itself byte for byte"
}
# Reference construction is also bounded, including the caller-selected UA.
bound 55 env UA="$UA" bash -c 'R=$1; . "$R/tests/lib.sh"; ua_ready' _ "$R" || exit 1
if [ "$mode" = windows ]; then windows "$2"; exit $?; fi
H=$(host_target)
[ -n "$H" ] || { echo "nativeboot: skip (no host target)"; exit 77; }
# NB_PART (0.0.31): a self-built compiler takes ~7.5 s per self-compile, so
# N1..N3 plus five cross images (~55 s) no longer fit one window.  "self"
# proves N1=N2=N3; "cross-a"/"cross-b" use N1 as the native compiler (equal
# to N2 by the self part) for three and two cross targets.  Default: all.
part=${NB_PART:-all}
compile "$T/N1" "$UA" "$H" || exit 1
runnable "$T/N1" "$T/r1" || { fail "prepare N1"; exit 1; }
if [ "$part" = all ] || [ "$part" = self ]; then
    compile "$T/N2" "$T/r1" "$H" || exit 1
    runnable "$T/N2" "$T/r2" || { fail "prepare N2"; exit 1; }
    compile "$T/N3" "$T/r2" "$H" || exit 1
    cmp -s "$T/N1" "$T/N2" && cmp -s "$T/N2" "$T/N3" || { fail "N1=N2=N3"; exit 1; }
    [ "$part" = self ] && { printf '  ok N1=N2=N3 on %s  %s\n' "$H" "$(shasum < "$T/N1" | cut -c1-16)"; exit 0; }
else
    cp "$T/r1" "$T/r2" || exit 1
fi
case $part in
    all) targets="lnx/x86_64 lnx/arm64 osx/x86_64 osx/arm64 win/x86_64 win/arm64";;
    cross-a) targets="lnx/x86_64 lnx/arm64 osx/x86_64 osx/arm64";;
    cross-b) targets="win/x86_64 win/arm64";;
    *) echo "nativeboot: unknown NB_PART $part" >&2; exit 2;;
esac
cross=0
for target in $targets; do
    [ "$target" = "$H" ] && continue
    compile "$T/reference" "$UA" "$target" && compile "$T/native" "$T/r2" "$target" || exit 1
    cmp -s "$T/reference" "$T/native" || { fail "cross $target"; exit 1; }
    cross=$((cross+1))
done
[ "$cross" -gt 0 ] || { fail "no cross target checked"; exit 1; }
if [ "$part" != all ]; then
    printf '  ok %s: cross %s  %s\n' "$part" "$cross" "$(shasum < "$T/N1" | cut -c1-16)"
    [ "$part" = cross-b ] && echo '  skip Windows self-build (run in CI: release-check winsuite; by hand: --windows win/arm64 and --windows win/x86_64)'
    exit 0
fi
printf '  ok N1=N2=N3 on %s, cross %s/5  %s\n' "$H" "$cross" "$(shasum < "$T/N1" | cut -c1-16)"
echo '  skip Windows self-build (run in CI: release-check winsuite; by hand: --windows win/arm64 and --windows win/x86_64)'
echo 'native bootstrap reached locally; Windows self-build unverified'

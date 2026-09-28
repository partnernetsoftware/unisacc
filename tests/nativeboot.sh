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
    bound 5 "$UTM" file push "$VM" "$stem.exe" < "$T/W" || return 1
    bound 5 "$UTM" file push "$VM" "$stem.c" < "$SELF" || return 1
    # Bound the guest process itself: a host-side alarm alone cannot stop it.
    cat > "$T/run.ps1" <<EOF
\$ErrorActionPreference = 'Stop'
\$rc = 1
try {
  \$p = Start-Process -FilePath '$stem.exe' -ArgumentList '$stem.c','-b','$target' -RedirectStandardOutput '$stem.out' -RedirectStandardError '$stem.err' -PassThru
  if (-not \$p.WaitForExit(30000)) { \$p.Kill(); [void]\$p.WaitForExit(2000); \$rc = 124 }
  else { \$p.WaitForExit(); \$rc = \$p.ExitCode }
} catch { \$rc = 1 }
[IO.File]::WriteAllText('$stem.rc', [string]\$rc)
EOF
    bound 5 "$UTM" file push "$VM" "$stem.ps1" < "$T/run.ps1" || return 1
    # exec may wait for PowerShell. Cover its 30s compile + 2s kill budget;
    # an asynchronous return polls only the remainder of the SAME 35s window.
    # The outer process-group watchdog caps transfer + execution at 55s.
    local deadline=$((SECONDS+35)) rc=''
    bound 35 "$UTM" exec "$VM" --hide --cmd powershell.exe -- -NoProfile -ExecutionPolicy Bypass -File "$stem.ps1" >/dev/null || return 1
    while [ "$SECONDS" -lt "$deadline" ]; do
        if bound 3 "$UTM" file pull "$VM" "$stem.rc" > "$T/rc" 2>/dev/null; then
            rc=$(tr -d '\r\n' < "$T/rc"); break
        fi
        sleep 1
    done
    [ "$rc" = 0 ] || { fail "$target guest compiler rc=${rc:-timeout}"; return 1; }
    bound 5 "$UTM" file pull "$VM" "$stem.out" > "$T/got" || return 1
    [ -s "$T/got" ] && cmp -s "$T/W" "$T/got" || { fail "$target guest self-build empty or different"; return 1; }
    echo "  ok on Windows: $target rebuilt itself byte for byte"
}
# Reference construction is also bounded, including the caller-selected UA.
bound 55 env UA="$UA" bash -c 'R=$1; . "$R/tests/lib.sh"; ua_ready' _ "$R" || exit 1
if [ "$mode" = windows ]; then windows "$2"; exit $?; fi
H=$(host_target)
[ -n "$H" ] || { echo "nativeboot: skip (no host target)"; exit 77; }
compile "$T/N1" "$UA" "$H" || exit 1
runnable "$T/N1" "$T/r1" || { fail "prepare N1"; exit 1; }
compile "$T/N2" "$T/r1" "$H" || exit 1
runnable "$T/N2" "$T/r2" || { fail "prepare N2"; exit 1; }
compile "$T/N3" "$T/r2" "$H" || exit 1
cmp -s "$T/N1" "$T/N2" && cmp -s "$T/N2" "$T/N3" || { fail "N1=N2=N3"; exit 1; }
cross=0
for target in lnx/x86_64 lnx/arm64 osx/x86_64 osx/arm64 win/x86_64 win/arm64; do
    [ "$target" = "$H" ] && continue
    compile "$T/reference" "$UA" "$target" && compile "$T/native" "$T/r2" "$target" || exit 1
    cmp -s "$T/reference" "$T/native" || { fail "cross $target"; exit 1; }
    cross=$((cross+1))
done
printf '  ok N1=N2=N3 on %s, cross %s/5  %s\n' "$H" "$cross" "$(shasum < "$T/N1" | cut -c1-16)"
echo '  skip Windows self-build (not run; independent proofs: --windows win/arm64 and --windows win/x86_64)'
echo 'native bootstrap reached locally; Windows self-build unverified'

#!/bin/sh
# Execute the emitted images on targets that are NOT this host, using local
# Linux VMs.  [A-27]
#
# native.sh can only ever check one of the six targets -- whichever machine it
# runs on.  Until this existed, lnx/x86_64 had never been executed anywhere:
# `--fold` interprets, `readelf` only parses, and the interpreter models no
# page protection and no two-operand ALU.  Three real codegen bugs (I-12,
# I-13, I-14) were sitting in that blind spot.
#
# Skipped, not failed, when the VMs are absent -- this is a dev-machine
# instrument, and CI covers lnx/x86_64 directly.
set -u
U="python3 -m unisa"
DRIVE=${DRIVE:-built}
command -v limactl >/dev/null || { echo "crossnative skipped (no limactl)"; exit 0; }

run_target() {   # run_target <vm> <target> <files...>
    vm=$1; target=$2; shift 2
    limactl list --format '{{.Name}} {{.Status}}' 2>/dev/null \
        | grep -q "^$vm Running" || { echo "  skip $target ($vm not running)"; return 0; }
    d=$(mktemp -d); : > "$d/.list"
    for f in "$@"; do
        b=$(basename "$f" .c)
        $U compile "$f" -o "$d/$b" --target "$target" --drive "$DRIVE" \
            >/dev/null 2>&1 || continue
        $U run "$f" --target "$target" --drive "$DRIVE" 2>/dev/null > "$d/$b.want"
        echo "$b" >> "$d/.list"
    done
    tar cf - -C "$d" . | base64 | limactl shell "$vm" sh -c '
        rm -rf /tmp/unisa_cn; mkdir -p /tmp/unisa_cn; cd /tmp/unisa_cn
        base64 -d | tar xf - 2>/dev/null; chmod +x * 2>/dev/null
        p=0; f=0
        while read b; do
            got=$(timeout 10 ./$b 2>/dev/null)
            want=$(cat $b.want)
            if [ "$got" = "$want" ]; then p=$((p+1))
            else f=$((f+1)); printf "  FAIL %-12s native [%s] vs interp [%s]\n" \
                "$b" "$got" "$want"; fi
        done < .list
        echo "$p $f" > /tmp/unisa_cn.score' || true
    read p f <<EOF2
$(limactl shell "$vm" cat /tmp/unisa_cn.score 2>/dev/null || echo "0 0")
EOF2
    printf "  %-12s ok %s   mismatch %s\n" "$target" "$p" "$f"
    rm -rf "$d"
    [ "${f:-1}" -eq 0 ] || bad=$((bad+1))
}

# osx/x86_64 on an Apple Silicon host: Rosetta 2 runs it, so this target is
# executed rather than merely parsed.  On an Intel Mac it just runs.
run_rosetta() {
    [ "$(uname -s)" = "Darwin" ] || return 0
    d=$(mktemp -d); p=0; f=0
    PAR_WAIT=4   # mostly the first-launch scan: waiting, not computing
    . "$(dirname "$0")/par.sh"
    for c in "$@"; do
        b=$(basename "$c" .c)
        throttle
        (
        $U compile "$c" -o "$d/$b" --target osx/x86_64 --drive "$DRIVE" \
            >/dev/null 2>&1 || exit 0
        chmod +x "$d/$b"; codesign -f -s - "$d/$b" >/dev/null 2>&1
        $U run "$c" --target osx/x86_64 --drive "$DRIVE" 2>/dev/null > "$d/$b.want"
        arch -x86_64 "$d/$b" 2>/dev/null > "$d/$b.got"
        ) &
    done
    wait
    for c in "$@"; do
        b=$(basename "$c" .c)
        [ -f "$d/$b.got" ] || continue
        got=$(cat "$d/$b.got"); want=$(cat "$d/$b.want")
        if [ "$got" = "$want" ]; then p=$((p+1))
        else f=$((f+1)); printf "  FAIL %-12s native [%s] vs interp [%s]\n" \
            "$b" "$got" "$want"; fi
    done
    rm -rf "$d"
    printf "  %-12s ok %s   mismatch %s\n" osx/x86_64 "$p" "$f"
    [ "$f" -eq 0 ] || bad=$((bad+1))
}

# Windows, in a UTM VM, through the guest agent.  Both targets: the arm64 one
# runs natively and the x86_64 one under Windows' own emulation.
run_windows() {
    target=$1; shift
    UTM=/Applications/UTM.app/Contents/MacOS/utmctl
    vm=${WINVM:-minicon-win-arm-64}
    [ -x "$UTM" ] || { echo "  skip $target (no UTM)"; return 0; }
    "$UTM" status "$vm" 2>/dev/null | grep -q started || {
        echo "  skip $target ($vm not started)"; return 0; }
    # status can say `started` for a machine that is going down, or whose
    # guest agent is not up yet: only an answered command counts
    perl -e 'alarm 20; exec @ARGV' "$UTM" exec "$vm" --cmd cmd.exe -- /c echo up \
        >/dev/null 2>&1 || { echo "  skip $target ($vm agent not answering)"; return 0; }
    t=$(date +%s)$RANDOM
    Z='C:\u\z'"$t"'.txt'; Y='C:\u\y'"$t"'.txt'
    D='C:\u\d'"$t"'.txt'; G='C:\u\g'"$t"'.bat'
    d=$(mktemp -d); : > "$d/.want"
    {   printf '@echo off\r\n'
        n=0
        for f in "$@"; do
            b=$(basename "$f" .c)
            $U compile "$f" -o "$d/$b.exe" --target "$target" \
                --drive "$DRIVE" >/dev/null 2>&1 || continue
            n=$((n+1)); X='C:\u\e'"$t"'_'"$n"'.exe'
            "$UTM" file push "$vm" "$X" < "$d/$b.exe" 2>/dev/null
            printf 'echo === %s >> %s\r\n' "$b" "$Z"
            printf '%s >> %s 2>&1\r\n' "$X" "$Z"
            $U run "$f" --target "$target" --drive "$DRIVE" 2>/dev/null \
                > "$d/$b.want"
            printf '%s\n' "=== $b" >> "$d/.want"
            cat "$d/$b.want" >> "$d/.want"
        done
        printf 'copy %s %s >nul\r\n' "$Z" "$Y"
        printf 'echo done > %s\r\n' "$D"
    } | "$UTM" file push "$vm" "$G" 2>/dev/null
    "$UTM" exec "$vm" --hide --cmd "cmd.exe" -- /c "$G" >/dev/null 2>&1
    i=0
    while [ $i -lt 150 ]; do
        case "$("$UTM" file pull "$vm" "$D" 2>&1)" in *done*) break;; esac
        i=$((i+1)); sleep 2
    done
    # cmd.exe's `echo ===\xa0x` keeps the space before the redirect, so the
    # marker lines come back with one trailing blank.  Strip it on both sides.
    "$UTM" file pull "$vm" "$Y" 2>&1 | tr -d '\r' | grep -v '^RC=' \
        | sed -e 's/[ \t]*$//' -e '/^$/d' > "$d/.got"
    sed -e 's/[ \t]*$//' -e '/^$/d' "$d/.want" > "$d/.w"
    if cmp -s "$d/.got" "$d/.w"; then
        printf "  %-12s ok %s   mismatch 0\n" "$target" \
            "$(grep -c '^=== ' "$d/.w")"
    else
        printf "  %-12s MISMATCH\n" "$target"
        diff "$d/.w" "$d/.got" | head -8
        bad=$((bad+1))
    fi
    rm -rf "$d"
}

# The targets are independent machines, so they run side by side; each job
# counts its own failures and the reports are printed in the fixed order.
O=$(mktemp -d); trap 'rm -rf "$O"' EXIT
job() { n=$1; shift; ( bad=0; "$@"; echo "$bad" > "$O/$n.bad" ) > "$O/$n.out" 2>&1 & }
job 1 run_target "${VM_X86:-minicon-lnx-x86_64}" lnx/x86_64 "$@"
job 2 run_target "${VM_ARM:-default}"            lnx/arm64  "$@"
job 3 run_rosetta "$@"
# both Windows targets share one VM: one job, in turn
run_win_both() { run_windows win/arm64 "$@"; run_windows win/x86_64 "$@"; }
job 4 run_win_both "$@"
wait
bad=0
for n in 1 2 3 4; do
    cat "$O/$n.out"
    bad=$((bad + $(cat "$O/$n.bad" 2>/dev/null || echo 1)))
done
echo
echo "crossnative failing targets $bad"
[ "$bad" -eq 0 ]

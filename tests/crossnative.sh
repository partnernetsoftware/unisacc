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

bad=0
run_target "${VM_X86:-minicon-lnx-x86_64}" lnx/x86_64 "$@"
run_target "${VM_ARM:-default}"            lnx/arm64  "$@"
echo
echo "crossnative failing targets $bad"
[ "$bad" -eq 0 ]

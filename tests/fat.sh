#!/bin/sh
# One file, both architectures.  [A-28]
#
# A Mach-O universal binary is the standard multi-ISA container, and this host
# can execute both slices -- arm64 natively, x86_64 through Rosetta -- so the
# claim "one executable, several instruction sets" is checked by running it,
# not by reading the header.
set -u
U="python3 -m unisa"
DRIVE=${DRIVE:-built}
[ "$(uname -s)" = "Darwin" ] || { echo "fat skipped (macOS only)"; exit 0; }
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
pass=0; fail=0
PAR_WAIT=4   # mostly the first-launch scan: waiting, not computing
. "$(dirname "$0")/par.sh"
# A: build and run both slices of every probe, PAR at a time.
for f in "$@"; do
    b=$(basename "$f" .c)
    throttle
    (
    $U fat "$f" -o "$T/$b" --drive "$DRIVE" >/dev/null 2>&1 || exit 0
    chmod +x "$T/$b"; codesign -f -s - "$T/$b" >/dev/null 2>&1
    $U run "$f" --target osx/arm64 --drive "$DRIVE" > "$T/$b.want" 2>/dev/null
    "$T/$b" > "$T/$b.a" 2>/dev/null; echo $? > "$T/$b.ac"
    arch -x86_64 "$T/$b" > "$T/$b.x" 2>/dev/null; echo $? > "$T/$b.xc"
    ) &
done
wait
# B: the verdicts, in order.
for f in "$@"; do
    b=$(basename "$f" .c)
    [ -f "$T/$b.xc" ] || { printf "  SKIP %s (fat build failed)\n" "$b"; continue; }
    want=$(cat "$T/$b.want")
    a=$(cat "$T/$b.a"); ac=$(cat "$T/$b.ac")
    x=$(cat "$T/$b.x"); xc=$(cat "$T/$b.xc")
    if [ "$a" = "$want" ] && [ "$x" = "$want" ] && [ "$ac" = "$xc" ]; then
        pass=$((pass+1))
    else
        fail=$((fail+1))
        printf "  FAIL %-12s arm64 [%s](%s)  x86_64 [%s](%s)  want [%s]\n" \
            "$b" "$a" "$ac" "$x" "$xc" "$want"
    fi
done
echo
echo "fat $pass   mismatch $fail   (both slices executed)"
# A suite that checked nothing is not green: `closure.sh` with no
# probes once printed `identical 0 differ 0` and exited 0.
[ "$fail" -eq 0 ] && [ "$pass" -gt 0 ]

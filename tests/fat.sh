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
for f in "$@"; do
    b=$(basename "$f" .c)
    $U fat "$f" -o "$T/$b" --drive "$DRIVE" >/dev/null 2>&1 || {
        printf "  SKIP %s (fat build failed)\n" "$b"; continue; }
    chmod +x "$T/$b"; codesign -f -s - "$T/$b" >/dev/null 2>&1
    want=$($U run "$f" --target osx/arm64 --drive "$DRIVE" 2>/dev/null)
    a=$("$T/$b" 2>/dev/null); ac=$?
    x=$(arch -x86_64 "$T/$b" 2>/dev/null); xc=$?
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
[ "$fail" -eq 0 ]

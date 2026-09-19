#!/bin/sh
# Native execution on this host.  The interpreter is the reference [TP-4]; this
# checks that the emitted image is a REAL program, not just a well-formed file.
set -u
U="python3 -m unisa"
HOST_TARGET=${HOST_TARGET:-osx/arm64}
T=$(mktemp -d); pass=0; fail=0
for f in "$@"; do
    b=$(basename "$f" .c)
    want=$($U run "$f" --target "$HOST_TARGET" --drive built 2>/dev/null); wcode=$?
    $U compile "$f" -o "$T/$b" --target "$HOST_TARGET" --drive built >/dev/null 2>&1
    chmod +x "$T/$b"; codesign -f -s - "$T/$b" >/dev/null 2>&1
    got=$( "$T/$b" 2>/dev/null & p=$!; ( sleep 8; kill -9 $p 2>/dev/null ) >/dev/null 2>&1 &
           wait $p 2>/dev/null ); gcode=$?
    if [ "$got" = "$want" ] && [ "$gcode" = "$wcode" ]; then
        pass=$((pass+1)); printf "  ok   %-10s %s\n" "$b" "$(echo "$got"|head -1)"
    else
        fail=$((fail+1)); printf "  FAIL %-10s native '%s'(%s) vs interp '%s'(%s)\n" \
            "$b" "$got" "$gcode" "$want" "$wcode"
    fi
done
rm -rf "$T"
echo; echo "native $pass   mismatch $fail"
[ "$fail" -eq 0 ]

#!/bin/sh
# Native execution on this host.  The interpreter is the reference [TP-4]; this
# checks that the emitted image is a REAL program, not just a well-formed file.
set -u
U="python3 -m unisa"
# Pick the target that matches this host, so the same script verifies the ELF
# on Linux and the Mach-O on macOS.
if [ -z "${HOST_TARGET:-}" ]; then
    case "$(uname -s)/$(uname -m)" in
        Darwin/arm64)  HOST_TARGET=osx/arm64;;
        Darwin/x86_64) HOST_TARGET=osx/x86_64;;
        Linux/x86_64)  HOST_TARGET=lnx/x86_64;;
        Linux/aarch64) HOST_TARGET=lnx/arm64;;
        *) echo "no native target for $(uname -s)/$(uname -m)"; exit 0;;
    esac
fi
T=$(mktemp -d); pass=0; fail=0
R=$(pwd)
# `python3 -m unisa` resolves against the cwd, and we are about to leave it
PYTHONPATH="$R${PYTHONPATH:+:$PYTHONPATH}"; export PYTHONPATH
for f in "$@"; do
    b=$(basename "$f" .c)
    # both sides run in $T, not the repo: some probes write files
    want=$(cd "$T" && $U run "$R/$f" --target "$HOST_TARGET" \
           --drive built 2>/dev/null); wcode=$?
    $U compile "$f" -o "$T/$b" --target "$HOST_TARGET" --drive built >/dev/null 2>&1
    chmod +x "$T/$b"
    # macOS/arm64 refuses to run an unsigned image; Linux needs nothing
    command -v codesign >/dev/null && codesign -f -s - "$T/$b" >/dev/null 2>&1
    got=$( cd "$T" && ./"$b" 2>/dev/null & p=$!; ( sleep 8; kill -9 $p 2>/dev/null ) >/dev/null 2>&1 &
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

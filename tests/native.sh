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
PAR_WAIT=4   # mostly the first-launch scan: waiting, not computing
. "$R/tests/par.sh"
# A: build, sign and run every probe, PAR at a time.  Most of the time here
# is macOS vetting each freshly signed image on its first launch -- waiting,
# not computing -- so this overlaps well.
for f in "$@"; do
    b=$(basename "$f" .c)
    throttle
    (
    D="$T/$b.d"; mkdir -p "$D"
    # both sides run in their own directory, not the repo: some probes write files
    (cd "$D" && $U run "$R/$f" --target "$HOST_TARGET" \
           --drive built 2>/dev/null) > "$T/$b.want"; echo $? > "$T/$b.wcode"
    $U compile "$f" -o "$D/$b" --target "$HOST_TARGET" --drive built >/dev/null 2>&1
    chmod +x "$D/$b"
    # macOS/arm64 refuses to run an unsigned image; Linux needs nothing
    command -v codesign >/dev/null && codesign -f -s - "$D/$b" >/dev/null 2>&1
    # 30s of wall clock: a hang detector.  The first-launch scan queues
    # system-wide, and at 8s trivial programs were killed while still queued.
    ( cd "$D" && ./"$b" 2>/dev/null & p=$!; ( sleep 30; kill -9 $p 2>/dev/null ) >/dev/null 2>&1 &
      wait $p 2>/dev/null ) > "$T/$b.got"; echo $? > "$T/$b.gcode"
    ) &
done
wait
# B: the verdicts, in order.
for f in "$@"; do
    b=$(basename "$f" .c)
    want=$(cat "$T/$b.want"); wcode=$(cat "$T/$b.wcode")
    got=$(cat "$T/$b.got"); gcode=$(cat "$T/$b.gcode")
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

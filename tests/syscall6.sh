#!/bin/bash
_BOUND=$(cd "$(dirname "$0")/.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# R19-9 (3): the generic system-call gate __syscall6(nr, a0..a4).  The probe
# picks the call number per OS/arch itself and must print "gate ok" on this
# machine (osx/arm64), under Rosetta (osx/x86_64) and, with Lima up, lnx/arm64.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
ok=0; bad=0; skip=0
chk() { if [ "$2" = "gate ok" ]; then ok=$((ok+1)); echo "  ok   $1"; else bad=$((bad+1)); echo "  FAIL $1: [$2]"; fi; }
f=tests/syscall6/write.c
chk "run (host)" "$("$_BOUND" 20 "$UA" "$f" 2>&1)"
if [ "$(uname -s)" = Darwin ]; then
    "$_BOUND" 20 "$UA" "$f" -b osx/x86_64 -o "$T/x" && codesign -f -s - "$T/x" >/dev/null 2>&1
    chk "osx/x86_64 (Rosetta)" "$("$_BOUND" 10 "$T/x" 2>&1)"
fi
VM=${LIMA_VM:-default}
if command -v limactl >/dev/null && [ "$(limactl list "$VM" --format '{{.Status}}' 2>/dev/null)" = Running ] && [ "$(limactl list "$VM" --format '{{.Arch}}' 2>/dev/null)" = aarch64 ]; then
    "$_BOUND" 20 "$UA" "$f" -b lnx/arm64 -o "$T/l"
    chk "lnx/arm64 (Lima)" "$("$_BOUND" 40 sh -c "cat '$T/l' | limactl shell '$VM' -- sh -c 'cat > /tmp/sc6 && chmod +x /tmp/sc6 && /tmp/sc6'" 2>&1)"
else skip=$((skip+1)); echo "  skip lnx/arm64 (Lima $VM not running)"; fi
echo
echo "syscall6  ok $ok   wrong $bad   skipped $skip"
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ] && { [ "${STRICT:-0}" != 1 ] || [ "$skip" -eq 0 ]; }

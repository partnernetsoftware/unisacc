#!/bin/bash
_BOUND=$(cd "$(dirname "$0")/.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# R18-10: host-capability headers (sys/stat.h, fcntl.h, poll.h, termios.h,
# sys/ioctl.h, unistd.h isatty, dirent.h on macOS) -- each probe in
# tests/hosthdr/ is compiled by unisacc and by the system compiler and both
# programs run from a scratch directory; the outputs must be equal.  On this
# machine the host target; with LIMA_VM up (default: default) also lnx/arm64
# against gcc inside the VM.  Windows refuses these ops by name (not here).
# The probes live outside tests/c on purpose: the Python reference VM has no
# such system calls, and tests/c feeds the Python-route suites.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
ok=0; bad=0; skip=0
case "$(uname -s)/$(uname -m)" in Darwin/arm64) HOST=osx/arm64;; Darwin/x86_64) HOST=osx/x86_64;; Linux/x86_64) HOST=lnx/x86_64;; Linux/aarch64) HOST=lnx/arm64;; *) echo "hosthdr: no native target"; exit 1;; esac
# R20-6: every carried header compiles when included on its own, for one
# target per OS (a header that leans on another's includes, or a body that
# names a call the target lacks, fails here before any program sees it)
for h in $(cd include && find . -name '*.h' | sed 's|^\./||' | sort); do
    printf '#include <%s>\nint main(void) { return 0; }\n' "$h" > "$T/alone.c"
    for t in "$HOST" lnx/x86_64 win/x86_64; do
        # 0.0.31 H3: pthread.h refuses Windows on purpose (#error names the version)
        [ "$h" = pthread.h ] && [ "$t" = win/x86_64 ] && continue
        if "$_BOUND" 30 "$UA" -fno-trim-libc "$T/alone.c" -b "$t" -o "$T/alone.out" 2>"$T/err"; then ok=$((ok+1)); else bad=$((bad+1)); echo "  FAIL alone $h $t: $(head -1 "$T/err")"; fi
    done
done
echo "  alone: every include/ header on its own, $HOST lnx/x86_64 win/x86_64"
for f in tests/hosthdr/*.c; do
    b=$(basename "$f" .c); mkdir -p "$T/u.$b" "$T/c.$b"
    "$_BOUND" 30 "$UA" "$f" -b "$HOST" -o "$T/u.$b/p" 2>"$T/err" || { bad=$((bad+1)); echo "  FAIL $b unisacc: $(head -1 "$T/err")"; continue; }
    "$_BOUND" 30 cc -std=c99 -w -o "$T/c.$b/p" "$f" || { skip=$((skip+1)); echo "  skip $b (cc)"; continue; }
    chmod +x "$T/u.$b/p"; command -v codesign >/dev/null && codesign -f -s - "$T/u.$b/p" >/dev/null 2>&1
    got=$(cd "$T/u.$b" && "$_BOUND" 10 ./p </dev/null 2>&1); want=$(cd "$T/c.$b" && "$_BOUND" 10 ./p </dev/null 2>&1)
    if [ "$got" = "$want" ]; then ok=$((ok+1)); echo "  ok   $b $HOST"; else bad=$((bad+1)); echo "  FAIL $b $HOST"; diff <(echo "$want") <(echo "$got") 2>/dev/null | head -4; fi
done
VM=${LIMA_VM:-default}
if command -v limactl >/dev/null && [ "$(limactl list "$VM" --format '{{.Status}}' 2>/dev/null)" = Running ] && [ "$(limactl list "$VM" --format '{{.Arch}}' 2>/dev/null)" = aarch64 ]; then
    for f in tests/hosthdr/*.c; do
        b=$(basename "$f" .c)
        "$_BOUND" 30 "$UA" "$f" -b lnx/arm64 -o "$T/$b.lnx" 2>/dev/null || { bad=$((bad+1)); echo "  FAIL $b lnx/arm64 compile"; continue; }
        out=$(tar -C "$T" -cf - "$b.lnx" -C "$R/tests/hosthdr" "$b.c" | "$_BOUND" 40 limactl shell "$VM" -- sh -c "rm -rf /tmp/hh_$b && mkdir -p /tmp/hh_$b/u /tmp/hh_$b/c && cd /tmp/hh_$b && tar xf - && chmod +x $b.lnx && (cd u && ../$b.lnx </dev/null > ../a.txt 2>&1); gcc -w -o ref $b.c && (cd c && ../ref </dev/null > ../b.txt 2>&1); cmp -s a.txt b.txt && echo SAME || diff b.txt a.txt | head -4")
        case "$out" in SAME) ok=$((ok+1)); echo "  ok   $b lnx/arm64";; *) bad=$((bad+1)); echo "  FAIL $b lnx/arm64"; echo "$out";; esac
    done
else
    skip=$((skip+1)); echo "  skip lnx/arm64 (Lima $VM not running)"
fi
echo
echo "hosthdr  ok $ok   wrong $bad   skipped $skip"
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ] && { [ "${STRICT:-0}" != 1 ] || [ "$skip" -eq 0 ]; }

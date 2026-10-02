#!/bin/bash
_BOUND=$(cd "$(dirname "$0")/.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# R19-10: under -run on macOS a prototyped function no unit and no bundled
# header defines is forwarded to the host libc (dlsym + the uffi bridge).
# Each probe declares host functions itself (gmtime_r, strftime,
# getpagesize) and must print what cc's build prints.  Not on Linux (no
# dynamic loader in -run) or Windows yet: skipped by name there.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
ok=0; bad=0; skip=0
if [ "$(uname -s)" != Darwin ]; then echo "  skip forward (macOS only in 0.0.19)"; echo; echo "forward  ok 0   wrong 0   skipped 1"; [ "${STRICT:-0}" != 1 ]; exit; fi
for f in tests/forward/*.c; do
    b=$(basename "$f" .c)
    [ "$b" = win ] && continue
    "$_BOUND" 30 cc -std=c99 -w -o "$T/$b" "$f" || { skip=$((skip+1)); continue; }
    want=$("$_BOUND" 10 "$T/$b" 2>&1); got=$("$_BOUND" 20 "$UA" "$f" 2>&1)
    if [ "$got" = "$want" ]; then ok=$((ok+1)); echo "  ok   $b"; else bad=$((bad+1)); echo "  FAIL $b: got [$got] want [$want]"; fi
    if [ -n "${MODEL_COM:-}" ]; then
        got=$("$_BOUND" 40 sh "$MODEL_COM" "$f" 2>&1)
        if [ "$got" = "$want" ]; then ok=$((ok+1)); echo "  ok   $b (product)"; else bad=$((bad+1)); echo "  FAIL $b (product): got [$got] want [$want]"; fi
    fi
done
# R21-4a': written images forward too -- the host target, then lnx/arm64 in
# the Lima VM (a dynamic ELF against the guest's libc.so.6), each against
# the system compiler's build of the same probe
HOST=osx/arm64; [ "$(uname -m)" = x86_64 ] && HOST=osx/x86_64
for f in tests/forward/*.c; do
    b=$(basename "$f" .c)
    [ -x "$T/$b" ] || continue
    want=$("$_BOUND" 10 "$T/$b" 2>&1)
    if "$_BOUND" 30 "$UA" "$f" -b "$HOST" -o "$T/$b.img" 2>"$T/err"; then
        codesign -f -s - "$T/$b.img" >/dev/null 2>&1; got=$("$_BOUND" 10 "$T/$b.img" 2>&1)
        if [ "$got" = "$want" ]; then ok=$((ok+1)); echo "  ok   $b ($HOST image)"; else bad=$((bad+1)); echo "  FAIL $b ($HOST image): got [$got] want [$want]"; fi
    else bad=$((bad+1)); echo "  FAIL $b ($HOST image): $(head -1 "$T/err")"; fi
done
VM=${LIMA_VM:-default}
if command -v limactl >/dev/null && [ "$(limactl list "$VM" --format '{{.Status}}' 2>/dev/null)" = Running ] && [ "$(limactl list "$VM" --format '{{.Arch}}' 2>/dev/null)" = aarch64 ]; then
    for f in tests/forward/*.c; do
        b=$(basename "$f" .c); [ "$b" = win ] && continue
        "$_BOUND" 30 "$UA" "$f" -b lnx/arm64 -o "$T/$b.lnx" 2>/dev/null || { bad=$((bad+1)); echo "  FAIL $b lnx/arm64 compile"; continue; }
        out=$(tar -C "$T" -cf - "$b.lnx" -C "$R/tests/forward" "$b.c" | "$_BOUND" 40 limactl shell "$VM" -- sh -c "rm -rf /tmp/fw_$b && mkdir -p /tmp/fw_$b && cd /tmp/fw_$b && tar xf - && chmod +x $b.lnx && ./$b.lnx > a.txt 2>&1; gcc -w -o ref $b.c && ./ref > b.txt 2>&1; cmp -s a.txt b.txt && echo SAME || diff b.txt a.txt | head -4")
        case "$out" in SAME) ok=$((ok+1)); echo "  ok   $b lnx/arm64 image";; *) bad=$((bad+1)); echo "  FAIL $b lnx/arm64 image"; echo "$out";; esac
    done
else skip=$((skip+1)); echo "  skip lnx/arm64 images (Lima $VM not running)"; fi
# Windows images (win/arm64 and win/x86_64 in the UTM VM): kernel32/ucrtbase
# names forwarded through LoadLibraryA/GetProcAddress
if [ -n "$(/Applications/UTM.app/Contents/MacOS/utmctl ip-address "${WINVM:-minicon-win-arm-64}" 2>/dev/null | head -1)" ]; then
    for t in win/arm64 win/x86_64; do
        "$_BOUND" 30 "$UA" tests/forward/win.c -b $t -o "$T/win-${t#win/}.exe" 2>/dev/null || { bad=$((bad+1)); echo "  FAIL win $t compile"; continue; }
        out=$("$_BOUND" 55 ./tests/winrun.sh "$T/win-${t#win/}.exe" | grep -v '^===')
        if [ "$out" = "$(printf 'pid>0 1 tick>0 1 len 9\nrc=0')" ]; then ok=$((ok+1)); echo "  ok   win $t image"; else bad=$((bad+1)); echo "  FAIL win $t image: $out"; fi
    done
else skip=$((skip+1)); echo "  skip Windows images (VM not answering)"; fi
printf 'int no_such_host_function_xyz(int);\nint main(void){ return no_such_host_function_xyz(1); }\n' > "$T/missing.c"
m=$("$_BOUND" 20 "$UA" "$T/missing.c" 2>&1); rc=$?
if [ $rc -eq 127 ] && printf '%s' "$m" | grep -q 'no host function no_such_host_function_xyz'; then ok=$((ok+1)); echo "  ok   missing host function named, rc 127"; else bad=$((bad+1)); echo "  FAIL missing host function: rc $rc [$m]"; fi
echo
echo "forward  ok $ok   wrong $bad   skipped $skip"
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]

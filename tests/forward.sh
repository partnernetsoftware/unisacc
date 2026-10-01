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
    "$_BOUND" 30 cc -std=c99 -w -o "$T/$b" "$f" || { skip=$((skip+1)); continue; }
    want=$("$_BOUND" 10 "$T/$b" 2>&1); got=$("$_BOUND" 20 "$UA" "$f" 2>&1)
    if [ "$got" = "$want" ]; then ok=$((ok+1)); echo "  ok   $b"; else bad=$((bad+1)); echo "  FAIL $b: got [$got] want [$want]"; fi
    if [ -n "${MODEL_COM:-}" ]; then
        got=$("$_BOUND" 40 sh "$MODEL_COM" "$f" 2>&1)
        if [ "$got" = "$want" ]; then ok=$((ok+1)); echo "  ok   $b (product)"; else bad=$((bad+1)); echo "  FAIL $b (product): got [$got] want [$want]"; fi
    fi
done
printf 'int no_such_host_function_xyz(int);\nint main(void){ return no_such_host_function_xyz(1); }\n' > "$T/missing.c"
m=$("$_BOUND" 20 "$UA" "$T/missing.c" 2>&1); rc=$?
if [ $rc -eq 127 ] && printf '%s' "$m" | grep -q 'no host function no_such_host_function_xyz'; then ok=$((ok+1)); echo "  ok   missing host function named, rc 127"; else bad=$((bad+1)); echo "  FAIL missing host function: rc $rc [$m]"; fi
echo
echo "forward  ok $ok   wrong $bad   skipped $skip"
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]

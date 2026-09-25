#!/bin/bash
# Room to grow.  [J-scale]
#
# 1. Headroom: the compiler's own source and tape against its capacity
#    constants.  unisacc.c had reached 1,042,362 bytes of a 1 MB MAXSRC
#    before anyone looked; a suite that fails at half the limit gives the
#    warning while there is still room.
# 2. A large generated program -- about 2 MB of C, thousands of functions
#    and globals -- compiled at -O2 and run: it must print what the
#    generator says, within the 60 s ceiling (AGENTS.md).
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"; ua_ready
T=$(scratch)
ok=0; bad=0
cap() { grep -h "^#define $1 " src/*.c | awk '{print $3}'; }
half() {   # half <what> <used> <limit>
    if [ "$2" -le $(( $3 / 2 )) ]; then ok=$((ok+1)); printf "  ok   %-26s %9s of %9s\n" "$1" "$2" "$3"
    else bad=$((bad+1)); printf "  FAIL %-26s %9s of %9s -- over half: raise the limit\n" "$1" "$2" "$3"; fi
}
half "source (MAXSRC)" "$(wc -c < unisacc.c | tr -d ' ')" "$(cap MAXSRC)"
tape=$(perl -e 'alarm 30; exec @ARGV' "$UA" unisacc.c -t osx/arm64 2>/dev/null)
half "tape text (MAXOUT)" "$(printf '%s' "$tape" | wc -c | tr -d ' ')" "$(cap MAXOUT)"
half "tape lines (OPT_MAXL)" "$(printf '%s\n' "$tape" | wc -l | tr -d ' ')" "$(cap OPT_MAXL)"
half "tape instructions (BK_MAXI)" "$(printf '%s\n' "$tape" | grep -c '^  ')" "$(cap BK_MAXI)"

# the generated program: N functions, each folding its own global into a sum
N=${N:-6000}
python3 - "$N" > "$T/big.c" <<'PY'
import sys
n = int(sys.argv[1])
print("#include <stdio.h>")
for i in range(n):
    print("int g%d = %d;" % (i, i % 97))
    print("int f%d(int x) { int a = x * %d; int b = a + g%d; if (b > %d) b = b - %d; return b %% 1000; }"
          % (i, i % 13 + 1, i, i % 500 + 1, i % 7))
print("int main(void) {")
print("    long s = 0;")
for i in range(n):
    print("    s = s + f%d(%d);" % (i, i % 50))
print('    printf("%ld\\n", s);')
print("    return 0;")
print("}")
PY
want=$(python3 - "$N" <<'PY'
import sys
n = int(sys.argv[1]); s = 0
for i in range(n):
    x = i % 50; a = x * (i % 13 + 1); b = a + i % 97
    if b > i % 500 + 1: b = b - i % 7
    s += b % 1000
print(s)
PY
)
printf "  generated %s bytes, %s functions\n" "$(wc -c < "$T/big.c" | tr -d ' ')" "$N"
s=$(date +%s)
got=$(cd "$T" && perl -e 'alarm 55; exec @ARGV' "$UA" -O2 big.c -run 2>&1); rc=$?
t=$(( $(date +%s) - s ))
if [ $rc -eq 0 ] && [ "$got" = "$want" ]; then ok=$((ok+1)); printf "  ok   %-26s %ss\n" "large program at -O2" "$t"
else bad=$((bad+1)); printf "  FAIL %-26s rc=%s got [%s] want [%s]\n" "large program at -O2" "$rc" "$(printf '%s' "$got" | head -c 80)" "$want"; fi
echo
echo "scale  ok $ok   wrong $bad"
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]

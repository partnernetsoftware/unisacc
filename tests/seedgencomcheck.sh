#!/bin/sh
# 0.0.32 D2: the shipped .com builds seed/gen.c with -O2, and that build constructs parse2
# byte-identical to the cc build within 10 CPU seconds.      usage: MODEL_COM=PATH tests/seedgencomcheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound; COM=${MODEL_COM:-$R/unisacc.com}
T=${TMPDIR:-/tmp}/unisacc-seedgencom.$$; mkdir -p "$T"; trap 'rm -rf "$T"' EXIT
"$B" 55 sh "$COM" -O2 -Iseed seed/gen.c -o "$T/gcom" || { echo "seedgencom: .com does not build gen.c"; exit 1; }
"$B" 55 cc -std=c99 -O2 -Iseed -o "$T/gcc" seed/gen.c || { echo "seedgencom: cc does not build gen.c"; exit 1; }
"$B" 30 "$T/gcc" parse2 "$T/cc.json" >/dev/null || { echo "seedgencom: cc parse2 failed"; exit 1; }
# CPU seconds (user+sys), not wall time: in a 4-job queue the same run took 11 s wall (q13) and 9 s alone
/usr/bin/time -p "$B" 20 "$T/gcom" parse2 "$T/com.json" >/dev/null 2>"$T/time" || { echo "seedgencom: .com parse2 failed or over 20 s"; exit 1; }
dt=$(awk '$1=="user"||$1=="sys"{s+=$2} END{printf "%d", s+0.999}' "$T/time")
cmp -s "$T/cc.json" "$T/com.json" || { echo "seedgencom: parse2 differs"; exit 1; }
[ "$dt" -le 10 ] || { echo "seedgencom: parse2 ${dt}s > 10 s"; exit 1; }
echo "seedgencom  .com -O2 parse2 identical, ${dt}s (<= 10)"

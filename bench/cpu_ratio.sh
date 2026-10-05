#!/bin/sh
# bench/cpu_ratio.sh -- 0.0.27 O1: user CPU time of the same C program built by the host cc -O2 and
# by unisacc -O2 (an image, not -run), median of RUNS runs each; outputs must be identical.
#   UA=./unisacc.com RUNS=3 bench/cpu_ratio.sh        (prints a markdown table)
# Each run is bounded; with RUNS=3 the five programs take about a minute in total, so run it in the
# background or one program at a time:  bench/cpu_ratio.sh fib
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
UA=${UA:-./unisacc.com}; RUNS=${RUNS:-3}
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
progs=${*:-fib sieve matmul hash qsort}
median() { sort -n | awk '{a[NR]=$1} END {print a[int((NR+1)/2)]}'; }
utime() { { /usr/bin/time -p "$@" > /dev/null; } 2>&1 | awk '/^user/ {print $2}'; }
echo "| program | cc -O2 user s (median of $RUNS) | unisacc -O2 user s (median of $RUNS) | ratio | same output |"
echo "|---|---|---|---|---|"
for p in $progs; do
    cc -O2 -w -o "$T/$p.cc" "bench/$p.c" || { echo "| $p | cc failed | | | |"; continue; }
    tests/bound 30 "$UA" "bench/$p.c" -O2 -o "$T/$p.ua" 2>/dev/null && chmod +x "$T/$p.ua" || { echo "| $p | | unisacc failed | | |"; continue; }
    same=no; [ "$("$T/$p.cc")" = "$(tests/bound 30 "$T/$p.ua")" ] && same=yes
    a=$(for i in $(seq "$RUNS"); do utime "$T/$p.cc"; done | median)
    b=$(for i in $(seq "$RUNS"); do utime tests/bound 30 "$T/$p.ua"; done | median)
    r=$(awk -v a="$a" -v b="$b" 'BEGIN { if (a > 0) printf "%.1f", b / a; else print "-" }')
    echo "| $p | $a | $b | $r | $same |"
done

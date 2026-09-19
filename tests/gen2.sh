#!/bin/bash
# The second generation: unisacc.c compiled by unisacc, running as a tape,
# compiling programs.  Its output must match the first generation's. [A-22]
set -u
pass=0; fail=0
for f in "$@"; do
    b=$(basename "$f" .c)
    want=$(python3 -m unisa run "$f" --drive built 2>/dev/null)
    python3 -m unisa vm /tmp/self.tape "$f" -c > /tmp/g2_$b.tape 2>/dev/null || {
        printf "  UNS  %-12s\n" "$b"; continue; }
    got=$(python3 -m unisa vm /tmp/g2_$b.tape 2>/dev/null)
    if [ "$got" = "$want" ]; then pass=$((pass+1)); printf "  ok   %-12s %s\n" "$b" "$got"
    else fail=$((fail+1)); printf "  FAIL %-12s got '%s' want '%s'\n" "$b" "$got" "$want"; fi
done
echo; echo "gen2-compiled $pass   wrong $fail"
[ "$fail" -eq 0 ]

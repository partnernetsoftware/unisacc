#!/bin/bash
# Side-by-side with other C compilers, for Paper A's comparison table.
#
#   TCC=/path/to/tcc/build-dir tests/bench_vs.sh
#
# Every row is the same job -- `-O2 unisacc.c -b osx/arm64` -- done by a
# unisacc that a different compiler built, and every output must be the
# same bytes (a comparison of speed between compilers that disagree is not
# a comparison).  tcc is optional: without $TCC its rows are SKIPPED, named.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"; ua_ready
T=$(scratch)
HT=$(host_target); [ -n "$HT" ] || { echo "bench_vs  SKIP: host is not a target"; exit 0; }
cat tests/refshim.h unisacc.c tests/reffoot.h > "$T/sr.c"
best() {   # best of 3 wall-clock seconds for "$@"
    local b=999 t k
    for k in 1 2 3; do
        /usr/bin/time -p perl -e 'alarm 30; exec @ARGV' "$@" >/dev/null 2>"$T/tm" || true
        t=$(awk '/^real/{print $2}' "$T/tm")
        b=$(echo "$t $b" | awk '{print ($1<$2)?$1:$2}')
    done; echo "$b"
}
row() { printf "%-26s %8s s %10s B  %s\n" "$1" "$2" "$3" "$4"; }
echo "build unisacc.c (1 MB) with:"
row "cc -O2"  "$(best cc -w -std=c99 -O2 -o "$T/ua_cc2" "$T/sr.c")" "$(wc -c <"$T/ua_cc2")" ""
row "cc -O0"  "$(best cc -w -std=c99 -O0 -o "$T/ua_cc0" "$T/sr.c")" "$(wc -c <"$T/ua_cc0")" ""
if [ -n "${TCC:-}" ] && [ -x "$TCC/tcc" ]; then
    row "tcc" "$(best "$TCC/tcc" -B"$TCC" -I"$TCC/include" -w -o "$T/ua_tcc" "$T/sr.c")" "$(wc -c <"$T/ua_tcc")" ""
else echo "  tcc SKIPPED (set TCC=build dir)"; fi
bound 30 "$T/ua_cc2" -O2 unisacc.c -b "$HT" -o "$T/ua_self2"; chmod +x "$T/ua_self2"
bound 30 "$T/ua_cc2" -O0 unisacc.c -b "$HT" -o "$T/ua_self0"; chmod +x "$T/ua_self0"
row "unisacc -O2 (itself)" "$(best "$T/ua_cc2" -O2 unisacc.c -b "$HT" -o "$T/x")" "$(wc -c <"$T/ua_self2")" ""
row "unisacc -O0 (itself)" "$(best "$T/ua_cc2" -O0 unisacc.c -b "$HT" -o "$T/x")" "$(wc -c <"$T/ua_self0")" ""
echo; echo "then each of those compilers runs: unisacc -O2 unisacc.c -b $HT"
bound 30 "$T/ua_self2" -O2 unisacc.c -b "$HT" -o "$T/want" >/dev/null 2>&1   # first launch scan
wrong=0; n=0
for b in ua_cc2 ua_cc0 ua_tcc ua_self0 ua_self2; do
    [ -x "$T/$b" ] || continue
    t=$(best "$T/$b" -O2 unisacc.c -b "$HT" -o "$T/out_$b")
    cmp -s "$T/out_$b" "$T/want" && v=same || { v=DIFFERENT; wrong=$((wrong+1)); }
    row "$b" "$t" "" "$v"; n=$((n+1))
done
echo; echo "bench_vs  compared $n   outputs differ $wrong"
[ $n -ge 3 ] && [ $wrong -eq 0 ]

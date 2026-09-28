#!/bin/bash
# The bootstrap fixed point. [A-23]
#
#   A = cc(unisacc.c)          generation 0, built by a foreign compiler
#   B = A(unisacc.c)           generation 1
#   C = B(unisacc.c)           generation 2
#
# B == C is the classic equivalence: the compiler reproduces itself exactly.
# We additionally check U = unisa(unisacc.c) agrees, i.e. the Python driver and
# the foreign compiler build the SAME compiler.
set -eu
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"; ua_ready
T=$(scratch)
bound 30 "$UA" unisacc.c -c > "$T/bs_B.tape"                       # B
HOST_TARGET=${HOST_TARGET:-osx/arm64}
bound 45 python3 -m unisa compile "$T/bs_B.tape" --from-tape -o "$T/bs_B" \
    --target "$HOST_TARGET" --drive built >/dev/null
chmod +x "$T/bs_B"
if command -v codesign >/dev/null; then bound 10 codesign -f -s - "$T/bs_B" >/dev/null 2>&1; fi
bound 30 "$T/bs_B" unisacc.c -c > "$T/bs_C.tape"                          # C
bound 45 python3 -m unisa compile unisacc.c -o "$T/bs_U" --target "$HOST_TARGET" \
    --drive built >/dev/null
chmod +x "$T/bs_U"
if command -v codesign >/dev/null; then bound 10 codesign -f -s - "$T/bs_U" >/dev/null 2>&1; fi
bound 30 "$T/bs_U" unisacc.c -c > "$T/bs_UT.tape"                         # U
ok=1
cmp -s "$T/bs_B.tape" "$T/bs_C.tape"  || { echo "  FAIL B != C"; ok=0; }
cmp -s "$T/bs_B.tape" "$T/bs_UT.tape" || { echo "  FAIL B != U"; ok=0; }
printf "  %s  B=C=U  %s  (%s lines)\n" \
    "$([ $ok = 1 ] && echo ok || echo FAIL)" \
    "$(shasum < "$T/bs_B.tape" | cut -c1-16)" "$(wc -l < "$T/bs_B.tape")"
echo
[ $ok = 1 ] && echo "bootstrap fixed point reached" || echo "bootstrap NOT reached"
[ $ok = 1 ]

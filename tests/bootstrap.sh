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
./tests/build_ref.sh >/dev/null
/tmp/ua_ref unisacc.c -c > /tmp/bs_B.tape                       # B
HOST_TARGET=${HOST_TARGET:-osx/arm64}
python3 -m unisa compile /tmp/bs_B.tape --from-tape -o /tmp/bs_B \
    --target "$HOST_TARGET" --drive built >/dev/null
chmod +x /tmp/bs_B
command -v codesign >/dev/null && codesign -f -s - /tmp/bs_B >/dev/null 2>&1 || true
/tmp/bs_B unisacc.c -c > /tmp/bs_C.tape                          # C
python3 -m unisa compile unisacc.c -o /tmp/bs_U --target "$HOST_TARGET" \
    --drive built >/dev/null
chmod +x /tmp/bs_U
command -v codesign >/dev/null && codesign -f -s - /tmp/bs_U >/dev/null 2>&1 || true
/tmp/bs_U unisacc.c -c > /tmp/bs_UT.tape                         # U
ok=1
cmp -s /tmp/bs_B.tape /tmp/bs_C.tape  || { echo "  FAIL B != C"; ok=0; }
cmp -s /tmp/bs_B.tape /tmp/bs_UT.tape || { echo "  FAIL B != U"; ok=0; }
printf "  %s  B=C=U  %s  (%s lines)\n" \
    "$([ $ok = 1 ] && echo ok || echo FAIL)" \
    "$(shasum < /tmp/bs_B.tape | cut -c1-16)" "$(wc -l < /tmp/bs_B.tape)"
echo
[ $ok = 1 ] && echo "bootstrap fixed point reached" || echo "bootstrap NOT reached"
[ $ok = 1 ]

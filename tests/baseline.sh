#!/bin/sh
# The SGD control arm.  Run this on purpose; nothing else does.  [E-18] [E-31]
#
# Training is minutes of full-core work and is NOT on the shipping path -- the
# weights we ship are constructed, `unisa acc` verifies those, and the suite
# never touches this.  What the control arm is for is the measured contrast
# the paper rests on:
#
#   E-18 / P-8b  an exact solution exists and gradient descent does not find it
#   E-31 / E-37  extending a table costs construction a known amount and costs
#                training a capacity decision
#
# Usage:  ./tests/baseline.sh            train once and report
#         ./tests/baseline.sh --epochs N
set -u
U="python3 -m unisa"
echo "This trains all 11 nets.  It is CPU-bound for minutes.  Ctrl-C to stop."
echo
$U train "$@" || exit 1
echo
echo "== trained arm, FULL gold =="
$U acc --trained || echo "(an underfit here is a capacity result, not a bug)"
echo
echo "== shipped arm, FULL gold =="
$U acc
echo
echo "== the two arms must emit the same image [P-5] =="
$U compile examples/fib.c -o /tmp/bl_tr --target osx/arm64 --drive spec  >/dev/null
$U compile examples/fib.c -o /tmp/bl_bu --target osx/arm64 --drive built >/dev/null
cmp -s /tmp/bl_tr /tmp/bl_bu && echo "identical" || echo "DIFFER"

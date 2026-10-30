#!/bin/bash
# examples/win/run.sh -- build and run every probe in this directory with the
# reference compiler, and print the first lines of each one's output.
#
# The reference compiler is needed because the shipped model product refuses
# every Win32 prototype on a win target (archive/plans/v0.0.23.md item A1). On a
# machine with no host cc, build one with the product itself:
#
#   bash tests/export_ref.sh out/unisacc-flat.c
#   dist/unisacc.com -O2 -b win/x86_64 -o out/ua-ref-win.exe out/unisacc-flat.c
#
# Usage:  bash examples/win/run.sh [target]      (default win/x86_64)
set -u
R=$(cd "$(dirname "$0")/../.."; pwd); cd "$R"; exit 2
T=${1:-win/x86_64}
UA=${UA:-out/ua-ref-win.exe}
T_OUT=dist/tmp
[ -x "$UA" ]; { echo "run.sh: $UA not found or not executable"; exit 2; }
mkdir -p "$T_OUT"
pass=0; compile_fail=0
for f in examples/win/*.c; do
  n=$(basename "$f" .c)
  if "$UA" -b "$T" -o "$T_OUT/$n.exe" "$f" 2>"$T_OUT/$n.err"; then
    printf '%-11s ' "$n"
    "$T_OUT/$n.exe" 2>&1 | head -3 | tr '\n' '|'
    rc=$?
    echo " [rc=$rc]"
    pass=$((pass + 1))
  else
    printf '%-11s COMPILE-FAIL %s\n' "$n" "$(head -1 "$T_OUT/$n.err")"
    compile_fail=$((compile_fail + 1))
  fi
done
echo "compiled $pass, refused $compile_fail, target $T"
echo "gui/ and refused/ are expected to fail; see examples/win/README.md"
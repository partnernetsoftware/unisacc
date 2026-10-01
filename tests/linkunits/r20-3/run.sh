#!/bin/bash
# R20-3 probes: compile each unit with -c -funit, link with unisacc, compare.
# usage: tests/linkunits/r20-3/run.sh UA   (prints observed behaviour per case)
set -u
UA=${1:?UA}; D=$(cd "$(dirname "$0")" && pwd); R=$(cd "$D/../../.." && pwd)
B="python3 $R/tests/bound.py 20"; T=$(mktemp -d)
case "$(uname -s)/$(uname -m)" in Darwin/arm64) H=osx/arm64;; Darwin/x86_64) H=osx/x86_64;; Linux/x86_64) H=lnx/x86_64;; *) H=lnx/arm64;; esac
for p in "tent_a tent_b" "init1 tentm" "tentm init1" "initm init2" "init2 initm" "main_a main_b" "ext_a ext_b" "stat_a stat_b"; do
  set -- $p; echo "== $1 + $2"
  for f in $1 $2; do $B "$UA" "$D/$f.c" -c -b $H -funit -o "$T/$f.o" 2>&1; done
  echo "-- linked"; $B "$UA" "$T/$1.o" "$T/$2.o" 2>&1; echo "rc=$?"
  echo "-- one-step"; $B "$UA" "$D/$1.c" "$D/$2.c" 2>&1; echo "rc=$?"
done
rm -rf "$T"

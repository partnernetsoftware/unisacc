#!/bin/sh
# usage: rec.sh OUTDIR MODULE... : record gen2 in 8 modes
cd /Users/wjc/repos/unisacc/.claude/worktrees/agent-a9d5e998a42e27432
D=$1; shift
mkdir -p $D
i=0
for m in "" "--locations" "--warnings" "--errors" "--errors --locations" "--warnings --locations" "--warnings --errors" "--warnings --errors --locations"; do
  /private/tmp/claude-501/-Users-wjc-repos-unisacc/a4100558-c3da-4eaf-85bb-27b1ebf587d1/scratchpad/slot.sh python3 tests/bound.py 55 python3 tests/k2translate.py record $D m$i exec/parse2/gen2.py $m $D/out.json -- "$@"
  echo "m$i [$m] exit $?"
  echo "$m" > $D/m$i.flags
  i=$((i+1))
done
rm -f $D/out.json

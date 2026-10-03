#!/bin/sh
# usage: cmp.sh [gen2 args]: compare working gen2 with HEAD gen2 in one mode
cd /Users/wjc/repos/unisacc/.claude/worktrees/agent-a9d5e998a42e27432
S=/private/tmp/claude-501/-Users-wjc-repos-unisacc/a4100558-c3da-4eaf-85bb-27b1ebf587d1/scratchpad/slot.sh
git show HEAD:exec/parse2/gen2.py > exec/parse2/gen2_headcopy.py
$S python3 tests/bound.py 55 python3 exec/parse2/gen2.py "$@" .k2tmp/new.json
$S python3 tests/bound.py 55 python3 exec/parse2/gen2_headcopy.py "$@" .k2tmp/old.json
rm -f exec/parse2/gen2_headcopy.py
cmp .k2tmp/new.json .k2tmp/old.json && echo SAME

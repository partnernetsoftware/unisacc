#!/bin/sh
# Library bodies on demand (-libneed, the reference for the E2 model).
# 1. every carried static body in include/*.h sits alone in its own guard,
#    and the table derived from the headers is well formed;
# 2. a program compiled with -libneed behaves exactly as without it, while
#    its tape carries fewer functions.
# Runs outside the tree so the compiler's embedded headers are the ones used.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
b() { perl -e 'alarm 15; exec @ARGV' "$@"; }
b python3 unisa/libneed.py > /dev/null || { echo "libneed: headers not fully guarded"; python3 unisa/libneed.py; exit 1; }
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
same=0; bad=0; smaller=0
for f in examples/apps/calc.c examples/apps/bf.c examples/apps/wordfreq.c examples/apps/life.c \
         examples/apps/queens.c examples/apps/dijkstra.c examples/apps/colorpack.c \
         tests/c/b_hdrmac_stdlib.c tests/c/b_malloc.c tests/c/b_math.c tests/c/b_ctype.c \
         tests/c/b_sprintf.c tests/c/b_fprintf.c tests/c/b_hdrmac_string.c; do
    [ -f "$f" ] || continue
    a=$(cd "$T" && b "$UA" -run "$R/$f" </dev/null 2>&1; echo "rc=$?")
    n=$(cd "$T" && b "$UA" -libneed -run "$R/$f" </dev/null 2>&1; echo "rc=$?")
    if [ "$a" = "$n" ]; then same=$((same+1)); else bad=$((bad+1)); echo "  DIFF $f"; printf '%s\n' "$n" | tail -2; fi
    t0=$(cd "$T" && b "$UA" -S -o - "$R/$f" 2>/dev/null | grep -c '^[A-Za-z_][A-Za-z0-9_]*:$')
    t1=$(cd "$T" && b "$UA" -libneed -S -o - "$R/$f" 2>/dev/null | grep -c '^[A-Za-z_][A-Za-z0-9_]*:$')
    [ "$t1" -lt "$t0" ] && smaller=$((smaller+1))
done
echo "libneed  same $same   differ $bad   fewer-labels $smaller"
[ "$bad" -eq 0 ] && [ "$same" -gt 0 ] && [ "$smaller" -gt 0 ]

#!/bin/sh
# 0727 driver: serial, setsid -w (util-linux, Linux), real child rc, first red stops (second job must not start)
W=/tmp/cc40-prep/k5-1h/wt; E=/tmp/cc40-prep/seedparse2-mem-0727-driver
mono() { python3 -c 'import time;print(time.monotonic_ns())'; }
cd $W
{ echo written_at=$(date -Iseconds); echo mono_ns=$(mono); echo tip=$(git rev-parse HEAD); echo status=$(git status --short | wc -l); echo "setsid=$(command -v setsid) $(setsid --version 2>&1 | head -1)"; echo "shell_pid=$$ shell_pgid=$(ps -o pgid= -p $$ | tr -d ' ')"
  python3 -c "import sys;sys.path.insert(0,'tests');import seedmemory as m;k=m.reference_key('parse2');print('reference_key_parse2=',k,'MATCH' if k==m.REFERENCE_KEYS['parse2'] else 'MISMATCH')"; } > $E/pre-identity.txt
for j in "seedparse2-1:x locations warnings errors" "seedparse2-2:locations,warnings locations,errors warnings,errors locations,warnings,errors"; do
    n=${j%%:*}; a=${j#*:}
    echo "$n started" >> $E/starts.txt
    s=$(mono)
    setsid -w sh -c 'echo "$$ $(ps -o pgid= -p $$ | tr -d " ")" > "$1"; shift; exec "$@"' _ $E/pgid-$n env NETWORK=0 TMPDIR=$E/tmp timeout 58 ./tests/seedparse2check.sh $a > $E/out-$n.log 2>&1; rc=$?
    e=$(mono)
    read pid pg < $E/pgid-$n
    left=$(ps -o pid= -g "$pg" 2>/dev/null | wc -l); psrc=$?
    echo "$n outer_start_ns=$s outer_end_ns=$e child_rc=$rc wall_ms=$(( (e-s)/1000000 )) pid=$pid pgid=$pg pg_left=$left" >> $E/times.txt
    [ $rc -eq 0 ] || { echo "FIRST_RED_STOP at $n rc=$rc" >> $E/times.txt; break; }
done

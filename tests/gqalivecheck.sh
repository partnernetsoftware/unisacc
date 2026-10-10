#!/bin/bash
# gqalivecheck (0.0.40): release/tools/gqalive.py counts only processes RUNNING gatequeue.py (argv0, or a python
# interpreter whose first non-option argument is it, e.g. python3 -u / -B), never text that mentions the name or a
# file such as notgatequeue.py; an unreadable process table is exit 2, not "none".  Controlled fixture processes.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-gqalive.XXXXXX"); pids=()
trap 'kill ${pids[@]+"${pids[@]}"} 2>/dev/null; rm -rf "$T"' EXIT
fail() { echo "gqalive: $*"; exit 1; }
mkdir -p "$T/a" "$T/b"; printf 'import time\ntime.sleep(40)\n' > "$T/a/gatequeue.py"; cp "$T/a/gatequeue.py" "$T/b/notgatequeue.py"
python3 -u "$T/a/gatequeue.py" & pids+=($!); real_u=$!
python3 -B -X dev "$T/a/gatequeue.py" & pids+=($!); real_bx=$!
python3 "$T/b/notgatequeue.py" & pids+=($!); not_=$!
bash -c 'sleep 40; echo tests/gatequeue.py --com >/dev/null' & pids+=($!); mention=$!
sleep 0.3 2>/dev/null || :
tr '\0' ' ' < /proc/$mention/cmdline 2>/dev/null | grep -q gatequeue.py || ps -o args= -p $mention | grep -q gatequeue.py || fail "mention fixture lost its text (exec'd away)"
for mode in proc ps; do
  out=$(GQALIVE_TABLE=$([ $mode = ps ] && echo ps || echo auto) python3 "$R/release/tools/gqalive.py"); rc=$?
  [ "$rc" = 0 ] || fail "$mode: running gatequeue.py not found (rc $rc)"
  for p in $real_u $real_bx; do printf '%s\n' "$out" | grep -qx "$p" || fail "$mode: python3 -u/-B gatequeue.py ($p) missed"; done
  for p in $not_ $mention; do printf '%s\n' "$out" | grep -qx "$p" && fail "$mode: $p counted (only mentions/suffix-matches gatequeue.py)"; done
done
mkdir -p "$T/shim"; printf '#!/bin/sh\nexit 1\n' > "$T/shim/ps"; chmod +x "$T/shim/ps"
GQALIVE_TABLE=ps PATH="$T/shim:$PATH" python3 "$R/release/tools/gqalive.py" >/dev/null 2>&1; [ $? = 2 ] || fail "an unreadable process table was not exit 2"
echo "gqalive  python3 -u and -B -X dev gatequeue.py found (proc and ps tables); notgatequeue.py and a live shell whose text mentions gatequeue.py not counted; a failing ps is exit 2"

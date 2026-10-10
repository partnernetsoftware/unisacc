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
# malformed argv (a trailing -X / -W with no value) never crashes the classifier into a bare exit 1 ("none")
python3 - "$R/release/tools/gqalive.py" <<'PY' || fail "malformed argv crashed or misclassified"
import importlib.util, sys
src = open(sys.argv[1]).read().split("try: rows = table()")[0]
ns = {}; exec(compile(src, 'gqalive', 'exec'), ns)
for argv in (['python3', '-X'], ['python3', '-W'], ['python3', '-u', '-X'], ['python3'], []):
    assert ns['runs_gatequeue'](argv) is False, argv
assert ns['runs_gatequeue'](['python3', '-X', 'dev', 'x/gatequeue.py']) is True
PY
mkdir -p "$T/shim"; printf '#!/bin/sh\nexit 1\n' > "$T/shim/ps"; chmod +x "$T/shim/ps"
GQALIVE_TABLE=ps PATH="$T/shim:$PATH" python3 "$R/release/tools/gqalive.py" >/dev/null 2>&1; [ $? = 2 ] || fail "an unreadable process table was not exit 2"
# queue.sh accepts only 0 (running) and 1 (none) from the helper: 2, 127, 137 refuse the window (helper stubbed)
mkdir -p "$T/q/release/tools" "$T/q/tests"; cp "$R/release/tools/queue.sh" "$T/q/release/tools/"
for f in queuestart.py queuestart.sh installpair.sh stagelog.py; do cp "$R/release/tools/$f" "$T/q/release/tools/"; done
(cd "$T/q" && git init -q . && git -c user.name=t -c user.email=t@t commit -q --allow-empty -m b)
D=$T/cand; W=$T/wt; mkdir -p "$D" "$W/tests" "$T/seed"; printf c > "$D/unisacc-next.com"; printf s > "$T/seed/unisacc-seed.com"
printf '{"artifact_sha256":"%s"}\n' "$(shasum -a 256 "$D/unisacc-next.com" | cut -d' ' -f1)" > "$D/unisacc-next.com.build.json"
printf '#!/bin/sh\nexit 0\n' > "$T/ua"; chmod +x "$T/ua"; printf '#!/bin/sh\necho x >> "%s/launched"\nexit 0\n' "$T" > "$W/tests/term.sh"; chmod +x "$W/tests/term.sh"
for code in 2 127 137; do
  printf 'import sys\nsys.exit(%s)\n' "$code" > "$T/q/release/tools/gqalive.py"; rm -f "$T/launched"
  env -u QUEUE_START STAGELOG_RUN=0 QUEUE_WORKTREE="$W" QUEUE_STATE="$T/qs$code" QUEUE_BACKUP="$T/qb$code" QUEUE_WINDOWS=1 UNISACC_FFI_X86_PROVIDER=/x \
    bash "$T/q/release/tools/queue.sh" "$D" "$T/ua" "$T/seed" > "$T/out" 2>&1; rc=$?
  [ "$rc" = 2 ] && [ ! -e "$T/launched" ] || fail "helper exit $code: queue rc=$rc, launched=$([ -e "$T/launched" ] && echo yes || echo no) $(cat "$T/out")"
done
echo "gqalive  python3 -u and -B -X dev gatequeue.py found (proc and ps tables); notgatequeue.py and a live shell whose text mentions gatequeue.py not counted; a failing ps is exit 2; a trailing -X/-W never crashes the classifier; queue.sh refuses the window on helper exit 2/127/137"

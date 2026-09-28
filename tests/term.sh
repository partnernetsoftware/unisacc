#!/bin/sh
# term.sh CMD... -- run CMD inside Terminal.app and hand back its output and
# exit status.
#
# Why: macOS scans every freshly written executable on its first launch
# (0.3-0.9 s each, CPU idle), and the suites write and run hundreds.  An app
# listed under Privacy & Security > Developer Tools is exempt -- but the
# exemption follows the RESPONSIBLE app, and a shell under a long-lived tmux
# server (parent launchd) is nobody's, so it is not exempt even when
# Terminal is.  Handing the command to Terminal.app makes Terminal
# responsible: measured 0.31-0.86 s -> 0.00 s per new binary.
#
# Off macOS, or with TERM_SH=0, the command just runs here.
set -u
TERM_SH_ALARM=${TERM_SH_ALARM:-60}
case $TERM_SH_ALARM in
    ''|*[!0-9]*) echo 'term.sh: timeout must be an integer from 1 to 60 seconds' >&2; exit 2;;
esac
if [ "$TERM_SH_ALARM" -lt 1 ] || [ "$TERM_SH_ALARM" -gt 60 ]; then
    echo 'term.sh: timeout must be from 1 to 60 seconds' >&2; exit 2
fi
if [ "$(uname -s)" != Darwin ] || [ "${TERM_SH:-1}" = 0 ] || [ -n "${TERM_SH_INSIDE:-}" ]; then
    exec python3 "$(dirname "$0")/bound.py" "$TERM_SH_ALARM" "$@"
fi
BOUND=$(cd "$(dirname "$0")" && pwd)/bound.py
d=$(mktemp -d); q=""
started=$(date +%s)
deadline=$((started + TERM_SH_ALARM))
for a in "$@"; do q="$q '$(printf '%s' "$a" | sed "s/'/'\\\\''/g")'"; done
cat > "$d/run.sh" <<EOS
#!/bin/sh
cd '$(pwd)'
TERM_SH_INSIDE=1; export TERM_SH_INSIDE
if [ \$(date +%s) -ge $deadline ]; then echo 142 > '$d/rc'; exit 142; fi
python3 '$BOUND' $TERM_SH_ALARM $q > '$d/out' 2>&1 &
echo \$! > '$d/pid'
wait \$!
echo \$? > '$d/rc.tmp' && mv '$d/rc.tmp' '$d/rc'
EOS
chmod +x "$d/run.sh"
# Explicit suite settings only. Quote values as shell literals (including
# apostrophes and newlines), preserving the distinction between unset and empty.
: > "$d/env"
for name in UA UA_RUN PAR SHARD LIMA_VM STRICT DRIVE CC CFLAGS JOBS TARGET NETWORK EXEC_CC \
    E4STRICT CHAINKEEP CHAINV E3KEEP E3V E3REF E3DUMP E3DELTA \
    E2_AUTOINC E2REF E2NOAUTO; do
    eval 'present=${'"$name"'+x}'
    if [ "$present" = x ]; then
        eval 'value=${'"$name"'}'
        escaped=$(printf '%s' "$value" | sed "s/'/'\\\\''/g"; printf x)
        escaped=${escaped%x}
        printf "export %s='%s'\n" "$name" "$escaped" >> "$d/env"
    fi
done
sed -i '' "2i\\
. '$d/env'
" "$d/run.sh"
python3 "$BOUND" 5 osascript -e "tell application \"Terminal\" to do script \"'$d/run.sh'; exit\"" >/dev/null 2>&1 || {
    rm -rf "$d"; exec python3 "$BOUND" "$TERM_SH_ALARM" "$@"; }
# One polling process streams output; no per-poll date/wc/tail subprocesses.
python3 - "$d" "$deadline" <<'PYWATCH'
import os, pathlib, signal, sys, time
root = pathlib.Path(sys.argv[1]); deadline = float(sys.argv[2]); position = 0

def drain():
    global position
    try:
        with (root/'out').open('rb') as f:
            f.seek(position)
            while time.time() < deadline:
                chunk = f.read(65536)
                if not chunk: break
                sys.stdout.buffer.write(chunk); sys.stdout.buffer.flush()
                position += len(chunk)
    except FileNotFoundError:
        pass

def stop(rc):
    try:
        os.kill(int((root/'pid').read_text()), signal.SIGTERM)
    except (FileNotFoundError, ProcessLookupError):
        pass
    drain()
    print(f'term.sh: stopped (rc {rc}; log: {root}/out)', file=sys.stderr)
    raise SystemExit(rc)

signal.signal(signal.SIGINT, lambda *_: stop(130))
signal.signal(signal.SIGTERM, lambda *_: stop(143))
while True:
    drain()
    try:
        rc = int((root/'rc').read_text()); drain(); sys.exit(rc)
    except FileNotFoundError:
        pass
    if time.time() >= deadline: stop(142)
    time.sleep(.1)
PYWATCH
rc=$?
# Preserve failed logs for diagnosis; successful handoffs need no temporary tree.
[ "$rc" != 0 ] || rm -rf "$d"
exit "$rc"

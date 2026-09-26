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
if [ "$(uname -s)" != Darwin ] || [ "${TERM_SH:-1}" = 0 ] || [ -n "${TERM_SH_INSIDE:-}" ]; then
    exec "$@"
fi
d=$(mktemp -d); q=""
for a in "$@"; do q="$q '$(printf '%s' "$a" | sed "s/'/'\\\\''/g")'"; done
cat > "$d/run.sh" <<EOS
#!/bin/sh
cd '$(pwd)'
TERM_SH_INSIDE=1; export TERM_SH_INSIDE
perl -e 'alarm shift; exec @ARGV' ${TERM_SH_ALARM:-600} $q > '$d/out' 2>&1
echo \$? > '$d/rc.tmp' && mv '$d/rc.tmp' '$d/rc'
EOS
chmod +x "$d/run.sh"
# Explicit suite settings only. Quote values as shell literals (including
# apostrophes and newlines), preserving the distinction between unset and empty.
: > "$d/env"
for name in UA UA_RUN PAR SHARD LIMA_VM STRICT DRIVE CC CFLAGS JOBS TARGET \
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
osascript -e "tell application \"Terminal\" to do script \"'$d/run.sh'; exit\"" >/dev/null 2>&1 || {
    rm -rf "$d"; exec "$@"; }
# the window runs on its own; the caller's alarm bounds this wait
while [ ! -f "$d/rc" ]; do sleep 0.2; done
cat "$d/out"; rc=$(cat "$d/rc"); rm -rf "$d"; exit "$rc"

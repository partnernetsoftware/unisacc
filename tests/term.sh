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
    exec perl "$(dirname "$0")/bound.pl" "$TERM_SH_ALARM" "$@"
fi
BOUND=$(cd "$(dirname "$0")" && pwd)/bound.pl
d=$(mktemp -d); q=""
started=$(date +%s)
deadline=$((started + TERM_SH_ALARM))
for a in "$@"; do q="$q '$(printf '%s' "$a" | sed "s/'/'\\\\''/g")'"; done
cat > "$d/run.sh" <<EOS
#!/bin/sh
cd '$(pwd)'
TERM_SH_INSIDE=1; export TERM_SH_INSIDE
if [ \$(date +%s) -ge $deadline ]; then echo 142 > '$d/rc'; exit 142; fi
perl '$BOUND' $TERM_SH_ALARM $q > '$d/out' 2>&1 &
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
perl "$BOUND" 5 osascript -e "tell application \"Terminal\" to do script \"'$d/run.sh'; exit\"" >/dev/null 2>&1 || {
    rm -rf "$d"; exec perl "$BOUND" "$TERM_SH_ALARM" "$@"; }
# One polling process streams output; no per-poll date/wc/tail subprocesses.
perl - "$d" "$deadline" <<'PERL'
use strict;
use warnings;
use Time::HiRes qw(time sleep);
my ($dir, $deadline) = @ARGV;
$| = 1;
my $position = 0;
sub drain {
    if (open(my $out, '<', "$dir/out")) {
        binmode $out; seek($out, $position, 0);
        my $bytes;
        while (read($out, $bytes, 65536)) {
            print $bytes; $position += length($bytes);
            last if time >= $deadline;
        }
        close $out;
    }
}
sub stop {
    my ($rc) = @_;
    if (open(my $pidfile, '<', "$dir/pid")) {
        my $pid = <$pidfile>; close $pidfile;
        if (defined($pid) && $pid =~ /^([0-9]+)\s*$/) { kill 'TERM', $1; }
    }
    drain();
    print STDERR "term.sh: stopped (rc $rc; log: $dir/out)\n";
    exit $rc;
}
$SIG{INT} = sub { stop(130) };
$SIG{TERM} = sub { stop(143) };
while (1) {
    drain();
    if (open(my $result, '<', "$dir/rc")) {
        my $rc = <$result>; close $result;
        drain(); exit int($rc);
    }
    stop(142) if time >= $deadline;
    sleep .1;
}
PERL
rc=$?
# Preserve failed logs for diagnosis; successful handoffs need no temporary tree.
[ "$rc" != 0 ] || rm -rf "$d"
exit "$rc"

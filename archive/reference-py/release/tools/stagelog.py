#!/usr/bin/env python3
"""stagelog: per-stage wall-clock events for the release pipeline (0.0.38 P6).

  stagelog.py begin --run R --phase PHASE [--subphase S] [--attempt N] [--parent-id ID] [--source-commit SHA]
      prints the event id; pair it with
  stagelog.py end --id ID --rc N [--acceptance-status PASS|FAILED|UNVERIFIED|ACCEPTED_BY_RULING]
                  [--execution-status S] [--authorization-ref REF] [--cpu-user S] [--cpu-sys S] [--rss-kib K]
  stagelog.py wait --action begin|end --run R [--id ID] [--phase PHASE] [--reason REASON]

Records one JSON line per event in a private log outside the repository (default
~/.unisacc/stagelog/events.jsonl, or --log).  Only whitelisted fields are written: never argv,
environment, cwd, user names or credentials.  Wall time uses this boot's monotonic clock (boot_id
says which); UTC is for ordering across machines only.  What was not measured stays null, never 0.
An end's rc is the command's; acceptance_status is separate and defaults to UNVERIFIED.
"""
import argparse, json, os, pathlib, re, secrets, sys, time

PHASES = ('code', 'matrix', 'freeze', 'candidate', 'fixedpoint', 'accept17', 'seal', 'queue', 'draft', 'public')
ACCEPT = ('PASS', 'FAILED', 'UNVERIFIED', 'ACCEPTED_BY_RULING')
ROOT = pathlib.Path(__file__).resolve().parents[2]
TOKEN = re.compile(r'^[A-Za-z0-9][A-Za-z0-9._:+-]{0,63}$')   # ids, names, reasons: no paths, no spaces


def boot_id():
    try: return pathlib.Path('/proc/sys/kernel/random/boot_id').read_text().strip()
    except OSError:
        try:   # macOS: kern.boottime identifies the boot
            import subprocess
            return subprocess.run(['sysctl', '-n', 'kern.boottime'], capture_output=True, text=True, timeout=5).stdout.split('}')[0].strip(' {') or None
        except Exception: return None


def token(v, what):
    if v is None: return None
    if not TOKEN.match(v): raise SystemExit('stagelog: %s must be a short token without paths or spaces: %r' % (what, v))
    return v


def logpath(arg):
    p = pathlib.Path(arg).expanduser() if arg else pathlib.Path.home() / '.unisacc' / 'stagelog' / 'events.jsonl'
    if p.is_symlink(): raise SystemExit('stagelog: the event log must not be a symlink')
    p = p.parent.resolve() / p.name
    if p == ROOT or ROOT in p.parents: raise SystemExit('stagelog: the event log stays outside the repository')
    return p


def write(path, rec):
    path.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
    line = json.dumps(rec, sort_keys=True, separators=(',', ':')) + '\n'
    # never follow a planted symlink; one write() per line keeps concurrent appends whole
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_APPEND | getattr(os, 'O_NOFOLLOW', 0), 0o600)
    try: os.write(fd, line.encode())
    finally: os.close(fd)


def num(v, what):
    if v is None: return None
    try: x = float(v)
    except ValueError: raise SystemExit('stagelog: %s must be a number' % what)
    if x != x or x in (float('inf'), float('-inf')) or x < 0: raise SystemExit('stagelog: %s must be a finite non-negative number' % what)
    return x


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--log')
    sub = ap.add_subparsers(dest='cmd', required=True)
    b = sub.add_parser('begin')
    b.add_argument('--run', required=True); b.add_argument('--phase', required=True, choices=PHASES)
    b.add_argument('--subphase'); b.add_argument('--attempt', type=int, default=1)
    b.add_argument('--parent-id'); b.add_argument('--source-commit')
    e = sub.add_parser('end')
    e.add_argument('--id', required=True); e.add_argument('--rc', type=int)
    e.add_argument('--acceptance-status', choices=ACCEPT, default='UNVERIFIED')
    e.add_argument('--execution-status'); e.add_argument('--authorization-ref')
    e.add_argument('--cpu-user'); e.add_argument('--cpu-sys'); e.add_argument('--rss-kib')
    w = sub.add_parser('wait')
    w.add_argument('--action', required=True, choices=('begin', 'end'))
    w.add_argument('--run', required=True); w.add_argument('--id'); w.add_argument('--phase', choices=PHASES)
    w.add_argument('--reason')
    a = ap.parse_args(argv)
    path = logpath(a.log)
    rec = {'mono': round(time.monotonic(), 6), 'boot_id': boot_id(),
           'utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())}
    if a.cmd == 'begin':
        if a.attempt < 1: raise SystemExit('stagelog: attempt starts at 1')
        if a.source_commit and not re.match(r'^[0-9a-f]{7,40}$', a.source_commit):
            raise SystemExit('stagelog: source commit must be a hex sha')
        eid = 'e-' + secrets.token_hex(8)
        rec.update(event='begin', id=eid, run=token(a.run, 'run'), phase=a.phase, subphase=token(a.subphase, 'subphase'),
                   attempt=a.attempt, parent_id=token(a.parent_id, 'parent id'), source_commit=a.source_commit)
        write(path, rec); print(eid)
    elif a.cmd == 'end':
        rec.update(event='end', id=token(a.id, 'id'), rc=a.rc, acceptance_status=a.acceptance_status,
                   execution_status=token(a.execution_status, 'execution status'),
                   authorization_ref=token(a.authorization_ref, 'authorization ref'),
                   cpu_user=num(a.cpu_user, 'cpu user'), cpu_sys=num(a.cpu_sys, 'cpu sys'), rss_kib=num(a.rss_kib, 'rss'))
        write(path, rec)
    else:
        wid = token(a.id, 'id') or ('w-' + secrets.token_hex(8))
        rec.update(event='wait-' + a.action, id=wid, run=token(a.run, 'run'), phase=a.phase, reason=token(a.reason, 'reason'))
        write(path, rec); print(wid)
    return 0


if __name__ == '__main__':
    sys.exit(main())

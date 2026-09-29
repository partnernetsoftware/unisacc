#!/usr/bin/env python3
"""Multi-architecture demo suite for the shipped `unisacc.com`.

Runs, on whatever machine this is, the examples and the deterministic demo apps
through `unisacc.com -run`, and the system apps in their structural modes, then
writes one JSON record. Records from several machines are compared by
tests/comdemo_matrix.py (every architecture must print the same bytes for the
deterministic programs; system programs must satisfy their structural checks).

usage: comdemo.py --com PATH --out RESULT.json [--expected EXPECTED.json] [--label TEXT]
The optional expected file (from the Python reference, computed once) is compared
program by program; missing entries are reported as 'no reference', not failures.
Every child is bounded; nothing here fabricates a result.
"""
import argparse, hashlib, json, os, pathlib, platform, subprocess, sys, time
ROOT = pathlib.Path(__file__).resolve().parents[1]
EXAMPLES = ['hello', 'fact', 'fib', 'ptr', 'struct', 'switch', 'do']
HOST_EXAMPLES = ['host']   # prints host facts: must run (exit 0, output), not compared across machines
DETERMINISTIC_APPS = ['calc', 'life', 'wordfreq', 'queens', 'bf', 'dijkstra', 'colorpack']
SYSTEM_APPS = {  # name -> (args after '--', kind of check)
    'exeinfo': (['SELF'], 'exit0'),   # SELF = the candidate's own path (the root unisacc.com is not in a checkout)
    'procview': (['--capture'], 'processes'),
    'memmap': (['--capture'], 'maps'),
}

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--com', type=pathlib.Path, required=True)
    ap.add_argument('--out', type=pathlib.Path, required=True)
    ap.add_argument('--expected', type=pathlib.Path)
    ap.add_argument('--label', default='')
    ap.add_argument('--timeout', type=int, default=40)
    a = ap.parse_args()
    com = a.com.resolve()
    expected = json.loads(a.expected.read_text()) if a.expected else {}
    prefix = [str(com)]
    if os.name != 'nt' and com.read_bytes()[:2] == b'MZ':
        prefix = ['/bin/sh', str(com)]   # the APE runs through sh on POSIX hosts
    def run(args, stdin=b''):
        t = time.time()
        try:
            p = subprocess.run(prefix + args, input=stdin, capture_output=True, timeout=a.timeout, cwd=ROOT)
            return p.returncode, p.stdout, p.stderr, round(time.time() - t, 3)
        except subprocess.TimeoutExpired:
            return None, b'', b'timeout', round(time.time() - t, 3)
    rec = {'schema': 1, 'label': a.label, 'host': {'system': platform.system(), 'machine': platform.machine(), 'release': platform.release()},
           'com_sha256': hashlib.sha256(com.read_bytes()).hexdigest(), 'com_bytes': com.stat().st_size, 'programs': {}, 'failures': []}
    rc, out, err, dt = run(['--version'])
    rec['version'] = {'rc': rc, 'stdout': out.decode(errors='replace').strip(), 'seconds': dt}
    if rc != 0: rec['failures'].append('version')
    def record(name, path, args, kind):
        rc, out, err, dt = run(['-run', str(path), *(['--', *args] if args else [])])
        r = {'rc': rc, 'seconds': dt, 'stdout_sha256': hashlib.sha256(out).hexdigest(), 'stdout_bytes': len(out), 'kind': kind}
        if kind == 'deterministic':
            r['stdout'] = out.decode(errors='replace')
            if name in expected:
                r['reference'] = 'match' if (rc == 0 and out.decode(errors='replace') == expected[name]) else 'MISMATCH'
            else:
                r['reference'] = 'no reference'
            ok = rc == 0 and r['reference'] != 'MISMATCH'
        elif kind == 'exit0':
            ok = rc == 0 and len(out) > 0
        else:
            ok = rc == 0 and len(out) > 0
            try:
                sys.path.insert(0, str(ROOT / 'tests'))
                import appsstructurecheck as S
                count = (S.processes if kind == 'processes' else S.maps)(out)
                r['structural'] = {'ok': True, 'count': count}
            except Exception as ex:  # structural failure or unsupported platform: recorded, judged below
                r['structural'] = {'ok': False, 'error': repr(ex)[:300]}
                ok = False
        if err and rc != 0: r['stderr'] = err.decode(errors='replace')[-400:]
        r['ok'] = ok
        rec['programs'][name] = r
        if not ok: rec['failures'].append(name)
    for e in EXAMPLES: record(e, ROOT / f'examples/{e}.c', [], 'deterministic')
    for e in HOST_EXAMPLES: record(e, ROOT / f'examples/{e}.c', [], 'exit0')
    for e in DETERMINISTIC_APPS: record(e, ROOT / f'examples/apps/{e}.c', [], 'deterministic')
    for e, (args, kind) in SYSTEM_APPS.items(): record(e, ROOT / f'examples/apps/{e}.c', [str(com) if x == 'SELF' else x for x in args], kind)
    # System apps are allowed to report an unsupported platform (exit 0 with a notice) but not to crash.
    rec['status'] = 'passed' if not rec['failures'] else 'failed'
    a.out.write_text(json.dumps(rec, indent=1))
    print(json.dumps({k: rec[k] for k in ('status', 'host', 'failures')}), 'programs', len(rec['programs']),
          'reference matches', sum(1 for r in rec['programs'].values() if r.get('reference') == 'match'))
    return 0 if rec['status'] == 'passed' else 1

if __name__ == '__main__':
    sys.exit(main())

#!/usr/bin/env python3
"""gateaudit: run one declared gate job with every undeclared repository read denied (0.0.23 E).

A declaration in tests/gatedeps.json lets a queue reuse a job's result while its declared inputs
are unchanged.  That is only safe if the job reads nothing else in the repository.  This runs the
job under macOS sandbox-exec with a profile that denies file reads under the repository root except
the job's declared files and trees (plus the queue's own helpers); metadata reads stay allowed.  A
job that passes normally but fails here, or prints "Operation not permitted", read something it
did not declare.  usage: tests/gateaudit.py NAME [NAME...]   (each run bounded to 55 s)
"""
import json, os, pathlib, subprocess, sys, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests'))
HELPERS = ['tests/bound', 'tests/bound.c', 'tests/bound.py', 'tests/gate.sh', 'tests/gatequeue.py', 'tests/gatedeps.json']

def jobs():
    out = subprocess.run(['sh', 'tests/gate.sh', '--com', '--list'], cwd=ROOT, capture_output=True, text=True, timeout=30,
                         env=dict(os.environ, UNISACC_FFI_X86_PROVIDER=os.environ.get('UNISACC_FFI_X86_PROVIDER', 'LIST')))
    import gatequeue
    return gatequeue.parse_jobs(out.stdout) if hasattr(gatequeue, 'parse_jobs') else None

def declared(name):
    d = json.loads((ROOT / 'tests/gatedeps.json').read_text())
    e = d['suites'].get(name)
    if e is None: sys.exit('gateaudit: %s has no declaration (it would use global inputs; nothing to audit)' % name)
    fam = d['families'].get(e.get('family', ''), {})
    files = set(e.get('files', [])) | set(fam.get('files', [])) | set(e.get('guards', {})) | set(fam.get('guards', {}))
    trees = set(e.get('trees', [])) | set(fam.get('trees', [])) | set(fam.get('reviewed_trees', {})) | set(e.get('reviewed_trees', {}))
    return e['command'], files | set(HELPERS), trees

def profile(files, trees):
    q = lambda p: '"%s"' % str(ROOT / p).replace('"', '\\"')
    allow = ' '.join(['(literal %s)' % q(f) for f in sorted(files)] + ['(subpath %s)' % q(t) for t in sorted(trees)])
    # directory listings stay allowed (imports and globs read directories); file contents do not
    return ('(version 1)\n(allow default)\n(deny file-read-data (subpath "%s"))\n(allow file-read-data %s)\n'
            '(allow file-read-data (vnode-type DIRECTORY))\n' % (ROOT, allow))

def main(names):
    bad = 0
    # 0.0.24 T6: audit the job's real command line (shard arguments, env prefix) when the plan has it;
    # a "contains" declaration alone runs the script with no arguments, which proves nothing for shards
    os.chdir(ROOT)
    import gatequeue
    try: planned = gatequeue.plan(True)
    except Exception: planned = {}
    for name in names:
        command, files, trees = declared(name)
        if name in planned: command = planned[name]
        mc = os.environ.get('MODEL_COM') or './unisacc.com'
        command = [mc if a == '{MODEL_COM}' else a for a in command]
        if command and '=' in command[0].split('/')[0]: command = ['env'] + command   # NAME=value prefixes (gate.sh job lines)
        files = files | {'unisacc.com', 'unisacc.com.build.json'} if mc == './unisacc.com' else files
        with tempfile.NamedTemporaryFile('w', suffix='.sb', delete=False) as f:
            f.write(profile(files, trees)); sb = f.name
        r = subprocess.run([sys.executable, str(ROOT / 'tests/bound.py'), '55', 'sandbox-exec', '-f', sb] + command,
                           cwd=ROOT, capture_output=True, text=True)
        os.unlink(sb)
        out = r.stdout + r.stderr
        log = pathlib.Path(tempfile.gettempdir()) / ('gateaudit-%s.log' % name); log.write_text(out)
        denied = 'Operation not permitted' in out
        verdict = 'UNDECLARED READ' if denied or r.returncode else 'ok'
        if verdict != 'ok': bad += 1
        tail = (r.stdout + r.stderr).strip().splitlines()[-2:] if verdict != 'ok' else []
        print('gateaudit %-28s %s (rc %d) %s  [full output: %s]' % (name, verdict, r.returncode, ' | '.join(tail)[:200], log))
    return 1 if bad else 0

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]) if sys.argv[1:] else 'usage: tests/gateaudit.py NAME...')

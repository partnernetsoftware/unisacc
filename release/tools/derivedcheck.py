#!/usr/bin/env python3
"""Read-only derived-consistency check for a commit (0.0.43, research/c43-derived-consistency.md).

usage:
  derivedcheck.py                       the staged commit: base HEAD, overlay the index
  derivedcheck.py --base P --over C      a historical replay: base P, overlay the content of commit C
  derivedcheck.py --paths PATH...       only these paths (taken from the index)

Maps the changed paths to the gate suites in release/derived-map.tsv (plus script-inventory when a test
script is added, deleted or renamed), builds a clean export (git archive of the base, with the overlay
applied on top, nothing from the working tree), runs each mapped suite's declared command there and reports
rc.  The root product is referenced read-only by a symlink into the export (never copied).  It writes
nothing in the checkout.  Exit 0 only when every mapped suite ran and returned 0.
"""
import fnmatch, json, os, pathlib, shutil, subprocess, sys, tempfile

R = pathlib.Path(__file__).resolve().parents[2]
MAP = R / 'release/derived-map.tsv'
ROOT_PRODUCT = ('unisacc.com', 'unisacc.com.build.json')

def git(*args, check=True):
    return subprocess.run(['git', *args], cwd=R, capture_output=True, check=check).stdout.decode()

def changed(base, over):
    """(status, path) pairs between base and over; over=None means the index."""
    if over is None:
        out = git('diff', '--cached', '--name-status', base)
    else:
        out = git('diff', '--name-status', base, over)
    rows = []
    for line in out.splitlines():
        parts = line.split('\t')
        status = parts[0][:1]
        if status == 'R':
            rows.append(('D', parts[1])); rows.append(('A', parts[2]))
        else:
            rows.append((status, parts[-1]))
    return rows

def suites_for(rows):
    rules = [l.split('\t') for l in MAP.read_text().splitlines() if l.strip() and not l.startswith('#')]
    want, why = set(), {}
    for status, path in rows:
        for glob, suite in rules:
            if fnmatch.fnmatch(path, glob):
                want.add(suite); why.setdefault(suite, []).append(path)
        if path.startswith('tests/') and path.endswith(('.sh', '.py', '.cx')) and status in ('A', 'D'):
            want.add('script-inventory'); why.setdefault('script-inventory', []).append(path)
        if path == 'tests/gate.sh':
            want.add('script-inventory'); why.setdefault('script-inventory', []).append(path)
    return sorted(want), why

def export(base, over, rows, tmp):
    """clean export of base, with the overlay applied; the root product is linked read-only."""
    archive = subprocess.run(['git', 'archive', base], cwd=R, capture_output=True, check=True).stdout
    subprocess.run(['tar', '-x', '-C', str(tmp)], input=archive, check=True)
    for status, path in rows:
        dst = tmp / path
        if status == 'D':
            if dst.exists() or dst.is_symlink(): dst.unlink()
            continue
        dst.parent.mkdir(parents=True, exist_ok=True)
        spec = f'{over}:{path}' if over else f':{path}'
        content = subprocess.run(['git', 'show', spec], cwd=R, capture_output=True, check=True).stdout
        dst.write_bytes(content)
    for name in ROOT_PRODUCT:
        if (R / name).is_file():
            (tmp / name).unlink(missing_ok=True)
            (tmp / name).symlink_to(R / name)

def declared_commands():
    d = json.loads((R / 'tests/gatedeps.json').read_text())['suites']
    return {n: v['command'] for n, v in d.items() if 'command' in v}

def main(argv):
    base, over, paths = 'HEAD', None, None
    if '--base' in argv: base = argv[argv.index('--base') + 1]
    if '--over' in argv: over = argv[argv.index('--over') + 1]
    if '--paths' in argv:
        paths = argv[argv.index('--paths') + 1:]
        rows = [('M', p) for p in paths]
    else:
        rows = changed(base, over)
    suites, why = suites_for(rows)
    print('changed: %d path(s); derived checks: %s' % (len(rows), ', '.join(suites) or 'none (no derived file or test-script change)'))
    if not suites:
        return 0
    commands = declared_commands()
    missing = [s for s in suites if s not in commands]
    if missing:
        print('FAIL: suite(s) without a gatedeps declaration (fail closed): ' + ', '.join(missing))
        return 1
    tmp = pathlib.Path(tempfile.mkdtemp(prefix='derivedcheck-'))
    worst = 0
    try:
        export(base, over, rows, tmp)
        env = dict(os.environ, PYTHONDONTWRITEBYTECODE='1')
        for s in suites:
            cmd = commands[s]
            try:
                p = subprocess.run(cmd, cwd=tmp, env=env, capture_output=True, timeout=55)
                rc, tail = p.returncode, (p.stdout + p.stderr).decode(errors='replace').strip().splitlines()[-1:]
            except subprocess.TimeoutExpired:
                rc, tail = 124, ['timeout 55 s']
            worst = max(worst, 0 if rc == 0 else 1)
            print('  %-16s rc=%-3d %s   (because: %s)' % (s, rc, (tail[0] if tail else '')[:90], ', '.join(why[s][:3])))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print('derivedcheck: ' + ('ok' if worst == 0 else 'FAIL'))
    return worst

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

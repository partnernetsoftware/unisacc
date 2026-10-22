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

REPO = pathlib.Path(__file__).resolve().parents[2]   # the checkout this tool lives in (default)
# every step has its own bound (<= 55 s): each git call of the export and each mapped suite.  The bound is not a
# sum: several suites together may exceed 55 s, each one still runs inside its own limit.  DERIVEDCHECK_BOUND
# lowers it for the self-test only (the self-test checks the sum rule with small numbers).
BOUND = min(55, int(os.environ.get('DERIVEDCHECK_BOUND', '55')))
R = REPO                                              # set by --root (commitgate passes the checked repository)
MAP = None
ROOT_PRODUCT = ('unisacc.com', 'unisacc.com.build.json')

def run_step(argv, **kw):
    """one bounded step: tests/bound.py with BOUND seconds; 142 on timeout (the bound helper's own code)"""
    return subprocess.run([sys.executable, '-I', str(REPO / 'tests/bound.py'), str(BOUND)] + list(argv), **kw)

def git(*args, check=True):
    return subprocess.run(['git', *args], cwd=R, capture_output=True, check=check).stdout.decode()

def worktree_rows(paths):
    """(status, path) for the working-tree content of the given paths, as a commit with --only would take it."""
    files = []
    for p in paths:   # a directory argument stands for its tracked and untracked files, as git commit -- DIR does
        if (R / p).is_dir():
            files += [q for q in subprocess.run(['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard', '--', p],
                                                cwd=R, capture_output=True, check=True).stdout.decode().split('\0') if q]
        else:
            files.append(p)
    rows = []
    for p in dict.fromkeys(files):
        in_head = subprocess.run(['git', 'cat-file', '-e', 'HEAD:' + p], cwd=R, capture_output=True).returncode == 0
        if not (R / p).exists() and not (R / p).is_symlink(): rows.append(('D', p))
        else: rows.append(('M' if in_head else 'A', p))
    return rows

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
    # no map in this checkout means no derived-consistency rows: an empty mapping costs nothing
    rules = [l.split('\t') for l in MAP.read_text().splitlines() if l.strip() and not l.startswith('#')] if MAP.exists() else []
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
    """clean export of base, with the overlay applied; the root product is linked read-only.
    over: a commit, None (the index) or 'WORKTREE' (the working-tree bytes the commit would take)."""
    # git's default archive umask is 002 (0664 files) and the reviewed-tree stamps hash st_mode: export with git's own
    # checkout modes, as tests/refresh_gatedeps.py does, or reviewed-trees is red on any host without tar.umask set
    archive = run_step(['git', '-c', 'tar.umask=022', 'archive', base], cwd=R, capture_output=True, check=True).stdout
    run_step(['tar', '-x', '-C', str(tmp)], input=archive, check=True)
    # the checks that call git (rev-parse HEAD) need a repository: a fresh one with one commit of this export
    env = dict(os.environ, GIT_AUTHOR_NAME='derived', GIT_AUTHOR_EMAIL='d@d', GIT_COMMITTER_NAME='derived', GIT_COMMITTER_EMAIL='d@d')
    run_step(['git', 'init', '-q'], cwd=tmp, check=True, env=env)
    run_step(['git', 'add', '-A', '--', '.'], cwd=tmp, check=True, env=env, capture_output=True)
    run_step(['git', 'commit', '-q', '-m', 'derived export'], cwd=tmp, check=True, env=env, capture_output=True)
    for status, path in rows:
        dst = tmp / path
        if status == 'D':
            if dst.exists() or dst.is_symlink(): dst.unlink()
            continue
        dst.parent.mkdir(parents=True, exist_ok=True)
        if over == 'WORKTREE':
            content = (R / path).read_bytes()
        else:
            spec = f'{over}:{path}' if over else f':{path}'
            content = run_step(['git', 'show', spec], cwd=R, capture_output=True, check=True).stdout
        dst.write_bytes(content)
        dst.chmod(git_file_mode(path, over))
    for name in ROOT_PRODUCT:
        if (R / name).is_file():
            (tmp / name).unlink(missing_ok=True)
            (tmp / name).symlink_to(R / name)

def git_file_mode(path, over=None):
    """The mode git would check the file out with: 0755 for an executable entry, else 0644.
    A plain write_bytes leaves the writer's umask mode (a new shell script came out 0644 where git
    records 0755), and st_mode goes into the reviewed_trees snapshot, so an export carries git's
    mode, not the writer's."""
    if over in (None, 'WORKTREE'):
        if over == 'WORKTREE':
            return 0o755 if os.stat(R / path).st_mode & 0o111 else 0o644
        out = run_step(['git', 'ls-files', '-s', '--', path], cwd=R, capture_output=True, check=True).stdout.split()
    else:
        out = run_step(['git', 'ls-tree', over, '--', path], cwd=R, capture_output=True, check=True).stdout.split()
    return 0o755 if out and out[0] == b'100755' else 0o644

def export_worktree(base, rows, tmp):
    export(base, None, [], tmp)              # base tree first (index entries are overwritten below)
    for status, path in rows:
        dst = tmp / path
        if status == 'D':
            if dst.exists() or dst.is_symlink(): dst.unlink()
            continue
        dst.parent.mkdir(parents=True, exist_ok=True)
        dst.write_bytes((R / path).read_bytes())
        dst.chmod(git_file_mode(path, 'WORKTREE'))
    for name in ROOT_PRODUCT:
        if (R / name).is_file():
            (tmp / name).unlink(missing_ok=True)
            (tmp / name).symlink_to(R / name)

def declared_commands():
    d = json.loads((R / 'tests/gatedeps.json').read_text())['suites']
    return {n: v['command'] for n, v in d.items() if 'command' in v}

def main(argv):
    global R, MAP
    base, over, paths = 'HEAD', None, None
    if '--root' in argv:
        R = pathlib.Path(argv[argv.index('--root') + 1]).resolve()
    MAP = R / 'release/derived-map.tsv'
    if '--base' in argv: base = argv[argv.index('--base') + 1]
    if '--over' in argv: over = argv[argv.index('--over') + 1]
    if '--worktree' in argv:
        paths = argv[argv.index('--worktree') + 1:]
        if paths and paths[0] == '--': paths = paths[1:]
        rows = worktree_rows(paths); over = 'WORKTREE'
    elif '--paths' in argv:
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
    # A derived check runs on a clean export with only the (stale) root product linked: a suite whose declared
    # command names the product under test ({MODEL_COM}) cannot be checked here and must not be mapped (c43: the
    # stage-contract job got the literal "{MODEL_COM}" and blocked an unrelated commit).
    product = [s for s in suites if any('{MODEL_COM}' in a for a in commands[s])]
    if product:
        print('FAIL: product-dependent suite(s) mapped in release/derived-map.tsv (fail closed; run them in the gate with MODEL_COM): ' + ', '.join(product))
        return 1
    tmp = pathlib.Path(tempfile.mkdtemp(prefix='derivedcheck-'))
    worst = 0
    try:
        export(base, None if over == 'WORKTREE' else over, rows, tmp) if over != 'WORKTREE' else export_worktree(base, rows, tmp)
        env = dict(os.environ, PYTHONDONTWRITEBYTECODE='1')
        for s in suites:
            cmd = commands[s]
            # the declared command may carry its own bound (tests/bound N); this step's bound replaces it
            if len(cmd) >= 2 and cmd[0].endswith('tests/bound') and cmd[1].isdigit(): cmd = cmd[2:]
            p = run_step(cmd, cwd=tmp, env=env, capture_output=True)
            rc, tail = p.returncode, (p.stdout + p.stderr).decode(errors='replace').strip().splitlines()[-1:]
            if rc == 142: tail = ['timeout %d s (bound)' % BOUND]
            worst = max(worst, 0 if rc == 0 else 1)
            print('  %-16s rc=%-3d %s   (because: %s)' % (s, rc, (tail[0] if tail else '')[:90], ', '.join(why[s][:3])))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print('derivedcheck: ' + ('ok' if worst == 0 else 'FAIL'))
    return worst

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

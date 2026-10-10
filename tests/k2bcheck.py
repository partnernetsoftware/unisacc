#!/usr/bin/env python3
"""k2bcheck (0.0.40 K2b, 机房主任 18:55): narrow reuse only for the named whitelist; every invalidation path
falls back to the whole tree; a missed run (narrow-reused result != whole-tree result) is RED, revokes the suite
in the run state and voids its recorded result.  Runs in a scratch copy of this checkout; never edits it."""
import hashlib, json, os, pathlib, platform, shutil, subprocess, sys, tempfile
SRC = pathlib.Path(__file__).resolve().parents[1]
def fail(m): print('k2b: ' + m); sys.exit(1)
T = pathlib.Path(tempfile.mkdtemp(prefix='unisacc-k2b.'))
try:
    names = subprocess.check_output(['git', '-C', str(SRC), 'ls-files', '-z', '-co', '--exclude-standard']).decode().split('\0')
    for n in filter(None, names):
        s = SRC / n
        if s.is_file() and not s.is_symlink(): (T / n).parent.mkdir(parents=True, exist_ok=True); shutil.copy2(s, T / n)
    # the whitelist row needs a review record; this one is a FIXTURE written into the private copy before its first
    # git snapshot -- it is not a signed review and nothing outside the scratch copy reads it
    (T / 'research').mkdir(exist_ok=True)
    (T / 'research/k2b-review-volatile-comma.md').write_text('FIXTURE ONLY (tests/k2bcheck.py): not a signed closure review\n')
    os.chdir(T); env = dict(os.environ, GIT_AUTHOR_NAME='t', GIT_AUTHOR_EMAIL='t@t', GIT_COMMITTER_NAME='t', GIT_COMMITTER_EMAIL='t@t')
    for c in (['git', 'init', '-q', '.'], ['git', 'add', '-A'], ['git', 'commit', '-q', '-m', 'k2b fixture']): subprocess.run(c, check=True, env=env)
    sys.path.insert(0, str(T / 'tests')); import gatequeue as q
    plan = q.plan(False); jobs = {n: plan[n] for n in ('volatile-comma', 'bound')}
    def fp():
        parts = {}; st = q.fingerprint(jobs, parts, revoked=REVOKED); return st, parts
    def k(p, n='volatile-comma'): return p[n]['k2b']
    def reset(): subprocess.run(['git', 'checkout', '-q', '--', '.'], check=True); subprocess.run(['git', 'clean', '-qfd'], check=True)
    def whitelist(decl_mutator=None, host=None):
        m = json.loads(pathlib.Path('tests/gatedeps.json').read_text())
        if decl_mutator: decl_mutator(m); pathlib.Path('tests/gatedeps.json').write_text(json.dumps(m, indent=1))
        raw = m['suites']['volatile-comma']
        d = hashlib.sha256(json.dumps({'suite': raw, 'family': None}, sort_keys=True).encode()).hexdigest()
        r = hashlib.sha256(pathlib.Path('research/k2b-review-volatile-comma.md').read_bytes()).hexdigest()
        pathlib.Path('tests/k2b-whitelist.tsv').write_text('volatile-comma\t%s\tresearch/k2b-review-volatile-comma.md\t%s\t%s\tUA=sha256:%s\n'
            % (d, r, host or '%s/%s' % (platform.system(), platform.machine()), hashlib.sha256(pathlib.Path(os.environ.get('K2B_UA', '/tmp/ua_ref')).read_bytes()).hexdigest()))
    REVOKED = frozenset()
    os.environ['UA'] = os.environ.get('K2B_UA', '/tmp/ua_ref')   # the reviewed warm route (UA given)
    whitelist()
    subprocess.run(['git', 'add', '-A'], check=True); subprocess.run(['git', 'commit', '-q', '-m', 'whitelist'], check=True, env=env)
    del os.environ['UA']; _, p = fp()
    if not k(p).startswith('condition UA=sha256'): fail('cold route (UA unset) narrowed: %s' % k(p))
    os.environ['UA'] = os.environ.get('K2B_UA', '/tmp/ua_ref')
    base, p = fp()
    if k(p) != 'narrow': fail('whitelisted suite not narrow: %s' % k(p))
    if k(p, 'bound') != 'not whitelisted': fail('N1 audited non-whitelisted suite narrowed: %s' % k(p, 'bound'))
    # P1 unrelated change keeps the narrow stamp; the whole-tree (double) stamp moves; N1 bound moves too
    pathlib.Path('ARCHITECTURE.md').write_text('unrelated\n'); s, p2 = fp()
    if s['volatile-comma'] != base['volatile-comma']: fail('P1 unrelated change invalidated the narrow stamp')
    if p2['volatile-comma']['k2b_tree_stamp'] == p['volatile-comma']['k2b_tree_stamp']: fail('double stamp did not see the unrelated change')
    if s['bound'] == base['bound']: fail('N1 non-whitelisted suite reused across an unrelated change (must be whole tree)')
    reset()
    # N7/N8 related changes: content, mode, deletion, a new member of a declared tree
    for name, act in (('N7 content', lambda: open('unisa/vm.py', 'a').write('\n# x\n')),
                      ('N8 mode', lambda: os.chmod('tests/volatilecomma.py', 0o755)),
                      ('N8 delete', lambda: os.remove('unisa/fp.py')),
                      ('N8 new member', lambda: (pathlib.Path('unisa/k2bnew.py').write_text('x=1\n'), subprocess.run(['git', 'add', 'unisa/k2bnew.py'], check=True)))):
        act(); s, _ = fp()
        if s['volatile-comma'] == base['volatile-comma']: fail(name + ' did not invalidate')
        reset()
    # N9 shared executable input (UA)
    ua0 = os.environ['UA']; os.environ['UA'] = sys.executable; s, _ = fp(); os.environ['UA'] = ua0
    if s['volatile-comma'] == base['volatile-comma']: fail('N9 a changed UA executable did not invalidate')
    # N2 declaration changed, N3 guard broken, N4 host, N5 untracked in scope, N6 dynamic, review record changed
    m = json.loads(pathlib.Path('tests/gatedeps.json').read_text()); m['suites']['volatile-comma']['files'].append('README.md')
    pathlib.Path('tests/gatedeps.json').write_text(json.dumps(m, indent=1)); _, p = fp()
    if k(p) != 'declaration changed': fail('N2 changed declaration kept narrow: %s' % k(p))
    reset(); open('tests/lib.sh', 'a').write('\n# x\n'); _, p = fp()
    if k(p) != 'not audited': fail('N3 broken guard kept narrow: %s' % k(p))
    reset(); whitelist(host='Plan9/mips'); _, p = fp()
    if not k(p).startswith('host'): fail('N4 other host kept narrow: %s' % k(p))
    reset(); pathlib.Path('unisa/untracked_k2b.py').write_text('x=1\n'); _, p = fp()
    if k(p) != 'untracked file inside the declaration': fail('N5 untracked input kept narrow: %s' % k(p))
    reset(); whitelist(lambda m: m['suites']['volatile-comma'].__setitem__('dynamic', 'unknown')); _, p = fp()
    if k(p) != 'dynamic inputs not measured': fail('N6 dynamic inputs kept narrow: %s' % k(p))
    reset(); open('research/k2b-review-volatile-comma.md', 'a').write('edited\n'); _, p = fp()
    if k(p) != 'review record changed or missing': fail('review record change kept narrow: %s' % k(p))
    reset()
    # the real production channel: two real gatequeue runs of the suite (narrow, then whole tree) and the CLI
    vc = 'volatile-comma'; reset(); whitelist(); subprocess.run(['git', 'add', '-A'], check=True); subprocess.run(['git', 'commit', '-q', '-m', 'real channel'], check=True, env=env)
    def queue(state):
        shutil.rmtree(state, ignore_errors=True)
        r = subprocess.run([sys.executable, 'tests/gatequeue.py', '--state', str(state), '--suite', vc, '--jobs', '1', '--window', '30'],
                           capture_output=True, text=True, timeout=50, env=dict(os.environ))
        if ('DONE %s ' % vc) not in r.stdout: fail('no fresh DONE line for %s in this run (resumed result is not a new execution): %s' % (vc, r.stdout[-300:]))
        return r.returncode, json.loads((state / 'results.json').read_text())
    rcn, sn = queue(T / 'rn')
    os.environ['K2B_FORCE_TREE'] = '1'; rct, st_ = queue(T / 'rt'); del os.environ['K2B_FORCE_TREE']
    if 'K2B_FORCE_TREE' in pathlib.Path(T / 'rt' / (vc + '.log')).read_text(errors='replace'): fail('K2B_FORCE_TREE leaked into the suite')
    rn, rt = sn['results'].get(vc, {}), st_['results'].get(vc, {})
    if rcn or rct or rn.get('rc') != 0 or rt.get('rc') != 0: fail('real channel runs failed: %s %s %s %s' % (rcn, rct, rn, rt))
    if rn['k2b'].get('scope') != 'narrow' or not str(rt['k2b'].get('scope')).startswith('forced whole tree') or not rn.get('out_sha'): fail('real run provenance missing: %s %s' % (rn, rt))
    if rn['k2b'].get('tree_stamp') != rt['k2b'].get('stamp'): fail('real narrow run whole-tree stamp != real whole-tree run stamp (same inputs)')
    r = subprocess.run([sys.executable, 'tests/gatequeue.py', '--k2b-compare', str(T / 'rn'), str(T / 'rt')], capture_output=True, text=True, timeout=30)
    if r.returncode != 0: fail('real same-input narrow vs whole-tree compare not ok: %s' % r.stdout)
    # N10 controlled missing declaration: unisa/ removed from the declaration (and re-whitelisted), so a real
    # dependency changes while the narrow stamp stays -> the reused result is compared with a real run
    whitelist(lambda m: m['suites']['volatile-comma'].__setitem__('trees', [t for t in m['suites']['volatile-comma']['trees'] if t != 'unisa']))
    subprocess.run(['git', 'add', '-A'], check=True); subprocess.run(['git', 'commit', '-q', '-m', 'underdeclared'], check=True, env=env)
    s0, p = fp()
    if k(p) != 'narrow': fail('N10 fixture not narrow: %s' % k(p))
    p_vm = pathlib.Path('unisa/vm.py'); p_vm.write_text(p_vm.read_text() + '\nraise SystemExit(3)\n')
    s1, _ = fp()
    if s1['volatile-comma'] != s0['volatile-comma']: fail('N10 fixture: the undeclared change should be invisible to the narrow stamp')
    state = T / 'state'; state.mkdir()
    q.atomic(state / 'results.json', {'results': {'volatile-comma': {'rc': 0, 'status': 'PASS', 'stamp': s0['volatile-comma']}}})
    ua = os.environ.get('K2B_UA', '/tmp/ua_ref')
    run = subprocess.run(['bash', '-c', 'R=$PWD; . tests/lib.sh && ua_ready && python3 ./tests/volatilecomma.py'],
                         env=dict(os.environ, UA=ua), capture_output=True, timeout=50)
    actual = {'rc': run.returncode, 'status': 'PASS' if run.returncode == 0 else 'FAIL'}
    if actual['rc'] == 0: fail('N10 fixture: the broken undeclared dependency did not change the real run')
    if q.k2b_compare(state, 'volatile-comma', {'rc': 0, 'status': 'PASS'}, actual) != 'RED': fail('N10 missed run not RED')
    d = json.loads((state / 'results.json').read_text())
    if 'volatile-comma' in d['results'] or len(d.get('voided', {}).get('volatile-comma', [])) != 1: fail('N10 polluted result not voided (kept)')
    REVOKED = q.k2b_revoked(state); _, p = fp()
    if k(p) != 'revoked in run state': fail('N10 revoked suite still narrow: %s' % k(p))
    if q.k2b_compare(state, 'x', {'rc': 0, 'status': 'PASS', 'out_sha': 'o'}, {'rc': 0, 'status': 'PASS', 'out_sha': 'o'}) != 'ok': fail('equal results not ok')
    # identical results that differ only in timing/identity fields are not a missed run (keys precedence)
    if q.k2b_compare(state, 'x', {'rc': 0, 'status': 'PASS', 'stamp': 'a', 'seconds': 1.0, 'limit': 12, 'kind': 'tail', 'out_sha': 'o'}, {'rc': 0, 'status': 'PASS', 'stamp': 'b', 'seconds': 9.0, 'attempt': 2, 'limit': 46, 'kind': 'full', 'out_sha': 'o'}) != 'ok':
        fail('timing/stamp/limit/kind-only difference judged RED')
    lg1, lg2 = T / 'a.log', T / 'b.log'; lg1.write_text('volatile ok 6 reads 0.13s\n'); lg2.write_text('volatile ok 5 reads 0.13s\n')
    if q.k2b_compare(state, 'y', {'rc': 0, 'out_sha': q.k2b_output_digest(lg1)}, {'rc': 0, 'out_sha': q.k2b_output_digest(lg2)}) != 'RED': fail('same rc, different real output not RED')
    lg2.write_text('volatile ok 6 reads 9.87s\n')
    if q.k2b_output_digest(lg1) == q.k2b_output_digest(lg2): fail('output digest masked a number (must stay conservative)')
    if q.k2b_compare(state, 'z', {'rc': 0}, {'rc': 0}) != 'RED': fail('results without output digests judged ok')
    lg1.write_bytes(b'ok\xff\n'); lg2.write_bytes(b'ok\xfe\n')
    if q.k2b_output_digest(lg1) == q.k2b_output_digest(lg2): fail('non-UTF-8 output bytes collapsed in the digest')
    lg2.write_bytes(b'ok\xff\r\n')
    if q.k2b_output_digest(lg1) == q.k2b_output_digest(lg2): fail('newline form collapsed in the digest')
    # dependents (gateorder AFTER) of a missed suite are voided too; a second attempt is appended, not overwritten
    q.AFTER = {'dep-1': 'base-x', 'dep-2': ['dep-1', 'other-root']}
    q.atomic(state / 'results.json', {'results': {'base-x': {'rc': 0}, 'dep-1': {'rc': 0}, 'dep-2': {'rc': 0}, 'other': {'rc': 0}}})
    q.k2b_compare(state, 'base-x', {'rc': 0}, {'rc': 1})
    q.atomic(state / 'results.json', dict(json.loads((state / 'results.json').read_text()), results={'base-x': {'rc': 0, 'attempt': 2}}))
    q.k2b_compare(state, 'base-x', {'rc': 0}, {'rc': 1})
    d = json.loads((state / 'results.json').read_text())
    if set(d['voided']) < {'base-x', 'dep-1', 'dep-2'} or 'other' in d['voided']: fail('dependents not voided or unrelated voided: %s' % sorted(d['voided']))
    if len(d['voided']['base-x']) != 2: fail('second voided attempt overwrote the first')
    # production entry: --k2b-compare demands the same inputs on both sides, a narrow run and a whole-tree run
    whitelist(); vc = 'volatile-comma'
    def states(tree_stamp_b, scope_b='forced whole tree (comparison run)', out_b='o', narrow_ts='S', narrow_stamp='N', jobs=True, drop_tree=False, smap='ok'):
        for d in ('cn', 'ct'): shutil.rmtree(T / d, ignore_errors=True); (T / d).mkdir()
        J = {vc: ['x']} if jobs else {}
        sa = {'ok': {vc: narrow_stamp}, 'none': None, 'str': 'x', 'missing-suite': {'other': 'N'}}[smap]
        sb = {'ok': {vc: tree_stamp_b}, 'none': None, 'str': 'x', 'missing-suite': {'other': 'S'}}[smap]
        A_ = {'jobs': J, 'results': {vc: {'rc': 0, 'out_sha': 'o', 'k2b': {'scope': 'narrow', 'tree_stamp': narrow_ts, 'stamp': narrow_stamp}}}}
        B_ = {'jobs': J, 'results': {} if drop_tree else {vc: {'rc': 0, 'out_sha': out_b, 'k2b': {'scope': scope_b, 'stamp': tree_stamp_b}}}}
        if sa is not None: A_['stamp'] = sa
        if sb is not None: B_['stamp'] = sb
        q.atomic(T / 'cn/results.json', A_); q.atomic(T / 'ct/results.json', B_)
        r = subprocess.run([sys.executable, 'tests/gatequeue.py', '--k2b-compare', str(T / 'cn'), str(T / 'ct')], capture_output=True, text=True, timeout=30)
        rv = T / 'cn/k2b-revoked.json'
        return r.returncode, r.stdout, (vc in json.loads(rv.read_text())['revoked']) if rv.is_file() else False
    rc, out, revoked = states('S')
    if rc != 0 or revoked: fail('same-input equal results not ok via CLI: %s' % out)
    rc, out, revoked = states('OTHER')
    if rc == 0 or not revoked or 'MISSING' not in out: fail('different inputs not refused+revoked via CLI: %s' % out)
    rc, out, revoked = states('S', scope_b='narrow')
    if rc == 0 or not revoked: fail('a narrow "tree" side accepted via CLI: %s' % out)
    rc, out, revoked = states(None, narrow_ts=None)
    if rc == 0 or not revoked: fail('both tree stamps missing (None == None) accepted via CLI: %s' % out)
    rc, out, revoked = states('S', scope_b=None)
    if rc == 0 or not revoked: fail('tree side without a scope accepted via CLI: %s' % out)
    rc, out, revoked = states('S', out_b=None)
    if rc == 0 or not revoked: fail('missing output digest accepted via CLI: %s' % out)
    for label, kw in (('prefix-only tree scope', dict(scope_b='forced whole tree (x)')), ('empty tree scope', dict(scope_b='')), ('unnamed tree scope', dict(scope_b='not whitelisted')),
                      ('missing narrow stamp', dict(narrow_stamp=None)), ('missing commands on both sides', dict(jobs=False))):
        rc, out, revoked = states('S', **kw)
        if rc == 0 or not revoked: fail('%s accepted via CLI: %s' % (label, out))
    rc, out, revoked = states('S')
    d = json.loads((T / 'cn/results.json').read_text()); d['stamp'] = {vc: 'OTHER'}; q.atomic(T / 'cn/results.json', d)
    r = subprocess.run([sys.executable, 'tests/gatequeue.py', '--k2b-compare', str(T / 'cn'), str(T / 'ct')], capture_output=True, text=True, timeout=30)
    if r.returncode == 0: fail('result provenance disagreeing with its state stamp accepted')
    for sm in ('none', 'str', 'missing-suite'):
        rc, out, revoked = states('S', smap=sm)
        if rc == 0 or not revoked: fail('state stamp map %s accepted via CLI: %s' % (sm, out))
    rc, out, revoked = states('S', drop_tree=True)
    d = json.loads((T / 'cn/results.json').read_text())
    if rc == 0 or not revoked or vc in d['results'] or not d.get('voided', {}).get(vc): fail('deleted tree result not MISSING+revoked+voided: %s' % out)
    rc, out, revoked = states('S', out_b='p')
    if rc == 0 or not revoked or 'RED' not in out: fail('same-input different output not RED via CLI: %s' % out)
    import fcntl
    for side in ('cn', 'ct'):
      (T / side).mkdir(exist_ok=True); held = (T / side / 'lock').open('a'); fcntl.flock(held, fcntl.LOCK_EX)
      r = subprocess.run([sys.executable, 'tests/gatequeue.py', '--k2b-compare', str(T / 'cn'), str(T / 'ct')], capture_output=True, text=True, timeout=30)
      held.close()
      if r.returncode == 0 or 'locked' not in (r.stdout + r.stderr): fail('compare ran on a locked (running) %s state' % side)
    # unknown condition forms fail closed
    whitelist(); pathlib.Path('tests/k2b-whitelist.tsv').write_text(pathlib.Path('tests/k2b-whitelist.tsv').read_text().replace('UA=sha256:', 'UA=bogus:'))
    REVOKED = frozenset(); _, p = fp()
    if 'unknown form' not in k(p): fail('unknown condition form kept narrow: %s' % k(p))
    whitelist()
    # a missing toolchain member (nested digest) refuses narrowing
    real = q.subprocess.check_output
    def no_cc1(cmd, *a, **kw):
        if isinstance(cmd, list) and any(str(x).startswith('-print-prog-name=cc1') for x in cmd): return '/nonexistent/cc1\n'
        return real(cmd, *a, **kw)
    q.subprocess.check_output = no_cc1; REVOKED = frozenset()
    try: _, p = fp()
    finally: q.subprocess.check_output = real
    if k(p) != 'toolchain identity incomplete': fail('missing cc1 kept narrow: %s' % k(p))
    if 'volatile-comma' not in pathlib.Path('tests/k2b-whitelist.tsv').read_text(): fail('revocation edited the in-repo whitelist')
    print('k2b  only the named whitelist narrows (audited non-whitelisted = whole tree); unreviewed cold route (UA unset) whole tree; unrelated change reused, double stamp moves; content/mode/delete/new member/UA invalidate; declaration/guard/host/untracked/dynamic/review change -> whole tree; missed run RED, revoked in run state, result and gateorder dependents voided and kept per attempt, timing/limit/kind-only difference ok, lossless output digest (non-UTF-8 bytes, newline forms), missing toolchain member and unknown condition forms refuse narrowing, same-rc different-output RED, no-digest not ok, real fresh gatequeue narrow+whole-tree runs (DONE lines) compare ok on the same inputs; CLI compare locks both states, needs commands, a named whole-tree side, both stamps, complete state stamp maps equal to each result stamp, same inputs, output digests; a deleted result is MISSING, revoked and voided and revokes on mismatch, whitelist file untouched')
finally:
    shutil.rmtree(T, ignore_errors=True)

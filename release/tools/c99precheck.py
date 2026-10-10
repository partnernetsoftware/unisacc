#!/usr/bin/env python3
"""c99precheck.py GROUP.tsv OUT.json [--ua UA] (0.0.39 WF1).

PRECHECK, never acceptance: run a named probe group through the dynamic route -- the same-source C99
reference built by tests/build_ref.sh with the host cc -O2 -- via tests/difftest_o.sh (PROBES=...), so
the verdicts are difftest_o's own (-O0/-O1/-O2 against cc -O2; refuse = explicit `not covered:`).
- identity: commit, provenance source_digest, UA sha256, host cc version, probe and group sha256;
  a previous receipt with another identity is reported STALE and never reused;
- cold preparation (build_ref) and warm execution are timed separately;
- each probe is mapped to the formal shards that run it (difftest_o-K and com-difftest_o-K, K of 4);
- a counterexample that does not refuse, or a positive that does not agree, is a red PRECHECK.
The product (unisacc.com / APE) obligations and the full c99/queue exit are unchanged by any result."""
import argparse, glob, hashlib, json, os, pathlib, re, subprocess, sys, time

ROOT = pathlib.Path(__file__).resolve().parents[2]
SHARDS = 4


def sha(path):
    return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()


def group(path):
    rows = []
    for line in pathlib.Path(path).read_text().splitlines():
        if not line.strip() or line.startswith('#'): continue
        probe, role, expect = line.split('\t')[:3]
        assert expect in ('agree', 'refuse'), line
        rows.append({'probe': probe, 'role': role, 'expect': expect})
    return rows


def shard_files(root=ROOT):
    return [f for pat in ('tests/c/*.c', 'examples/*.c')
            for f in sorted(str(pathlib.Path(p).relative_to(root)) for p in glob.glob(str(root / pat)))
            if pathlib.Path(f).stem != 'host']


def shard_of(probe, root=ROOT, n=SHARDS):
    """difftest_o.sh's own ordering: tests/c/*.c then examples/*.c (glob order), host.c skipped, i % n."""
    files = [f for pat in ('tests/c/*.c', 'examples/*.c')
             for f in sorted(str(pathlib.Path(p).relative_to(root)) for p in glob.glob(str(root / pat)))
             if pathlib.Path(f).stem != 'host']
    if probe not in files: return None
    k = files.index(probe) % n + 1
    return ['difftest_o-%d' % k, 'com-difftest_o-%d' % k]


SUMMARY = re.compile(r'difftest_o SHARD=\S+ probes=(\d+)\s+agree (\d+)\s+wrong (\d+)\s+refuse (\d+)\s+known (\d+)\s+revived (\d+)')


def verdicts(text, rows, rc=None):
    """Per probe from difftest_o lines: REFUSE / WRONG / FAIL lines name the probe; otherwise agree.
    Fail closed: without difftest_o's summary for exactly these probes, with known/revived entries, or with
    totals that do not account for 3 -O levels per probe, every probe is 'unproven' (never agree)."""
    out = {}
    m = SUMMARY.search(text)
    n, agree, wrong, refuse, known, revived = map(int, m.groups()) if m else (None,) * 6
    complete = bool(m) and n == len(rows) and known == 0 and revived == 0 and agree + wrong + refuse == 3 * n
    # conservation: the summary's refuse/wrong counts must equal the named per-probe lines, and every named line
    # must belong to a probe of this group -- a missing or foreign diagnostic never lets a probe read as agree
    named = [l.split() for l in text.splitlines() if re.match(r'\s+(REFUSE|WRONG|FAIL) ', l)]
    names = {pathlib.Path(r['probe']).stem for r in rows}
    if complete and (sum(1 for l in named if l[0] == 'REFUSE') != refuse or sum(1 for l in named if l[0] != 'REFUSE') != wrong
                     or any(l[1].rstrip(':') not in names for l in named)): complete = False
    # difftest_o exits 0 only when everything agrees; a nonzero rc must be explained by refuse/wrong counts
    # only 0 (all agree) or 1 (refuse/wrong counted) are normal; a timeout (142), signal (<0) or other rc is unproven
    if rc is not None and complete and not ((rc == 0 and wrong == 0 and refuse == 0) or (rc == 1 and (wrong or refuse))): complete = False
    for r in rows:
        b = pathlib.Path(r['probe']).stem
        lines = [l for l in text.splitlines() if re.match(r'\s+(REFUSE|WRONG|FAIL) %s\b' % re.escape(b), l)]
        kinds = {l.split()[0] for l in lines}
        levels = sorted(l.split()[2].rstrip(':') for l in lines if l.split()[0] == 'REFUSE')
        got = ('unproven' if not complete else
               'refuse' if kinds == {'REFUSE'} and levels == ['-O0', '-O1', '-O2'] else 'agree' if not lines else 'other')
        out[r['probe']] = {'got': got, 'lines': lines, 'ok': got == r['expect']}
    return out


def entity(cmd):
    """The file a command name resolves to on PATH (a launcher script stays itself; never parsed)."""
    import shutil
    path = shutil.which(cmd)
    if not path: raise SystemExit('c99precheck: %r not found on PATH' % cmd)
    return os.path.realpath(path)


def backend(path):
    """A wrapper is not its compiler: only the registered launcher (release/c38-host-launcher.json) is mapped to
    its recorded target; any other script is reported unmapped rather than parsed."""
    try: rec = json.load(open(ROOT / 'release/c38-host-launcher.json'))
    except Exception: rec = {}
    if rec and sha(path) == rec.get('sha256'):
        return {'registered_launcher': 'release/c38-host-launcher.json', 'target': rec.get('target_realpath'),
                'target_sha256': rec.get('target_sha256'), 'target_now_sha256': sha(rec['target_realpath']) if rec.get('target_realpath') and os.path.exists(rec['target_realpath']) else None}
    with open(path, 'rb') as f: script = f.read(2) == b'#!'
    return 'UNMAPPED wrapper script' if script else 'native executable'


def identity(ua, rows, group_path):
    cc = subprocess.run(['cc', '--version'], capture_output=True, text=True).stdout.splitlines()[0]
    sys.path.insert(0, str(ROOT / 'exec/c')); import provenance
    return {'commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
            'source_digest': provenance.source_digest(), 'ua_sha256': sha(ua), 'cc': cc,
            'tools': {'oracle_cc': (os.environ.get('CC') or 'cc'), 'oracle_cc_realpath': entity((os.environ.get('CC') or 'cc')),
                      'oracle_cc_sha256': sha(entity((os.environ.get('CC') or 'cc'))),   # runtime: difftest_o's ${CC:-cc} reference
                      'oracle_cc_backend': backend(entity(os.environ.get('CC') or 'cc')),
                      'difftest_o.sh': sha(ROOT / 'tests/difftest_o.sh'), 'build_ref.sh': sha(ROOT / 'tests/build_ref.sh'),
                      'c99precheck.py': sha(__file__), 'cflags': os.environ.get('CFLAGS', '-O2')},
            'group_sha256': sha(group_path), 'probes': {r['probe']: sha(ROOT / r['probe']) for r in rows}}


def main(argv=None):
    ap = argparse.ArgumentParser(); ap.add_argument('group'); ap.add_argument('out'); ap.add_argument('--ua')
    a = ap.parse_args(argv)
    rows = group(a.group)
    ua = pathlib.Path(a.ua or '/tmp/cc39-wf1/ua'); cold = None
    sys.path.insert(0, str(ROOT / 'exec/c')); import provenance
    key = pathlib.Path(str(ua) + '.source_digest')
    flags = os.environ.get('CFLAGS', '-O2'); cc_id = subprocess.run(['cc', '--version'], capture_output=True, text=True).stdout.splitlines()[0]
    # build manifest (validity key): source, flags, the cc build_ref runs (version AND entity sha) and build_ref itself
    want = '%s cflags=%s cc=%s cc_sha=%s build_ref=%s' % (provenance.source_digest(), flags, cc_id, sha(entity('cc')), sha(ROOT / 'tests/build_ref.sh'))
    # source key: a UA is used only when it was built from this source digest (a sidecar written at build)
    side = key.read_text().split('\n') if key.exists() else []
    # sidecar: line 1 = source digest + build flags + cc identity, line 2 = UA sha256 as built
    if not ua.exists() or len(side) < 2 or side[0] != want or side[1] != sha(ua):
        ua.parent.mkdir(parents=True, exist_ok=True); t = time.monotonic()
        subprocess.run([sys.executable, str(ROOT / 'tests/bound.py'), '58', str(ROOT / 'tests/build_ref.sh'),
                        str(ua) + '.c', str(ua)], cwd=ROOT, check=True)
        cold = round(time.monotonic() - t, 2); key.write_text(want + '\n' + sha(ua) + '\n')
    ident = identity(ua, rows, a.group)
    prev = None
    try: prev = json.load(open(a.out)).get('identity')
    except Exception: pass
    t = time.monotonic()
    env = dict(os.environ, UA=str(ua), PROBES=' '.join(r['probe'] for r in rows), SHARD='1/1', WF1_PRECHECK='1')
    run = subprocess.run([sys.executable, str(ROOT / 'tests/bound.py'), '55', './tests/difftest_o.sh'],
                         cwd=ROOT, env=env, capture_output=True, text=True)
    warm = round(time.monotonic() - t, 2)
    v = verdicts(run.stdout, rows, run.returncode)
    rec = {'schema': 1, 'label': 'PRECHECK (not acceptance; product/APE and full exit obligations unchanged)',
           'route': 'tests/build_ref.sh host cc -O2 dynamic reference via tests/difftest_o.sh PROBES',
           'group': a.group, 'identity': ident,
           'stale_previous': bool(prev) and prev != ident,
           'cold_prepare_s': cold, 'warm_execute_s': warm, 'difftest_o_rc': run.returncode,
           'summary': (run.stdout.strip().splitlines() or [''])[-1],
           'probes': [{**r, **v[r['probe']], 'formal_shards': shard_of(r['probe'])} for r in rows]}
    files = shard_files()
    tracked = [f for f in subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', 'HEAD', 'tests/c', 'examples'], cwd=ROOT, text=True).split()
               if f.endswith('.c') and f.count('/') == (2 if f.startswith('tests/c/') else 1) and pathlib.Path(f).stem != 'host']
    rec['worktree_vs_head'] = {'missing': sorted(set(tracked) - set(files)), 'untracked': sorted(set(files) - set(tracked))}
    rec['formal_plan'] = {'shards': SHARDS, 'members': len(files), 'ordered_list_sha256': hashlib.sha256('\n'.join(files).encode()).hexdigest(),
                          'note': 'mapping holds only for this member list; any added/removed .c moves K. PRECHECK is not the formal shard run: com-difftest_o-K (product) obligations stay open'}
    side = key.read_text().split('\n')
    rec['ua_build_key'] = {'source_flags_cc': side[0], 'ua_sha256': side[1]}
    clean = not rec['worktree_vs_head']['missing'] and not rec['worktree_vs_head']['untracked']   # the mapping run is the committed plan
    rec['precheck'] = 'GREEN' if all(p['ok'] for p in rec['probes']) and side[0] == want and side[1] == sha(ua) and clean else 'RED'
    pathlib.Path(a.out).write_text(json.dumps(rec, indent=1) + '\n')
    print('c99precheck %s  %s  cold %s s  warm %s s%s' % (rec['precheck'], a.group, cold, warm, '  (previous receipt STALE)' if rec['stale_previous'] else ''))
    if not clean: print('  difftest_o members differ from HEAD (dirty tree): missing %s untracked %s' % (rec['worktree_vs_head']['missing'], rec['worktree_vs_head']['untracked']))
    for p in rec['probes']: print('  %-48s %-14s expect %-6s got %-6s %s' % (p['probe'], p['role'], p['expect'], p['got'], ','.join(p['formal_shards'] or ['UNMAPPED'])))
    return 0 if rec['precheck'] == 'GREEN' else 1


if __name__ == '__main__':
    sys.exit(main())

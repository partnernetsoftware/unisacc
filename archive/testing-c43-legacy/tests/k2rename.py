#!/usr/bin/env python3
"""Rename the anonymous =vN facts of one K2 stem to named facts (0.0.24 T1c).

  python3 tests/k2rename.py STEM            dry run: print the plan
  python3 tests/k2rename.py STEM --apply    rewrite, then graphhash exec/parse2

=vN is only a dict key (exec/assemble.py loads `=NAME` rows, manifests reach
them through cells()/value()/_path), so renaming definition and references
together leaves every generated graph byte-identical.  The name is the
referencing key path with '.', '@', ':' -> '_' and a leading 'out_' removed.
Conflicts, a vN under several keys, or an unreferenced vN must be resolved in
tests/k2rename-STEM.tsv (`vN<TAB>name`); otherwise the whole stem is refused.
After --apply a graphhash mismatch restores both files and exits 1.
Test infrastructure only.
"""
import collections, glob, json, re, subprocess, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TOK = re.compile(r'\bv\d+\b')


def refs(text):
    """(vN, key) for every vN token in a manifest; key is the referencing key path."""
    out = []
    for m in TOK.finditer(text):
        pre = re.split(r'[,\t{]', text[max(0, m.start() - 200):m.start()])[-1]
        j = re.fullmatch(r'\s*"(\w+)": "', pre)
        k = re.fullmatch(r'([^=\s"]+)=(?:@out:=)?', pre)
        if not (j or k):
            raise SystemExit('refuse: unknown reference context %r before %s' % (pre, m.group()))
        out.append((m.group(), (j or k).group(1)))
    return out


def name_of(key, cell=None):
    m = re.fullmatch(r'spell\.[^.]+\.(\d+)', key)
    if m and cell:  # spell family: name by the decoded spelling of byte N
        t = ''.join(chr(b) for _, b in json.loads(cell))
        return 'spell_' + (t if t.isalnum() else 'x%02x' % int(m.group(1)))
    n = re.sub(r'[.@:]', '_', key)
    return n[4:] if n.startswith('out_') else n


def plan(stem):
    facts = ROOT / 'exec/facts' / (stem + '.tsv')
    mstem = stem[3:] if stem.startswith('k2-') else stem  # k2-X facts serve exec/*/X-manifest.tsv
    mans = sorted(Path(p) for p in glob.glob(str(ROOT / 'exec/*' / (mstem + '*-manifest.tsv'))))
    assert mans, 'no manifest for ' + stem
    lines = facts.read_text().split('\n')
    defs = [ln.split('\t', 1)[0][1:] for ln in lines if re.match(r'=v\d+\t', ln)]
    cells = {ln.split('\t')[0][1:]: ln.split('\t', 2)[2] for ln in lines if re.match(r'=v\d+\tjson\t', ln)}
    taken = {ln.split('\t', 1)[0][1:] for ln in lines if ln[:1] in '=@' and not re.match(r'=v\d+\t', ln)}
    keys, count = collections.defaultdict(set), collections.Counter()
    for m in mans:
        for v, k in refs(m.read_text()):
            keys[v].add(k); count[m] += 1
    manual = {}
    mt = ROOT / 'tests' / ('k2rename-%s.tsv' % stem)
    if mt.exists():
        for ln in mt.read_text().splitlines():
            if ln and not ln.startswith('#'):
                v, n = ln.split('\t'); manual[v] = n
    stray = set(keys) - set(defs)
    if stray:
        raise SystemExit('refuse: references without definition: %s' % sorted(stray))
    cand = {v: (name_of(next(iter(keys[v])), cells.get(v)) if len(keys[v]) == 1 else None) for v in defs}
    byname = collections.Counter(n for n in cand.values() if n)
    new, bad = {}, []
    for v in defs:
        n = manual.get(v)
        if n is None:
            n = cand[v]
            if n is None or byname[n] > 1 or n in taken:
                bad.append('%s keys=%s' % (v, sorted(keys[v]) or 'unreferenced')); continue
        new[v] = n
    if bad:
        raise SystemExit('refuse %s (add to %s):\n  %s' % (stem, mt.relative_to(ROOT), '\n  '.join(bad)))
    names = list(new.values())
    for n in names:
        assert re.fullmatch(r'[A-Za-z_]\w*', n) and not n.isdigit() and not TOK.fullmatch(n), n
    assert len(set(names)) == len(names) and not set(names) & taken, 'name clash'
    return facts, mans, new, count


def rewrite(facts, mans, new, count):
    sub = lambda m: new[m.group()]
    ft = facts.read_text()
    ft2, nd = re.subn(r'(?m)^=(v\d+)(?=\t)', lambda m: '=' + new[m.group(1)], ft)
    assert nd == len(new), (nd, len(new))
    texts = {facts: (ft, ft2)}
    for m in mans:
        t = m.read_text()
        t2, n = TOK.subn(sub, t)
        assert n == count[m], (m, n, count[m])
        texts[m] = (t, t2)
    return texts


def graphhash(stage):
    n = int(subprocess.check_output([sys.executable, 'tests/graphhash.py', '--only', stage, '--list'], cwd=ROOT, text=True).count('\n'))
    for k in range(1, n + 1):
        cmd = [sys.executable, 'tests/bound.py', '58', sys.executable, 'tests/graphhash.py',
               '--only', stage, '--shard', '%d/%d' % (k, n)]
        for attempt in (1, 2):  # FAIL:142 is a timeout, not a hash verdict: rerun that entry once
            p = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
            sys.stdout.write(p.stdout + p.stderr); sys.stdout.flush()
            timeout = p.returncode == 142 or ('FAIL:142' in p.stdout and 'graphhash: 1 entries, 1 bad' in p.stdout)
            if not p.returncode or not timeout:
                break
            print('shard %d/%d timed out; rerun once' % (k, n))
        if p.returncode:
            return False
    return True


def main(argv):
    stem, apply = argv[0], '--apply' in argv
    facts, mans, new, count = plan(stem)
    texts = rewrite(facts, mans, new, count)
    print('%s: %d definitions, %d references in %s' % (stem, len(new), sum(count.values()),
          ', '.join(str(m.relative_to(ROOT)) for m in mans)))
    for v, n in new.items():
        print('  %s -> %s' % (v, n))
    if not apply:
        return 0
    for p, (_, t2) in texts.items():
        p.write_text(t2)
    stages = sorted({str(m.parent.relative_to(ROOT)) for m in mans})
    if not all(graphhash(st) for st in stages):
        for p, (t, _) in texts.items():
            p.write_text(t)
        print('graphhash mismatch: %s restored' % stem)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

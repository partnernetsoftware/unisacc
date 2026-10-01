#!/usr/bin/env python3
"""C99 clause ledger gate (archive/plans/v0.0.14.md R14-6).

tests/c99/clauses.tsv has one row per normative subclause of ISO/IEC 9899:1999
+ TC1-TC3 (WG14 N1256): clause, title, part (env/lang/lib/annex), status
(covered / partial / unsupported / n/a), the probes that exercise it, a note.
This gate keeps the ledger honest:
  * every listed probe exists and lives where a gate suite runs it;
  * no language or environment clause is `unmapped`, and the only language-
    side `unsupported` rows are the ones README's limitations table names;
  * README's generated summary (between the c99-ledger markers) equals what
    this file computes -- so "how much of C99" is a number from the ledger,
    never a sentence someone typed.
`--write` refreshes the README region.
"""
import collections, pathlib, re, sys
R = pathlib.Path(__file__).resolve().parents[1]
LEDGER = R / 'tests/c99/clauses.tsv'
STATUSES = {'covered', 'partial', 'unsupported', 'n/a'}
GATED = ('tests/c99/', 'tests/c/', 'examples/', 'tests/fb12/', 'tests/diag.sh')
# language/environment gaps README must name (the keyword that finds its row)
KNOWN_LANG_GAPS = {'5.2.1.1': 'trigraph', '6.3.1.6': '_Complex', '6.3.1.7': '_Complex', '6.10.4': '#line'}
BEGIN, END = '<!-- c99-ledger:begin -->', '<!-- c99-ledger:end -->'

def load():
    lines = LEDGER.read_text().splitlines()
    assert lines[0].split('\t') == ['clause', 'title', 'part', 'status', 'probes', 'note'], 'ledger header'
    rows = [l.split('\t') for l in lines[1:] if l.strip()]
    for r in rows: assert len(r) == 6, ('ledger row', r)
    return rows

def summary(rows):
    c = collections.Counter((r[2], r[3]) for r in rows)
    def part(p):
        cov, par, uns = c[(p, 'covered')], c[(p, 'partial')], c[(p, 'unsupported')]
        n = cov + par + uns
        return cov, par, uns, n
    out = []
    for p, name in (('lang', 'Language (clause 6)'), ('env', 'Environment (clause 5)'), ('lib', 'Library (clause 7)'), ('annex', 'Annexes F, G')):
        cov, par, uns, n = part(p)
        out.append('| %s | %d | %d | %d | %d | %d%% |' % (name, n, cov, par, uns, (100 * cov // n) if n else 0))
    return ('Clause coverage from [tests/c99/clauses.tsv](tests/c99/clauses.tsv), one row per normative subclause of\n'
            'ISO/IEC 9899:1999 + TC1-TC3 (WG14 N1256); headings and informative clauses are excluded:\n\n'
            '| Part | Subclauses | Covered | Partial | Unsupported | Covered |\n|---|---|---|---|---|---|\n' + '\n'.join(out))

def check(rows):
    bad = []
    readme = (R / 'README.md').read_text()
    seen = set()
    for clause, title, part, status, probes, note in rows:
        if clause in seen: bad.append('duplicate clause ' + clause)
        seen.add(clause)
        if status not in STATUSES: bad.append('%s: status %r' % (clause, status))
        if part in ('lang', 'env'):
            if status == 'unsupported' and clause not in KNOWN_LANG_GAPS:
                bad.append('%s: language-side unsupported without a README limitation' % clause)
            if status == 'covered' and not probes.strip():
                bad.append('%s: covered with no probe' % clause)
        for pr in probes.split():
            if not pr.startswith(GATED): bad.append('%s: probe %s is not in a gated location' % (clause, pr))
            elif not (R / pr).exists(): bad.append('%s: probe %s does not exist' % (clause, pr))
    for clause, kw in KNOWN_LANG_GAPS.items():
        row = [r for r in rows if r[0] == clause]
        if row and row[0][3] == 'unsupported':
            lim = readme.split('## Known limitations', 1)[-1]
            if kw not in lim: bad.append('%s unsupported but README limitations do not mention %r' % (clause, kw))
    m = re.search(re.escape(BEGIN) + r'\n(.*?)\n' + re.escape(END), readme, re.S)
    if not m: bad.append('README has no c99-ledger region')
    elif m.group(1) != summary(rows): bad.append('README c99-ledger region is stale (run tests/c99ledger.py --write)')
    return bad

def main():
    rows = load()
    if '--write' in sys.argv:
        p = R / 'README.md'; t = p.read_text()
        new = BEGIN + '\n' + summary(rows) + '\n' + END
        t = re.sub(re.escape(BEGIN) + r'.*?' + re.escape(END), lambda _m: new, t, flags=re.S) if BEGIN in t else t
        p.write_text(t)
    bad = check(rows)
    for b in bad: print('  FAIL ' + b)
    c = collections.Counter(r[3] for r in rows)
    print('c99 ledger  rows %d   covered %d   partial %d   unsupported %d   n/a %d   problems %d'
          % (len(rows), c['covered'], c['partial'], c['unsupported'], c['n/a'], len(bad)))
    return 1 if bad else 0

if __name__ == '__main__':
    sys.exit(main())

"""The gold .tsv input contract [J10]: a damaged table must be REJECTED,
not built into a net that looks exact.  Each case breaks prec.tsv one way."""
import sys, os, shutil, tempfile
R = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, R)
from unisa import tsvgold
src = os.path.join(R, 'weights', 'gold', 'prec.tsv')
L = open(src).read().split('\n')
body = [i for i, l in enumerate(L) if l and not l.startswith('#')][1:]   # data rows
cases = {
    'ok': L,
    'missing row': [l for i, l in enumerate(L) if i != body[0]],
    'duplicate row': L[:body[1]] + [L[body[0]]] + L[body[1] + 1:],
    'bad label': [l.replace('\t1', '\tNOPE') if i == body[0] else l for i, l in enumerate(L)],
    'extra column': [l + '\tx' if i == body[0] else l for i, l in enumerate(L)],
    'bad key': [('zz' + l) if i == body[0] else l for i, l in enumerate(L)],
}
d = tempfile.mkdtemp()
for name, lines in cases.items():
    p = os.path.join(d, 'prec.tsv'); open(p, 'w').write('\n'.join(lines))
    try:
        tsvgold.load_stage(p); r = 'accepted'
    except tsvgold.GoldDataError as e:
        r = 'rejected: ' + str(e).split(': ', 1)[1]
    print('%-14s %s' % (name, r))
shutil.rmtree(d)
badn = sum(1 for n in cases if n != 'ok')
print('contract  cases %d' % len(cases))

#!/usr/bin/env python3
"""fresh-order (0.0.24 T1d (c)): the global fresh-label order is a checked contract.

Each mode runs `exec/build/gen.py parse2|parse2/units ...` with UNISACC_FRESH_LOG set
(exec/build/procs.py) and compares sha256 + count of the (n, owner, kind, caller) rows with
tests/freshorder.tsv.  Modes 0-7 are the 8 parse2 flag combinations (warnings x errors x locations), 8 is
parse2/units.  One mode per run (each ~30 s, under tests/bound.py 58).

  python3 tests/freshordercheck.py MODE            compare
  python3 tests/freshordercheck.py MODE --write    record
  python3 tests/freshordercheck.py --negative      swap two fresh bindings in
        exec/parse2/scope-manifest.tsv, expect mode 0 to go red, restore
"""
import hashlib, itertools, os, subprocess, sys, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TSV = ROOT / 'tests' / 'freshorder.tsv'
MODES = [('parse2',) + tuple(['--warnings'] * w + ['--errors'] * e + ['--locations'] * l)
         for w, e, l in itertools.product((0, 1), repeat=3)] + [('parse2/units',)]
NEG = ROOT / 'exec/parse2/scope-manifest.tsv'
NEG_A, NEG_B = 'array_test=fresh:U:DECL:b,after_dims=fresh:U:DC:r', 'after_dims=fresh:U:DC:r,array_test=fresh:U:DECL:b'


def key(m): return ' '.join(m)


def sequence(m):
    with tempfile.TemporaryDirectory(prefix='freshorder-') as tmp:
        log, out = Path(tmp) / 'log.tsv', Path(tmp) / 'out.json'
        env = dict(os.environ, PYTHONHASHSEED='0', UNISACC_FRESH_LOG=str(log))
        p = subprocess.run([sys.executable, 'tests/bound.py', '58', sys.executable, 'exec/build/gen.py',
                            m[0], str(out), *m[1:]], cwd=ROOT, env=env,
                           stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        try:
            data = log.read_bytes()
        except FileNotFoundError:
            data = b''
        if p.returncode or not data:
            raise SystemExit('fresh-order: %s FAIL rc=%d %s' % (key(m), p.returncode, p.stderr.decode(errors='replace')[-300:]))
        return '%s\t%d' % (hashlib.sha256(data).hexdigest(), data.count(b'\n'))


def baseline():
    d = {}
    try:
        for ln in TSV.read_text().splitlines():
            if ln and not ln.startswith('#'):
                k, h, c = ln.split('\t'); d[k] = h + '\t' + c
    except FileNotFoundError:
        pass
    return d


def main(argv):
    if '--negative' in argv:
        s = NEG.read_text()
        assert s.count(NEG_A) == 2, 'negative anchor moved'   # rows entry / entry-warnings; mode 0 reads the first
        try:
            NEG.write_text(s.replace(NEG_A, NEG_B, 1))
            got = sequence(MODES[0])
        finally:
            NEG.write_text(s)
        if got == baseline().get(key(MODES[0])):
            print('fresh-order negative: swap NOT detected'); return 1
        print('fresh-order negative: swap detected (red as expected), manifest restored'); return 0
    m = MODES[int(argv[0])]
    got = sequence(m)
    old = baseline()
    if '--write' in argv:
        old[key(m)] = got
        TSV.write_text('# tests/freshordercheck.py --write: sha256 and row count of UNISACC_FRESH_LOG, PYTHONHASHSEED=0\n'
                       + ''.join('%s\t%s\n' % kv for kv in sorted(old.items())))
        print('fresh-order: %s written %s' % (key(m), got)); return 0
    if old.get(key(m)) != got:
        print('fresh-order: %s DIFF %s -> %s' % (key(m), old.get(key(m)), got)); return 1
    print('fresh-order: %s ok %s' % (key(m), got)); return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

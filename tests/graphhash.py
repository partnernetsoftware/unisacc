#!/usr/bin/env python3
"""Golden hashes of every stage generator x every mode flag it honours (K2 step 0).

Each entry runs `python3 GEN OUT ARGS...` with PYTHONHASHSEED=0 under
tests/bound.py 55 and records sha256(OUT).  Test infrastructure only.

  python3 tests/graphhash.py [--only DIR]...           compare with tests/graphhash.tsv
  python3 tests/graphhash.py --write [--only DIR]...   (re)record those entries
  python3 tests/graphhash.py --list
  python3 tests/graphhash.py --only exec/parse2 --shard 1/3
At most 3 generators run at once.  Exit 0 all equal, 1 mismatch/failure.
"""
import hashlib, itertools, os, subprocess, sys, tempfile
import os
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TSV = ROOT / 'tests' / 'graphhash.tsv'


def _entries():
    e = []
    add = lambda gen, *args: e.append((gen, tuple(args)))
    # pp: target flags (lnx/x86_64 default), --locations, --shared-predefines, --no-autoinc
    for t in ((), ('--osx', '--arm64'), ('--win',)):
        for f in ((), ('--locations',), ('--shared-predefines',), ('--locations', '--shared-predefines')):
            add('exec/build/gen.py pp', *(t + f))
    add('exec/build/gen.py pp', '--shared-predefines', '--no-autoinc')
    # lex: --typed/--positions/--locations imply each other downwards; --sourcefacts
    for f in ((), ('--typed',), ('--positions',), ('--locations',), ('--sourcefacts',)):
        add('exec/build/gen.py lex', *f)
    # parse2: warnings/errors imply locations; all 8 combinations are listed
    for w, er, l in itertools.product((0, 1), repeat=3):
        add('exec/build/gen.py parse2', *(['--warnings'] * w + ['--errors'] * er + ['--locations'] * l))
    add('exec/build/gen.py parse2/units'); add('exec/parse2/units.py', '--locations')
    add('exec/build/gen.py opt'); add('exec/build/gen.py opt', '--o2')
    add('exec/build/gen.py prune'); add('exec/build/gen.py nativeabi')
    for os_ in ((), ('--osx',), ('--win',)):
        for a in ((), ('--arm64',)):
            add('exec/build/gen.py lower', *(('--full',) + os_ + a))
    add('exec/build/gen.py lower')
    add('exec/build/gen.py lower', '--full', '--object'); add('exec/build/gen.py lower', '--full', '--object', '--arm64')
    for enc in ('exec/build/gen.py enc', 'exec/build/gen.py enc/arm'):
        for f in ((), ('--elf',), ('--macho',), ('--pe',), ('--object',)):
            add(enc, *f)
    return e


def key(gen, args):
    return gen + ' ' + ' '.join(args) if args else gen


def run(gen, args, tmp):
    out = Path(tmp) / (hashlib.sha1(key(gen, args).encode()).hexdigest() + '.json')
    env = dict(os.environ, PYTHONHASHSEED='0')
    p = subprocess.run([sys.executable, 'tests/bound.py', '55', sys.executable, *gen.split(), str(out), *args],
                       cwd=ROOT, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    if p.returncode or not out.exists():
        return 'FAIL:%d:%s' % (p.returncode, p.stderr.decode(errors='replace').strip().splitlines()[-1:] or '')
    h = hashlib.sha256(out.read_bytes()).hexdigest(); out.unlink()
    return h


def main(argv):
    write = '--write' in argv
    only = [argv[i + 1].rstrip('/') for i, a in enumerate(argv) if a == '--only']
    ents = [x for x in _entries() if not only or any(x[0].startswith(d + '/') or x[0].startswith('exec/' + d.split('exec/')[-1] + '/') or x[0].endswith(' ' + d.split('/')[-1]) for d in only)]
    if '--shard' in argv:
        shard = argv[argv.index('--shard') + 1]
        part, total = (int(x) for x in shard.split('/'))
        if total < 1 or part < 1 or part > total:
            raise ValueError('expected --shard K/N with 1 <= K <= N')
        ents = [entry for i, entry in enumerate(ents) if i % total == part - 1]
    if '--list' in argv:
        for g, a in ents: print(key(g, a))
        return 0
    old = {}
    if TSV.exists():
        for ln in TSV.read_text().splitlines():
            if ln and not ln.startswith('#'):
                k, h = ln.split('\t'); old[k] = h
    tmp = tempfile.mkdtemp(prefix='graphhash-', dir=os.environ.get('GRAPHHASH_TMP'))
    with ThreadPoolExecutor(int(os.environ.get('GRAPHHASH_JOBS', '1'))) as ex:   # heat: 1 by default (slot.sh counts one slot)
        res = dict(zip((key(*x) for x in ents), ex.map(lambda x: run(x[0], x[1], tmp), ents)))
    bad = 0
    for k, h in res.items():
        if h.startswith('FAIL'):
            bad += 1; print('FAIL\t' + k + '\t' + h)
        elif not write and old.get(k) != h:
            bad += 1; print('DIFF\t' + k + '\t' + str(old.get(k)) + ' -> ' + h)
    if write and not bad:
        old.update(res)
        TSV.write_text('# tests/graphhash.py --write: sha256 of generator output, PYTHONHASHSEED=0\n'
                       + ''.join('%s\t%s\n' % kv for kv in sorted(old.items())))
    print('graphhash: %d entries, %d bad%s' % (len(res), bad, ' (written)' if write and not bad else ''))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

#!/usr/bin/env python3
"""rowcov: which transitions of the pp delta do the probe programs actually take (0.0.25 T2, first slice).

Design: research/t2-coverage-design.md.  This is the cc half that needs no change under exec/: run
the reference simulator (exec/pp/sim.py, which already records (state, key) per step) over every
examples/*.c and tests/c/*.c, sharded so each run stays under the 60 s ceiling, and report how many
non-rejecting edges were taken.  The row -> edge provenance (UNISACC_ROW_LOG) is the exec/ half;
once it exists this report gains a per-manifest-row column and a witness per row.
  tests/rowcov.py pp K/N        run shard K of N, write $X/rowcov/pp-K.json   (exec/stamp.sh's $X)
  tests/rowcov.py pp merge N    merge the N shards and print the coverage line
Judged only from the TSV-built delta and the Python simulator: no graphhash, no reference binary.
"""
import json, os, pathlib, subprocess, sys
ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'exec/pp'))

def xdir():
    out = subprocess.run(['sh', '-c', '. exec/stamp.sh && printf %s "$X"'], cwd=ROOT, capture_output=True, text=True).stdout
    return pathlib.Path(out)

def edges(delta):
    """every (state, key) whose action is not the totalising REJECT unreachable"""
    seqs = delta['seqs']; total = set()
    for name, (mode, row) in delta['states'].items():
        if name in ('DEAD', 'HALT'): continue
        for k, (nxt, sq) in row.items():
            if any(a and a[0] == 'REJECT' and len(a) > 1 and a[1] == 'unreachable' for a in seqs[sq]): continue
            total.add((name, str(k)))
    return total

def main():
    stage, what = sys.argv[1], sys.argv[2]
    assert stage in ('pp', 'lex'), 'stages so far: pp, lex'
    X = xdir(); out = X / 'rowcov'; out.mkdir(parents=True, exist_ok=True)
    ppj = X / 'e2/d.json'
    # lex runs on the preprocessor's output, so a lex shard first runs pp; its delta is the product's e1 (--typed)
    dj = ppj if stage == 'pp' else out / 'lex.json'
    def build(log=None):
        if subprocess.run(['sh', 'exec/pp/run.sh', 'gen'], cwd=ROOT, capture_output=True).returncode:
            sys.exit('rowcov: building the pp delta failed (exec/pp/run.sh gen)')
        if stage == 'lex' or log:
            target = out / (stage + ('.sidecar.json' if log else '.json'))
            env = dict(os.environ, UNISACC_ROW_LOG=str(log)) if log else os.environ
            r = subprocess.run([sys.executable, 'exec/build/gen.py', stage, str(target)] + (['--typed'] if stage == 'lex' else []),
                               cwd=ROOT, env=env, capture_output=True)
            if r.returncode: sys.exit('rowcov: gen.py %s failed' % stage)
            return target
    if what == 'all':
        # one gate run under 60 s: side table (~3 s), 8 shards 4 at a time (~15 s each), merge
        side = out / (stage + '.rows.tsv')
        side.unlink(missing_ok=True)
        if stage == 'lex': build()
        sc = build(side)
        if sc.read_bytes() != dj.read_bytes():
            sys.exit('rowcov: the side-table build changed the delta')
        procs = []
        for k in range(1, 9):
            procs.append(subprocess.Popen([sys.executable, __file__, stage, '%d/8' % k], cwd=ROOT))
            if len(procs) == 4: [p.wait() for p in procs]; procs = []
        [p.wait() for p in procs]
        sys.argv[3:] = ['8']; what = 'merge'
    if what == 'merge':
        n = int(sys.argv[3]); seen = set()
        for k in range(1, n + 1):
            seen |= {tuple(e) for e in json.loads((out / ('%s-%d.json' % (stage, k))).read_text())['seen']}
        total = edges(json.loads(dj.read_text()))
        hit = seen & total
        print('rowcov %s  edges %d   taken %d   (%.1f%%)   probes: examples/*.c tests/c/*.c' % (stage, len(total), len(hit), 100.0 * len(hit) / len(total)))
        # Row level, once the side table exists (cdx, UNISACC_ROW_LOG: state, key, manifest_path, line;
        # a synthesised edge names its main declaring row).  A row is covered when any edge it wrote
        # was taken; its witness is the first probe that took one.
        side = out / (stage + '.rows.tsv')
        try:
            lines = side.read_text().splitlines()
        except FileNotFoundError:
            print('rowcov %s  row level: no side table yet (%s)' % (stage, side)); return 0
        wit = {}
        for k in range(1, n + 1):
            for e, f in json.loads((out / ('%s-%d.json' % (stage, k))).read_text()).get('witness', {}).items(): wit.setdefault(e, f)
        rows = {}
        for l in lines:
            st, key, path, line = l.split('\t')
            r = rows.setdefault((path, int(line)), [0, None])
            if (st, key) in total and r[1] is None and (st, key) in hit: r[1] = wit.get('%s\t%s' % (st, key))
            r[0] += 1
        covered = sum(1 for r in rows.values() if r[1])
        (out / (stage + '.cov.tsv')).write_text(''.join('%s\t%d\t%d\t%s\n' % (p, ln, r[0], r[1] or '-') for (p, ln), r in sorted(rows.items())))
        print('rowcov %s  rows %d   covered %d   (%.1f%%)   report %s' % (stage, len(rows), covered, 100.0 * covered / max(1, len(rows)), out / (stage + '.cov.tsv')))
        # ratchet: covered rows may only rise (tests/rowcov.baseline); raise the number when they do
        base = dict(l.split()[:2] for l in (ROOT / 'tests/rowcov.baseline').read_text().splitlines() if l and not l.startswith('#'))
        floor = int(base.get(stage, 0))
        if covered < floor:
            print('rowcov %s  FELL below the baseline %d: a probe stopped reaching rows it used to' % (stage, floor)); return 1
        if covered > floor: print('rowcov %s  above the baseline %d: raise it in tests/rowcov.baseline' % (stage, floor))
        return 0
    k, n = map(int, what.split('/'))
    if not dj.exists() or stage == 'pp': build()
    import sim
    ppdelta = json.loads(ppj.read_text()); pploaded = sim.load(ppdelta)
    if stage == 'lex':
        import importlib.util
        spec = importlib.util.spec_from_file_location('lexsim', ROOT / 'exec/lex/sim.py'); lexsim = importlib.util.module_from_spec(spec); spec.loader.exec_module(lexsim)
        delta = json.loads(dj.read_text())
        # the typed lexer runs on the shared core simulator, whose cov keys are state indices in this order
        names = list(delta['states']) + (['HALT'] if 'HALT' not in delta['states'] else [])
    else:
        delta, loaded, names = ppdelta, pploaded, pploaded[0]
    files = sorted(str(p.relative_to(ROOT)) for p in list((ROOT / 'examples').glob('*.c')) + list((ROOT / 'tests/c').glob('*.c')))
    mine = files[k - 1::n]; cov = set(); ran = 0; first = {}
    os.chdir(ROOT)
    for f in mine:
        c = set()
        try:
            if stage == 'pp':
                sim.run(delta, open(f, 'rb').read(), f, sim.Files(), cov=c, maxsteps=20_000_000, loaded=loaded)
            else:
                res, val, _ = sim.run(ppdelta, open(f, 'rb').read(), f, sim.Files(), maxsteps=20_000_000, loaded=pploaded)
                if res == 'accept': lexsim.run(delta, val, cov=c, maxsteps=20_000_000)
        except Exception:
            pass                                   # a rejected or failing input still took the edges it took
        for e in c - cov: first[e] = f        # the first probe that took an edge is its witness
        cov |= c; ran += 1
    name = lambda q: names[q] if isinstance(q, int) else q
    seen = sorted({(name(q), str(key)) for q, key in cov})
    wit = {'%s\t%s' % (name(q), key): f for (q, key), f in first.items()}
    (out / ('%s-%d.json' % (stage, k))).write_text(json.dumps({'shard': what, 'files': ran, 'seen': seen, 'witness': wit}))
    print('rowcov %s %s  files %d   edges taken %d' % (stage, what, ran, len(seen)))
    return 0

if __name__ == '__main__':
    sys.exit(main())

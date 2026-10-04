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
    assert stage == 'pp', 'only the pp slice exists so far'
    X = xdir(); out = X / 'rowcov'; out.mkdir(parents=True, exist_ok=True)
    dj = X / 'e2/d.json'
    if what == 'merge':
        n = int(sys.argv[3]); seen = set()
        for k in range(1, n + 1):
            seen |= {tuple(e) for e in json.loads((out / ('pp-%d.json' % k)).read_text())['seen']}
        total = edges(json.loads(dj.read_text()))
        hit = seen & total
        print('rowcov pp  edges %d   taken %d   (%.1f%%)   probes: examples/*.c tests/c/*.c' % (len(total), len(hit), 100.0 * len(hit) / len(total)))
        # Row level, once the side table exists (cdx, UNISACC_ROW_LOG: state, key, manifest_path, line;
        # a synthesised edge names its main declaring row).  A row is covered when any edge it wrote
        # was taken; its witness is the first probe that took one.
        side = out / 'pp.rows.tsv'
        try:
            lines = side.read_text().splitlines()
        except FileNotFoundError:
            print('rowcov pp  row level: no side table yet (%s)' % side); return 0
        wit = {}
        for k in range(1, n + 1):
            for e, f in json.loads((out / ('pp-%d.json' % k)).read_text()).get('witness', {}).items(): wit.setdefault(e, f)
        rows = {}
        for l in lines:
            st, key, path, line = l.split('\t')
            r = rows.setdefault((path, int(line)), [0, None])
            if (st, key) in total and r[1] is None and (st, key) in hit: r[1] = wit.get('%s\t%s' % (st, key))
            r[0] += 1
        covered = sum(1 for r in rows.values() if r[1])
        (out / 'pp.cov.tsv').write_text(''.join('%s\t%d\t%d\t%s\n' % (p, ln, r[0], r[1] or '-') for (p, ln), r in sorted(rows.items())))
        print('rowcov pp  rows %d   covered %d   (%.1f%%)   report %s' % (len(rows), covered, 100.0 * covered / max(1, len(rows)), out / 'pp.cov.tsv'))
        return 0
    k, n = map(int, what.split('/'))
    if subprocess.run(['sh', 'exec/pp/run.sh', 'gen'], cwd=ROOT, capture_output=True).returncode:
        sys.exit('rowcov: building the pp delta failed (exec/pp/run.sh gen)')
    import sim
    delta = json.loads(dj.read_text()); loaded = sim.load(delta); names = loaded[0]
    files = sorted(str(p.relative_to(ROOT)) for p in list((ROOT / 'examples').glob('*.c')) + list((ROOT / 'tests/c').glob('*.c')))
    mine = files[k - 1::n]; cov = set(); ran = 0; first = {}
    os.chdir(ROOT)
    for f in mine:
        c = set()
        try:
            sim.run(delta, open(f, 'rb').read(), f, sim.Files(), cov=c, maxsteps=20_000_000, loaded=loaded)
        except Exception:
            pass                                   # a rejected or failing input still took the edges it took
        for e in c - cov: first[e] = f        # the first probe that took an edge is its witness
        cov |= c; ran += 1
    name = lambda q: names[q] if isinstance(q, int) else q
    seen = sorted({(name(q), str(key)) for q, key in cov})
    wit = {'%s\t%s' % (name(q), key): f for (q, key), f in first.items()}
    (out / ('pp-%d.json' % k)).write_text(json.dumps({'shard': what, 'files': ran, 'seen': seen, 'witness': wit}))
    print('rowcov pp %s  files %d   edges taken %d' % (what, ran, len(seen)))
    return 0

if __name__ == '__main__':
    sys.exit(main())

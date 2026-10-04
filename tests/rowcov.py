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
        return 0
    k, n = map(int, what.split('/'))
    if subprocess.run(['sh', 'exec/pp/run.sh', 'gen'], cwd=ROOT, capture_output=True).returncode:
        sys.exit('rowcov: building the pp delta failed (exec/pp/run.sh gen)')
    import sim
    delta = json.loads(dj.read_text()); loaded = sim.load(delta); names = loaded[0]
    files = sorted(str(p.relative_to(ROOT)) for p in list((ROOT / 'examples').glob('*.c')) + list((ROOT / 'tests/c').glob('*.c')))
    mine = files[k - 1::n]; cov = set(); ran = 0
    os.chdir(ROOT)
    for f in mine:
        c = set()
        try:
            sim.run(delta, open(f, 'rb').read(), f, sim.Files(), cov=c, maxsteps=20_000_000, loaded=loaded)
        except Exception:
            pass                                   # a rejected or failing input still took the edges it took
        cov |= c; ran += 1
    seen = sorted({(names[q] if isinstance(q, int) else q, str(key)) for q, key in cov})
    (out / ('pp-%d.json' % k)).write_text(json.dumps({'shard': what, 'files': ran, 'seen': seen}))
    print('rowcov pp %s  files %d   edges taken %d' % (what, ran, len(seen)))
    return 0

if __name__ == '__main__':
    sys.exit(main())

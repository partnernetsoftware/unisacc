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
import hashlib, json, os, pathlib, subprocess, sys
ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'exec/pp'))

def shard_baseline(stage):
    """STAGE.shards lines `STAGE/K/N ROWS [PROBES]`: PROBES (sha256[:12] of the shard's probe names) is optional."""
    try: text = (ROOT / 'tests' / 'rowcov' / (stage + '.shards')).read_text()
    except FileNotFoundError: return {}
    out = {}
    for l in text.splitlines():
        if not l or l.startswith('#'): continue
        w = l.split(); out[w[0]] = (int(w[1]), w[2] if len(w) > 2 else None)
    return out

def baseline(stage, kind):
    """The ratchet floors: tests/rowcov/STAGE.shards (keys STAGE/K/N) and STAGE.union (key STAGE).
    0.0.30 P7': one file per stage and kind, so raising one floor re-runs only the jobs that read it
    (the single tests/rowcov.baseline re-ran all 37 rowcov jobs on any change).  Rows may only rise."""
    try: text = (ROOT / 'tests' / 'rowcov' / ('%s.%s' % (stage, kind))).read_text()
    except FileNotFoundError: return {}
    return dict(l.split()[:2] for l in text.splitlines() if l and not l.startswith('#'))


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

def shard(files, k, n, stage):
    """Files of shard K of N.  Round robin, except parse2: a probe that includes <math.h> costs ~30 s there
    (0.0.32 q11: the two shards holding one each took 45 and 37 s, the rest ~14 s, and one hit the 53 s
    watchdog under a 4-job queue), so each heavy probe gets a shard of its own and the rest skip those shards."""
    if stage != 'parse2': return files[k - 1::n]
    heavy = [f for f in files if '<math.h>' in (ROOT / f).read_text(errors='replace')]
    if k <= len(heavy): return [heavy[k - 1]]
    light = [f for f in files if f not in heavy]; m = n - len(heavy)
    return light[k - len(heavy) - 1::m]

def probe_print(files):
    """0.0.28 E22: what a shard's result depends on besides the delta -- its probe files and the reference."""
    import hashlib
    h = hashlib.sha256()
    for f in files:
        h.update(f.encode() + b'\0')
        try: h.update(hashlib.sha256((ROOT / f).read_bytes()).digest())
        except FileNotFoundError: h.update(b'-')
    ua = os.environ.get('UA', '/tmp/ua_ref')
    try: h.update(hashlib.sha256(open(ua, 'rb').read()).digest())
    except OSError: h.update(b'no-ua')
    return h.hexdigest()


def main():
    stage, what = sys.argv[1], sys.argv[2]
    assert stage in ('pp', 'pp-locations', 'pp-shared', 'lex', 'parse2', 'lower', 'enc'), 'stages: pp, pp-locations, pp-shared, lex, parse2, lower, enc'
    X = xdir(); out = X / 'rowcov'; out.mkdir(parents=True, exist_ok=True)
    ppj = X / 'e2/d.json'
    # lex runs on the preprocessor's output, so a lex shard first runs pp; its delta is the product's e1 (--typed)
    dj = ppj if stage == 'pp' else out / (stage + '.json')
    lexj = out / 'lex.json'
    lowerj = out / 'lower.json'
    # 0.0.32 T3': the pp variant tables (location, shared-predefine) exist only in a delta built with
    # their flag, so the plain pp build reported none of their rows; each variant gets its own graph
    FLAGS = {'lex': ['--typed'], 'lower': ['--full'], 'enc': ['--elf'], 'pp-locations': ['--locations'], 'pp-shared': ['--shared-predefines']}
    gstage = 'pp' if stage.startswith('pp-') else stage
    def build(log=None):
        if subprocess.run(['sh', 'exec/pp/run.sh', 'gen'], cwd=ROOT, capture_output=True).returncode:
            sys.exit('rowcov: building the pp delta failed (exec/pp/run.sh gen)')
        if stage == 'parse2' and not log:     # parse2 reads the typed lexer's tokens
            r = subprocess.run([sys.executable, 'exec/build/gen.py', 'lex', str(lexj), '--typed'], cwd=ROOT, capture_output=True)
            if r.returncode: sys.exit('rowcov: gen.py lex failed')
        if stage == 'enc' and not log:        # enc reads lower's full TIns text
            r = subprocess.run([sys.executable, 'exec/build/gen.py', 'lower', str(lowerj), '--full'], cwd=ROOT, capture_output=True)
            if r.returncode: sys.exit('rowcov: gen.py lower failed')
        if stage != 'pp' or log:
            target = out / (stage + ('.sidecar.json' if log else '.json'))
            env = dict(os.environ, UNISACC_ROW_LOG=str(log)) if log else os.environ
            r = subprocess.run([sys.executable, 'exec/build/gen.py', gstage, str(target)] + FLAGS.get(stage, []),
                               cwd=ROOT, env=env, capture_output=True)
            if r.returncode: sys.exit('rowcov: gen.py %s failed' % stage)
            return target
    def cached():
        """delta + side table for this stage, rebuilt only when exec/ changes; atomic, so parallel shard jobs can share it"""
        key = subprocess.run(['sh', '-c', 'git ls-files -s exec weights | git hash-object --stdin; git diff -- exec weights | git hash-object --stdin; git ls-files -o --exclude-standard -z -- exec weights | xargs -0 git hash-object --'],
                             cwd=ROOT, capture_output=True, text=True).stdout.replace('\n', '')
        stamp = out / (stage + '.key')
        try:
            if stamp.read_text() == key and dj.exists() and (out / (stage + '.rows.tsv')).exists(): return
        except FileNotFoundError:
            pass
        tmp = out / ('%s.%d.rows.tsv' % (stage, os.getpid()))
        if stage != 'pp': build()
        sc = build(tmp)
        if sc.read_bytes() != dj.read_bytes(): sys.exit('rowcov: the side-table build changed the delta')
        os.replace(tmp, out / (stage + '.rows.tsv')); stamp.write_text(key)
    if what == 'build':
        # 0.0.31 R4: one job builds the shared delta + side table; the shards start after it
        # (0.0.30: shards 1/2 both built it concurrently, 51/53 s each, the rest ~11 s)
        cached()
        print('rowcov %s build: cached delta and side table ready' % stage)
        return 0
    if what.startswith('gate'):
        # gate shape for the slow stages: `STAGE gate K/N` = cached build + one shard + that shard's own
        # row count against tests/rowcov/STAGE.{shards,union} (key STAGE/K/N); no cross-job merge is needed
        k, n = map(int, what[4:].lstrip(':').split('/'))
        cached()
        r = subprocess.run([sys.executable, __file__, stage, '%d/%d' % (k, n)], cwd=ROOT)
        if r.returncode: return r.returncode
        seen = {tuple(e) for e in json.loads((out / ('%s-%d.json' % (stage, k))).read_text())['seen']}
        total = edges(json.loads(dj.read_text()))
        rowsof = {}
        for l in (out / (stage + '.rows.tsv')).read_text().splitlines():
            st, key, path, line = l.split('\t')
            if path.startswith('synthetic'): continue
            if (st, key) in seen and (st, key) in total: rowsof[(path, line)] = 1
        got = len(rowsof)
        key = '%s/%d/%d' % (stage, k, n)
        base = shard_baseline(stage)
        floor, names = base.get(key, (0, None))
        files = sorted(str(p.relative_to(ROOT)) for p in list((ROOT / 'examples').glob('*.c')) + list((ROOT / 'tests/c').glob('*.c')))
        mine = hashlib.sha256('\n'.join(shard(files, k, n, stage)).encode()).hexdigest()[:12]
        print('rowcov %s shard %d/%d  rows reached %d (baseline %d)' % (stage, k, n, got, floor))
        # 0.0.33: a shard floor holds only for the probe names it was measured on.  Adding a probe moves
        # files between round-robin shards (5b1961a2: lower 2/3 and enc 2/4/7/8 fell, unions did not);
        # then the union job is the ratchet and this shard prints its new line instead of failing.
        if names is not None and names != mine:
            print('rowcov  probe set of this shard changed since its baseline: the union decides; new line: %s %d %s' % (key, got, mine))
            return 0
        if got < floor: print('rowcov  FELL below the baseline'); return 1
        if got > floor or names is None: print('rowcov  above the baseline: set "%s %d %s" in tests/rowcov/%s.shards' % (key, got, mine, stage))
        return 0
    if what.startswith('union'):
        # 0.0.28 E22: the union ratchet (key STAGE in tests/rowcov/STAGE.{shards,union}) after the gate shards: their
        # results are reused when probes and reference are unchanged, a stale or missing shard is rerun
        n = int(what[5:] or 8)
        cached()
        files = sorted(str(p.relative_to(ROOT)) for p in list((ROOT / 'examples').glob('*.c')) + list((ROOT / 'tests/c').glob('*.c')))
        import time
        def fresh(k):
            try: return json.loads((out / ('%s-%d.json' % (stage, k))).read_text()).get('probes') == probe_print(shard(files, k, n, stage))
            except (FileNotFoundError, ValueError): return False
        deadline = time.time() + 40           # in a queue the shard jobs may still be finishing: wait for them first
        while time.time() < deadline and not all(fresh(k) for k in range(1, n + 1)): time.sleep(2)
        for k in range(1, n + 1):
            if not fresh(k):
                r = subprocess.run([sys.executable, __file__, stage, '%d/%d' % (k, n)], cwd=ROOT)
                if r.returncode: return r.returncode
        sys.argv[3:] = [str(n)]; what = 'merge'
    if what == 'all':
        # one gate run under 60 s: side table (~3 s), 8 shards 4 at a time (~15 s each), merge
        side = out / (stage + '.rows.tsv')
        side.unlink(missing_ok=True)
        if stage != 'pp': build()
        sc = build(side)
        if sc.read_bytes() != dj.read_bytes():
            sys.exit('rowcov: the side-table build changed the delta')
        procs = []
        for k in range(1, 9):
            procs.append(subprocess.Popen([sys.executable, __file__, stage, '%d/8' % k], cwd=ROOT))
            if len(procs) == (8 if stage == 'parse2' else 4): [p.wait() for p in procs]; procs = []   # parse2 shards take ~33 s
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
            if path.startswith('synthetic'): continue     # template-edit edges with no declaring row
            r = rows.setdefault((path, int(line)), [0, None, 0])
            if (st, key) in total and r[1] is None and (st, key) in hit: r[1] = wit.get('%s\t%s' % (st, key))
            r[0] += 1
            if (st, key) in total: r[2] += 1          # edges that survive in the built delta
        covered = sum(1 for r in rows.values() if r[1])
        logged = {tuple(l.split('\t')[:2]) for l in lines}
        orphan = len(total - logged)                # surviving edges the side table does not name (graph edits)
        print('rowcov %s  surviving edges without a source row %d (0 = side table complete)' % (stage, orphan))
        # uncovered rows split two ways: dead (none of its edges survives in the delta -- unreachable
        # state or totalised away: a candidate for deletion) versus live (reachable, no probe takes it)
        dead = sum(1 for r in rows.values() if not r[1] and r[2] == 0)
        print('rowcov %s  uncovered %d = dead %d (no edge left in the delta) + live %d (needs a probe)' % (stage, len(rows) - covered, dead, len(rows) - covered - dead))
        # 0.0.32 T3': dead/live per table, so a variant table's rows are visible on their own line
        per = {}
        for (p, ln), r in rows.items():
            t = per.setdefault(pathlib.Path(p).name, [0, 0, 0])
            t[0 if r[1] else (1 if r[2] == 0 else 2)] += 1
        for t, (c, d, l) in sorted(per.items()):
            if d or l: print('rowcov %s  table %-34s covered %4d  dead %3d  live %3d' % (stage, t, c, d, l))
        (out / (stage + '.cov.tsv')).write_text(''.join('%s\t%d\t%d\t%s\n' % (p, ln, r[0], r[1] or ('dead' if r[2] == 0 else '-')) for (p, ln), r in sorted(rows.items())))
        print('rowcov %s  rows %d   covered %d   (%.1f%%)   report %s' % (stage, len(rows), covered, 100.0 * covered / max(1, len(rows)), out / (stage + '.cov.tsv')))
        # ratchet: covered rows may only rise (tests/rowcov/STAGE.{shards,union}); raise the number when they do
        base = baseline(stage, 'union')
        floor = int(base.get(stage, 0))
        if covered < floor:
            print('rowcov %s  FELL below the baseline %d: a probe stopped reaching rows it used to' % (stage, floor)); return 1
        if covered > floor: print('rowcov %s  above the baseline %d: raise it in tests/rowcov/%s.union' % (stage, floor, stage))
        return 0
    k, n = map(int, what.split('/'))
    if not dj.exists() or stage.startswith('pp'): build()
    import sim
    ppdelta = json.loads(ppj.read_text()); pploaded = sim.load(ppdelta)
    if stage in ('lex', 'parse2'):
        import importlib.util
        spec = importlib.util.spec_from_file_location('lexsim', ROOT / 'exec/lex/sim.py'); lexsim = importlib.util.module_from_spec(spec); spec.loader.exec_module(lexsim)
        delta = json.loads(dj.read_text())
        lexdelta = json.loads(lexj.read_text()) if stage == 'parse2' else delta
        if stage == 'parse2': loaded = sim.load(delta)
        # the typed lexer runs on the shared core simulator, whose cov keys are state indices in this order
        names = list(delta['states']) + (['HALT'] if 'HALT' not in delta['states'] else [])
    elif stage in ('lower', 'enc'):
        # the tape comes from the reference (-S), the inputs these stages see in the product chain
        delta = json.loads(dj.read_text()); loaded = sim.load(delta); names = loaded[0]
        lowerdelta = json.loads(lowerj.read_text()) if stage == 'enc' else delta
        lowerloaded = sim.load(lowerdelta) if stage == 'enc' else loaded
        ua = os.environ.get('UA', '/tmp/ua_ref')
        subprocess.run(['sh', '-c', 'UA=%s; . ./tests/lib.sh; ua_ready' % ua], cwd=ROOT, capture_output=True)
    else:
        delta, loaded = (ppdelta, pploaded) if stage == 'pp' else (json.loads(dj.read_text()), None)
        if loaded is None: loaded = sim.load(delta)
        names = loaded[0]
    def ppfiles():
        # pp-shared reads its predefines as resources (exec/pp/sharedcheck.py): give it them, target lnx/x86_64
        fs = sim.Files()
        if stage == 'pp-shared':
            sys.path.insert(0, str(ROOT / 'exec')); import assemble
            for key, v in assemble.load_facts('pp-gen')['predefres'].items(): fs.cache[b'\0predefines/' + key.encode()] = v.encode()
            fs.cache[b'\0cli/target'] = b'lnx/x86_64'
        return fs
    files = sorted(str(p.relative_to(ROOT)) for p in list((ROOT / 'examples').glob('*.c')) + list((ROOT / 'tests/c').glob('*.c')))
    mine = shard(files, k, n, stage); cov = set(); ran = 0; first = {}
    os.chdir(ROOT)
    for f in mine:
        c = set()
        try:
            if stage in ('lower', 'enc'):
                tape = subprocess.run([str(ROOT / 'tests/bound'), '20', ua, '-S', f, '-o', '/dev/stdout'], cwd=ROOT, capture_output=True).stdout
                if tape:
                    if stage == 'lower':
                        sim.run(delta, tape, f, sim.Files(), cov=c, maxsteps=200_000_000, loaded=loaded)
                    else:
                        res, tins, _ = sim.run(lowerdelta, tape, f, sim.Files(), maxsteps=200_000_000, loaded=lowerloaded)
                        if res == 'accept': sim.run(delta, tins, f, sim.Files(), cov=c, maxsteps=200_000_000, loaded=loaded)
            elif stage.startswith('pp'):
                sim.run(delta, open(f, 'rb').read(), f, ppfiles(), cov=c, maxsteps=20_000_000, loaded=loaded)
            else:
                res, val, _ = sim.run(ppdelta, open(f, 'rb').read(), f, sim.Files(), maxsteps=20_000_000, loaded=pploaded)
                if res == 'accept' and stage == 'lex': lexsim.run(delta, val, cov=c, maxsteps=20_000_000)
                elif res == 'accept':
                    res, tok, _ = lexsim.run(lexdelta, val, maxsteps=20_000_000)
                    if res == 'accept': sim.run(delta, tok, f, sim.Files(), cov=c, maxsteps=200_000_000, loaded=loaded)
        except Exception:
            pass                                   # a rejected or failing input still took the edges it took
        for e in c - cov: first[e] = f        # the first probe that took an edge is its witness
        cov |= c; ran += 1
    name = lambda q: names[q] if isinstance(q, int) else q
    seen = sorted({(name(q), str(key)) for q, key in cov})
    wit = {'%s\t%s' % (name(q), key): f for (q, key), f in first.items()}
    (out / ('%s-%d.json' % (stage, k))).write_text(json.dumps({'shard': what, 'files': ran, 'seen': seen, 'witness': wit,
                                                               'probes': probe_print(mine)}))
    print('rowcov %s %s  files %d   edges taken %d' % (stage, what, ran, len(seen)))
    return 0

if __name__ == '__main__':
    sys.exit(main())

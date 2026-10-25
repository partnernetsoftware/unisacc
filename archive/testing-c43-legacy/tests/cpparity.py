#!/usr/bin/env python3
"""C/P constructor parity over one case table (tests/cpparity/cases.tsv).

usage: cpparity.py [--family copy,mode,letexit] [CASES_TSV] [REPO_ROOT]
Families:
  copy, mode  probe templates (rows are the full template text), built by P (exec/build/gen.py) and C (seed-gen,
              cwd = a temporary tree); JSON compared exactly; executor output compared (starts x inputs per row).
  letexit     the seed-let-exit fixture manifest with one let value replaced; for each value a hit run (--stop) and a
              miss run; rc/stdout/stderr/artifact compared between C and P as seedletexitcheck did.
Self-contained: copies exec/ and seed/ and the fixture into a temporary tree, compiles seed/gen.c and exec/c/run.c with cc.
Printed lines keep the copycontractcheck format for family copy/mode.  First red stops the run.  No SKIP channel.
"""
import argparse, json, pathlib, shutil, subprocess, sys, tempfile

HERE = pathlib.Path(__file__).resolve().parent
HEAD = "#! base build/parsebase.py\n#! start {start}\ntemplate\tcopyprobe\tmain\t-\t-\t-\t-\t-\t-\n"
FAMILIES = ('copy', 'mode', 'letexit')


def dec(s):
    out = []; i = 0
    while i < len(s):
        c = s[i]
        if c == '\\' and i + 1 < len(s):
            out.append({'t': '\t', 'n': '\n', '\\': '\\'}[s[i + 1]]); i += 2
        else:
            out.append(c); i += 1
    return ''.join(out)


def load_cases(path):
    lines = path.read_text().splitlines()
    hdr = lines[0].split('\t')
    assert hdr == ['case_id', 'family', 'stage', 'rows', 'starts', 'inputs', 'expect', 'note'], hdr
    cases = [dict(zip(hdr, l.split('\t'))) for l in lines[1:] if l.strip()]
    ids = [c['case_id'] for c in cases]
    assert len(ids) == len(set(ids)), 'duplicate case_id in the table'
    return cases


def run(argv, cwd, timeout=55, **kw):
    return subprocess.run(argv, cwd=cwd, capture_output=True, timeout=timeout, **kw)


def build_tree(root, tmp):
    tree = tmp / 'tree'
    shutil.copytree(root / 'exec', tree / 'exec', ignore=shutil.ignore_patterns('__pycache__'))
    shutil.copytree(root / 'tests/fixtures/seed-let-exit', tree / 'exec/letexit')
    gen = tmp / 'seed-gen'; ex = tmp / 'run'
    q = run(['cc', '-std=c99', '-O2', '-w', '-I', str(root / 'seed'), str(root / 'seed/gen.c'), '-o', str(gen)], tmp)
    if q.returncode: raise SystemExit('cpparity FAIL: build seed/gen.c: ' + q.stderr.decode()[:300])
    q = run(['cc', '-O2', '-w', str(root / 'exec/c/run.c'), '-o', str(ex)], tmp)   # default dialect: run.c calls setenv (POSIX)
    if q.returncode: raise SystemExit('cpparity FAIL: build exec/c/run.c: ' + q.stderr.decode()[:300])
    return tree, gen, ex


def copy_mode_case(tree, gen, ex, work, case):
    name = case['case_id']; stage = 'cc-' + name
    starts = case['starts'].split(',')
    inputs = case['inputs'].split('|')          # '' is the empty input; '|A' means ['', 'A']
    for start in starts:
        d = tree / 'exec' / stage; d.mkdir(parents=True, exist_ok=True)
        (d / 'copyprobe-template.tsv').write_text(dec(case['rows']))
        (d / 'gen-manifest.tsv').write_text(HEAD.format(start=start))
        for inp in inputs:
            tag = f"{name}-{start.split('.')[1].lower()}-{inp.encode().hex() or 'e'}"
            p_json = work / f'{tag}.p.json'; c_json = work / f'{tag}.c.json'
            q = run(['python3', 'exec/build/gen.py', stage, str(p_json)], tree); assert q.returncode == 0, ('P build', tag, q.stderr[-200:])
            q = run([str(gen), stage, str(c_json)], tree); assert q.returncode == 0, ('C build', tag, q.stderr[-200:])
            outs = []
            for jp in (p_json, c_json):
                tbl = jp.with_suffix('.tbl'); q = run(['python3', 'exec/c/tbl.py', str(jp), str(tbl)], tree)
                assert q.returncode == 0, ('tbl', tag)
                inf = jp.with_suffix('.in'); inf.write_bytes(inp.encode())
                r = run([str(ex), str(tbl), str(inf)], tree)
                outs.append((r.returncode, r.stdout.decode('latin-1')))
            same_json = p_json.read_bytes() == c_json.read_bytes()
            (c_rc, c_out), (p_rc, p_out) = outs[1], outs[0]
            ok = p_rc == c_rc and p_out == c_out and same_json
            print(f'{tag}: C={c_out!r} rc{c_rc} P={p_out!r} rc{p_rc} json_exact={same_json} {"PASS" if ok else "FAIL"}')
            if not ok:
                print('first red:', tag); return False
    return True


def letexit_case(root, tree, gen, work, case):
    values = json.loads(dec(case['rows']))   # the JSON text is the let row's value: {"exit": ...}
    original = (root / 'tests/fixtures/seed-let-exit/gen-manifest.tsv').read_text()   # fresh each value, never the rewritten copy
    lines = original.splitlines(); row = lines[4].split('\t')
    assert len(row) == 9 and row[0] == 'let', row[:2]
    row[8] = json.dumps(values); lines[4] = '\t'.join(row)
    (tree / 'exec/letexit/gen-manifest.tsv').write_text('\n'.join(lines) + '\n')
    for hit in (False, True):
        result = []; data = []
        for cmd in ([str(gen)], ['python3', str(tree / 'exec/build/gen.py')]):
            out = work / f'{len(result)}.json'; out.write_bytes(b'SENTINEL')
            q = run([*cmd, 'letexit', str(out), *(['--stop'] if hit else [])], tree)
            result.append(q); data.append(out.read_bytes())
        a, b = result
        if not (a.returncode == b.returncode and a.stdout == b.stdout and data[0] == data[1]): return False, ('rc/stdout/artifact', values, hit)
        if hit and not (a.stderr == b.stderr and data[0] == b'SENTINEL'): return False, ('hit diagnostic', values)
        if not hit and not (a.returncode == 0 and data[0] != b'SENTINEL'): return False, ('miss', values)
    return True, None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--family', default=','.join(FAMILIES), help='comma list of copy,mode,letexit')
    ap.add_argument('cases', nargs='?', default=str(HERE / 'cpparity' / 'cases.tsv'))
    ap.add_argument('root', nargs='?', default=str(HERE.parent))
    args = ap.parse_args()
    want = set(args.family.split(','))
    assert want <= set(FAMILIES), want
    root = pathlib.Path(args.root).resolve()
    cases = [c for c in load_cases(pathlib.Path(args.cases).resolve()) if c['family'] in want]
    with tempfile.TemporaryDirectory(prefix='cpparity-') as tmp:
        tmp = pathlib.Path(tmp); tree, gen, ex = build_tree(root, tmp)
        work = tmp / 'work'; work.mkdir()
        n_copy = n_let = 0
        for case in cases:
            if case['family'] in ('copy', 'mode'):
                if not copy_mode_case(tree, gen, ex, work, case): raise SystemExit(1)
                n_copy += 1
            else:
                ok, why = letexit_case(root, tree, gen, work, case)
                if not ok:
                    print('first red:', case['case_id'], why); raise SystemExit(1)
                n_let += 1
        print(f'cpparity PASS: {n_copy} copy/mode cases (C = P), {n_let} let-exit values (hit/miss exact)')


if __name__ == '__main__':
    main()

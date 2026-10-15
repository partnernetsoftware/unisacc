#!/usr/bin/env python3
"""copy contract: six C/P differential cases (independent clone).  Outside-repo draft; not gated yet.

usage: copycontractcheck.py
  Self-contained: copies exec/ into a temporary tree, compiles seed/gen.c (C constructor) and exec/c/run.c
  (executor) with cc, runs the cases there.  No arguments.
Each case: one probe-shaped template (exec/cc<N>/copyprobe-template.tsv) built by P and by C, run from
two starts (SRC and CLONE); the executor output must agree between P and C for every start.  Exit 0 only
when every case agrees; any mismatch stops the run at the first red (prints the case and both outputs).
"""
import json, os, pathlib, shutil, subprocess, sys, tempfile

EXEC = None   # set by main(): the executor compiled from exec/c/run.c
TBL = 'exec/c/tbl.py'
HEAD = "#! base build/parsebase.py\n#! start {start}\ntemplate\tcopyprobe\tmain\t-\t-\t-\t-\t-\t-\n"
RULE_SRC = "main\tsrc\t-\t-\trule\tCP.SRC\t*\tCP.DONE\t[[\"OUT\",65]]\n"
RULE_DONE = "main\tdone\t-\t-\trule\tCP.DONE\t*\tDEAD\t[[\"ACCEPT\"]]\n"

def case(name, body):
    return name, body

# each body: template rows after RULE_SRC/RULE_DONE; CLONE is the copy target of the case
CASES = [
    case('two-way', "main\tclone\t-\t-\tcopy\tCP.CLONE\tCP.SRC\t-\t-\n"
                    "main\tcl-app\t-\t-\tappend\tCP.CLONE\t*\t-\t[[\"OUT\",66]]\n"
                    "main\tsrc-app\t-\t-\tappend\tCP.SRC\t*\t-\t[[\"OUT\",67]]\n"),
    case('set-mode', "main\tclone\t-\t-\tcopy\tCP.CLONE\tCP.SRC\t-\t-\n"
                     "main\tmode\t-\t-\tset-mode\tCP.CLONE\tr\tb\t-\n"),
    case('drop-edge', "main\tclone\t-\t-\tcopy\tCP.CLONE\tCP.SRC\t-\t-\n"
                      "main\tdrop\t-\t-\tdrop-edge\tCP.CLONE\t65\t-\t-\n"),
    case('copy-chain', "main\tclone\t-\t-\tcopy\tCP.CLONE\tCP.SRC\t-\t-\n"
                       "main\tclone2\t-\t-\tcopy\tCP.CLONE2\tCP.CLONE\t-\t-\n"
                       "main\tcl2-app\t-\t-\tappend\tCP.CLONE2\t*\t-\t[[\"OUT\",68]]\n"),
    case('drop-edge', ''),
    case('mode-b', ''), case('mode-t', ''), case('mode-r', ''), case('mode-bare', ''),
    case('copy-state', "main\tclone\t-\t-\tcopy-state\tCP.SRC\tCP.CLONE\t-\t-\n"
                       "main\tcl-app\t-\t-\tappend\tCP.CLONE\t*\t-\t[[\"OUT\",66]]\n"),
    case('copy-replace', "main\tclone\t-\t-\tcopy-replace\tCP.SRC\tCP.CLONE\t[\"OUT\",65]\t[[\"OUT\",69]]\n"),
]

DROP_ROWS = ("main\tk65\t-\t-\trule:b\tCP.SRC\t65\tCP.DONE\t[[\"OUT\",70]]\n"
             "main\tk66\t-\t-\trule:b\tCP.SRC\t66\tCP.DONE\t[[\"OUT\",71]]\n"
             "main\tsrc\t-\t-\trule:b\tCP.SRC\t*\tCP.DONE\t[[\"OUT\",65]]\n"
             "main\tdone\t-\t-\trule\tCP.DONE\t*\tDEAD\t[[\"ACCEPT\"]]\n"
             "main\tclone\t-\t-\tcopy\tCP.CLONE\tCP.SRC\t-\t-\n"
             "main\tdrop\t-\t-\tdrop-edge\tCP.CLONE\t65\t-\t-\n")

def mode_rows(kind):
    k = kind
    return ("main\tk65\t-\t-\t%s\tCP.SRC\t65\tCP.DONE\t[[\"OUT\",70]]\n"
            "main\tk66\t-\t-\t%s\tCP.SRC\t66\tCP.DONE\t[[\"OUT\",71]]\n"
            "main\tsrc\t-\t-\t%s\tCP.SRC\t*\tCP.DONE\t[[\"OUT\",65]]\n"
            "main\tdone\t-\t-\trule\tCP.DONE\t*\tDEAD\t[[\"ACCEPT\"]]\n") % (k, k, k)

def write_tree(tree, name, body, start):
    d = tree / 'exec' / ('cc-' + name)
    d.mkdir(parents=True, exist_ok=True)
    if name == 'drop-edge': rows = DROP_ROWS
    elif name.startswith('mode-'): rows = mode_rows({'mode-b':'rule:b','mode-t':'rule:t','mode-r':'rule:r','mode-bare':'rule'}[name])
    else: rows = RULE_SRC + RULE_DONE + body
    (d / 'copyprobe-template.tsv').write_text(rows)
    (d / 'gen-manifest.tsv').write_text(HEAD.format(start=start))

def build_p(tree, name, out):
    subprocess.run(['python3', 'exec/build/gen.py', 'cc-' + name, str(out)], cwd=tree, check=True, timeout=55)

def build_c(c_gen, tree, name, out):
    # seed-gen: a non-prune stage reads exec/<stage>/gen-manifest.tsv relative to the cwd (seed/gen.c:8978)
    subprocess.run([c_gen, 'cc-' + name, str(out)], cwd=tree, check=True, timeout=55)

def run_table(tree, out_json, tag, inp=b''):
    tbl = out_json.with_suffix('.tbl')
    subprocess.run(['python3', TBL, str(out_json), str(tbl)], cwd=tree, check=True, timeout=55)
    inf = out_json.with_suffix('.in')
    inf.write_bytes(inp)
    r = subprocess.run([EXEC, str(tbl), str(inf)], capture_output=True, timeout=55)
    return r.returncode, r.stdout.decode('latin-1')

def run_cases(tree, c_gen, work):
    work.mkdir(parents=True, exist_ok=True)
    only = os.environ.get('ONLY')
    skipped = []
    passed = 0
    for name, body in CASES:
        if only and name != only: continue
        if name.startswith('mode-'): plan = [('CP.SRC', b''), ('CP.SRC', b'A')]
        elif name == 'drop-edge': plan = [(x, i) for x in ('CP.SRC', 'CP.CLONE') for i in (b'', b'A')]
        else: plan = [(x, b'') for x in ('CP.SRC', 'CP.CLONE')]
        for start, inp in plan:
            tag = f'{name}-{start.split(".")[1].lower()}-{inp.hex() or "e"}'
            write_tree(tree, name, body, start)
            p_json = work / f'{tag}.p.json'; c_json = work / f'{tag}.c.json'
            build_p(tree, name, p_json)
            build_c(c_gen, tree, name, c_json)
            p_rc, p_out = run_table(tree, p_json, tag, inp)
            c_rc, c_out = run_table(tree, c_json, tag + '-c', inp)
            same_json = p_json.read_bytes() == c_json.read_bytes()
            ok = p_rc == c_rc and p_out == c_out and same_json
            print(f'{tag}: C={c_out!r} rc{c_rc} P={p_out!r} rc{p_rc} json_exact={same_json} {"PASS" if ok else "FAIL"}')
            if not ok:
                print('first red:', tag); sys.exit(1)
            passed += 1
    print('copycontractcheck PASS: %d runs over %d cases (C = P); SKIP: %d case(s) (%s)' % (passed, len(set(c for c,_ in CASES)) - len(set(skipped)), len(set(skipped)), ', '.join(sorted(set(skipped))) or 'none'))

def main():
    global EXEC
    ROOT = pathlib.Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory(prefix='copy-contract-') as tmp:
        tmp = pathlib.Path(tmp)
        tree = tmp / 'tree'
        shutil.copytree(ROOT / 'exec', tree / 'exec', ignore=shutil.ignore_patterns('__pycache__'))
        c_gen = tmp / 'seed-gen'; EXEC = str(tmp / 'run')
        q = subprocess.run(['cc', '-std=c99', '-O2', '-w', '-I', str(ROOT / 'seed'), str(ROOT / 'seed/gen.c'), '-o', str(c_gen)], capture_output=True, timeout=55)
        if q.returncode: sys.exit('copycontractcheck FAIL: build seed/gen.c: ' + q.stderr.decode()[:300])
        q = subprocess.run(['cc', '-std=c99', '-O2', '-w', str(ROOT / 'exec/c/run.c'), '-o', EXEC], capture_output=True, timeout=55)
        if q.returncode: sys.exit('copycontractcheck FAIL: build exec/c/run.c: ' + q.stderr.decode()[:300])
        run_cases(tree, str(c_gen), tmp / 'work')

if __name__ == '__main__':
    main()

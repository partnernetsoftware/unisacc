#!/usr/bin/env python3
"""Read-only: which gate suites a set of changed paths can affect, by the gatedeps declarations.

usage: affected.py (--rev A..B | --rev C | PATH...)     (0.0.43 step 6: a recording tool, not a gate)
Declared suites: a path affects a suite if it is in its files, under its trees, a guard key, or
(executable_inputs) inside the product closure; family suites use the family's reviewed trees.
Undeclared gate jobs use the queue's global inputs, so they count as affected by any change.
Reports paths that no declaration claims (the risk: a missed suite), and never edits anything.
"""
import json, pathlib, subprocess, sys

import os
R = pathlib.Path(os.environ.get("AFFECTED_ROOT") or pathlib.Path(__file__).resolve().parents[1])   # AFFECTED_ROOT: the replays point it at the checkout
PRODUCT = ('exec/', 'unisa/', 'src/', 'kernel/', 'include/', 'weights/', 'seed/')
PRODUCT_FILES = ('unisacc.c',)

def in_tree(path, tree):
    tree = tree.rstrip('/')
    return path == tree or path.startswith(tree + '/')

def family_hit(fam, path):
    trees = fam.get('reviewed_trees') or {}
    for t in trees:
        if not in_tree(path, t):
            continue
        if path in (fam.get('files') or []):
            return True
        suffix = pathlib.PurePosixPath(path).suffix
        if t in (fam.get('all_files_trees') or []) or suffix in (fam.get('inventory_suffixes') or []):
            if not any(in_tree(path, x) for x in (fam.get('excluded_dirs') or [])):
                return True
    return path in (fam.get('guards') or {}) or path in (fam.get('files') or [])

def reasons(name, e, fams, path):
    out = []
    if path in (e.get('files') or []): out.append('files')
    if any(in_tree(path, t) for t in (e.get('trees') or [])): out.append('trees')
    if path in (e.get('guards') or {}): out.append('guard')
    if e.get('executable_inputs') and (path.startswith(PRODUCT) or path in PRODUCT_FILES): out.append('product')
    fam = fams.get(e.get('family')) if e.get('family') else None
    if fam and family_hit(fam, path): out.append('family')
    return out

# 0.0.43 step 6, second cut (hypothetical): the pp family reads the pp networks it constructs from
# tables (read-set by strace: exec/pp/*.tsv, exec/facts/pp-*.tsv, the builder, the executor and
# unisa/front/pp.py), not the candidate's bytes.  --model-identity pp drops the candidate ('product')
# reason for these suites only when the candidate's pp route hashes are equal.
PP_SUITES = ('exec-pp-directives', 'exec-pp-literals', 'exec-pp-pragmas', 'exec-pp-object', 'exec-pploc', 'pptruth')
PP_DEPS = ('exec/pp/', 'exec/facts/pp-', 'exec/facts/locations.tsv', 'exec/build/gen.py', 'exec/build/graph.py',
           'exec/c/tbl.py', 'exec/c/net.py', 'exec/c/core.c', 'exec/c/core.h', 'exec/c/codec.h', 'exec/c/packagefooter.h',
           'exec/c/run.c', 'exec/c/refsource.py', 'unisa/front/pp.py')
PP_ROUTES = ('e2', 'e1', 'units')

def pp_identity_equal(prev_dir, cur_dir):
    """True when the pp-stage network hashes (e2, e1, units, e3, e4) of two candidates are equal, read from
    the routes.tsv and models.tsv that tests/modelstreamextract.cx writes."""
    def stage_hashes(d):
        routes = [l.split('\t') for l in (pathlib.Path(d) / 'routes.tsv').read_text().splitlines() if l]
        models = {int(l.split('\t')[0]): l.split('\t')[2] for l in (pathlib.Path(d) / 'models.tsv').read_text().splitlines() if l}
        return {(r[1], models[int(r[4])]) for r in routes if r[1] in PP_ROUTES + ('e3', 'e4')}
    return stage_hashes(prev_dir) == stage_hashes(cur_dir)

# 0.0.43 step 6, third cut (hypothetical): a static class per gate job from its plan command and, for
# scripts, the script body.  "reads-candidate" names the product or MODEL_COM; "table-reference" names
# only the reference ($UA), the builder, cc or python.  --declared-reads keeps only files/guard reasons
# for table-reference jobs (their real read-set is tables and references, measured by strace).
def classify_jobs(jobs_args):
    import re
    prodstr = re.compile(r'MODEL_COM=|OBJ_PRODUCT=|unisacc\.com|unisacc-next|PRODUCT|\{MODEL_COM\}')
    out = {}
    for name, args in jobs_args:
        joined = ' '.join(args)
        if prodstr.search(joined): out[name] = 'reads-candidate'; continue
        script = next((a.lstrip('./') for a in args if a.endswith(('.sh', '.py', '.cx'))), None)
        if script and (R / script).is_file():
            body = (R / script).read_text(errors='replace')
            if prodstr.search(body) or re.search(r'\bsh\s+\S*unisacc', body): out[name] = 'reads-candidate'
            elif re.search(r'\$UA|build_ref|\bcc\b|python3|exec/c/run|refsource', body): out[name] = 'table-reference'
            else: out[name] = 'table-reference'
        else:
            out[name] = 'table-reference' if args and args[0] in ('python3', 'sh', 'bash', 'env', 'cc') else 'uncertain'
    return out

def plan_jobs():
    raw = subprocess.run([str(R / 'tests/gate.sh'), '--plan', '--com'], cwd=R, capture_output=True).stdout.split(b'\0')
    toks = [t.decode('utf-8', 'replace') for t in raw]; jobs = []; i = 0
    while i < len(toks) - 1:
        if not toks[i]: i += 1; continue
        try: n = int(toks[i + 1])
        except ValueError: i += 1; continue
        jobs.append((toks[i], toks[i + 2:i + 2 + n])); i += 2 + n
    return jobs

def changed(args):
    if args and args[0] == '--rev':
        spec = args[1]
        if '..' in spec:
            a, b = spec.split('..', 1)
        else:
            a, b = spec + '^', spec
        return subprocess.run(['git', 'diff', '--name-only', a, b], cwd=R, capture_output=True, text=True, check=True).stdout.split()
    return args

def main(argv):
    identity = None
    declared_reads = '--declared-reads' in argv
    if declared_reads: argv = [a for a in argv if a != '--declared-reads']
    if '--model-identity' in argv:
        i = argv.index('--model-identity'); mode = argv[i + 1]
        prev, cur = argv[argv.index('--identity-prev') + 1], argv[argv.index('--identity-cur') + 1]
        argv = argv[:i] + argv[i + 2:]
        identity = (mode, pp_identity_equal(prev, cur))
    paths = changed(argv)
    d = json.loads((R / 'tests/gatedeps.json').read_text())
    suites, fams = d['suites'], d['families']
    jobs = subprocess.run([str(R / 'tests/gate.sh'), '--list', '--com'], cwd=R, capture_output=True, text=True).stdout.split()
    undeclared = [j for j in jobs if j not in suites]
    affected, unclaimed = {}, []
    classes = classify_jobs(plan_jobs()) if declared_reads else {}
    for p in paths:
        claimed = False
        for n, e in suites.items():
            why = reasons(n, e, fams, p)
            if declared_reads and classes.get(n) == 'table-reference':
                why = [w for w in why if w in ('files', 'guard')]
            if identity and identity[0] == 'pp' and n in PP_SUITES:
                # the pp family's read-set is its construction inputs and its own script: the candidate reason
                # and the declared whole trees are replaced by that read-set (measured with strace, see PP_DEPS)
                script = [a.lstrip('./') for a in (e.get('command') or []) if a.endswith(('.py', '.sh'))]
                why = ['pp-deps'] if (p.startswith(PP_DEPS) or p in script) else []
            if why:
                claimed = True
                affected.setdefault(n, set()).update(why)
        if not claimed and not p.startswith(('.git/',)):
            unclaimed.append(p)
    # the queue's global inputs: every tracked or untracked file under these, for undeclared jobs
    GLOBAL = ('src', 'exec', 'tests', 'include', 'kernel', 'weights', 'unisa', 'examples', 'iterate', 'release', 'scripts', 'unisacc.c', 'README.md', 'ARCHITECTURE.md', 'AGENTS.md', 'Makefile')
    global_hit = any(p.split('/')[0] in GLOBAL or p in GLOBAL for p in paths)
    job_set = set(jobs)
    declared_jobs = {n for n in affected if n in job_set}
    undeclared_hit = set(undeclared) if (global_hit and paths) else set()
    print('changed paths: %d' % len(paths))
    if declared_reads:
        import collections
        print('declared-reads: table-reference jobs keep files/guard reasons only; classes %s' % dict(collections.Counter(classes.values())))
    if identity: print('model identity pp: pp-stage hashes %s' % ('EQUAL (candidate reason dropped for pp suites)' if identity[1] else 'DIFFER (candidate reason kept)'))
    print('gate jobs affected: %d of %d  (declared %d, undeclared %d)' % (
        len(declared_jobs | undeclared_hit), len(jobs), len(declared_jobs), len(undeclared_hit)))
    print('declared-only suites not in gate list (affected, informational): %d' % len({n for n in affected if n not in job_set}))
    print('paths no declaration claims: %d' % len(unclaimed))
    for p in unclaimed[:40]: print('  UNCLAIMED', p)
    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

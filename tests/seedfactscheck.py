#!/usr/bin/env python3
"""0.0.29 B4: seed/facts.h (C) against exec/assemble.py (Python) on every data binding the manifests write.

For each manifest row under exec/*/ the fact scope (FACTS column) and the binding cells (BIND and SEQ
columns) are evaluated by both sides with an empty environment; results are compared as JSON, and a
Python error must be a C error.  Graph value forms (@rej, @bytes, @out, @seqmap, @acts, @text*, fresh:)
are the dispatcher's, not facts.h's, and are skipped.  The C side is seed/gen.c + seed/facts.h built
by the system cc into a private driver (the shipped tools are built by unisacc.com in seedconstructcheck).
"""
import json, pathlib, subprocess, sys, tempfile
ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'exec'))
import assemble

GRAPH = ('@rej', '@bytes', '@out', '@seqmap', '@acts', '@textf', '@text', 'fresh:')
DRIVER = r'''
#define main seedgen_main
#include "seed/gen.c"
#undef main
#include "seed/facts.h"
int main(void) {
    static char line[1 << 20];
    while (fgets(line, sizeof line, stdin)) {
        char *t1 = strchr(line, '\t'), *t2; Value *env, *facts, *b; Buffer out = {0};
        size_t n = strlen(line); if (n && line[n-1] == '\n') line[n-1] = 0;
        if (!t1) continue; *t1 = 0; t2 = strchr(t1 + 1, '\t'); if (!t2) continue; *t2 = 0;
        env = value_json(t1 + 1, "env"); facts = seed_facts_scope(env, line, 0);
        b = seed_bind_cells(t2 + 1, facts, env);
        if (!b) puts("null"); else { buf_value(&out, b); puts(out.s); }
        fflush(stdout);
    }
    return 0;
}
'''

def cases():
    seen = set()
    for m in sorted(ROOT.glob('exec/*/*manifest.tsv')):
        for depth, row in assemble.Run.rows(m):
            factn, seq, bind = row[4], row[6], row[7]
            for cells in (bind, seq):
                if cells in ('', '-') or any(g in cells for g in GRAPH): continue
                key = (factn, cells)
                if key not in seen: seen.add(key); yield (str(m.relative_to(ROOT)), factn, cells)

def synth_env(cells):
    """Every $NAME and $$DOMAIN:KEY the cells read, bound to a marker, so the env forms are compared too."""
    import re
    env = {}
    for d, k in re.findall(r'\$\$(\w+):([\w.]+)', cells): env.setdefault(d, {})[k] = 'E:' + k
    for n in re.findall(r'(?<![\$\w])\$(\w+)', cells): env.setdefault(n, 'E:' + n)
    return env

def python(factn, cells, env=None):
    try:
        facts = {}
        for s in [] if factn in ('', '-') else factn.split('+'):
            facts.update(assemble.load_facts(s[5:] if s.startswith('load:') else s))
        facts = dict(env or {}, **facts)
        r = assemble.Run(None, None, {}, dict(env or {})).cells(cells, facts)
        return json.dumps(r, separators=(',', ':'), ensure_ascii=False)
    except Exception:
        return 'ERR'

def main():
    with tempfile.TemporaryDirectory() as d:
        drv = pathlib.Path(d) / 'drv'
        (pathlib.Path(d) / 'drv.c').write_text(DRIVER)
        subprocess.run(['cc', '-w', '-O1', '-I', str(ROOT), '-o', str(drv), str(pathlib.Path(d) / 'drv.c')], check=True, timeout=50)
        same = bad = 0
        errs = 0
        for where, factn, cells in cases():
            env = synth_env(cells)
            want = python(factn, cells, env); errs += want == 'ERR'
            r = subprocess.run([str(drv)], input=factn + '\t' + json.dumps(env) + '\t' + cells + '\n', capture_output=True, text=True, cwd=ROOT, timeout=10)
            got = r.stdout.strip() if r.returncode == 0 else 'ERR'
            if got == want: same += 1
            else:
                bad += 1
                if bad <= 10: print('  DIFF %s  facts=%s  cells=%s\n    python %s\n    c      %s' % (where, factn, cells[:80], want[:160], got[:160]))
    print('seedfacts  bindings same %d  differ %d  (both error %d)' % (same, bad, errs))
    return 1 if bad or not same else 0

if __name__ == '__main__':
    sys.exit(main())

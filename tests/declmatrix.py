#!/usr/bin/env python3
"""declmatrix: declarator forms x positions, three-way (0.0.22, TDD after the `T *v[]` defect).

The reference took a parameter `char *v[]` as `char *` and nothing caught it: no probe used
that form.  This suite generates the forms systematically instead of waiting for a program to
hit them: element type x pointer depth x (array / pointer / typedef) x position (global,
local, parameter, struct member, return), each read through sizeof, indexing and dereference.
Every generated program is compiled by the system cc (the expected output), the reference UA,
and, when MODEL_COM is set, the product; any difference is a failure.  Nothing checked is red.
"""
import itertools, os, pathlib, subprocess, sys, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
BOUND = [sys.executable, str(ROOT / 'tests/bound.py')]
ELEMS = [('char', "'a'+%d"), ('short', '100+%d'), ('int', '1000+%d'), ('long', '100000+%d'), ('double', '0.5+%d')]
FMT = {'char': '%d', 'short': '%d', 'int': '%d', 'long': '%ld', 'double': '%g'}

PROBES = [('q1a', 'q1a(a0)'), ('q1p', 'q1p(b0)'), ('q1n', 'q1n(l0)'), ('q1s', 'q1s(s.m)'),
          ('q2a', 'q2a(p1)'), ('q2c', 'q2c(l1)'), ('q2p', 'q2p(s.mp)'), ('q2t', 'q2t(tp)'),
          ('q3a', 'q3a(p2)'), ('q3p', 'q3p(p2)'), ('q4a', 'q4a(p3)'), ('qr', 'qr(ta)'), ('qra', 'qra(ta)'),
          ('r1', 'r1(p1)[0]'), ('r2', 'r2(p2)[0][1]'), ('g2', 'p2[1][0][2]'), ('g3', 'p3[0][1][0][0]'),
          ('m', 's.mpp[1][1]'), ('l', 'l1[0][2]'), ('tb', 'q1a(tb)'), ('tl', 'tl[1]'),
          ('z1a', 'z1a(a0)', 'd'), ('z2a', 'z2a(p1)', 'd'), ('z3a', 'z3a(p2)', 'd'), ('zp', 'zp(p1)', 'd'),
          ('zg3', '(int)sizeof p3[0][0][0]', 'd'), ('zs', '(int)sizeof s', 'd'), ('zta', '(int)sizeof ta', 'd'),
          ('ztp', '(int)sizeof tp', 'd'), ('ztb', '(int)sizeof tb', 'd'), ('ztl', '(int)sizeof tl', 'd')]

def program(t, init, probes):
    """All probes for one element type; a mismatch is then narrowed probe by probe."""
    L = ['#include <stdio.h>', 'typedef %s E;' % t, 'typedef E *EP;', 'typedef E EA3[3];']
    L += ['static E a0[3] = {%s};' % ', '.join(init % k for k in range(3)),
          'static E b0[3] = {%s};' % ', '.join(init % (k + 3) for k in range(3)),
          'static E *p1[2] = {a0, b0};', 'static E **p2[2] = {p1, p1 + 1};', 'static E ***p3[1] = {p2};',
          'static EP tp[2] = {a0, b0};', 'static E ta[2][3];', 'static EA3 tb;',
          'struct S { E m[3]; E *mp[2]; E **mpp; } s;']
    # parameters in every spelling: brackets, pointers, typedefs, qualifiers, sized/static brackets
    L += ['static E q1a(E v[]) { return v[2]; }', 'static E q1p(E *v) { return v[2]; }',
          'static E q1n(E v[3]) { return v[1]; }', 'static E q1s(E v[static 3]) { return v[0]; }',
          'static E q2a(E *v[]) { return v[1][2]; }', 'static E q2c(E *const v[]) { return v[1][1]; }',
          'static E q2p(E **v) { return v[1][0]; }', 'static E q2t(EP v[]) { return v[1][2]; }',
          'static E q3a(E **v[]) { return v[1][0][1]; }', 'static E q3p(E ***v) { return v[0][1][2]; }',
          'static E q4a(E ***v[]) { return v[0][0][1][0]; }',
          'static E qr(E (*v)[3]) { return v[1][2]; }', 'static E qra(E v[][3]) { return v[1][0]; }',
          'static int z1a(E v[]) { return (int)sizeof v[0]; }', 'static int z2a(E *v[]) { return (int)sizeof v[0]; }',
          'static int z3a(E **v[]) { return (int)sizeof v[0] + (int)sizeof *v[0]; }', 'static int zp(E *v[]) { return (int)sizeof v; }',
          'static E *r1(E *v[]) { return v[1]; }', 'static E **r2(E **v[]) { return v[1]; }']
    L += ['int main(void) {', '    E l0[3]; E *l1[2]; EA3 tl; int k;',
          '    for (k = 0; k < 3; k = k + 1) { l0[k] = a0[k]; ta[0][k] = a0[k]; ta[1][k] = b0[k]; s.m[k] = b0[k]; tb[k] = b0[k]; tl[k] = a0[k]; }',
          '    l1[0] = b0; l1[1] = a0; s.mp[0] = a0; s.mp[1] = b0; s.mpp = p1;',
          ] + ['    printf("%s %s\\n", %s);' % (p[0], '%d' if len(p) > 2 else FMT[t], p[1]) for p in probes] + ['    return 0;', '}']
    return '\n'.join(L) + '\n'

# Forms a compiler may refuse BY NAME ("not covered: ..."), but must never miscompile.
EDGES = [('tdarray', 'typedef int EA3[3]; static EA3 ta[2]; int main(void) { ta[1][2] = 5; printf("%d %d\\n", ta[1][2], (int)sizeof ta); return 0; }'),
         ('tdarrayparam', 'typedef int EA3[3]; static int f(EA3 v[]) { return v[1][2]; } static int m[2][3] = {{1,2,3},{4,5,6}}; int main(void) { printf("%d\\n", f(m)); return 0; }'),
         ('ptrarrayparam', 'static int f(int (*v[])(int)) { return v[1](3); } static int g(int x) { return x + 1; } static int h(int x) { return x * 2; } static int (*fs[2])(int) = {g, h}; int main(void) { printf("%d\\n", f(fs)); return 0; }')]

def launch(comp):
    # an APE (.com) is a shell script to exec(2): ENOEXEC from Python, so run it through sh
    return ['sh', comp] if comp.endswith('.com') else [comp]

def run(cmd, timeout=30):
    r = subprocess.run(BOUND + [str(timeout)] + cmd, capture_output=True)
    return r.returncode, r.stdout.decode(errors='replace'), r.stderr.decode(errors='replace')

ua = os.environ.get('UA')
if not ua:   # the gate's same-source reference, built and stamped by tests/lib.sh
    subprocess.run(['bash', '-c', '. "$0" && ua_ready', str(ROOT / 'tests/lib.sh')], cwd=ROOT, check=True, timeout=45)
    ua = '/tmp/ua_ref'
product = os.environ.get('MODEL_COM')
cc = os.environ.get('CC', 'cc')
ok = bad = refused = 0
WANT = {}
with tempfile.TemporaryDirectory(prefix='unisacc-declmatrix-') as td:
    d = pathlib.Path(td)
    refused = 0
    k, n = (int(x) for x in os.environ.get('SHARD', '1/1').split('/'))
    def check(t, init, probes, label, comp, narrow):
        """1 ok / 0 wrong for this set; narrows to single probes when the whole set disagrees."""
        global ok, bad, refused
        tag = '%s.%s' % (t, probes[0][0] if len(probes) == 1 else 'all')
        src = d / ('dm_%s_%s.c' % (t, tag.split('.')[1])); src.write_text(program(t, init, probes))
        # each new native binary costs a first-exec scan on macOS (~0.5-0.9 s): expected
        # outputs are computed once per program text, shared by both compilers
        text = src.read_text()
        if len(probes) == 1 and (t, 'all') in WANT:
            # the combined program printed one line per probe with the same prelude: its line is this program's output
            rc0, out0, err0 = WANT[(t, 'all')]
            line = [l for l in out0.splitlines() if l.split(' ')[0] == probes[0][0]]
            WANT[text] = (rc0, line[0] + '\n' if line else '', err0)
        if text not in WANT:
            rc, _, err = run([cc, '-std=c99', '-w', '-o', str(d / 'ref'), str(src)])
            if rc: print('declmatrix FAIL cc', tag, err[:200]); bad += 1; return
            WANT[text] = run([str(d / 'ref')], 10)
            if len(probes) > 1: WANT[(t, 'all')] = WANT[text]
        want = WANT[text]
        got = run(launch(comp) + ['-run', str(src)])
        if got[:2] == want[:2]: ok += len(probes); return
        if narrow and len(probes) > 1:
            for p in probes: check(t, init, [p], label, comp, False)
            return
        if got[0] != 0 and 'not covered' in got[2] and not got[1]:
            ok += 1; refused += 1; print('  refused  %-14s %-9s %s' % (tag, label, got[2].strip().splitlines()[-3][:90] if got[2].strip() else ''))
        else: bad += 1; print('  FAIL     %-14s %-9s rc %d want %r got %r %s' % (tag, label, got[0], want[1].strip(), got[1].strip(), got[2].strip()[:100]))
    for t, init in ELEMS[k - 1::n]:
        for label, comp in [('reference', ua)] + ([('product', product)] if product else []):
            check(t, init, PROBES, label, comp, True)
    for name, body in (EDGES if k == 1 else []):
        src = d / ('edge_%s.c' % name); src.write_text('#include <stdio.h>\n' + body + '\n')
        rc, _, err = run([cc, '-std=c99', '-w', '-o', str(d / 'ref'), str(src)])
        if rc: print('declmatrix FAIL cc', name, err[:200]); bad += 1; continue
        want = run([str(d / 'ref')], 10)
        for label, comp in [('reference', ua)] + ([('product', product)] if product else []):
            got = run(launch(comp) + ['-run', str(src)])
            if got[:2] == want[:2]: ok += 1
            elif got[0] != 0 and 'not covered' in got[2] and not got[1]: ok += 1; refused += 1; print('  refused  edge.%s %s' % (name, label))
            else: bad += 1; print('  FAIL declmatrix edge', name, label, 'rc', got[0], repr(got[1][:80]), got[2].strip()[:120])
print('declmatrix  ok %d   wrong %d   refused-by-name %d   (types %d x probes %d + edges %d, compilers %d)' % (ok, bad, refused, len(ELEMS), len(PROBES), len(EDGES), 1 + bool(product)))
sys.exit(1 if bad or not ok else 0)

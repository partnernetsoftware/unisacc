#!/usr/bin/env python3
"""Declarator metamorphisms: array-parameter adjustment preserves the element type.

This is independent of declmatrix's concrete-value grid. Each generated C program
contains two equivalent declaration spellings and compares both with a system C99
compiler, the private reference, and optionally the product. A named refusal is
reported as uncovered; an accepted wrong output is always a failure.
"""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BOUND = [sys.executable, str(ROOT / 'tests/bound.py'), '12']
CASES = {
    'plain-array': r'''
        #include <stdio.h>
        static int a[3] = {3, 5, 7};
        static int array_form(int v[]) { return v[2] + (int)sizeof v[0]; }
        static int pointer_form(int *v) { return v[2] + (int)sizeof v[0]; }
        int main(void) { printf("%d %d\n", array_form(a), pointer_form(a)); return 0; }
    ''',
    'array-of-pointers': r'''
        #include <stdio.h>
        static int a = 7, b = 11;
        static int *v[2] = {&a, &b};
        static int array_form(int *p[]) { return p[1][0] + (int)sizeof p[0]; }
        static int pointer_form(int **p) { return p[1][0] + (int)sizeof p[0]; }
        int main(void) { printf("%d %d\n", array_form(v), pointer_form(v)); return 0; }
    ''',
    'typedef-pointer': r'''
        #include <stdio.h>
        typedef int *IP;
        static int a = 13, b = 17;
        static IP v[2] = {&a, &b};
        static int alias_form(IP p[]) { return p[1][0] + (int)sizeof p[0]; }
        static int expanded_form(int **p) { return p[1][0] + (int)sizeof p[0]; }
        int main(void) { printf("%d %d\n", alias_form(v), expanded_form(v)); return 0; }
    ''',
    'array-row': r'''
        #include <stdio.h>
        static int a[2][3] = {{2, 3, 5}, {7, 11, 13}};
        static int array_form(int v[][3]) { return v[1][2] + (int)sizeof v[0]; }
        static int pointer_form(int (*v)[3]) { return v[1][2] + (int)sizeof v[0]; }
        int main(void) { printf("%d %d\n", array_form(a), pointer_form(a)); return 0; }
    ''',
}


def run(args):
    p = subprocess.run(BOUND + [str(x) for x in args], capture_output=True)
    return p.returncode, p.stdout, p.stderr.decode(errors='replace')


def launch(path):
    return ['sh', path] if path.endswith('.com') else [path]


def main():
    ua = os.environ.get('UA')
    if not ua:
        print('declshape: UA must name a private reference', file=sys.stderr)
        return 2
    product = os.environ.get('MODEL_COM')
    cc = os.environ.get('CC', 'cc')
    good = refused = bad = 0
    with tempfile.TemporaryDirectory(prefix='unisacc-declshape-') as tmp:
        d = Path(tmp)
        for name, body in CASES.items():
            src = d / (name + '.c')
            exe = d / (name + '.cc')
            src.write_text(body)
            c = run([cc, '-std=c99', '-w', '-o', exe, src])
            if c[0]:
                print('FAIL', name, 'system cc compile:', c[2][:160]); bad += 1; continue
            want = run([exe])
            vals = want[1].split()
            if want[0] or len(vals) != 2 or vals[0] != vals[1]:
                print('FAIL', name, 'metamorphic oracle:', want[:2]); bad += 1; continue
            for label, compiler in [('reference', ua)] + ([('product', product)] if product else []):
                got = run(launch(compiler) + ['-run', src])
                if got[:2] == want[:2]:
                    good += 1
                    print('ok', name, label)
                elif got[0] and not got[1] and ('not covered' in got[2] or 'UNCOVERED' in got[2]):
                    refused += 1
                    print('uncovered', name, label, got[2].strip().splitlines()[-1][:120])
                else:
                    bad += 1
                    print('FAIL', name, label, 'expected', want[:2], 'got', got[:2], got[2][:160])
    print('declshape: cases', len(CASES), 'equal', good, 'uncovered', refused, 'wrong', bad)
    return 1 if bad or not good else 0


if __name__ == '__main__':
    sys.exit(main())

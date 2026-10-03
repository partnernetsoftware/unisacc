#!/usr/bin/env python3
"""0.0.21 reference front-end fixes against product E3: reference tape bytes
or a named refusal ("not covered: ..."), never accepted-with-different-bytes.

Each probe isolates one construct the reference fixed in 0.0.21 (the a_* and
hosthdr/ccinterop probes).  Whole-program probes compare the E3 network's tape
with `UA -S -o -`.  Object probes (-c -b, \\0cli/object) cannot be compared on
tape (the reference writes its interop thunks only into objects), so the
reference must accept them (`-c -b lnx/x86_64`) and E3 must refuse by name.
Verdicts are printed; full implementation is 0.0.22 work.
"""
import os, pathlib, subprocess, sys, tempfile
R = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(R/'exec/c')); from pack import build
UA = os.environ.get('UA', '/tmp/ua_ref')

TD = 'typedef long jb[8];\n'
FP = 'typedef int (*F)(int); struct G { F f; int u; }; static int inc(int x){return x+1;}\n'
PT = 'typedef struct T { long a, b, c, d; } T;\n'
PROBES = [  # (item, name, source)
 (1, 'td-global', TD + 'jb g; int main(void){ g[1]=3; return (int)g[1] + (int)sizeof g; }\n'),
 (1, 'td-local', TD + 'int main(void){ jb l; l[1]=3; return (int)l[1]; }\n'),
 (1, 'td-static', TD + 'int main(void){ static jb s; s[2]=4; return (int)s[2]; }\n'),
 (1, 'td-member', TD + 'struct S { int x; jb b; int y; }; int main(void){ struct S s; s.y=2; return s.y + (int)sizeof(struct S); }\n'),
 (1, 'td-param', TD + 'static long f(jb e){ return e[0] + (long)sizeof e; } int main(void){ long a[8]; a[0]=1; return (int)f(a); }\n'),
 (1, 'td-sizeof', TD + 'int main(void){ return (int)sizeof(jb); }\n'),
 (2, 'late-sizeof', 'typedef struct CI CI; struct CI { long a, b; CI *next; }; int main(void){ return (int)sizeof(CI); }\n'),
 (3, 'extern-array-defined', 'extern int arr[];\nstatic int other[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};\nint arr[3] = {7, 8, 9};\nint main(void){ return arr[2] + (int)sizeof other; }\n'),
 (3, 'extern-array-undefined', 'extern int arr[];\nint main(void){ return 0; }\n'),
 (3, 'extern-arrays-then-definitions', 'extern const char a[];\nextern int b[];\nconst char a[] = "xy";\nint b[] = { 1, 2 };\nint main(void){return b[1]+(int)sizeof a;}\n'),
 (3, 'extern-array-then-object', 'extern int t[];\nint z;\nint t[] = {1,2,3};\nint main(void){return t[2]+z;}\n'),
 (3, 'extern-array-then-function', 'extern int t[];\nint f(void){ int q = 2; return t[0]+q; }\nint t[3] = {1,2,3};\nint main(void){return f();}\n'),
 (3, 'tentative-array', 'int arr[];\nint main(void){ return arr[0]; }\n'),
 (3, 'flexible-member', 'struct S { int n; int d[]; };\nint main(void){ int x = 1; return (int)sizeof(struct S) + x; }\n'),
 (4, 'ptrptr-member', 'typedef struct TS { long a, b, c, d; } TS; typedef struct st { TS **hash; int n; } st; int main(void){ TS *v[8]; st t; st *tb=&t; TS x; t.hash=v; tb->hash[5]=&x; return v[5]==&x; }\n'),
 (5, 'fp-member-star', FP + 'int main(void){ struct G g; struct G *p=&g; g.f=inc; return (*p->f)(4); }\n'),
 (5, 'fp-member-arrow', FP + 'int main(void){ struct G g; struct G *p=&g; g.f=inc; return p->f(4); }\n'),
 (6, 'ptr2d-local', PT + 'int main(void){ T t={1,2,3,4}; T *m[3][2]; m[2][1]=&t; return (int)m[2][1]->c; }\n'),
 (6, 'ptr2d-member', PT + 'struct G { int x; T *c[3][2]; }; int main(void){ static struct G g; T t={1,2,3,4}; g.c[1][1]=&t; return (int)g.c[1][1]->d; }\n'),
 (7, 'member-specifier-leak', 'typedef int (*CF)(void *L); typedef struct Reg { const char *name; CF func; } Reg; int main(void){ Reg r; r.name=0; return (int)sizeof(Reg) + (int)sizeof r; }\n'),
 (8, 'paren-name-static', 'static int (inc)(int x); static int inc(int x){return x+1;} int main(void){ return inc(2); }\n'),
 (8, 'paren-name-fptypedef', 'typedef int (*CF)(int); static int inc(int x){return x+1;}\nextern CF (getf) (int which);\nint use(void){ CF f = getf(0); return f(4); }\nCF getf (int which) { return inc; }\nint main(void){ return use(); }\n'),
 (9, 'builtin-offsetof', 'struct L { int a; long x; char c[3]; }; int main(void){ return (int)__builtin_offsetof(struct L, x); }\n'),
 (9, 'offsetof', '#include <stddef.h>\nstruct L { int a; long x; char c[3]; }; int main(void){ return (int)offsetof(struct L, c[2]); }\n'),
 (10, 'setjmp', '#include <setjmp.h>\nstatic jmp_buf b; int main(void){ if (setjmp(b) == 0) longjmp(b, 3); return 1; }\n'),
 (10, 'setjmp-member', '#include <setjmp.h>\nstruct lj { struct lj *prev; jmp_buf b; int s; };\nint main(void){ struct lj l; l.s = 0; if (_setjmp(l.b) == 0) _longjmp(l.b, 2); return l.s; }\n'),
 (11, 'comma-if', 'int main(void){ int a=0,b=1; if (a++, b) return a; return 9; }\n'),
 (11, 'comma-while', 'int main(void){ int a=0,b=3; while ((void)(a++), b-- > 0) ; return a; }\n'),
 (11, 'comma-for', 'int main(void){ int a=0,i; for (i=0; a++, i<3; i++) ; return a; }\n'),
 (11, 'comma-do', 'int main(void){ int a=0,b=3; do ; while (a++, b-- > 0); return a; }\n'),
 (0, 'hostcall-r2', 'long f(void){ long a[10] = {0}; return __hostcall(__hostaddr3(), a); }\nint main(void){ return (int)f(); }\n'),  # sysargs zero-fills r2 (found by e3self)
 (12, 'stale-struct-mark', 'struct S { long a; double d; }; static double ld(struct S *s){ return s->d; } int main(void){ struct S s; s.d=370.5; if (ld(&s) != 370.5) return 1; return 0; }\n'),
]
OBJECTS = [  # (item, name, source, refusal)
 (13, 'ccw-export', 'extern int f(int);\nint f(int x){return x+1;}\nint main(void){return f(1);}\n', 'not covered: cc interop export'),
 (13, 'ccx-call', 'int h(int);\nint main(void){return h(1);}\n', 'not covered: cc interop call'),
]
FILES = [(1, 'a_tdarr'), (2, 'a_tdlate'), (5, 'a_fpmemb'), (6, 'a_ptr2d'), (4, 'a_ptrptrmemb'),
         (7, 'a_regtab'), (3, 'a_externarr'), (3, 'fb12-13-extern-incomplete-array'), (8, 'a_parenfn'), (12, 'a_callres'),
         (10, 'a_setjmp_nested'), (14, 'a_strlit_subaddr'), (14, 'a_addr_parmember'), (14, 'a_addr_postmember'), (14, 'a_addr_premember'),
         (14, 'a_fpstar_stmt')]


def call(args, **kw):
    p = subprocess.run(list(map(str, args)), capture_output=True, timeout=55, **kw)
    assert p.returncode == 0, (args, p.returncode, p.stderr[-2000:])
    return p.stdout


def main():
    with tempfile.TemporaryDirectory(prefix='e3-r21-') as td:
        t = pathlib.Path(td)
        call([os.environ.get('EXEC_CC', 'cc'), '-O2', R/'exec/c/run.c', '-o', t/'run'])
        for name, script, flags in [('pp', 'build/gen.py', ['pp']), ('lex', 'lex/gen.py', ['--typed']), ('parse', 'parse2/gen2.py', [])]:
            call([sys.executable, R/'exec'/script, t/(name+'.json'), *flags])
            call([sys.executable, R/'exec/c/tbl.py', t/(name+'.json'), t/(name+'.tbl')])
            call([sys.executable, R/'exec/c/net.py', t/(name+'.tbl'), t/(name+'.net')])
        print(call([t/'run', '--check-net', t/'parse.tbl', t/'parse.net']).decode().strip())
        route = t/'route.tsv'
        route.write_text('c\te2\tsource\tpp\tpp.net\nc\te1\tpp\ttokens\tlex.net\nc\te3\ttokens\ttape\tparse.net\n')
        packages = {}
        for mode in ('program', 'object'):
            res = t/('res-'+mode); res.mkdir()
            (res/'error-limit').write_bytes(b'')  # default limit; a mount may not be empty
            if mode == 'object':
                (res/'object').write_bytes(b'\1')
            packages[mode] = t/('pkg-'+mode)
            packages[mode].write_bytes(build([route], [('00636c692f', res), ('006864722f', R/'include')], cache=False))

        def e3(mode, f):
            return subprocess.run([str(t/'run'), '--bundle', str(packages[mode]), 'c', str(f), str(f), str(R/'include')],
                                  capture_output=True, timeout=30, cwd=t)
        counts = {'equal': 0, 'refused': 0}
        cases = list(PROBES) + [(i, n, (R/'tests/c'/(n+'.c')).read_text()) for i, n in FILES]
        cases.append((10, 'hosthdr-setjmp', (R/'tests/hosthdr/setjmp.c').read_text()))
        for item, name, src in cases:
            f = t/(name+'.c'); f.write_text(src)
            ref = subprocess.run([UA, str(f), '-S', '-o', '-'], capture_output=True, timeout=30)
            assert ref.returncode == 0, (name, 'reference rejects', ref.stderr[-500:])
            got = e3('program', f)
            if got.returncode == 0:
                assert got.stdout == ref.stdout, (item, name, 'E3 accepted with different tape bytes')
                verdict = 'reference tape identical'; counts['equal'] += 1
            else:
                assert got.returncode == 1 and b'not covered: ' in got.stderr and not got.stdout, (item, name, got.returncode, got.stderr[-500:])
                verdict = 'refused: ' + got.stderr.decode().strip().splitlines()[-1].split('not covered: ', 1)[1]
                counts['refused'] += 1
            print('r21 item %d %s: %s' % (item, name, verdict), flush=True)
        for item, name, src, refusal in OBJECTS:
            f = t/(name+'.c'); f.write_text(src)
            ref = subprocess.run([UA, '-c', '-b', 'lnx/x86_64', str(f), '-o', str(t/(name+'.o'))], capture_output=True, timeout=30)
            assert ref.returncode == 0, (name, 'reference object rejects', ref.stderr[-500:])
            got = e3('object', f)
            assert got.returncode == 1 and refusal.encode() in got.stderr and not got.stdout, (item, name, got.returncode, got.stderr[-500:])
            counts['refused'] += 1
            print('r21 item %d %s: object refused: %s' % (item, name, refusal[len('not covered: '):]), flush=True)
        f = t/'object-plain.c'; f.write_text('static int g(int x){return x+1;}\nint main(void){return g(1);}\n')
        assert e3('object', f).returncode == 0, 'a plain object must stay accepted'
        print('r21: %d reference-identical, %d refused by name, 0 accepted with different bytes (networks)' % (counts['equal'], counts['refused']))


if __name__ == '__main__':
    main()

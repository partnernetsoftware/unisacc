#!/usr/bin/env python3
"""Execute delta-produced x86/ARM64 FP bytes against host C arithmetic/conversions.

Usage: fpcheck.py C_EXECUTOR DELTA.tbl [arm64]. Default macOS uses an x86_64 host harness
(Rosetta on arm64); Linux x86_64 runs natively. ARM64 mode currently requires macOS arm64. Other hosts exit 77, not pass.
The reference encoder is not called here. Only defined conversion inputs are
used. rbx is preserved by the harness because the tape ABI reserves it while
host SysV treats it as callee-saved. Every subprocess is bounded at 60 s.
"""
import pathlib
import platform
import subprocess
import sys
import tempfile

OPS = [a + w for a in ('fadd', 'fsub', 'fmul', 'fdiv', 'feq', 'flt', 'fle') for w in ('64', '32')]
OPS += ['cvtid', 'cvtis', 'cvtud', 'cvtus', 'cvtdi', 'cvtdu', 'cvtsd', 'cvtds', 'fsqrt64', 'fsqrt32']


def run(cmd, **kw):
    return subprocess.run(cmd, check=True, timeout=60, **kw)


def main():
    if len(sys.argv) not in (3,4) or (len(sys.argv)==4 and sys.argv[3]!='arm64'):
        raise SystemExit('usage: fpcheck.py C_EXECUTOR DELTA.tbl [arm64]')
    arm = len(sys.argv)==4
    darwin = sys.platform == 'darwin'
    if arm and not (darwin and platform.machine()=='arm64'):
        print('ARM64 fp execution skipped: requires macOS arm64'); return 77
    if not darwin and not (sys.platform.startswith('linux') and platform.machine() == 'x86_64'):
        print('fp execution not run: requires macOS Rosetta/x86_64 or Linux x86_64')
        return 77
    with tempfile.TemporaryDirectory(prefix='unisacc-fpexec-') as tmp:
        p = pathlib.Path(tmp)
        asm, decls, checks = ['.text'], [], []
        for op in OPS:
            binary = op[:3] in ('feq', 'flt', 'fle') or op[:4] in ('fadd', 'fsub', 'fmul', 'fdiv')
            inp = p / 'in.txt'
            inp.write_text(op + (' x0, x0' if arm else ' rax, rdi') + ((', x1' if arm else ', rsi') if binary else '') + '\n')
            blob = run([sys.argv[1], sys.argv[2], str(inp)], capture_output=True).stdout
            if not blob:
                raise RuntimeError('no bytes for ' + op)
            name = ('_' if darwin else '') + 'probe_' + op
            asm += ['.globl ' + name, name + ':'] + ([] if arm else ['pushq %rbx']) + [
                    '.byte ' + ','.join(str(b) for b in blob)] + ([] if arm else ['popq %rbx']) + ['ret']
            decls.append('extern U probe_' + op + '(U,U);')
            call = 'probe_' + op
            w = op[-2:]; typ = 'double' if w == '64' else 'float'; pack = 'd' if w == '64' else 's'
            if binary:
                cmp = op[:3] in ('feq', 'flt', 'fle')
                operator = {'fadd': '+', 'fsub': '-', 'fmul': '*', 'fdiv': '/', 'feq': '==', 'flt': '<', 'fle': '<='}[op[:-2]]
                values = '0.0,-0.0,1.5,-2.0,6.0' + (',NAN,INFINITY,-INFINITY' if cmp else '')
                exp = '(U)(a ' + operator + ' b)' if cmp else pack + '(a ' + operator + ' b)'
                checks.append('{ '+typ+' v[]={'+values+'}; for(int i=0;i<sizeof(v)/sizeof(v[0]);i++) for(int j=0;j<sizeof(v)/sizeof(v[0]);j++) {'
                              ' volatile '+typ+' a=v[i],b=v[j]; '+('' if cmp else 'if(b==0) continue;')+
                              ' check("'+op+'",'+call+'('+pack+'(a),'+pack+'(b)),'+exp+'); }}')
            elif op in ('cvtid', 'cvtis', 'cvtud', 'cvtus'):
                signed = op[3] == 'i'; pack = 'd' if op[-1] == 'd' else 's'; typ = 'double' if pack == 'd' else 'float'
                vals = '0,1,-5,9007199254740993LL,LLONG_MAX' if signed else '0,1,9007199254740993ULL,1ULL<<63,ULLONG_MAX'
                src = 'long long' if signed else 'U'
                checks.append('{ '+src+' v[]={'+vals+'}; for(int i=0;i<sizeof(v)/sizeof(v[0]);i++){volatile '+src+' x=v[i]; check("'+op+'",'+call+'((U)x,0),'+pack+'(('+typ+')x));}}')
            elif op in ('cvtdi', 'cvtdu'):
                vals = '-42.75,0,42.75,9007199254740992.0' if op == 'cvtdi' else '0,42.75,9223372036854775808.0,18446744073709549568.0'
                cast = '(U)(long long)x' if op == 'cvtdi' else '(U)x'
                checks.append('{ double v[]={'+vals+'}; for(int i=0;i<sizeof(v)/sizeof(v[0]);i++){volatile double x=v[i];check("'+op+'",'+call+'(d(x),0),'+cast+');}}')
            elif op in ('cvtsd', 'cvtds'):
                typ, src, dst, cast = ('float','s','d','double') if op == 'cvtsd' else ('double','d','s','float')
                checks.append('{ '+typ+' v[]={0,-0.0,1.5,-2.125,1.0/3};for(int i=0;i<5;i++){volatile '+typ+' x=v[i];check("'+op+'",'+call+'('+src+'(x),0),'+dst+'(('+cast+')x));}}')
            else:
                checks.append('{ '+typ+' v[]={0,4,2,16};for(int i=0;i<4;i++){volatile '+typ+' x=v[i];check("'+op+'",'+call+'('+pack+'(x),0),'+pack+'('+('sqrt' if w=='64' else 'sqrtf')+'(x)));}}')
        if not darwin:
            asm.append('.section .note.GNU-stack,"",@progbits')
        (p/'code.s').write_text('\n'.join(asm)+'\n')
        source = '''#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>
typedef unsigned long long U;
static U d(double x){ U u; memcpy(&u,&x,8);return u; }
static U s(float x){ uint32_t u;memcpy(&u,&x,4);return u; }
static int n,bad;
static void check(char *op,U got,U want){n++;if(got!=want){bad++;printf("FAIL %s %llx != %llx\\n",op,got,want);}}
'''+ '\n'.join(decls)+'\nint main(void){\n'+'\n'.join(checks)+'\nprintf("fp execution cases %d bad %d\\n",n,bad);return bad!=0 || n==0;}\n'
        (p/'check.c').write_text(source)
        run(['cc', *(['-arch','arm64' if arm else 'x86_64'] if darwin else []), '-O2', '-fno-fast-math', str(p/'check.c'),str(p/'code.s'),'-lm','-o',str(p/'check')])
        run([str(p/'check')])
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

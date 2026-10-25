#!/usr/bin/env python3
"""Witness-callee differential harness for BNK1 plans (R12-1 ①; SysV x86_64 first).

For each fixture: the oracle emits the plan; a generated C program defines the real
callee (which copies every argument's bytes into a witness buffer and returns a value
derived from them), calls it directly, then calls it again through the plan
(tests/bankplan.h moves + tests/bankgateway_x86_64.S) and compares witness bytes and
results, 100 iterations, at O0/O1/O2 with ASan/UBSan. Runs under the x86_64 personality
of this machine (Rosetta) or natively on an x86_64 host.

usage: bankcheck.py --arch x86_64 [--evidence OUT.json]
"""
import argparse, json, pathlib, platform, subprocess, sys, tempfile, hashlib
ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests'))
import bankplan_oracle as O

# fixture: name, profile, C param types (spelling, oracle type, size, 'slotval'|'bytes'), result (spelling, oracle, size), preceding pattern
CTYPES = {
    'i64': ('long', O.I(8, True), 8, 'val'), 'i32': ('int', O.I(4, True), 4, 'val'), 'u8': ('unsigned char', O.I(1), 1, 'val'),
    'f64': ('double', O.F(8), 8, 'val'), 'f32': ('float', O.F(4), 4, 'val'),
    'P5': ('P5', O.AGG(5, 1, memory=True), 5, 'bytes'),
    'UD': ('UD', O.AGG(16, 8, lanes=['INT', 'SSE']), 16, 'bytes'),
    'DU': ('DU', O.AGG(16, 8, lanes=['SSE', 'INT']), 16, 'bytes'),
    'II': ('II', O.AGG(16, 8, lanes=['INT', 'INT']), 16, 'bytes'),
    'B24': ('B24', O.AGG(24, 8, memory=True), 24, 'bytes'),
    # AAPCS64 spellings of the same C objects (classification differs per ABI)
    'a:UD': ('UD', O.AGG(16, 8), 16, 'bytes'), 'a:DU': ('DU', O.AGG(16, 8), 16, 'bytes'), 'a:II': ('II', O.AGG(16, 8), 16, 'bytes'),
    'a:P5': ('P5', O.AGG(5, 1), 5, 'bytes'), 'a:B24': ('B24', O.AGG(24, 8), 24, 'bytes'), 'a:B40': ('B40', O.AGG(40, 8), 40, 'bytes'),
    'a:HD2': ('HD2', O.AGG(16, 8, hfa=(2, 8)), 16, 'bytes'), 'a:HD3': ('HD3', O.AGG(24, 8, hfa=(3, 8)), 24, 'bytes'),
    'a:HF4': ('HF4', O.AGG(16, 4, hfa=(4, 4)), 16, 'bytes'), 'a:Q16': ('Q16', O.AGG(16, 16), 16, 'bytes'),
}
TYPEDEFS = '''
typedef struct __attribute__((packed)) { char c; int i; } P5;
typedef struct { unsigned long long a; double b; } UD;
typedef struct { double a; unsigned long long b; } DU;
typedef struct { unsigned long long a, b; } II;
typedef struct { unsigned long long a, b, c; } B24;
typedef struct { unsigned long long a, b, c, d, e; } B40;
typedef struct { double a, b; } HD2;
typedef struct { double a, b, c; } HD3;
typedef struct { float a, b, c, d; } HF4;
typedef struct { _Alignas(16) unsigned long long a, b; } Q16;
'''
FIXTURES_ARM64 = [  # AAPCS64 (macOS arm64 native): (name, params, result)
    ('A64-hfa-regs', ['f64'] * 6 + ['a:HD2', 'i64'], 'i64'),
    ('A64-hfa-all-or-nothing', ['f64'] * 6 + ['a:HD2', 'a:HD3', 'i64'], 'i64'),
    ('A64-hfa-return', ['i32'], 'a:HD3'),
    ('A64-hf4-return', ['f32', 'i32'], 'a:HF4'),
    ('A64-byref-40', ['i64', 'a:B40', 'i64'], 'i64'),
    ('A64-x8-return', ['i64', 'a:B24'], 'a:B24'),
    ('A64-gp-exhausted', ['i64'] * 7 + ['a:II', 'i64'], 'i64'),
    ('A64-packed-5', ['i64', 'a:P5', 'f64'], 'i32'),
    ('A64-mixed', ['a:UD', 'a:DU', 'f64', 'i64'], 'a:UD'),
]
FIXTURES = [  # SysV x86_64: (name, params, result)
    ('A-alone', ['P5', 'i64', 'f64'], 'i32'),
    ('A-after5gp', ['i64'] * 5 + ['P5', 'i64', 'f64'], 'i32'),
    ('A-after7gp', ['i64'] * 7 + ['P5', 'i64', 'f64'], 'i32'),
    ('A-return', ['i32', 'f64'], 'P5'),
    ('B-gp-exhausted', ['i64'] * 6 + ['UD', 'f64'], 'i64'),
    ('B-sse-exhausted', ['f64'] * 8 + ['DU', 'i64'], 'i64'),
    ('B-both-available', ['UD', 'DU', 'II'], 'UD'),
    ('B-mixed-return', ['i32', 'UD'], 'DU'),
    ('M-memory-return', ['i64', 'B24'], 'B24'),
    ('S-scalars', ['u8', 'i32', 'f32', 'f64', 'i64', 'u8', 'f32'], 'f64'),
]

def gen_c(name, params, result, plan_bytes, arch='x86_64'):
    ptys = [CTYPES[p] for p in params]; rty = CTYPES[result]
    args = ', '.join(f'{t[0]} a{i}' for i, t in enumerate(ptys))
    copies = ''.join(f'  memcpy(witness+wo[{i}], &a{i}, sizeof a{i});\n' for i in range(len(ptys)))
    # result: derive from witness bytes so any wrong argument changes the result too
    rexpr = 'sum'
    ret = {'i32': '(int)sum', 'i64': '(long)sum', 'f64': '(double)sum*0.5', 'f32': '(float)sum*0.25', 'u8': '(unsigned char)sum'}.get(result)
    if ret is None:  # aggregate result: fill bytes from sum
        ret = None
    body = f'''
static unsigned char witness[512];static const unsigned wo[{max(1,len(ptys))}]={{{','.join(str(16*i) for i in range(len(ptys))) or '0'}}};
static unsigned long long sum_witness(void){{unsigned long long s=1469598103934665603ull;for(unsigned i=0;i<sizeof witness;i++){{s^=witness[i];s*=1099511628211ull;}}return s;}}
static __attribute__((noinline)) {rty[0]} callee({args}){{
{copies}  unsigned long long sum=sum_witness();
'''
    if ret is not None: body += f'  return {ret};\n}}\n'
    else: body += f'  {rty[0]} r;memset(&r,0,sizeof r);for(unsigned i=0;i<sizeof r;i++)((unsigned char*)&r)[i]=(unsigned char)(sum>>(8*(i%8)))^(unsigned char)i;return r;\n}}\n'
    plan_arr = ','.join(str(b) for b in plan_bytes)
    # argument construction per iteration
    setup = ''; slots = ''; sizes = ', '.join(str(t[2]) for t in ptys) or '0'
    for i, (sp, ot, sz, mode) in enumerate(ptys):
        setup += f'  {sp} a{i}; memset(&a{i},0,sizeof a{i}); for(unsigned b=0;b<sizeof a{i};b++)((unsigned char*)&a{i})[b]=(unsigned char)(it*7+{i}*13+b*3+1);\n'
        if sp == 'float': setup += f'  a{i}=(float)(it*1.5+{i});\n'
        if sp == 'double': setup += f'  a{i}=it*2.25+{i};\n'
        if mode == 'val':
            if sp in ('double',): slots += f'  memcpy(&slots[{i}],&a{i},8);\n'
            elif sp == 'float': slots += f'  slots[{i}]=0;memcpy(&slots[{i}],&a{i},4);\n'
            else: slots += f'  slots[{i}]=(unsigned long long)(long long)a{i};\n'
        else: slots += f'  slots[{i}]=(unsigned long long)(uintptr_t)&a{i};\n'
    call_args = ', '.join(f'a{i}' for i in range(len(ptys)))
    return f'''#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "tests/bankplan.h"
{TYPEDEFS}
void us_bank_call_{arch}(const us_bank_frame *f, void *fn);
{body}
static const unsigned char plan[]={{{plan_arr}}};
int main(void){{
  us_bank_plan p;int rc=us_bank_plan_load(&p,plan,sizeof plan);if(rc){{printf("{name}: plan rejected rule %d\\n",rc);return 2;}}
  static unsigned char stackimg[512],scratch[1024];static unsigned char w1[512],w2[512];
  for(unsigned it=0;it<100;it++){{
{setup}    memset(witness,0,sizeof witness);
    {rty[0]} r1=callee({call_args});memcpy(w1,witness,sizeof witness);
    unsigned long long slots[{max(1,len(ptys))}];size_t sizes[]={{{sizes}}};
{slots}    us_bank_frame f;{rty[0]} r2;memset(&r2,0,sizeof r2);
    rc=us_bank_plan_apply(&p,slots,sizes,&f,stackimg,scratch,&r2);if(rc){{printf("{name}: apply failed %d\\n",rc);return 3;}}
    memset(witness,0,sizeof witness);
    us_bank_call_{arch}(&f,(void*)callee);
    if(!(p.flags&1))us_bank_plan_capture(&p,&f,&r2);
    memcpy(w2,witness,sizeof witness);
    if(memcmp(w1,w2,sizeof w1)){{printf("{name}: witness differs at iteration %u\\n",it);for(unsigned i=0;i<sizeof w1;i++)if(w1[i]!=w2[i]){{printf("  byte %u: direct %02x bank %02x\\n",i,w1[i],w2[i]);break;}}return 4;}}
    if(memcmp(&r1,&r2,sizeof r1)){{printf("{name}: result differs at iteration %u\\n",it);return 5;}}
  }}
  printf("{name}: 100 iterations, witness and result identical\\n");return 0;
}}
'''

def main():
    ap = argparse.ArgumentParser(); ap.add_argument('--arch', default='x86_64', choices=['x86_64', 'arm64']); ap.add_argument('--evidence', type=pathlib.Path); a = ap.parse_args()
    osname = 'lnx' if platform.system() == 'Linux' else 'osx'; profile = f'{osname}/{a.arch}'
    invoke = ['arch', '-' + a.arch] if platform.system() == 'Darwin' else []
    flags = ['-arch', a.arch] if platform.system() == 'Darwin' else []
    fixtures = FIXTURES if a.arch == 'x86_64' else FIXTURES_ARM64
    gateway = ROOT / f'tests/bankgateway_{a.arch}.S'
    ev = {'schema': 1, 'status': 'failed', 'profile': profile, 'host': f'{platform.system()}/{platform.machine()}', 'fixtures': {}, 'gateway_sha256': hashlib.sha256(gateway.read_bytes()).hexdigest()}
    with tempfile.TemporaryDirectory(prefix='r12-bank-') as td:
        t = pathlib.Path(td); bad = 0
        # negative control: the oracle must refuse what has not been probed on this ABI
        try:
            O.plan(profile, [O.AGG(16, 16)], O.I(8), b'refuse'); print('REFUSAL MISSING: 16-aligned composite'); bad += 1
        except RuntimeError as e:
            ev['refusals'] = {'16-aligned composite': str(e)}
        for name, params, result in fixtures:
            ptys = [CTYPES[p][1] for p in params]; rty = CTYPES[result][1]
            rec, alloc = O.plan(profile, ptys, rty, ('proto:' + name).encode())
            src = t / f'{name}.c'; src.write_text(gen_c(name, params, result, rec, a.arch)); runs = {}
            for opt in ('-O0', '-O1', '-O2'):
                exe = t / f'{name}{opt}'
                c = subprocess.run(['cc', *flags, '-std=c11', opt, '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-w', '-I', str(ROOT), str(src), str(gateway), '-o', str(exe)], capture_output=True, text=True, timeout=60)
                if c.returncode: runs[opt] = 'cc failed: ' + c.stderr[-300:]; bad += 1; continue
                r = subprocess.run([str(ROOT / 'tests/bound'), '20', *invoke, str(exe)], capture_output=True, text=True, timeout=25)
                runs[opt] = (r.stdout + r.stderr).strip()[-300:]; bad += r.returncode != 0
            ev['fixtures'][name] = {'moves': O.describe(alloc), 'stack': alloc['stack'], 'al': alloc['al'], 'runs': runs}
            print(name, '|', ' / '.join(f"{k}: {v.splitlines()[-1] if v else ''}" for k, v in runs.items()))
        ev['status'] = 'passed' if not bad else 'failed'
    if a.evidence: a.evidence.write_text(json.dumps(ev, indent=1))
    print('bankcheck', a.arch, ev['status'], len(fixtures), 'fixtures x 3 optimisation levels')
    return 0 if not bad else 1

if __name__ == '__main__':
    sys.exit(main())

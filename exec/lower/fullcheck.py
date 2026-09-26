#!/usr/bin/env python3
"""Full Linux lowering referee: all typed fields, code and labels; no filtering."""
import os,pathlib,subprocess,sys,tempfile
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[2]))
from unisa.tape import parse
from unisa.lower import lower
from unisa.__main__ import _oracle
from exec.enc.tins import parse as parse_tins


def same(a,b):
    if type(a)!=type(b):return False
    if isinstance(a,dict):return a.keys()==b.keys() and all(same(a[k],b[k]) for k in a)
    if isinstance(a,(list,tuple)):return len(a)==len(b) and all(same(x,y) for x,y in zip(a,b))
    return a==b


def check(raw,out,oracle):
    ref=lower(parse(raw),os.environ.get('LOWER_TARGET','lnx/x86_64'),oracle,drive='built');got=parse_tins(out)
    assert len(got.data)<=len(ref.data) and got.data==ref.data[:len(got.data)] and not any(ref.data[len(got.data):]),'data differs'
    assert same({k:v for k,v in vars(ref).items() if k not in ('code','data')},{k:v for k,v in vars(got).items() if k not in ('code','data')}),'layout/labels differ'
    assert len(ref.code)==len(got.code),'instruction count differs'
    for i,(a,b) in enumerate(zip(ref.code,got.code)):
        assert a.op==b.op and same(list(a.args),list(b.args)) and same(a.meta,b.meta),(i,vars(a),vars(b))
    return len(ref.code)


def main():
    if len(sys.argv)<5:raise SystemExit('usage: fullcheck.py RUN TABLE JSON TAPE...')
    oracle=_oracle('built')
    with tempfile.TemporaryDirectory() as d:
        p=pathlib.Path(d)/'input'
        # Fusion blockers, multiple labels, entry not first, syscall argument aliasing.
        fixture='f:\n  ret\n_start:\nsecond:\n  .frame 8\n  store64 [r7+0], r0\n  load64 r1, [r7+0]\n  .frame -8\n  .frame 8\nbarrier:\n  store64 [r7+0], r2\n  .sys write, r2, r1, r0\n  .sys6 mmap, r0, r1, r2, r3, r4, r5\n  .exit r0\n'
        if os.environ.get('LOWER_TARGET','').endswith('/arm64'):
            fixture += ''.join('  .sys '+op+', r0, r1, r2\n' for op in ('open','unlink','rename'))
            for width in (1,2,4,8):
                for barrier in ('','blocked'+str(width)+':\n'):
                    fixture += f'  .frame 8\n  .st [r7+0], r2, {width}\n{barrier}  .ld r1, [r7+0], {width}\n  .frame -8\n'
        cases=[('fixture',fixture)]+[(f,pathlib.Path(f).read_text()) for f in sys.argv[4:]]
        if os.environ.get('LOWER_TARGET','').endswith('/arm64'):
            # Immediate limits, power-of-two multiply, aliasing and liveness.
            chunks=['_start:']
            for op in ('add64','sub64','mul64'):
                for value in (0,1,-1,4095,4096,-4095,-4096,8,3,9223372036854775808,-9223372036854775808):
                    chunks += [f'imm r2, {value}',f'{op} r2, r1, r2']
            chunks += ['imm r2, 3','add64 r0, r1, r2','imm r2, 9',
                       'imm r2, 3','add64 r0, r1, r2','mov r3, r2',
                       'imm r2, 3','add64 r0, r1, r2','barrier:','imm r2, 9',
                       'imm r2, 3','pairbarrier:','add64 r2, r1, r2',
                       'imm r2, 3','add64 r2, r2, r2']
            for count in (31,32):
                chunks += ['imm r2, 3','add64 r0, r1, r2']+['mov r4, r5']*count+['imm r2, 9']
            chunks += ['ret']
            cases.insert(1,('immediate','\n'.join(chunks)+'\n'))

        for name,raw in cases:
            p.write_text(raw)
            commands=[[sys.argv[1],sys.argv[2],str(p)]]
            if name in ('fixture','immediate'):commands.append([sys.executable,'exec/pp/sim.py',sys.argv[3],str(p)])
            for cmd in commands:
                r=subprocess.run(cmd,capture_output=True,timeout=60)
                if r.returncode:raise RuntimeError((name,r.returncode,r.stderr))
                n=check(raw,r.stdout.decode(),oracle)
                if name=='fixture' and os.environ.get('LOWER_TARGET','').endswith('/arm64'):
                    sexts=[tuple(i.args) for i in parse_tins(r.stdout.decode()).code if i.op=='sext']
                    assert sexts==[('x1','x2',1),('x1','x2',2),('x1','x2',4)],sexts
            print('lower full',name,n,'instructions equal; executors',len(commands),flush=True)
if __name__=='__main__':main()

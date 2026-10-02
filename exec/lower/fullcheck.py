#!/usr/bin/env python3
"""Typed lowering referee: all typed fields, code and labels; no filtering."""
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
        assert a.op==b.op and same(list(a.args),list(b.args)) and same(a.meta,b.meta),(i,repr(a),repr(b))
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
        target=os.environ.get('LOWER_TARGET','lnx/x86_64')
        if target.startswith('lnx/'):
            fixture += ''.join('  .sys '+op+', r0, r1, r2\n' for op in (('ioctl','fstat','stat','lstat','fcntl','mkdir','chmod','ppoll') if target.endswith('/arm64') else ('ioctl','fstat','stat','lstat','fcntl','mkdir','chmod','poll','ppoll')))
        elif target.startswith('osx/'):
            fixture += ''.join('  .sys '+op+', r0, r1, r2\n' for op in ('ioctl','fstat','stat','lstat','fcntl','mkdir','chmod','poll'))
            fixture += '  .sys6 getdirentries64, r0, r1, r2, r3, r4, r5\n'
        if target.startswith(('lnx/', 'osx/')):
            fixture += '  .sys6 syscall, r0, r1, r2, r3, r4, r5\n'
        unavailable=[]
        if target.startswith('win/'):
            from unisa.catalog import WINAPI, winimp
            arch=target.split('/')[1]
            supported=[op for op in WINAPI if WINAPI[op] is not None and winimp(op,'win',arch)!='none']
            unavailable=[op for op in WINAPI if op not in supported]
            assert supported and unavailable, 'Windows import fixture classes must be nonempty'
            fixture += ''.join('  .sys '+op+', r0, r1, r2\n' for op in supported)
        fixture += '  .print r0\n  .print r3\n'
        regnames=('r0:\n  ret\nr1:\n  ret\nr7:\n  ret\n_start:\n'
                  '  call r1\n  jump r0\n  jumpz r2, r7\n'
                  '  .lea r3, r1\n  ret\n')
        cases=[('fixture',fixture),('regnames',regnames)]+[(f,pathlib.Path(f).read_text()) for f in sys.argv[4:]]
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
            if name in ('fixture','immediate','regnames'):commands.append([sys.executable,'exec/pp/sim.py',sys.argv[3],str(p)])
            for cmd in commands:
                r=subprocess.run(cmd,capture_output=True,timeout=60)
                if r.returncode:raise RuntimeError((name,r.returncode,r.stderr))
                n=check(raw,r.stdout.decode(),oracle)
                if name=='regnames':
                    code=parse_tins(r.stdout.decode()).code
                    assert any(i.op=='call' and i.args==['r1'] for i in code), 'call target became a register'
                    assert any(i.op=='jump' and i.args==['r0'] for i in code), 'jump target became a register'
                    assert any(i.op=='jumpz' and i.args[1]=='r7' for i in code), 'jumpz target became a register'
                    assert any(i.op=='.lea' and i.args[1]=='r1' for i in code), 'address name became a register'
                if name=='fixture' and os.environ.get('LOWER_TARGET','').endswith('/arm64'):
                    sexts=[tuple(i.args) for i in parse_tins(r.stdout.decode()).code if i.op=='sext']
                    assert sexts==[('x1','x2',1),('x1','x2',2),('x1','x2',4)],sexts
            print('lower full',name,n,'instructions equal; executors',len(commands),flush=True)
        # Planned API names with no declared import are refusals, not positives.
        # Keep every such entry checked; neither a crash nor an acceptance passes.
        for op in unavailable:
            raw='_start:\n  .sys '+op+', r0, r1, r2\n  ret\n'
            try: lower(parse(raw),target,oracle,drive='built')
            except ValueError as error:
                assert str(error)==op+': no Windows import for this op', (op,str(error))
            else: raise AssertionError('reference accepted missing Windows import: '+op)
            p.write_text(raw)
            for cmd in ([sys.argv[1],sys.argv[2],str(p)],
                        [sys.executable,'exec/pp/sim.py',sys.argv[3],str(p)]):
                r=subprocess.run(cmd,capture_output=True,timeout=60)
                assert r.returncode==1 and not r.stdout and r.stderr.startswith(b'reject: not covered:'), (op,r.returncode,r.stdout,r.stderr)
            print('lower unavailable',target,op,'reference + both executors rejected',flush=True)
        if os.environ.get('LOWER_TARGET','').endswith('/arm64'):
            subprocess.run([sys.executable, 'exec/lower/largecodecheck.py', sys.argv[1], sys.argv[2]],
                           check=True, timeout=45)
if __name__=='__main__':main()

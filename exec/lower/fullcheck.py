#!/usr/bin/env python3
"""Full Linux lowering referee: all typed fields, code and labels; no filtering."""
import pathlib,subprocess,sys,tempfile
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
    ref=lower(parse(raw),'lnx/x86_64',oracle,drive='built');got=parse_tins(out)
    assert same({k:v for k,v in vars(ref).items() if k!='code'},{k:v for k,v in vars(got).items() if k!='code'}),'layout/labels differ'
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
        cases=[('fixture',fixture)]+[(f,pathlib.Path(f).read_text()) for f in sys.argv[4:]]
        for name,raw in cases:
            p.write_text(raw)
            commands=[[sys.argv[1],sys.argv[2],str(p)]]
            if name=='fixture':commands.append([sys.executable,'exec/pp/sim.py',sys.argv[3],str(p)])
            for cmd in commands:
                r=subprocess.run(cmd,capture_output=True,timeout=60)
                if r.returncode:raise RuntimeError((name,r.returncode,r.stderr))
                n=check(raw,r.stdout.decode(),oracle)
            print('lower full',name,n,'instructions equal; executors',len(commands),flush=True)
if __name__=='__main__':main()

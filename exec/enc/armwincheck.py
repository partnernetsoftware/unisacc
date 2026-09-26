"""Windows ARM text: complete typed fixture, both runtimes, explicit rejects."""
import pathlib,subprocess,sys,tempfile
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[2]))
from unisa.__main__ import _oracle
from unisa.lower import lower
from unisa.tape import parse
from unisa.assemble import assemble
from tins import dump

def main():
    assert len(sys.argv)==4
    oracle=_oracle('built')
    ops=('exit','read','write','mmap','mprotect','munmap','close','open','lseek','unlink','rename')
    raw='_start:\n'+''.join('.sys '+op+', r0, r1, r2\n' for op in ops)+'.sys6 mmap, r0, r1, r2, r3, r4, r5\nret\n'
    tp=lower(parse(raw),'win/arm64',oracle,drive='built')
    src=dump(tp,full=True);want,stats=assemble(tp)
    assert stats['encoded']==stats['insns']
    cmds=[[sys.argv[1],sys.argv[2]],[sys.executable,'exec/pp/sim.py',sys.argv[3]]]
    with tempfile.TemporaryDirectory() as d:
        f=pathlib.Path(d)/'in';f.write_text(src)
        for cmd in cmds:
            r=subprocess.run(cmd+[str(f)],capture_output=True,timeout=60)
            if r.returncode or r.stdout!=want:
                at=next((i for i,(a,b) in enumerate(zip(r.stdout,want)) if a!=b),min(len(want),len(r.stdout)))
                raise AssertionError((cmd,r.returncode,r.stderr,at,len(want),len(r.stdout)))
        print('ARM Windows setup + 11 API bodies + return conversions:',stats['insns'],'instructions',len(want),'bytes; both executors',flush=True)
        prefix='@target win/arm64\n'
        base='gate form=winapi gate=winapi catop=write winimp=WriteFile retconv=wcount hstd=256 written=280'
        bad=['winrest 256, x8',base.replace('WriteFile','Missing'),base.replace('catop=write','catop=unknown'),base.replace('wcount','unknown'),base.replace('hstd=256','hstd=2147483648'),base.replace('written=280','written=-1'),base+' written=280']
        for b in bad:
            f.write_text(prefix+b+'\n')
            for cmd in cmds:
                r=subprocess.run(cmd+[str(f)],capture_output=True,timeout=60)
                assert r.returncode==1 and not r.stdout and b'not covered' in r.stderr,(b,r.returncode,r.stderr)
        print('ARM Windows:',len(bad),'invalid contracts rejected; both executors')
if __name__=='__main__':main()

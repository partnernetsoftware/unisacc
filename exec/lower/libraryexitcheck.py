#!/usr/bin/env python3
"""Lowering-only library exit boundary: table simulator, network and full-domain equality.
No library host execution is claimed here; the library harness supplies that evidence.
Usage: libraryexitcheck.py RUN JSON TARGET
"""
import json, pathlib, subprocess, sys, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/c')]
from exec.pp.sim import run as simulate
from pack import build

def command(args):
    p=subprocess.run(args,capture_output=True,timeout=55)
    if p.returncode: raise RuntimeError((args,p.returncode,p.stderr.decode()))
    return p.stdout

class Resources:
    def __init__(self, value): self.value=value
    def get(self,key): return self.value if key==b'\0library/exit' else None

def main():
    if len(sys.argv)!=4: raise SystemExit(__doc__)
    runtime, model, target=sys.argv[1:]
    d=json.loads(pathlib.Path(model).read_text())
    with tempfile.TemporaryDirectory(prefix='unisa-libraryexit-') as name:
        t=pathlib.Path(name); tbl=t/'m.tbl'; net=t/'m.net'
        command([sys.executable,'exec/c/tbl.py',model,str(tbl)])
        command([sys.executable,'exec/c/net.py',str(tbl),str(net)])
        command([runtime,'--check-net',str(tbl),str(net)])
        manifest=t/'routes.tsv'; manifest.write_text('test\tlower\ttape\ttins\tm.net\n')
        resource=t/'resources'; resource.mkdir(); package=t/'m.pkg'; source=t/'input'
        cases=['.exit r0','.sys exit, r0, r1, r2','.sys6 exit, r0, r1, r2, r3, r4, r5',
               '.sys6 exit_group, r0, r1, r2, r3, r4, r5']
        n=0
        for op in cases:
            raw=('_start:\nimm r0, 37\n'+op+'\n').encode();source.write_bytes(raw)
            previous=None
            for value in (None,bytes(8),(0x123456789abcd).to_bytes(8,'little'),b'bad'):
                status,out,_=simulate(d,raw,'input',files=Resources(value),maxsteps=1000000)
                (resource/'exit').write_bytes(value or bytes(8))
                package.write_bytes(build([manifest],[] if value is None else [('006c6962726172792f',resource)]))
                result=subprocess.run([runtime,'--bundle',str(package),'test',str(source)],capture_output=True,timeout=55)
                assert (result.returncode==0)==(status=='accept'),(op,value,status,result.returncode,result.stderr)
                if value not in (None, bytes(8)) and (len(value)!=8 or target.split('/')[0] not in ('osx','lnx','win')):
                    assert status=='reject' and result.returncode==1,'invalid/unsupported callback must reject'
                if status=='accept':
                    assert result.stdout==out,(op,value,'table/network difference')
                    if value is None: previous=out
                    elif value==bytes(8): assert out==previous,'zero resource changes ordinary output'
                    else:
                        assert b'320255973501901' in out and b'hostcall ' in out,'callback address/bridge missing'
                        assert b'gate ' not in out,'library exit still emits process gate'
                        resultreg=b'x0' if target.endswith('/arm64') else b'rax'
                        complete=b'hostcall '+(b'x1' if target.endswith('/arm64') else b'rdi')+b', '+resultreg+b'\n'
                        if op.startswith('.sys'):
                            complete+=b'mov '+resultreg+b', '+resultreg+b'\n'
                        assert complete in out,('library exit return register absent/incomplete',out)
                        assert all(not line.endswith(b', ') for line in out.splitlines()),'empty operand in library exit output'
                    if value is None and 'exit_group' not in op:
                        assert command([runtime,str(tbl),str(source)])==out,'C table differs'
                else: assert result.returncode==1,(status,result.stderr)
                n+=1
        if target.startswith('win/'):
            # The ordinary Windows host bridge needs the conditional IAT route.
            # Refuse it by name until that route is byte-equal to the reference.
            for raw in (b'_start:\n.hostcall r1, r0\n', b'_start:\n.hostaddr r0, 0\n'):
                source.write_bytes(raw)
                status,_,_=simulate(d,raw,'input',files=Resources(None),maxsteps=1000000)
                result=subprocess.run([runtime,str(tbl),str(source)],capture_output=True,timeout=55)
                assert status=='reject' and result.returncode==1,(raw,status,result.returncode)
                assert b'Windows .hostcall/.hostaddr forwarding' in result.stderr,result.stderr
                n+=1
        print('libraryexit:',target,n,'table/network verdicts; full-domain check-net; callback host execution separate')

if __name__=='__main__': main()

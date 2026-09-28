#!/usr/bin/env python3
"""Library-only Linux/Windows integer ABI bridge; table and network resource controls.
This checks encoded bytes, not real host callback execution.
Usage: libraryexitcheck.py RUN JSON ARCH
"""
import json,pathlib,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/c')]
from exec.pp.sim import run as simulate
from pack import build

def command(args):
    p=subprocess.run(args,capture_output=True,timeout=55)
    if p.returncode:raise RuntimeError((args,p.returncode,p.stderr.decode()))
    return p.stdout

class Resources:
    def __init__(self,value,callback='exit'):self.value=value;self.callback=callback
    def get(self,key):return self.value if key==('\0library/'+self.callback).encode() else None

def main():
    if len(sys.argv)!=4:raise SystemExit(__doc__)
    runtime,model,arch=sys.argv[1:];d=json.loads(pathlib.Path(model).read_text())
    regs=('x1','x0') if arch=='arm64' else ('rdi','rax')
    with tempfile.TemporaryDirectory(prefix='unisa-librarybridge-') as name:
        t=pathlib.Path(name);tbl=t/'m.tbl';net=t/'m.net';source=t/'input';pkg=t/'m.pkg'
        command([sys.executable,'exec/c/tbl.py',model,str(tbl)])
        command([sys.executable,'exec/c/net.py',str(tbl),str(net)])
        command([runtime,'--check-net',str(tbl),str(net)])
        manifest=t/'routes.tsv';manifest.write_text('test\tenc\ttins\tbytes\tm.net\n')
        resource=t/'resources';resource.mkdir();equalbytes=None;n=0
        for os_ in ('osx','lnx','win'):
            for op in ('hostcall','hostaddr'):
                operands=', '.join(regs) if op=='hostcall' else regs[0]+', 0'
                raw=('@target '+os_+'/'+arch+'\n'+op+' '+operands+'\n').encode();source.write_bytes(raw)
                for value in (None,bytes(8),(0x123456789abcd).to_bytes(8,'little'),b'bad'):
                    status,out,_=simulate(d,raw,'input',files=Resources(value),maxsteps=2000000)
                    (resource/'exit').write_bytes(value or bytes(8))
                    pkg.write_bytes(build([manifest],[] if value is None else [('006c6962726172792f',resource)]))
                    got=subprocess.run([runtime,'--bundle',str(pkg),'test',str(source)],capture_output=True,timeout=55)
                    accept=os_=='osx' or (os_ in ('lnx','win') and op=='hostcall' and value is not None and len(value)==8 and int.from_bytes(value,'little')!=0)
                    assert (status=='accept')==accept,(os_,op,value,status)
                    assert got.returncode==(0 if accept else 1),(os_,op,value,got.returncode,got.stderr)
                    if accept:
                        assert got.stdout==out,'table/network bytes differ'
                        if op=='hostcall' and os_=='win' and arch=='x86_64':
                            from unisa.hostabi import WIN_X86_BODY
                            assert out.endswith(WIN_X86_BODY), 'Win64 ABI declaration missing'
                        if op=='hostcall' and (os_!='win' or arch=='arm64'):
                            if equalbytes is None:equalbytes=out
                            assert out==equalbytes,'Linux/macOS integer ABI bridge differs'
                        if value is None:assert command([runtime,str(tbl),str(source)])==out,'C table differs'
                    n+=1
        raw=('@target lnx/'+arch+'\n'+'hostcall '+', '.join(regs)+'\n').encode();source.write_bytes(raw)
        for callback in ('mmap','munmap'):
            for value in (None,bytes(8),(0x123456789abcd).to_bytes(8,'little'),b'bad'):
                for p in resource.iterdir():p.unlink()
                (resource/callback).write_bytes(value or bytes(8))
                status,out,_=simulate(d,raw,'input',files=Resources(value,callback),maxsteps=2000000)
                pkg.write_bytes(build([manifest],[] if value is None else [('006c6962726172792f',resource)]))
                got=subprocess.run([runtime,'--bundle',str(pkg),'test',str(source)],capture_output=True,timeout=55)
                accept=value is not None and len(value)==8 and int.from_bytes(value,'little')!=0
                assert (status=='accept')==accept and got.returncode==(0 if accept else 1),(callback,value,status,got.returncode)
                if accept:assert out==got.stdout==equalbytes,'mapping bridge bytes differ'
                n+=1
        print('librarybridge:',arch,n,'resource verdicts; Windows library-only ABI route, Linux/macOS bridge equal; check-net full domain')

if __name__=='__main__':main()

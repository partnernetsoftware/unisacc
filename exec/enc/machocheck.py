"""Full Mach-O byte/signature oracle; Python lowering is test input only."""
import hashlib,pathlib,struct,subprocess,sys,tempfile,platform
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[2]))
from unisa.lower import TargetProgram,lower
from unisa.assemble import assemble
from unisa import image
from unisa.__main__ import _oracle
from unisa.driver import compile_file
from unisa.tape import DATA_BASE
from tins import dump

def main():
    arch=sys.argv[4] if len(sys.argv)>4 else 'arm64'
    assert arch in ('arm64','x86_64')
    target='osx/'+arch
    with tempfile.TemporaryDirectory() as t:
        p=pathlib.Path(t);inp=p/'in'
        def verify(tp,name,both=False):
            inp.write_text(dump(tp,full=True))
            code,st=assemble(tp);assert st['encoded']==st['insns']
            want=image.build(tp,code,image.relocate(tp,tp.data,st['data_va']-DATA_BASE),st['entry'])
            cmds=[[sys.argv[1],sys.argv[2],str(inp)]]
            if both:cmds.append([sys.executable,'exec/pp/sim.py',sys.argv[3],str(inp)])
            for cmd in cmds:
                r=subprocess.run(cmd,capture_output=True,timeout=60)
                assert r.returncode==0 and r.stdout==want,(name,r.returncode,r.stderr,len(r.stdout),len(want),next((i for i,(a,b) in enumerate(zip(r.stdout,want)) if a!=b),None))
            # Decode signature command and validate every page digest independently.
            off=32;sig=None
            for _ in range(struct.unpack_from('<I',want,16)[0]):
                c,n=struct.unpack_from('<II',want,off)
                if c==0x1d:sig=struct.unpack_from('<II',want,off+8)
                off+=n
            assert sig;so,sl=sig;assert so+sl==len(want)
            cd=so+20;ho=struct.unpack_from('>I',want,cd+16)[0];slots=struct.unpack_from('>I',want,cd+28)[0]
            assert slots==(so+4095)//4096
            for i in range(slots):assert want[cd+ho+32*i:cd+ho+32*(i+1)]==hashlib.sha256(want[i*4096:min((i+1)*4096,so)]).digest()
            print('Mach-O',name,len(want),'bytes equal; pages',slots,'executors',len(cmds),flush=True)
            return want
        for n in (0,16,16385):
            tp=TargetProgram(target,b'q'*n,{})
            tp.emit('nop');tp.labels={'_start':1};tp.emit('ret')
            verify(tp,'data-'+str(n),True)
        # A relocation beyond the old 2 MB DATA/SHA gap catches region aliasing.
        raw=bytearray(b'Q'*2000100);raw[2000000:2000008]=(DATA_BASE+123).to_bytes(8,'little')
        tp=TargetProgram(target,bytes(raw),{});tp.relocs=[2000000];tp.emit('ret')
        verify(tp,'large-data-sha-regions')
        bad=TargetProgram(target,b'',{});bad.emit('ret')
        inp.write_text(dump(bad,full=True).replace('@target '+target,'@target lnx/'+arch,1))
        for cmd in ([sys.argv[1],sys.argv[2],str(inp)],[sys.executable,'exec/pp/sim.py',sys.argv[3],str(inp)]):
            r=subprocess.run(cmd,capture_output=True,timeout=60)
            assert r.returncode==1 and not r.stdout and b'Mach-O input' in r.stderr
        oracle=_oracle('built')
        for name,expected in [('hello',b'hello from C99\n'),('fib',b'55\n')]:
            tp=lower(compile_file(['examples/'+name+'.c'],oracle,target),target,oracle,drive='built')
            b=verify(tp,name);f=p/name;f.write_bytes(b);f.chmod(0o755)
            if sys.platform=='darwin' and (platform.machine()=='arm64' or arch=='x86_64'):
                r=subprocess.run([str(f)],capture_output=True,timeout=10)
                assert (r.returncode,r.stdout,r.stderr)==(0,expected,b''),(name,r)
                print('Mach-O native',name,'passed',flush=True)
if __name__=='__main__':main()

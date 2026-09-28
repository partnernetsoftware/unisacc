#!/usr/bin/env python3
"""Dynamic process slots: table/network and independent TIns execution.
Usage: libraryprocesscheck.py RUN LOWER.json TARGET [--native]
Native wrapper executes two emitted bodies in a separate bounded process.
"""
import ctypes,json,mmap,pathlib,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/c')]
from exec.pp.sim import run as simulate
from exec.enc.tins import parse
from pack import build
class Resources:
 def __init__(self,v):self.v=v
 def get(self,k):return self.v.get(k)
def command(c):
 p=subprocess.run(list(map(str,c)),capture_output=True,timeout=55,cwd=ROOT)
 assert p.returncode==0,(c,p.returncode,p.stderr)
 return p.stdout
def evaluate(tp,memory,initial):
 regs=dict(initial)
 for ins in tp.code:
  a=ins.args
  if ins.op=='ret':break
  if ins.op=='setreg':assert a[1][0]=='imm';regs[a[0]]=a[1][1]
  elif ins.op=='load64':regs[a[0]]=memory[regs[a[1]]+a[2]]
  elif ins.op=='mul64':regs[a[0]]=regs[a[1]]*regs[a[2]]
  elif ins.op=='add64':regs[a[0]]=regs[a[1]]+regs[a[2]]
  else:raise AssertionError(('unexpected dynamic process op',ins))
 return regs

def main():
 runtime,model,target=sys.argv[1:4];delta=json.loads(pathlib.Path(model).read_text());arch=target.split('/')[1]
 from unisa.catalog import REGMAP
 regs={'r'+str(i):v for i,v in enumerate(REGMAP[arch])};out=regs['r0'];index=regs['r1'];address=0x123456789abcdef0;one=(1).to_bytes(8,'little')
 source=f'f:\n  .argc r0\n  .argv r1, r1\n  .argv r0, r0\n  ret\n'.encode()
 with tempfile.TemporaryDirectory(prefix='libraryprocess-') as name:
  t=pathlib.Path(name);tbl=t/'model.tbl';net=t/'model.net';command([sys.executable,'exec/c/tbl.py',model,tbl]);command([sys.executable,'exec/c/net.py',tbl,net]);command([runtime,'--check-net',tbl,net])
  manifest=t/'routes';manifest.write_text('test\tlower\ttape\ttins\tmodel.net\n');resource=t/'resource';resource.mkdir();inp=t/'input';pkg=t/'pkg'
  def execute(values,raw=source):
   for p in resource.iterdir():p.unlink()
   for k,v in values.items():(resource/k.decode().split('/')[-1]).write_bytes(v)
   pkg.write_bytes(build([manifest],[('006c6962726172792f',resource)] if values else []));inp.write_bytes(raw)
   status,tins,_=simulate(delta,raw,'probe',files=Resources(values),maxsteps=1000000)
   p=subprocess.run([runtime,'--bundle',str(pkg),'test',str(inp)],capture_output=True,timeout=55)
   assert (p.returncode==0)==(status=='accept'),(status,p.returncode,p.stderr)
   if status=='accept':assert p.stdout==tins
   return status,tins
  base={b'\0library/module':one,b'\0library/symbols':one};values={**base,b'\0library/process':address.to_bytes(8,'little')}
  status,tins=execute(values);assert status=='accept';tp=parse(tins.decode())
  assert any(i.op=='setreg' and i.args[1]==('imm',address) for i in tp.code)
  for argc,argv in [(2,0x1000),(4,0x2000)]:
   memory={address:argc,address+8:argv,argv+8:9001,argv+argc*8:12345}
   result=evaluate(tp,memory,{index:1});assert result[index]==9001 and result[out]==12345
  # The exact same generated instructions read changed slots, not stale literals.
  for broken in [None,b'',b'1',bytes(8),struct.pack('<Q',3)+b'x']:
   v=dict(base)
   if broken is not None:v[b'\0library/process']=broken
   assert execute(v)[0]=='reject'
  for bad in [b'f:\n.argc r99\nret\n',b'f:\n.argv r0, r99\nret\n',b'f:\n.argc r0, r1\nret\n',b'f:\n.argv r0\nret\n']:
   assert execute(values,bad)[0]=='reject'
  ordinary=b'_start:\n.argc r0\n.argv r1, r0\nret\n';assert execute({},ordinary)==execute({b'\0library/process':b'invalid'},ordinary)
  if '--native' in sys.argv:
   slots=(ctypes.c_long*2)();array1=(ctypes.c_ulong*3)(101,202,303);array2=(ctypes.c_ulong*3)(404,505,606)
   values[b'\0library/process']=ctypes.addressof(slots).to_bytes(8,'little')
   funcs=[];maps=[]
   for raw in [b'f:\n.argc r0\nret\n',b'f:\n.argv r0, r0\nret\n']:
    status,b=execute(values,raw);assert status=='accept';instructions=[i for i in parse(b.decode()).code if i.op!='ret']
    if arch=='arm64':
     from unisa.emit_arm import encode
     machine=b''.join(encode(i,0,{}) for i in instructions)+bytes.fromhex('c0035fd6')
    else:
     from unisa.emit_x86 import encode
     machine=bytes.fromhex('534889f8')+b''.join(encode(i,0,{}) for i in instructions)+b'\x5b\xc3'
    m=mmap.mmap(-1,4096,prot=mmap.PROT_READ|mmap.PROT_WRITE|mmap.PROT_EXEC);m.write(machine);maps.append(m)
    funcs.append(ctypes.CFUNCTYPE(ctypes.c_ulong,ctypes.c_ulong)(ctypes.addressof(ctypes.c_char.from_buffer(m))))
   slots[0]=2;slots[1]=ctypes.addressof(array1);assert funcs[0](0)==2 and funcs[1](1)==202
   slots[0]=3;slots[1]=ctypes.addressof(array2);assert funcs[0](0)==3 and funcs[1](1)==505
   for m in maps:m.close()
  print('libraryprocess:',target,'same TIns observe two slot states; table/network/full-domain; 9 negatives; default unchanged'+('; native two bodies execute after slot update' if '--native' in sys.argv else ''))
if __name__=='__main__':main()

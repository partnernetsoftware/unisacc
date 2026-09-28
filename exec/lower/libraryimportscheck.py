#!/usr/bin/env python3
"""Dedicated lowering capability proof; actual native ABI execution is separate.
Raw .librarycall is trusted E3 output: E3 proves its named signature, while lower
checks wire structure and module capability, not arbitrary register-address ABI."""
import hashlib,json,pathlib,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/c')]
from exec.pp.sim import run as simulate,load
from pack import build
U=lambda n:struct.pack('<Q',n)
D=lambda *v:b''.join(U(x) for x in v)
def binding(desc=None,name=b'hostadd',supported=1):
 desc=desc or D(0,999,777,1,8,0)
 p=U(len(name))+name+bytes(4)+U(4096)+U(2)+desc*3+bytes([supported])
 return b'USBIND1\n'+U(1)+U(len(p))+p
class Resources:
 def __init__(self,values):self.values=values
 def get(self,key):return self.values.get(key)
def command(args):
 p=subprocess.run(args,capture_output=True,timeout=20)
 if p.returncode:raise RuntimeError((args,p.returncode,p.stderr.decode()))
 return p.stdout
def main():
 runtime,model,target=sys.argv[1:];d=json.loads(pathlib.Path(model).read_text());loaded=load(d)
 raw=b'f:\n.librarycall r1, r0\n.librarycall r1, r0\nret\n'
 valid=binding(); base={b'\0library/module':U(1),b'\0library/symbols':U(1)}
 with tempfile.TemporaryDirectory(prefix='r10-lowerimportscheck-') as t:
  t=pathlib.Path(t);tbl=t/'m.tbl';net=t/'m.net'
  command([sys.executable,'exec/c/tbl.py',model,str(tbl)])
  command([sys.executable,'exec/c/net.py',str(tbl),str(net)])
  checked=command([runtime,'--check-net',str(tbl),str(net)]).decode()
  manifest=t/'routes.tsv';manifest.write_text('test\tlower\ttape\ttins\tm.net\n')
  rdir=t/'resources';rdir.mkdir();source=t/'input';pkg=t/'m.pkg'
  def check(tape,values):
   status,out,_=simulate(d,tape,'input',files=Resources(values),loaded=loaded,maxsteps=200000)
   for p in rdir.iterdir():p.unlink()
   for k,v in values.items(): (rdir/k.split(b'/')[-1].decode()).write_bytes(v)
   pkg.write_bytes(build([manifest],[('006c6962726172792f',rdir)],cache=False));source.write_bytes(tape)
   p=subprocess.run([runtime,'--bundle',str(pkg),'test',str(source)],capture_output=True,timeout=10)
   assert (p.returncode==0)==(status=='accept'),(status,p.returncode,p.stderr)
   if status=='accept':assert p.stdout==out
   else:assert p.returncode==1
   return status,out
  values=dict(base);values[b'\0library/bindings']=valid
  status,out=check(raw,values)
  assert status=='accept',(target,status,out)
  if status=='accept':assert out.count(b'hostcall ')==2
  if status=='accept':
   for unrelated in (binding(supported=0),binding(D(0,0,0,3,8,0))):
    assert check(raw,{**base,b'\0library/bindings':unrelated})[0]=='accept'
  controls=0
  for bad in [None,b'',b'USBIND1\n'+U(0),valid[:-1],valid+b'X',binding(D(0,0,0,1,3,0)),binding(D(0,0,0,0,0,0)),binding(name=b'3bad'),valid[:8]+U(1<<63)+valid[16:],valid[:16]+U(1<<63)+valid[24:],valid[:8]+U(2)+valid[16:]+valid[16:],binding(D(0,0,0,1,1<<32|1,0)),binding(D(1,0,0,2,8,1))]:
   vals=dict(base)
   if bad is not None:vals[b'\0library/bindings']=bad
   assert check(raw,vals)[0]=='reject';controls+=1
  for key,val in [(b'\0library/module',None),(b'\0library/module',U(0)),(b'\0library/module',b'bad')]:
   vals=dict(values)
   if val is None:vals.pop(key)
   else:vals[key]=val
   assert check(raw,vals)[0]=='reject';controls+=1
  for tape in [b'f:\n.librarycall r8, r0\nret\n',b'f:\n.librarycall r1\nret\n']:
   assert check(tape,values)[0]=='reject';controls+=1
  if target.startswith('lnx/'):
   assert check(b'f:\n.hostcall r1, r0\nret\n',values)[0]=='reject';controls+=1
  # All truncation positions are checked on the model; representative errors also above on native executor.
  for n in range(1,len(valid)):
   vals=dict(base);vals[b'\0library/bindings']=valid[:n]
   assert simulate(d,raw,'input',files=Resources(vals),loaded=loaded,maxsteps=200000)[0]=='reject'
  ordinary=b'f:\nimm r0, 1\nret\n'
  before=simulate(d,ordinary,'input',files=Resources({}),loaded=loaded,maxsteps=200000)
  after=simulate(d,ordinary,'input',files=Resources({b'\0library/bindings':b'bad'}),loaded=loaded,maxsteps=200000)
  assert before[:2]==after[:2] and before[0]=='accept'
  print(json.dumps(dict(target=target,controls=controls,truncations=len(valid)-1,full_domain=checked.strip(),model_sha256=hashlib.sha256(pathlib.Path(model).read_bytes()).hexdigest(),ordinary_unchanged=True)))
if __name__=='__main__':main()

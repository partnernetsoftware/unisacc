#!/usr/bin/env python3
"""Private E3 injected bindings proof. Explicit typed tokens isolate E3.
No root product rebuild. Wrapper tape proof is distinct from native ABI execution.
"""
import json,struct,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'exec/parse2'),str(ROOT/'exec/pp')]
import subprocess,sim
U=lambda n:struct.pack('<Q',n)
DESC=b''.join(U(n) for n in (0,0,0,1,8,0))
TOKENS=b'type=long\nid=hostadd\n(\ntype=long\nid=a\n,\ntype=long\nid=b\n)\n;\ntype\nid=main\n(\ntype=void\n)\n{\nreturn\nid=hostadd\n(\nnum=10\n,\nnum=20\n)\n==\nnum=30\n?\nnum=0\n:\nnum=1\n;\n}\neof\n32 tokens\n'
def binding(desc=DESC,nargs=2,supported=1,kind=0,address=4096):
 name=b'hostadd';payload=U(len(name))+name+bytes((kind,0,0,0))+U(address)+U(nargs)+desc*(1+nargs)+bytes((supported,))
 return b'USBIND1\n'+U(1)+U(len(payload))+payload

def build():
 with tempfile.TemporaryDirectory(prefix='libimports-') as td:
  out=Path(td)/'parse.json'
  subprocess.run([sys.executable,str(ROOT/'exec/build/gen.py'),'parse2',str(out)],check=True)
  return json.loads(out.read_text())

def check(delta):
 loaded=sim.load(delta)
 def execute(raw,tokens=TOKENS):
  files=sim.Files()
  if raw is not None:files.cache[b'\0library/bindings']=raw
  return sim.run(delta,tokens,'typed-fixture',files=files,loaded=loaded,maxsteps=100000)
 raw=binding();v,out,n=execute(raw);assert v=='accept',(v,out,n)
 assert out.count(b'hostadd:\n')==1 and b'call hostadd\n' in out
 assert b'  imm r1, 4096\n  mov r0, r7\n  .librarycall r1, r0\n' in out
 assert out.endswith(b'  .frame -48\n  ret\n')
 for address in (0x123456789abcd,0x7fffffffffffffff):
  status,high,_=execute(binding(address=address));assert status=='accept'
  assert ('  imm r1, %d\n'%address).encode() in high,'native address was truncated'
 assert execute(None)[0]=='reject' # undefined source function, no fake FND
 for bad in [raw[:i] for i in range(1,len(raw))]+[raw+b'X',b'X'+raw[1:],binding(supported=0),binding(kind=1),binding(nargs=1),binding(b''.join(U(n) for n in (0,0,0,1,4,0)))]:
  assert execute(bad)[0]=='reject',('invalid binding accepted',len(bad))
 no_prototype=TOKENS[TOKENS.index(b'type\nid=main'):]
 assert execute(raw,no_prototype)[0]=='reject'
 # base/shape local IDs are ignored, ABI descriptor fields remain identical.
 local=b''.join(U(n) for n in (0,999,777,1,8,0));assert execute(binding(local))[0]=='accept'
 empty=b'USBIND1\n'+U(0);assert execute(empty)[0]=='reject' # unresolved actual call remains
 print('USBIND1 E3:',len(delta['states']),'states; real wrapper accepted',n,'steps;',len(raw)-1+6,'malformed/signature controls reject; base/shape local IDs ignored; no native runtime claim')
 return out

if __name__=='__main__':
 delta=build() if len(sys.argv)==1 else json.loads(Path(sys.argv[1]).read_text())
 out=check(delta)
 if len(sys.argv)>2:Path(sys.argv[2]).write_bytes(out)

"""E3 data declaration and deferred-address proof; not native ABI execution."""
import json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path[:0]=[str(ROOT/'exec/parse2'),str(ROOT/'exec/pp')]
import sim
U=lambda n:struct.pack('<Q',n)
def binding(name=b'value',desc=(0,0,0,1,8,0),writable=1,extent=8):
 p=U(len(name))+name+bytes((1,0,0,0))+U(4096)+U(0)+b''.join(U(n) for n in desc)+U(extent)+bytes((writable,1))
 return b'USBIND1\n'+U(1)+U(len(p))+p
def tokens(head=b'type=extern\ntype=long\nid=value\n;\n',body=b'return\nid=value\n;\n',tail=b''):
 return head+b'type\nid=main\n(\ntype=void\n)\n{\n'+body+b'}\n'+tail+b'eof\n32 tokens\n'
def check(d):
 l=sim.load(d)
 def run(raw,src):
  f=sim.Files()
  if raw is not None:f.cache[b'\0library/bindings']=raw
  return sim.run(d,src,'librarydata-fixture',files=f,loaded=l,maxsteps=200000)
 raw=binding();v,out,n=run(raw,tokens());assert v=='accept',(v,out,n)
 assert b'.bss g_value' not in out and b'.libraryaddr r0, g_value, 0, 1, 8, 0\n' in out,out
 v,real,n=run(raw,tokens(tail=b'type=long\nid=value\n;\n'));assert v=='accept',(v,real,n)
 assert real.count(b'.bss g_value')==1 and b'.libraryaddr' in real
 assert run(binding(desc=(0,0,0,1,4,0)),tokens())[0]=='reject'
 assert run(binding(writable=0),tokens())[0]=='reject'
 assert run(binding(extent=7),tokens())[0]=='reject'
 for desc in [(2**32,0,0,1,8,0),(0,0,0,1,2**32+4,0),(0,0,0,1,8,2**32),(0,0,0,2**32+1,8,0)]:assert run(binding(desc=desc),tokens())[0]=='reject'
 assert run(binding(extent=2**64-1),tokens())[0]=='reject'
 ptr=tokens(head=b'type=extern\ntype=long\n*\nid=value\n;\n');assert run(binding(desc=(1,0,0,2,8,0)),ptr)[0]=='accept'
 for bad in [raw[:i] for i in range(1,len(raw))]+[raw+b'x']:
  assert run(bad,tokens())[0]=='reject'
 v,classic,n=run(None,tokens());assert v=='accept' and b'.libraryaddr' not in classic and b'.bss g_value' in classic
 print(json.dumps({'scope':'typed E3 fixture only','data_scalar':True,'pointer':True,'later_definition':True,'default_unchanged_path':True,'truncations_reject':len(raw)-1,'native_execution':False}))
if __name__=='__main__':check(json.loads(Path(sys.argv[1]).read_text()))

#!/usr/bin/env python3
"""Constructed lower network/table proof for typed borrowed data.
Independent expected immediates and real data priority; no old built.json oracle.
"""
import json,pathlib,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[2];sys.path[:0]=[str(ROOT),str(ROOT/'exec/c')]
from exec.pp.sim import run,load
from pack import build
U=lambda n:struct.pack('<Q',n)
D=lambda *n:b''.join(U(v) for v in n)
def data(name=b'counter',address=4096,desc=None,extent=8,writable=1,supported=1):
 desc=desc or D(0,333,444,1,8,0)
 p=U(len(name))+name+bytes((1,0,0,0))+U(address)+U(0)+desc+U(extent)+bytes((writable,supported))
 return U(len(p))+p
def function():
 name=b'hostadd';p=U(len(name))+name+bytes(4)+U(8192)+U(2)+D(0,0,0,1,8,0)*3+b'\1';return U(len(p))+p
def binding(*records):return b'USBIND1\n'+U(len(records))+b''.join(records)
class Files:
 def __init__(self,values):self.values=values
 def get(self,key):return self.values.get(key)
def cmd(args):
 p=subprocess.run(list(map(str,args)),capture_output=True,timeout=25);assert p.returncode==0,(args,p.returncode,p.stderr);return p.stdout

def main():
 runtime,model,target=sys.argv[1:];d=json.loads(pathlib.Path(model).read_text());loaded=load(d)
 tape=b'f:\n.libraryaddr r0, g_counter, 0, 1, 8, 0\nret\n';valid=binding(function(),data());base={b'\0library/module':U(1),b'\0library/symbols':U(1)}
 with tempfile.TemporaryDirectory(prefix='r10-lowerdata-check-') as name:
  t=pathlib.Path(name);tbl=t/'model.tbl';net=t/'model.net';cmd([sys.executable,ROOT/'exec/c/tbl.py',model,tbl]);cmd([sys.executable,ROOT/'exec/c/net.py',tbl,net]);full=cmd([runtime,'--check-net',tbl,net]).decode().strip();manifest=t/'route.tsv';manifest.write_text('test\tlower\ttape\ttins\tmodel.net\n');rdir=t/'resources';rdir.mkdir();source=t/'tape';pkg=t/'package'
  def check(raw,wire=valid,module=True):
   values=dict(base) if module else {};values[b'\0library/bindings']=wire
   status,out,_=run(d,raw,'fixture',files=Files(values),loaded=loaded,maxsteps=200000)
   for p in rdir.iterdir():p.unlink()
   for k,v in values.items():(rdir/k.split(b'/')[-1].decode()).write_bytes(v)
   pkg.write_bytes(build([manifest],[('006c6962726172792f',rdir)],cache=False));source.write_bytes(raw);p=subprocess.run([runtime,'--bundle',str(pkg),'test',str(source)],capture_output=True,timeout=10)
   assert (p.returncode==0)==(status=='accept'),(status,p.returncode,p.stderr)
   if status=='accept':assert p.stdout==out
   else:assert p.returncode==1
   return status,out
  status,out=check(tape);posix=target.startswith(('lnx/','osx/'));assert status==('accept' if posix else 'reject')
  if posix:
   register=b'x0' if target.endswith('/arm64') else b'rax';assert b'imm '+register+b', 4096\n' in out
   mixed=tape.replace(b'ret\n',b'.librarycall r1, r0\nret\n');assert check(mixed)[1].count(b'hostcall ')==1
   own=b'.bss g_counter 8\n'+tape;status,out=check(own);assert status=='accept' and b'.lea '+register+b', g_counter' in out and b'imm '+register+b', 4096' not in out
   ptr=tape.replace(b'0, 1, 8, 0',b'1, 2, 8, 0');assert check(ptr,binding(data(desc=D(1,987,654,2,8,0))))[0]=='accept'
  bad=[binding(data(desc=D((1<<32)|1,0,0,2,8,0))),binding(data(address=(1<<63)|4096)),binding(data(extent=7)),binding(data(extent=(1<<64)-1)),binding(data(desc=D(0,0,0,(1<<32)|1,8,0))),binding(data(desc=D(0,0,0,1,8,(1<<32)|1))),binding(data(writable=0)),binding(data(supported=0)),binding(data(desc=D(0,0,0,1,3,0))),binding(data(desc=D(0,0,0,0,0,0))),binding(data(address=0)),valid[:-1],valid+b'X',valid[:8]+U(1<<63)+valid[16:],binding(data(),data())]
  for wire in bad:assert check(tape,wire)[0]=='reject'
  if posix:
   for huge in (2147487744,(1<<32)+4096,0x105e46de8,10*(1<<32),(1<<63)-1):
    status,out=check(tape,binding(data(address=huge)));assert status=='accept' and ('imm '+register.decode()+', '+str(huge)+'\n').encode() in out
  for raw in [tape.replace(b'0, 1, 8, 0',b'0, 1, 4, 0'),tape.replace(b'g_counter',b'g_absent'),tape.replace(b'g_counter',b'counter'),tape.replace(b'0, 1, 8, 0',b'0, 1, 8, 1'),tape.replace(b'0, 1, 8, 0',b'0, 1, 999999999999999999999, 0')]:assert check(raw)[0]=='reject'
  assert check(tape,module=False)[0]=='reject'
  for n in range(1,len(valid)):
   values=dict(base);values[b'\0library/bindings']=valid[:n];assert run(d,tape,'fixture',files=Files(values),loaded=loaded,maxsteps=200000)[0]=='reject'
  print(json.dumps({'target':target,'full_domain':full,'mixed_records':posix,'own_definition_priority':posix,'wire_controls':len(bad),'operand_controls':5,'truncations':len(valid)-1,'scope':'constructed lower network and independent typed-address expectations; native object access separate'}))
if __name__=='__main__':main()

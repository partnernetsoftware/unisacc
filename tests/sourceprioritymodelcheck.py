#!/usr/bin/env python3
"""Source definitions and unused bindings outrank semantic import support checks.
C constructed network equals table and sim; no host ABI execution is claimed.
"""
import json,os,pathlib,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path[:0]=[str(ROOT),str(ROOT/'exec/c')]
from exec.pp.sim import run,load
from pack import build
U=lambda n:struct.pack('<Q',n)
D=lambda k=1,w=8:b''.join(map(U,(0,0,0,k,w,0)))
def binding(k=1,w=8,n=1,var=0,supported=1,data=False,writable=1):
 name=b'own';payload=U(len(name))+name+bytes((int(data),0))+U(0)+bytes((0,var))+U(4096)+U(n)+D(k,w)*(1+n)
 if data:payload+=U(max(w,8))+bytes((writable,))
 payload+=bytes((supported,));return b'USBIND2\n'+U(1)+U(len(payload))+payload
class Files:
 def __init__(self,v):self.v=v
 def get(self,k):return self.v.get(k)
def main():
 model,runtime,dumper=sys.argv[1:];d=json.loads(pathlib.Path(model).read_text());loaded=load(d)
 with tempfile.TemporaryDirectory(prefix='r10-sourcepriority-check-') as name:
  t=pathlib.Path(name)
  def cmd(*args):
   env=dict(os.environ,UA_TYPESPELL='1');p=subprocess.run(list(map(str,args)),capture_output=True,timeout=25,env=env);assert p.returncode==0,(args,p.stderr);return p.stdout
  tbl=t/'parse.tbl';net=t/'parse.net';cmd(sys.executable,ROOT/'exec/c/tbl.py',model,tbl);cmd(sys.executable,ROOT/'exec/c/net.py',tbl,net);full=cmd(runtime,'--check-net',tbl,net).decode().strip()
  mf=t/'routes';mf.write_text('parse\tparse\ttokens\ttape\tparse.net\n');rd=t/'resource';rd.mkdir();pkg=t/'pkg';src=t/'probe.c';tokens=t/'tokens'
  def check(source,wire,accept):
   src.write_text(source);raw=cmd(dumper,'-dump-tokens',src);assert b'type=long\n' in raw, 'dumper must preserve type spelling (mkdump instrument)';tokens.write_bytes(raw);values={b'\0library/bindings':wire,b'\0library/module':U(1),b'\0library/symbols':U(1)}
   for k,v in values.items():(rd/k.split(b'/')[-1].decode()).write_bytes(v)
   pkg.write_bytes(build([mf],[('006c6962726172792f',rd)],cache=False));status,out,_=run(d,raw,'probe',files=Files(values),loaded=loaded,maxsteps=500000)
   p=subprocess.run([runtime,'--bundle',str(pkg),'parse',str(tokens)],capture_output=True,timeout=10);assert (p.returncode==0)==(status=='accept'),(status,p.stderr);assert (status=='accept')==accept,(source,status,out)
   if accept:assert p.stdout==out
   return out
  own='long own(long n){return n+7;}long check(long n){return own(n);}'
  external='long own(long n);long check(long n){return own(n);}'
  unused='long own(long n);long check(long n){return n+7;}'
  variants=[binding(3,8,supported=0),binding(5,16,supported=0),binding(var=1,supported=0),binding(n=7,supported=0)]
  for wire in variants:
   check(own,wire,True);check(unused,wire,True);check(external,wire,False)
  # Readonly data is legal wire but unsupported for mutation; actual source wins.
  wire=binding(n=0,data=True,writable=0,supported=0)
  check('int own=7;long check(long n){return own+n;}',wire,True)
  wide=bytearray(wire);wide[-10:-2]=U((1<<32)+8);check('int own=7;long check(long n){return own+n;}',bytes(wide),True)
  check('extern int own;long check(long n){return n;}',wire,True)
  check('extern long own;long check(long n){return own+n;}',wire,False)
  check(external,binding(),True)
  malformed=bytearray(binding(3,8,supported=0));malformed[-1]=2;check(own,bytes(malformed),False)
  print(json.dumps({'states':len(d['states']),'full_domain':full,'function_shapes':4,'source_priority':True,'unused_unsupported':True,'external_unsupported_rejected':True,'readonly_data_cases':4,'malformed_source_override_rejected':True,'scope':'E3 constructed C network equals simulator; native follow-up separate'}))
if __name__=='__main__':main()

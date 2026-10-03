#!/usr/bin/env python3
"""Independent USBIND3 typed winner/recursive-layout controls, C network equals delta simulator."""
import importlib.util,json,pathlib,struct,sys,tempfile,subprocess
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'exec'),str(ROOT)]
spec=importlib.util.spec_from_file_location('candidatesbase',ROOT/'exec/parse/gen.py');E=importlib.util.module_from_spec(spec);spec.loader.exec_module(E)
import assemble
install=lambda E,fail='DEAD':assemble.run(assemble.FACTS.parent/'modelcandidates-manifest.tsv',E,E.P,{},dict(fail=fail))  # modelcandidates-manifest.tsv
from exec.pp.sim import run,load
U=lambda n:struct.pack('<Q',n)
def rec(origin=0,ordinal=0,address=4096,name=b'host',supported=1,desc=(0,0,0,1,8,0),kind=0):
 p=U(len(name))+name+bytes((kind,origin))+U(ordinal)+bytes((0,0))+U(address)+U(0)+b''.join(map(U,desc))
 if kind:p+=U(8)+b'\1'
 p+=bytes((supported,));return U(len(p))+p
def wire(*r):return b'USBIND2\n'+U(len(r))+b''.join(r)
def v1(r):
 n=struct.unpack('<Q',r[8:16])[0];at=16+n+1
 p=r[8:at]+b'\0'+r[at+9:];return U(len(p))+p
def d2(kind=1,width=8,alignment=8,payload=b'',tag=0,base=4,shape=999,depth=0):
 return struct.pack('<7Q',depth,base,shape,kind,width,0,alignment)+bytes([tag])+U(len(payload))+payload
I=d2();D=d2(3,8,8,base=8);F=d2(3,4,4,base=7);PTR=d2(2,8,8,depth=1)
PAIR=d2(5,16,8,U(2)+U(0)+U(0)+U(0)+U(8)+D+U(8)+U(0)+U(0)+U(4)+d2(1,4,4),1,base=7001)
def sig(name=b'host',params=(),result=I,supported=1):
 return b'USLSIG2\n'+U(1)+U(len(name))+name+bytes([0,1,0,int(len(params)>6)])+U(len(params))+result+U(len(params))+b''.join(params)+bytes([supported])
def typed(origin=0,ordinal=0,address=4096,name=b'host',signature=None,supported=1,dispatcher=8192,plan=12288,count=9):
 if signature is None:signature=sig(name,(I,D,F,PTR,PAIR,I,D,I,I),PAIR,supported)
 p=U(len(name))+name+bytes([0,origin])+U(ordinal)+bytes([0,0])+U(address)+U(count)+bytes([1])+U(dispatcher)+U(plan)+U(len(signature))+signature+bytes([supported])
 return U(len(p))+p
def legacy3(r):
 p=r[8:];n=struct.unpack_from('<Q',p)[0];at=8+n+2+8+2+8+8;p=p[:at]+b'\0'+p[at:];return U(len(p))+p
def wire3(*r):return b'USBIND3\n'+U(len(r))+b''.join(r)
def normalized3(r):
 b=bytearray(r);n=struct.unpack('<Q',r[8:16])[0];at=16+n+1;b[at:at+9]=bytes(9);return bytes(b)
class Files:
 def __init__(self,blob):self.blob=blob
 def get(self,k):return self.blob if k==b'input' else None

def main():
 import sys as _sys
 if len(_sys.argv)-1 < 1:
     _sys.stderr.write("usage: %s MODEL.json\n" % _sys.argv[0])
     _sys.exit(2)

 runtime=sys.argv[1]
 install(E)
 E.P('START').a(('SBCLR',),*[('SBOUT',x) for x in b'input'],('SBFIND','mc_blob'),('BLEN','mc_len','mc_blob')).call('MC.normalize').a(('INPUSH','mc_blob'),('LDI','mc_zero',0),('SPAN2','mc_zero','mc_len'),('INPOP',)).goto('DONE')
 E.P('DONE').a(('ACCEPT',)).goto('DONE')
 E.g.finish();d={'start':'START','states':{n:[m,{str(k):v for k,v in r.items()}] for n,(m,r) in E.g.st.items()},'seqs':[list(map(list,s)) for s in E.g.seqs]};loaded=load(d)
 with tempfile.TemporaryDirectory(prefix='r10-candidates-') as name:
  t=pathlib.Path(name);model=t/'m.json';model.write_text(json.dumps(d));tbl=t/'m.tbl';net=t/'m.net'
  def cmd(*args):
   p=subprocess.run(list(map(str,args)),capture_output=True,timeout=25);assert p.returncode==0,(args,p.stderr);return p.stdout
  cmd(sys.executable,ROOT/'exec/c/tbl.py',model,tbl);cmd(sys.executable,ROOT/'exec/c/net.py',tbl,net)
  full=cmd(runtime,'--check-net',tbl,net).decode().strip()
  # bundle resources reuse existing package builder, no host normalization.
  sys.path.insert(0,str(ROOT/'exec/c'));from pack import build
  mf=t/'routes';mf.write_text('test\tresolve\tbytes\tbytes\tm.net\n');rd=t/'r';rd.mkdir();src=t/'empty';src.write_bytes(b'');pkg=t/'p'
  def check(blob,expected=None):
   status,out,_=run(d,b'','fixture',files=Files(blob),loaded=loaded,maxsteps=2000000)
   (rd/'input').write_bytes(blob);pkg.write_bytes(build([mf],[('',rd)],cache=False));p=subprocess.run([runtime,'--bundle',str(pkg),'test',str(src)],capture_output=True,timeout=10)
   assert (p.returncode==0)==(status=='accept'),(p.returncode,status,p.stderr)
   if status=='accept':assert p.stdout==out
   if expected is not None:assert status=='accept' and out==expected,(status,out,expected)
   return status,out
  winner=rec();expected=b'USBIND1\n'+U(1)+v1(winner)
  candidates=[rec(2,3,8192),rec(1,0,12288),winner,rec(2,1,16384,supported=0,desc=(0,0,0,5,16,0))]
  for rows in [candidates,list(reversed(candidates)),candidates[1:]+candidates[:1]]:check(wire(*rows),expected)
  check(b'USBIND1\n'+U(1)+v1(winner),expected)
  bestowned=rec(2,1,16384);check(wire(rec(2,64,8192),bestowned),b'USBIND1\n'+U(1)+v1(bestowned))
  # A semantically unsupported winner is deliberately retained for typed decoder.
  unsupported=rec(supported=0,desc=(0,0,0,5,16,0));check(wire(unsupported,rec(1,0)),b'USBIND1\n'+U(1)+v1(unsupported))
  bad=[wire(winner,winner),wire(rec(0,1)),wire(rec(2,0)),wire(rec(2,65)),wire(rec(3)),wire(rec(supported=2)),wire(rec(address=0)),wire(rec(address=1<<63)),wire(rec(desc=(0,0,0,(1<<32)|1,8,0))),wire(rec(desc=(0,0,0,1,(1<<32)|8,0))),wire(rec(desc=(0,0,0,1,8,1<<32))),wire(rec(kind=1,desc=(0,0,0,1,8,0)))+b'X',b'USBIND2\n'+U(1<<32),b'USBIND2\n'+U(1)+U((1<<64)-1)]
  for blob in bad:assert check(blob)[0]=='reject'
  opaque=rec(2,2,8192,supported=0,desc=((1<<32)+1,0,(1<<64)-1,5,(1<<32)+16,0))
  check(wire(opaque,winner),expected)
  pointerloser=rec(2,2,8192,supported=0,desc=((1<<32)+1,0,0,2,8,0))
  check(wire(pointerloser,winner),expected)
  datawinner=rec(kind=1);check(wire(rec(1,0,8192,kind=1),datawinner),b'USBIND1\n'+U(1)+v1(datawinner))
  # Invalid data candidate structure must fail even below a valid function.
  malformed_data=[]
  r=bytearray(rec(2,1,8192,name=b'host',kind=1,supported=0));n=struct.unpack('<Q',r[8:16])[0];r[16+n+11]=1;malformed_data.append(bytes(r)) # variadic
  malformed_data.append(rec(2,1,8192,kind=1,supported=0,desc=(0,0,0,0,0,0)))
  r=bytearray(rec(2,1,8192,kind=1,supported=0,desc=(0,0,0,5,0,0)));r[-10:-2]=U(0);malformed_data.append(bytes(r))
  for r in malformed_data:assert check(wire(winner,r))[0]=='reject'
  valid=wire(*candidates)
  for n in range(len(valid)):assert run(d,b'','fixture',files=Files(valid[:n]),loaded=loaded,maxsteps=2000000)[0]=='reject'
  tw=typed();te=wire3(tw)
  variants=[typed(2,3,address=8192),typed(1,0,address=12288),tw,typed(2,1,address=16384,supported=0,plan=0)]
  for rows in (variants,list(reversed(variants)),variants[1:]+variants[:1]):check(wire3(*rows),te)
  check(wire3(typed(2,64),typed(2,1)),wire3(normalized3(typed(2,1))))
  check(wire3(typed(supported=0,plan=0),typed(1,0)),wire3(typed(supported=0,plan=0)))
  olddata=legacy3(rec(2,1,address=16384,name=b'data',kind=1))
  check(wire3(tw,olddata),wire3(tw,normalized3(olddata)))
  check(wire3(legacy3(rec(1,0)),tw),te)
  oldwinner=legacy3(rec());check(wire3(typed(1,0),oldwinner),wire3(oldwinner))
  # All typed records are validated even when they lose priority.
  typedbad=[wire3(typed(),typed()),wire3(typed(dispatcher=0)),wire3(typed(plan=0)),
            wire3(typed(signature=sig(b'other',(I,)*9),count=9)),
            wire3(typed(signature=sig(params=(I,)*8),count=9)),
            wire3(typed(signature=sig(params=(I,)*9,result=d2(5,16,8,U(65),1)),count=9)),
            wire3(typed(signature=sig(params=(I,)*9,result=d2(5,16,8,U(1)+bytes(32)+I[:-1],1)),count=9)),
            wire3(typed(signature=sig(params=(I,)*9,result=d2(5,16,8,U(3)+U(8)+I,3)),count=9)),
            wire3(typed(signature=sig(params=(I,)*9,result=d2(3,8,3)),count=9))]
  for blob in typedbad:assert check(blob)[0]=='reject'
  for n in range(len(te)):assert run(d,b'','fixture',files=Files(te[:n]),loaded=loaded,maxsteps=2000000)[0]=='reject'
  print(json.dumps({'states':len(d['states']),'full_domain':full,'typed_priority_orders':3,'mixed_format0_and_format1':True,'typed_controls':len(typedbad),'typed_truncations':len(te),'legacy_controls':len(bad)+len(malformed_data),'legacy_truncations':len(valid),'V1_passthrough':True,'V2_exact_normalization':True,'typed_unsupported_winner_retained':True}))
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Manual ordered source facts and independently declared V2 carrier recipes.
Model/simulator/C network only; true-C qualification is a separate obligation.
"""
import argparse,json,pathlib,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT),str(ROOT/'exec'),str(ROOT/'exec/c'),str(ROOT/'tests')]
from exec.pp.sim import run,load
from pack import build
from modelnativecarriercheck import desc,structure,signature as sig2,plan
U=lambda n:struct.pack('<Q',n)
def d3(kind=1,width=4,align=4,unsigned=1,payload=b'',tag=0,rank=0,fmt=0,origin=2,known=3,natural=None,flags=0,depth=0):
 if natural is None:natural=align
 return struct.pack('<7Q',depth,321,987,kind,width,unsigned,align)+bytes([rank,fmt])+U(natural)+bytes([flags,known,origin,tag])+U(len(payload))+payload
I4=d3();I8=d3(width=8,align=8);D=d3(3,8,8,0,rank=2,fmt=2);F=d3(3,4,4,0,rank=1,fmt=1)
def ent(child,index,offset=0,bit=0,bits=0,kind=0,align=None):
 if align is None:align=struct.unpack_from('<Q',child,48)[0]
 width=struct.unpack_from('<Q',child,32)[0]
 return U(index)+bytes([kind])+U(align)+struct.pack('<4Q',offset,bit,bits,width)+child
def agg(entries,width,align,tag=1,**kw):return d3(5,width,align,0,U(len(entries))+b''.join(entries),tag,**kw)
def sig3(obj,name=b'entry'):
 return b'USLSIG3\n'+U(1)+U(len(name))+name+bytes([0,1,0,0])+U(1)+obj+U(1)+obj+b'\0'
def recipe(classes,unit):
 leaves=[desc(1 if c=='I' else 3,unit,unit,unsigned=int(c=='I'),base=0,shape=0) for c in classes]
 return structure(leaves,list(range(0,len(leaves)*unit,unit)),len(leaves)*unit,unit).replace(struct.pack('<2Q',321,987),bytes(16))
def cases():
 B4=agg([ent(I4,i,bit=o,bits=w,kind=1) for i,(o,w) in enumerate(((0,5),(5,7),(12,20)))],4,4)
 B8=agg([ent(I8,i,bit=o,bits=w,kind=1) for i,(o,w) in enumerate(((0,9),(9,17),(26,38)))],8,8)
 B16=agg([ent(I8,i,offset=(i//2)*8,bit=(0 if i%2==0 else (9 if i<2 else 17)),bits=w,kind=1) for i,w in enumerate((9,55,17,47))],16,8)
 BD16=agg([ent(B8,0),ent(D,1,offset=8)],16,8);DB16=agg([ent(D,0),ent(B8,1,offset=8)],16,8)
 BF8=agg([ent(B4,0),ent(F,1,offset=4)],8,4)
 Cross8=agg([ent(I4,0,bits=20,kind=1),ent(I4,1,offset=4,bits=20,kind=1)],8,4)
 Cross16=agg([ent(I8,0,bits=40,kind=1),ent(I8,1,offset=8,bits=40,kind=1)],16,8)
 barrier=agg([ent(I4,0,bits=3,kind=1),ent(I4,1,offset=4,kind=3),ent(I4,2,offset=4,bits=3,kind=1)],8,4)
 hfa_barrier=agg([ent(F,0),ent(I4,1,offset=4,kind=3),ent(F,2,offset=4)],8,4)
 anon=agg([ent(I4,0,bits=3,kind=1),ent(I4,1,bit=3,bits=5,kind=2),ent(I4,2,bit=8,bits=24,kind=1)],4,4)
 anonymous_agg=agg([ent(B4,0,kind=4)],4,4)
 # First member may use otherwise-unallocated container bytes: full extent4.
 reuse=agg([ent(I4,0,bits=24,kind=1),ent(d3(width=1,align=1),1,offset=3)],4,4)
 B12=agg([ent(B4,i,offset=i*4) for i in range(3)],12,4)
 HFA12=agg([ent(F,i,offset=i*4) for i in range(3)],12,4)
 values={'B12':B12,'HFA12':HFA12,'B4':B4,'B8':B8,'B16':B16,'BD16':BD16,'DB16':DB16,'BF8':BF8,'Cross8':Cross8,'Cross16':Cross16,'barrier':barrier,'hfa_barrier':hfa_barrier,'anonymous':anon,'anonymous_aggregate':anonymous_agg,'container_reuse':reuse}
 def expected(name,target):
  arm=target.endswith(b'arm64');win64=target==b'win/x86_64'
  words={'B12':('III',4),'HFA12':('III' if win64 else 'SSS',4),'B4':('I',4),'B8':('I',8),'B16':('II',8),'BD16':('II' if arm or win64 else 'IS',8),'DB16':('II' if arm or win64 else 'SI',8),'BF8':('II',4),'Cross8':('II',4),'Cross16':('II',8),'barrier':('II',4),'hfa_barrier':('II' if win64 else 'SS',4),'anonymous':('I',4),'anonymous_aggregate':('I',4),'container_reuse':('I',4)}
  classes,unit=words[name];return recipe(classes,unit)
 # Lane classification of anonymous-only bits differs by SysV ABI compat policy.
 unknown=agg([ent(I8,0,bits=1,kind=2),ent(D,1)],8,8,tag=2)
 bad=[d3(origin=0,known=0,natural=0),d3(known=0),d3(flags=1),d3(natural=8),agg([ent(I4,0,bits=1,kind=1)],8,4)]
 # Misrepresented natural container address, overlapping fields, stale order,
 # barrier-induced FP base/alignment mismatch, and unknown nested completeness.
 bad += [agg([ent(I4,0,bits=1,kind=1),ent(I4,1,bit=0,bits=2,kind=1)],4,4),agg([ent(I4,1,bits=1,kind=1)],4,4),agg([ent(d3(width=1,align=1),0),ent(I4,1,offset=1,bits=3,kind=1)],8,4),agg([ent(d3(origin=0,known=0,natural=0),0,bits=1,kind=1)],4,4)]
 # rank-3 (long double) stored as F64: certified only where the profile's long double IS IEEE64.
 longdouble=d3(3,8,8,0,rank=3,fmt=2)
 # Explicit packed external (origin 2) layouts: natural>alignment, members at reduced
 # effective alignment. AAPCS64/Win64 carry them as declared-alignment integer arrays;
 # SysV refuses (unaligned fields are MEMORY class, needs BANK).
 C1=d3(width=1,align=1);S2=d3(width=2,align=2)
 packed={'P5':(agg([ent(C1,0),ent(I4,1,offset=1,align=1)],5,1,natural=4),('I'*5,1)),
         'P6':(agg([ent(C1,0),ent(I4,1,offset=2,align=2)],6,2,natural=4),('III',2)),
         'P11':(agg([ent(S2,0,align=1),ent(I8,1,offset=2,align=1),ent(C1,2,offset=10)],11,1,natural=8),('I'*11,1)),
         'PN':(agg([ent(C1,0),ent(agg([ent(I4,0),ent(I4,1,offset=4)],8,4),1,offset=1,align=1)],9,1,natural=4),('I'*9,1))}
 packedbad=[agg([ent(C1,0),ent(D,1,offset=1,align=1)],9,1,natural=8),  # FP leaf inside packed
  agg([ent(C1,0),ent(I4,1,offset=1,align=1)],5,1,natural=4,origin=1),  # source-origin packed
  agg([ent(C1,0),ent(I4,1,offset=2,align=1)],6,1,natural=4),  # offset not the packed placement
  agg([ent(C1,0),ent(I4,1,offset=1,bits=3,kind=1,align=1)],5,1,natural=4),  # bitfield at reduced alignment
  agg([ent(I4,0)],4,4,natural=1),  # natural below alignment
  agg([ent(C1,0),ent(I4,1,offset=1,align=1)],8,1,natural=4)]  # extent not the packed size
 return values,expected,unknown,bad,longdouble,packed,packedbad
class Files:
 def __init__(self,target):self.target=target
 def get(self,key):return self.target if key==b'\0cli/target' else None

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--runtime');ap.add_argument('--model',type=pathlib.Path);ap.add_argument('--output',type=pathlib.Path);a=ap.parse_args()
 with tempfile.TemporaryDirectory(prefix='r10-native-bitfield-') as tmp:
  t=pathlib.Path(tmp)
  def cmd(*args):
   p=subprocess.run(list(map(str,args)),capture_output=True,timeout=20);assert p.returncode==0,(args,p.stderr);return p.stdout
  runtime=a.runtime or str(t/'run')
  if not a.runtime:cmd(ROOT/'tests/bound',20,'cc','-O2',ROOT/'exec/c/run.c','-o',runtime)
  model=a.model or t/'model.json'
  if not a.model:cmd(sys.executable,ROOT/'exec/nativeabi/gen.py',model)
  d=json.loads(model.read_text());loaded=load(d);tbl=t/'m.tbl';net=t/'m.net';cmd(sys.executable,ROOT/'exec/c/tbl.py',model,tbl);cmd(sys.executable,ROOT/'exec/c/net.py',tbl,net);full=cmd(runtime,'--check-net',tbl,net).decode().strip()
  mf=t/'routes';mf.write_text('test\tordered\tbytes\tbytes\tm.net\n');rd=t/'r';(rd/'cli').mkdir(parents=True);pkg=t/'pkg';inp=t/'input';passed=0;refused=0
  def check(original,target,expected=None):
   nonlocal passed,refused
   status,out,_=run(d,original,'ordered',files=Files(target),loaded=loaded,maxsteps=4000000)
   (rd/'cli/target').write_bytes(target);pkg.write_bytes(build([mf],[('00',rd)],cache=False));inp.write_bytes(original);p=subprocess.run([runtime,'--bundle',str(pkg),'test',str(inp)],capture_output=True,timeout=10)
   assert (p.returncode==0)==(status=='accept'),(status,p.stderr)
   if expected is None:assert status=='reject' and not p.stdout,(status,p.stdout);refused+=1
   else:assert status=='accept' and p.stdout==out==expected,(target,status,len(out),len(expected),repr(out)[:300],repr(expected)[:300]);passed+=1
  values,expected,unknown,bad,longdouble,packed,packedbad=cases()
  for target in (f'{os}/{arch}'.encode() for os in ('osx','lnx','win') for arch in ('arm64','x86_64')):
   for name,obj in values.items():
    source=sig3(obj);carrier=expected(name,target);check(source,target,plan(target,source,sig2((carrier,),carrier,support=1)))
   for obj in bad:check(sig3(obj),target)
   if target in (b'osx/x86_64',b'lnx/x86_64'):check(sig3(unknown),target)
   for name,(obj,(classes,unit)) in packed.items():
    src=sig3(obj)
    if target in (b'osx/arm64',b'lnx/arm64',b'win/arm64',b'win/x86_64'):
     v=recipe(classes,unit);check(src,target,plan(target,src,sig2((v,),v,support=1)))
    else:check(src,target)
   for obj in packedbad:check(sig3(obj),target)
   ldsrc=sig3(longdouble)
   if target in (b'osx/arm64',b'win/arm64',b'win/x86_64'):
    f64=desc(3,8,8,unsigned=0,base=0,shape=0);check(ldsrc,target,plan(target,ldsrc,sig2((f64,),f64,support=1)))
   else:check(ldsrc,target)
   aligned_hfa=sig3(agg([ent(I8,0,kind=3),ent(F,1),ent(F,2,offset=4)],8,8))
   if target.endswith(b'arm64'):check(aligned_hfa,target)
   else:
    value=recipe('I' if target==b'win/x86_64' else 'S',8);check(aligned_hfa,target,plan(target,aligned_hfa,sig2((value,),value,support=1)))
   # Explicit complete-source origin1 control, independent of source projection.
   obj=d3(origin=1);orig=sig3(obj);leaf=desc(1,4,4,unsigned=1,base=0,shape=0);check(orig,target,plan(target,orig,sig2((leaf,),leaf,support=1)))
  result={'status':'passed','scope':'V3 complete ordered facts -> model carrier; not native ABI execution','accepted':passed,'rejected':refused,'states':len(d['states']),'full_domain':full,'manual_fixtures':list(values)}
  if a.output:a.output.write_text(json.dumps(result,indent=2)+'\n')
  print(json.dumps(result))
if __name__=='__main__':main()

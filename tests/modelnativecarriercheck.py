#!/usr/bin/env python3
"""Independent exact carrier oracle, raw graph controls, actual sim/C network."""
import importlib.util,json,pathlib,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/c')]
from exec.pp.sim import run,load
from pack import build
U=lambda n:struct.pack('<Q',n)
def desc(kind=1,width=8,alignment=8,unsigned=0,depth=0,tag=0,payload=b'',base=321,shape=987):
 return struct.pack('<7Q',depth,base,shape,kind,width,unsigned,alignment)+bytes([tag])+U(len(payload))+payload
D=desc(3);UI=desc(unsigned=1);I=desc();F=desc(3,4,4);P=desc(2,depth=1);VOID=desc(0,0,0)
def union(children=(D,UI),layout=(0,0,0,8),**kw):
 return desc(5,tag=2,payload=U(len(children))+b''.join(b''.join(map(U,layout))+x for x in children),**kw)
MIX=union()
def natural(children,width):
 return desc(5,width,width,tag=2,payload=U(len(children))+b''.join(bytes(24)+U(struct.unpack_from('<Q',x,32)[0])+x for x in children))

def signature(params=(MIX,),result=MIX,support=0,var=0,mode=None,name=b'entry'):
 return b'USLSIG2\n'+U(1)+U(len(name))+name+bytes([0,1,var,int(var or len(params)>6) if mode is None else mode])+U(len(params))+result+U(len(params))+b''.join(params)+bytes([support])
CUI=desc(unsigned=1,base=0,shape=0)
def identity(d):
 b=bytearray(d);b[8:24]=bytes(16);return bytes(b)
def carrier(params,result,name=b'entry',mode=None):
 return signature(tuple(CUI if x==MIX or x==union((UI,D)) else identity(x) for x in params),CUI if result==MIX or result==union((UI,D)) else identity(result),support=1,mode=mode,name=name)
def callback_def(ident,params=(),result=I,var=0,mode=None,support=0,canon=False):
 body=bytes([1,0])+U(ident)+bytes([var,int(var or len(params)>6) if mode is None else mode])+U(len(params))+result+U(len(params))+b''.join(params)+bytes([support])
 return desc(4,depth=1,tag=4,payload=body,base=0 if canon else 321,shape=0 if canon else 987)
def callback_ref(ident,canon=False):return desc(4,depth=1,tag=4,payload=bytes([1,1])+U(ident),base=0 if canon else 321,shape=0 if canon else 987)
def plan(target,original,converted):return b'USLNCAR1\n'+U(len(target))+target+U(len(original))+original+U(len(converted))+converted
class Files:
 def __init__(self,target):self.target=target
 def get(self,key):return self.target if key==b'\0cli/target' else None

def main():
 with tempfile.TemporaryDirectory(prefix='r10-native-carrier-') as name:
  t=pathlib.Path(name)
  def cmd(*args):
   p=subprocess.run(list(map(str,args)),capture_output=True,timeout=20);assert p.returncode==0,(args,p.returncode,p.stderr);return p.stdout
  runtime=pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else t/'run'
  if len(sys.argv)==1:cmd(ROOT/'tests/bound',20,'cc','-O2',ROOT/'exec/c/run.c','-o',runtime)
  model,tbl,net=t/'m.json',t/'m.tbl',t/'m.net';cmd(sys.executable,ROOT/'exec/nativeabi/gen.py',model)
  d=json.loads(model.read_text());loaded=load(d);cmd(sys.executable,ROOT/'exec/c/tbl.py',model,tbl);cmd(sys.executable,ROOT/'exec/c/net.py',tbl,net)
  full=cmd(runtime,'--check-net',tbl,net).decode().strip()
  routes=t/'routes';routes.write_text('carrier\tnativeabi\tbytes\tbytes\tm.net\n');rd=t/'resources';rd.mkdir();(rd/'cli').mkdir();(rd/'sentinel').write_bytes(b'x');pkg=t/'p';src=t/'input'
  def check(original,target=b'osx/arm64',expected=None):
   status,out,_=run(d,original,'carrier',files=Files(target),loaded=loaded,maxsteps=2000000)
   if target is not None:(rd/'cli/target').write_bytes(target)
   else:
    try:(rd/'cli/target').unlink()
    except FileNotFoundError:pass
   pkg.write_bytes(build([routes],[('00',rd)],cache=False));src.write_bytes(original)
   p=subprocess.run([str(runtime),'--bundle',str(pkg),'carrier',str(src)],capture_output=True,timeout=20)
   assert (status=='accept')==(p.returncode==0),(status,p.returncode,p.stderr)
   if expected is None:assert status=='reject' and not p.stdout,(status,p.stdout)
   else:assert status=='accept' and out==p.stdout==expected,(status,out,expected,p.stderr)
  successes=0
  for target in (f'{os}/{arch}'.encode() for os in ('osx','lnx','win') for arch in ('arm64','x86_64')):
   for params,result in [((MIX,),MIX),((union((UI,D)),),union((UI,D))),((I,D,F,P,MIX),I),((I,)*8+(MIX,),MIX),((),VOID)]:
    original=signature(params,result);check(original,target,plan(target,original,carrier(params,result)));successes+=1
  # Profile oracle is independent of rules.tsv and the model classifier.
  natural_cases=0;arm_narrow_rejections=0
  for target in (f'{os}/{arch}'.encode() for os in ('osx','lnx','win') for arch in ('arm64','x86_64')):
   for width in (1,2,4,8):
    children=(desc(1,width,width),desc(1,width,width,unsigned=1),desc(1,1,1))
    value=natural(children,width);original=signature((value,),value)
    if target.endswith(b'/arm64') and width<8:check(original,target);arm_narrow_rejections+=1
    else:
     converted=desc(1,width,width,unsigned=1,base=0,shape=0)
     check(original,target,plan(target,original,signature((converted,),converted,support=1)));natural_cases+=1
   for width in (4,8):
    children=(desc(3,width,width),)*7;value=natural(children,width);original=signature((value,),value)
    converted=desc(1 if target==b'win/x86_64' else 3,width,width,unsigned=int(target==b'win/x86_64'),base=0,shape=0)
    check(original,target,plan(target,original,signature((converted,),converted,support=1)));natural_cases+=1
   for children in ((I,D,UI,F),(F,UI,D,I),(D,)*9+(I,)*8,(F,UI,desc(1,2,2))):
    value=natural(children,8);original=signature((value,),value)
    check(original,target,plan(target,original,signature((CUI,),CUI,support=1)));natural_cases+=1
   # Profile-specific FP carrier nested inside a shared cyclic callback graph.
   value=natural((F,F,F),4)
   mapped=desc(1 if target==b'win/x86_64' else 3,4,4,unsigned=int(target==b'win/x86_64'),base=0,shape=0)
   original=signature((callback_ref(1),),callback_def(1,(value,callback_ref(1)),value))
   converted=signature((callback_ref(1,True),),callback_def(1,(mapped,callback_ref(1,True)),mapped,support=1,canon=True),support=1)
   check(original,target,plan(target,original,converted));natural_cases+=1
  # Independent explicit graph wire oracle: shared references, back edges,
  # mutual recursion, factory return and a nested mode1 nine-argument signature.
  cI=identity(I)
  fixtures=[]
  leaf=callback_def(1,(MIX,),MIX);cleaf=callback_def(1,(CUI,),CUI,support=1,canon=True)
  fixtures.append((signature((leaf,callback_ref(1)),MIX),signature((cleaf,callback_ref(1,True)),CUI,support=1)))
  selfsig=callback_def(1,(callback_ref(1),MIX),callback_ref(1))
  cself=callback_def(1,(callback_ref(1,True),CUI),callback_ref(1,True),support=1,canon=True)
  fixtures.append((signature((callback_ref(1),MIX),selfsig),signature((callback_ref(1,True),CUI),cself,support=1)))
  mutual=callback_def(1,(callback_def(2,(callback_ref(1),MIX),MIX),),MIX)
  cmutual=callback_def(1,(callback_def(2,(callback_ref(1,True),CUI),CUI,support=1,canon=True),),CUI,support=1,canon=True)
  fixtures.append((signature((mutual,),MIX),signature((cmutual,),CUI,support=1)))
  factory=callback_def(1,(MIX,),callback_def(2,(MIX,),MIX))
  cfactory=callback_def(1,(CUI,),callback_def(2,(CUI,),CUI,support=1,canon=True),support=1,canon=True)
  fixtures.append((signature((MIX,),factory),signature((CUI,),cfactory,support=1)))
  wide=callback_def(1,(I,)*8+(MIX,),MIX)
  cwide=callback_def(1,(cI,)*8+(CUI,),CUI,support=1,canon=True)
  fixtures.append((signature((wide,),MIX),signature((cwide,),CUI,support=1)))
  for target in (f'{osname}/{arch}'.encode() for osname in ('osx','lnx','win') for arch in ('arm64','x86_64')):
   for original,converted in fixtures:check(original,target,plan(target,original,converted));successes+=1
  # Decode each certified callback graph through the existing bridge parser.
  host=t/'graphcheck.c';host.write_text('#include "exec/c/libraryexports.h"\nint main(int argc,char **argv){FILE *f=fopen(argv[1],"rb");fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);unsigned char *b=malloc(n);fread(b,1,n,f);fclose(f);us_exports x={0};char e[200]={0};int rc=us_exports_load_bridge(&x,b,n,e,sizeof e);if(rc)fprintf(stderr,"%s\\n",e);us_exports_clear(&x);free(b);return rc;}\n')
  hostrun=t/'graphcheck';cmd('cc','-O0','-I',ROOT,host,'-lffi','-o',hostrun)
  for _,converted in fixtures:wire=t/'certified-graph';wire.write_bytes(converted);cmd(hostrun,wire)
  # Equal object extent alone is insufficient, and all layout facts matter.
  rejects=[signature(result=natural((D,F),8)),signature(result=natural((desc(3,unsigned=1),),8)),signature(result=natural((desc(1,4,4),),8)),
   signature(result=union(unsigned=1)),signature(result=union(width=16)),signature(result=union(alignment=4)),signature(result=union(depth=1)),
   signature(result=union(layout=(1,0,0,8))),signature(result=union(layout=(0,1,0,8))),
   signature(result=union(layout=(0,0,1,8))),signature(result=union(layout=(0,0,0,4))),
   signature(result=union((desc(3,4,4),UI))),signature(result=union((D,desc(unsigned=1,depth=1)))),
   signature(result=union((D,desc(unsigned=1,tag=3,payload=U(1)+U(8)+UI)))),
   signature(result=desc(5,tag=1,payload=U(1)+bytes(24)+U(8)+MIX)),
   signature(result=desc(5,tag=3,payload=U(1)+U(8)+MIX)),signature(support=1),signature(var=1),
   signature(result=desc(4,depth=1,tag=4)),signature(result=desc(3,8,4)),signature(result=desc(0,0,0,unsigned=1)),signature(params=(VOID,),result=I),
   signature(mode=1),signature()+b'x']
  rejects.extend([
   signature((MIX,),callback_def(1,(MIX,),callback_def(2,(natural((D,F),8),),MIX))),
   signature((MIX,),callback_def(1,(callback_ref(2),),MIX)),
   signature((callback_def(1,(natural((D,F),8),),MIX),),MIX),
   signature((callback_def(1,(MIX,),union(layout=(0,0,1,8))),),MIX),
   signature((callback_def(1,(MIX,),MIX,var=1),),MIX),
   signature((callback_ref(1),),MIX),
   signature((callback_def(1,(MIX,),MIX),callback_def(1,(MIX,),MIX)),MIX),
   signature((callback_def(1,(desc(5,tag=1,payload=U(1)+bytes(24)+U(8)+MIX),),MIX),),MIX),
   signature((callback_def(1,(desc(5,tag=3,payload=U(1)+U(8)+MIX),),MIX),),MIX),
   signature((desc(4,depth=2,tag=4,payload=leaf[65:]),),MIX)])
  for original in rejects:check(original)
  for target in (None,b'',b'osx/arm',b'osx/arm64\0',b'osx/arm64x',b'win/riscv64',b'OSX/arm64'):check(signature(),target)
  original=signature()
  root_truncations=len(original)
  for length in range(root_truncations):
   assert run(d,original[:length],'truncated',files=Files(b'osx/arm64'),loaded=loaded,maxsteps=2000000)[0]=='reject'
  for length in (0,7,16,len(original)//2,len(original)-1):check(original[:length])
  for original,_ in fixtures:
   for length in (len(original)//2,len(original)-1):check(original[:length])
  print(json.dumps({'prototype_only':True,'states':len(d['states']),'full_domain':full,'six_explicit_profile_rules':True,'exact_sim_network_bytes':successes+natural_cases,'raw_graph_rejections':len(rejects),'unknown_profile_controls':7,'root_truncations':root_truncations,'callback_truncation_controls':2*len(fixtures),'reversed_members':True,'natural_scalar_union_cases':natural_cases,'arm_narrow_rejections':arm_narrow_rejections,'heterogeneous_fp_rejected':True,'one_logical_aggregate_one_carrier':True,'original_bytes_untouched':True,'fixed_mode1_nine_count':True,'callback_graph_cases':len(fixtures),'shared_self_mutual_factory':True,'nested_proof1_bridge_decode':True}))
if __name__=='__main__':main()

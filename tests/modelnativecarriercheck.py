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
def structure(children,offsets,width,alignment):
 return desc(5,width,alignment,tag=1,payload=U(len(children))+b''.join(U(off)+bytes(16)+U(struct.unpack_from('<Q',x,32)[0])+x for x,off in zip(children,offsets)))
def array(child,count,stride,width,alignment):
 return desc(5,width,alignment,tag=3,payload=U(count)+U(stride)+child)

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
  model,tbl,net=t/'m.json',t/'m.tbl',t/'m.net';cmd(sys.executable,ROOT/'exec/build/gen.py','nativeabi',model)
  d=json.loads(model.read_text());loaded=load(d);cmd(sys.executable,ROOT/'exec/c/tbl.py',model,tbl);cmd(sys.executable,ROOT/'exec/c/net.py',tbl,net)
  full=cmd(runtime,'--check-net',tbl,net).decode().strip()
  routes=t/'routes';routes.write_text('carrier\tnativeabi\tbytes\tbytes\tm.net\n');rd=t/'resources';rd.mkdir();(rd/'cli').mkdir();(rd/'sentinel').write_bytes(b'x');pkg=t/'p';src=t/'input'
  def check(original,target=b'osx/arm64',expected=None,reason=None):
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
   if reason is not None:assert reason in p.stderr.decode(),p.stderr
  # A valid V3 scalar must pass framing and reach the named certification guard.
  v3desc=struct.pack('<7Q',0,0,0,1,8,0,8)+bytes(14)+U(0)
  v3=b'USLSIG3\n'+U(1)+U(5)+b'entry'+bytes((0,1,0,0))+U(1)+v3desc+U(1)+v3desc+b'\0'
  for profile in (f'{os}/{arch}'.encode() for os in ('osx','lnx','win') for arch in ('arm64','x86_64')):check(v3,profile,reason='not covered: native ABI ordered source facts')
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
  aggregate_cases=0
  for target in (f'{os}/{arch}'.encode() for os in ('osx','lnx','win') for arch in ('arm64','x86_64')):
   fp=natural((F,F),4);cfp=desc(1 if target==b'win/x86_64' else 3,4,4,unsigned=int(target==b'win/x86_64'),base=0,shape=0)
   afp=array(fp,3,4,12,4);cafp=identity(array(cfp,3,4,12,4))
   au=array(MIX,2,8,16,8);cau=identity(array(CUI,2,8,16,8))
   su=structure((desc(1,1,1),MIX,afp),(0,8,16),32,8)
   csu=identity(structure((identity(desc(1,1,1)),CUI,cafp),(0,8,16),32,8))
   # Self-cycle definition traversed through a struct; later references retain ID1.
   cb=callback_def(1,(callback_ref(1),su),au)
   ccb=callback_def(1,(callback_ref(1,True),csu),cau,support=1,canon=True)
   outer=structure((cb,au),(0,8),24,8);couter=identity(structure((ccb,cau),(0,8),24,8))
   for originalvalue,convertedvalue in ((afp,cafp),(au,cau),(su,csu),(outer,couter)):
    original=signature((originalvalue,),I);converted=signature((convertedvalue,),identity(I),support=1)
    check(original,target,plan(target,original,converted));aggregate_cases+=1
  # Independent nine concrete schemas; no model/rule table used for gold.
  i4=desc(1,4,4,unsigned=1)
  is8=structure((UI,D),(0,8),16,8);si8=structure((D,UI),(0,8),16,8)
  ss8=structure((D,D),(0,8),16,8)
  is4=structure((i4,i4,F,F),(0,4,8,12),16,4)
  si4=structure((F,F,i4,i4),(0,4,8,12),16,4)
  ss4=structure((F,F,F,F),(0,4,8,12),16,4)
  ii4=structure((i4,i4,i4,i4),(0,4,8,12),16,4)
  double2=array(D,2,8,16,8);float4=array(F,4,4,16,4)
  schemas=[((is8,structure((UI,F,F),(0,8,12),16,8)),8,'IS',False),
   ((si8,structure((F,F,UI),(0,4,8),16,8)),8,'SI',False),
   ((double2,ss8),8,'SS',True),((is8,si8),8,'II',False),
   ((is4,is4),4,'IS',False),((si4,si4),4,'SI',False),
   ((float4,ss4),4,'SS',True),((float4,ii4),4,'II',False),
   ((double2,float4),8,'SS',False)]
  union16_cases=0
  def recipe(classes,alignment):
   codes=''.join(x*(2 if alignment==4 else 1) for x in classes)
   children=tuple(desc(1 if x=='I' else 3,alignment,alignment,unsigned=int(x=='I'),base=0,shape=0) for x in codes)
   return identity(structure(children,tuple(i*alignment for i in range(len(codes))),16,alignment))
  for target in (f'{os}/{arch}'.encode() for os in ('osx','lnx','win') for arch in ('arm64','x86_64')):
   for children,alignment,sysv,hfa in schemas:
    classes=('SS' if hfa else 'II') if target.endswith(b'/arm64') else 'II' if target==b'win/x86_64' else sysv
    convertedvalue=recipe(classes,alignment)
    for alternatives in (children,tuple(reversed(children)),children+children):
     value=natural(alternatives,16)
     # natural helper alignment defaults width; set exact original align4/8.
     b=bytearray(value);b[48:56]=U(alignment);value=bytes(b)
     original=signature((value,),value)
     converted=signature((convertedvalue,),convertedvalue,support=1)
     check(original,target,plan(target,original,converted));union16_cases+=1
  # Parser aliases and a signature cycle do not affect the carrier recipe.
  for target in (f'{os}/{arch}'.encode() for os in ('osx','lnx','win') for arch in ('arm64','x86_64')):
   value=bytearray(natural((double2,float4),16));value[48:56]=U(8);value[8:24]=U(77)+U(88);value=bytes(value)
   convertedvalue=recipe('SS' if target.endswith(b'/x86_64') and target!=b'win/x86_64' else 'II',8)
   original=signature((callback_ref(1),),callback_def(1,(value,callback_ref(1)),value))
   converted=signature((callback_ref(1,True),),callback_def(1,(convertedvalue,callback_ref(1,True)),convertedvalue,support=1,canon=True),support=1)
   check(original,target,plan(target,original,converted));union16_cases+=1
   for length in (len(original)//2,len(original)-1):check(original[:length],target)
  n16bad=[natural((structure((I,D),(0,4),16,8),),16),
   natural((structure((desc(1,1,1),D),(0,8),16,8),),16),
   natural((array(D,2,4,16,8),),16),natural((structure((callback_def(1),I),(0,8),16,8),),16),
   natural((structure((desc(2,depth=1,unsigned=1),P),(0,8),16,8),),16),natural((structure((P,P),(0,8),16,8),),16)]
  # Pointer leaves are I (positive), callable object leaves remain opaque-unsafe.
  validptr=bytearray(n16bad.pop());validptr[48:56]=U(8);original=signature((bytes(validptr),),I)
  check(original,b'osx/arm64',plan(b'osx/arm64',original,signature((recipe('II',8),),identity(I),support=1)));union16_cases+=1
  for value in n16bad:
   b=bytearray(value);b[48:56]=U(8);check(signature(result=bytes(b)))
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
   signature(result=desc(5,tag=1,payload=U(1)+U(1)+bytes(16)+U(8)+MIX)),
   signature(result=desc(5,tag=3,payload=U(2)+U(8)+MIX)),signature(support=1),signature(var=1),
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
   signature((callback_def(1,(desc(5,tag=1,payload=U(1)+U(1)+bytes(16)+U(8)+MIX),),MIX),),MIX),
   signature((callback_def(1,(desc(5,tag=3,payload=U(2)+U(8)+MIX),),MIX),),MIX),
   signature((desc(4,depth=2,tag=4,payload=leaf[65:]),),MIX)])
  rejects.extend([
   signature(result=structure((I,I),(0,4),16,8)),
   signature(result=structure((I,),(0,),16,8)),
   signature(result=structure((I,),(0,),8,4)),
   signature(result=structure((I,),(0,),8,16)),
   signature(result=array(I,2,4,8,8)),
   signature(result=array(I,0,8,8,8)),
   signature(result=array(I,2,8,16,4)),
   signature(result=array(natural((D,F),8),2,8,16,8)),
   signature(result=structure((natural((desc(1,4,4),),4),),(0,),4,4)),
   signature(result=structure((VOID,),(0,),8,8)),
   signature(result=desc(5,tag=1,payload=U(1)+U(0)+U(0)+U(1)+U(8)+I)),
   signature(result=desc(5,tag=1,payload=U(1)+U(0)+U(1)+U(0)+U(8)+I))])
  for original in rejects:check(original)
  for target in (None,b'',b'osx/arm',b'osx/arm64\0',b'osx/arm64x',b'win/riscv64',b'OSX/arm64'):check(signature(),target)
  original=signature()
  root_truncations=len(original)
  for length in range(root_truncations):
   assert run(d,original[:length],'truncated',files=Files(b'osx/arm64'),loaded=loaded,maxsteps=2000000)[0]=='reject'
  for length in (0,7,16,len(original)//2,len(original)-1):check(original[:length])
  for original,_ in fixtures:
   for length in (len(original)//2,len(original)-1):check(original[:length])
  print(json.dumps({'prototype_only':True,'V3_uncertified_profile_rejects':6,'states':len(d['states']),'full_domain':full,'six_explicit_profile_rules':True,'exact_sim_network_bytes':successes+natural_cases+aggregate_cases+union16_cases,'union16_cases':union16_cases,'union16_layout_negatives':len(n16bad),'union16_cycle_truncations':12,'aggregate_cases':aggregate_cases,'raw_graph_rejections':len(rejects),'unknown_profile_controls':7,'root_truncations':root_truncations,'callback_truncation_controls':2*len(fixtures),'reversed_members':True,'natural_scalar_union_cases':natural_cases,'arm_narrow_rejections':arm_narrow_rejections,'heterogeneous_fp_rejected':True,'one_logical_aggregate_one_carrier':True,'original_bytes_untouched':True,'fixed_mode1_nine_count':True,'callback_graph_cases':len(fixtures),'shared_self_mutual_factory':True,'nested_proof1_bridge_decode':True}))
if __name__=='__main__':main()

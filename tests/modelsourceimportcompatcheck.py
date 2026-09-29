#!/usr/bin/env python3
"""Manual USLSIG2/3 graphs: independent oracle, simulator, generic C network.
Only complete origin1/origin2 descriptors can differ. No host ABI claims.
"""
import importlib.util,json,pathlib,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'exec'),str(ROOT/'tests'),str(ROOT)]
from exec.pp.sim import run,load
from modelsignature3check import oracle as parse3
from modelgraphequalitycheck import semantic as parse2
U=lambda n:struct.pack('<Q',n)

def desc(kind=1,width=8,align=8,depth=0,unsigned=0,rank=0,format=0,natural=8,flags=0,known=3,origin=1,tag=0,payload=b''):
 return struct.pack('<7Q',depth,0,0,kind,width,unsigned,align)+bytes([rank,format])+U(natural)+bytes([flags,known,origin,tag])+U(len(payload))+payload

def entry(child,ordinal=0,kind=0,align=8,offset=0,bitoffset=0,bitwidth=0,storage=8):
 return U(ordinal)+bytes([kind])+U(align)+struct.pack('<4Q',offset,bitoffset,bitwidth,storage)+child

def aggregate(entries,origin=1,width=8,tag=1,**kw):return desc(5,width,8,origin=origin,tag=tag,payload=U(len(entries))+b''.join(entries),**kw)

def array(child,origin=1,count=2,stride=8):return desc(5,count*stride,8,origin=origin,tag=3,payload=U(count)+U(stride)+child)

def callback(origin=1,ident=1,params=(),result=None,reference=False,var=0,mode=None):
 if mode is None:mode=int(var or len(params)>6)
 payload=bytes([1,int(reference)])+U(ident)
 if not reference:payload+=bytes([var,mode])+U(len(params))+(desc(origin=origin) if result is None else result)+U(len(params))+b''.join(params)+b'\0'
 return desc(4,depth=1,origin=origin,tag=4,payload=payload)

def sig(params=(),result=None):
 return b'USLSIG3\n'+U(1)+U(4)+b'host'+bytes([0,1,0,int(len(params)>6)])+U(len(params))+(desc() if result is None else result)+U(len(params))+b''.join(params)+b'\0'

def graph(blob):
 if blob[:8]==b'USLSIG3\n':return parse3(blob)[0],3
 root=parse2(blob)
 def convert(node,seen):
  if id(node) in seen:return
  seen.add(id(node));node['entries']=node.pop('layout')
  for child in node['edges']:convert(child,seen)
 convert(root,set());return root,2

def compare(left,right,compatible=False):
 a,av=graph(left);b,bv=graph(right)
 if av!=bv:return False
 queue=[(a,b)];seen=set()
 while queue:
  a,b=queue.pop();key=(id(a),id(b))
  if key in seen:continue
  seen.add(key);af,bf=a['fields'],b['fields']
  if af!=bf:
   # Descriptor fields are depth/kind/width/unsigned/align, then six metadata
   # facts and tag. The oracle parses actual wire independently of MG banks.
   if not (compatible and av==3 and len(af)>=12 and len(af)==len(bf) and
           isinstance(af[0],int) and af[:10]==bf[:10] and af[11:]==bf[11:] and
           af[9]==bf[9]==3 and {af[10],bf[10]}=={1,2}):return False
  if a['entries']!=b['entries'] or len(a['edges'])!=len(b['edges']):return False
  queue.extend(zip(a['edges'],b['edges']))
 return True

class Files:
 def __init__(self,a,b):self.a=a;self.b=b
 def get(self,key):return self.a if key==b'left' else self.b if key==b'right' else None

def fixtures():
 pairs=[];bad=[]
 def pair(name,a,b,want):pairs.append((name,a,b,want))
 def wrap(d):return sig((d,))
 one,two=desc(),desc(origin=2)
 pair('complete scalar',wrap(one),wrap(two),True)
 pair('same source',wrap(one),wrap(one),True)
 pair('same external',wrap(two),wrap(two),True)
 pair('same unknown',wrap(desc(origin=0)),wrap(desc(origin=0)),True)
 pair('same incomplete external',wrap(desc(origin=2,known=1)),wrap(desc(origin=2,known=1)),True)
 pair('incomplete origin bridge',wrap(desc(origin=0,known=1)),wrap(desc(origin=2,known=1)),False)
 def nested(o):return aggregate((entry(array(desc(origin=o),o),storage=16),entry(aggregate((entry(desc(origin=o)),),o),ordinal=1,offset=16,storage=8)),o,width=24)
 pair('nested aggregate array',wrap(nested(1)),wrap(nested(2)),True)
 def cycle(o):return callback(o,params=(callback(o,reference=True),))
 def mutual(o):return callback(o,params=(callback(o,2,params=(callback(o,reference=True),)),))
 pair('self cycle',wrap(cycle(1)),wrap(cycle(2)),True)
 pair('self vs two cycle',wrap(cycle(1)),wrap(mutual(2)),True)
 def shared(o):return sig((callback(o,params=(desc(origin=o),)),callback(o,reference=True)))
 def copied(o):return sig((callback(o,params=(desc(origin=o),)),callback(o,2,params=(desc(origin=o),))))
 pair('shared vs duplicate',shared(1),copied(2),True)
 for name,kw in [('depth',dict(depth=1)),('width',dict(width=4)),('alignment',dict(align=4)),('signedness',dict(unsigned=1)),('natural',dict(natural=4)),('flags',dict(flags=1)),('known',dict(known=1)),('unknown origin',dict(origin=0)),('unknown incomplete',dict(origin=0,known=0)),('incomplete external',dict(known=0))]:
  args=dict(origin=2);args.update(kw);pair(name,wrap(one),wrap(desc(**args)),False)
 pair('kind',wrap(one),wrap(desc(kind=2,origin=2)),False)
 pair('FP rank',wrap(desc(3,rank=2,format=2)),wrap(desc(3,rank=3,format=2,origin=2)),False)
 pair('FP format',wrap(desc(3,16,16,rank=3,format=3,natural=16)),wrap(desc(3,16,16,rank=3,format=4,natural=16,origin=2)),False)
 baseline=aggregate((entry(one,kind=1,bitoffset=1,bitwidth=3),))
 for name,kw in [('member kind',dict(kind=2)),('effective alignment',dict(align=4)),('byte offset',dict(offset=1)),('bit offset',dict(bitoffset=2)),('bit width',dict(bitwidth=2))]:
  args=dict(kind=1,bitoffset=1,bitwidth=3);args.update(kw)
  # Byte offset needs spare room to stay structurally valid.
  a=baseline if name!='byte offset' else aggregate((entry(one,kind=1,bitoffset=1,bitwidth=3),),width=16)
  pair(name,wrap(a),wrap(aggregate((entry(two,**args),),2,width=16 if name=='byte offset' else 8)),False)
 pair('storage bytes',wrap(aggregate((entry(one),))),wrap(aggregate((entry(desc(width=4,origin=2),storage=4),),2)),False)
 pair('aggregate tag',wrap(aggregate((entry(one),))),wrap(aggregate((entry(two),),2,tag=2)),False)
 pair('member count',wrap(aggregate((entry(one),),tag=2)),wrap(aggregate((entry(two),entry(two,ordinal=1)),2,tag=2)),False)
 pair('member order',wrap(aggregate((entry(one),entry(desc(unsigned=1),ordinal=1,offset=8)),width=16)),wrap(aggregate((entry(desc(unsigned=1,origin=2)),entry(two,ordinal=1,offset=8)),2,width=16)),False)
 pair('array dimensions',wrap(array(one)),wrap(array(desc(width=4,origin=2),2,count=4,stride=4)),False)
 pair('callback mode',wrap(callback(params=(one,))),wrap(callback(2,params=(two,),var=1)),False)
 pair('callback result',wrap(callback()),wrap(callback(2,result=desc(unsigned=1,origin=2))),False)
 pair('callback ninth parameter',wrap(callback(params=(one,)*9)),wrap(callback(2,params=(two,)*8+(desc(unsigned=1,origin=2),))),False)
 from modeltypedcandidatescheck import sig as sig2,I as i2,D as d2
 from modelcallbackgraphcheck import definition as def2,ref as ref2
 pair('V2 unchanged',sig2(params=(i2,)),sig2(params=(i2,)),True)
 pair('V2 unequal',sig2(params=(i2,)),sig2(params=(d2,)),False)
 v2cycle=sig2(params=(def2(1,params=(ref2(1),),support=0),),supported=0)
 pair('V2 cyclic unchanged',v2cycle,v2cycle,True)
 pair('V2 vs V3',sig2(params=(i2,)),wrap(one),False)
 bad += [wrap(desc(origin=1,known=1)),wrap(aggregate((entry(two,ordinal=1),),2)),wrap(callback(2,mode=1)),shared(1)[:-1],wrap(callback(2,reference=True))]
 return pairs,bad

def main():
 runtime=sys.argv[1]
 spec=importlib.util.spec_from_file_location('compatmodel',ROOT/'exec/parse/gen.py');E=importlib.util.module_from_spec(spec);spec.loader.exec_module(E)
 from modelgraphequality import install
 install(E);p=E.P('START')
 for key,reg in ((b'left','mg_left_blob'),(b'right','mg_right_blob')):p.a(('SBCLR',),*[('SBOUT',c) for c in key],('SBFIND',reg),('BLEN',reg.replace('blob','len'),reg))
 # Every fixture executes both entry points repeatedly in one model invocation.
 # An equality call following compatibility must reset its mode every time.
 operations=('MG.equal','MG.compatible','MG.equal','MG.compatible','MG.equal')
 for operation in operations:p.call(operation).a(('OUTW','mg_equal'))
 p.goto('DONE');E.P('DONE').a(('ACCEPT',)).goto('DONE');E.g.finish()
 d={'start':'START','states':{n:[m,{str(k):v for k,v in row.items()}] for n,(m,row) in E.g.st.items()},'seqs':[list(map(list,s)) for s in E.g.seqs]};loaded=load(d)
 with tempfile.TemporaryDirectory(prefix='r10-import-compat-') as temp:
  t=pathlib.Path(temp);model=t/'m.json';model.write_text(json.dumps(d));tbl=t/'m.tbl';net=t/'m.net'
  def cmd(*args):
   cp=subprocess.run(list(map(str,args)),capture_output=True,timeout=20);assert cp.returncode==0,(args,cp.stderr);return cp.stdout
  cmd(sys.executable,ROOT/'exec/c/tbl.py',model,tbl);cmd(sys.executable,ROOT/'exec/c/net.py',tbl,net);full=cmd(runtime,'--check-net',tbl,net).decode().strip()
  sys.path.insert(0,str(ROOT/'exec/c'));from pack import build
  routes=t/'routes';routes.write_text('test\tcompat\tbytes\tbytes\tm.net\n');rd=t/'resources';rd.mkdir();src=t/'empty';src.write_bytes(b'');pkg=t/'p';checks=0
  def check(name,a,b,expected=None):
   nonlocal checks
   checks+=1;status,out,_=run(d,b'','fixture',files=Files(a,b),loaded=loaded,maxsteps=12000000)
   (rd/'left').write_bytes(a);(rd/'right').write_bytes(b);pkg.write_bytes(build([routes],[('',rd)],cache=False));cp=subprocess.run([runtime,'--bundle',str(pkg),'test',str(src)],capture_output=True,timeout=10)
   assert (cp.returncode==0)==(status=='accept'),(name,status,cp.returncode,cp.stderr)
   if status=='accept':assert cp.stdout==out,(name,cp.stdout,out)
   if expected is None:assert status=='reject',name
   else:assert status=='accept' and out==expected,(name,status,out,expected)
  pairs,bad=fixtures()
  for name,a,b,want in pairs:
   strict=compare(a,b);assert compare(a,b,True)==want,name
   expected=bytes([strict,want,strict,want,strict]);check(name,a,b,expected);check(name+' reversed',b,a,expected)
  for a in bad:
   try:graph(a)
   except (AssertionError,IndexError,struct.error):pass
   else:raise AssertionError('oracle accepted malformed fixture')
   check('invalid left',a,pairs[0][1]);check('invalid right',pairs[0][1],a)
  print(json.dumps({'states':len(d['states']),'full_domain':full,'checks':checks,'mixed_operations_per_check':len(operations),'pairs_both_directions':len(pairs)*2,'malformed_both_inputs':len(bad)*2,'oracle':'manual wires + independent recursive graph parser','executors':'oracle + sim + generic C network','scope':'declaration compatibility; strict equality unchanged; no host ABI claims'}))
if __name__=='__main__':main()

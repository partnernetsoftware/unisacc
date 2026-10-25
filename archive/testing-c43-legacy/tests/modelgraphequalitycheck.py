#!/usr/bin/env python3
"""Independent recursive canonical-byte oracle; C network versus simulator."""
import importlib.util,json,pathlib,struct,sys,tempfile,subprocess
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'exec'),str(ROOT/'tests'),str(ROOT)]
from modeltypedcandidatescheck import U,d2,I,D,F,PTR,PAIR,sig,Files
import assemble
install=lambda E,fail='DEAD':assemble.run(assemble.FACTS.parent/'modelgraphequality-manifest.tsv',E,E.P,{},dict(fail=fail))  # modelgraphequality-manifest.tsv
from exec.pp.sim import run,load
spec=importlib.util.spec_from_file_location('canonicalbase',ROOT/'exec/build/parsebase.py');E=importlib.util.module_from_spec(spec);spec.loader.exec_module(E)
from modelcallbackgraphcheck import canonical,ref,definition

def semantic(blob):
 canonical(blob) # Independent validation before building our independent graph.
 p=16;n=struct.unpack_from('<Q',blob,p)[0];p+=8+n;var=blob[p+2];p+=4;count=struct.unpack_from('<Q',blob,p)[0];p+=8;ids={}
 def word():
  nonlocal p
  v=struct.unpack_from('<Q',blob,p)[0];p+=8;return v
 def byte():
  nonlocal p
  v=blob[p];p+=1;return v
 def desc():
  nonlocal p
  f=[word() for _ in range(7)];tag=byte();length=word();end=p+length
  fields=(f[0],f[3],f[4],f[5],f[6],tag);edges=[];layouts=[]
  if tag in (1,2):
   n=word();fields+=(n,)
   for _ in range(n):layouts.append(tuple(word() for _ in range(4)));edges.append(desc())
  elif tag==3:fields+=(word(),word());edges.append(desc())
  elif tag==4 and length:
   assert byte()==1;form=byte();ident=word()
   if form==0:
    child={'fields':None,'edges':[],'layout':[]};ids[ident]=child
    v=byte();mode=byte();n=word();child['fields']=('sig',v,mode,n);child['edges'].append(desc());assert word()==n
    for _ in range(n):child['edges'].append(desc())
    byte()
   edges.append(ids[ident])
  assert p==end;return {'fields':fields,'edges':edges,'layout':layouts}
 root={'fields':('root',var,count),'edges':[desc()],'layout':[]};assert word()==count
 for _ in range(count):root['edges'].append(desc())
 byte();assert p==len(blob);return root

def equal(left,right):
 queue=[(semantic(left),semantic(right))];seen=set()
 while queue:
  a,b=queue.pop();pair=(id(a),id(b))
  if pair in seen:continue
  seen.add(pair)
  if a['fields']!=b['fields'] or a['layout']!=b['layout'] or len(a['edges'])!=len(b['edges']):return False
  queue.extend(zip(a['edges'],b['edges']))
 return True

class GraphFiles:
 def __init__(self,left,right):self.left=left;self.right=right
 def get(self,key):return self.left if key==b'left' else self.right if key==b'right' else None

def main():
 runtime=sys.argv[1];install(E)
 p=E.P('START')
 for key,reg in ((b'left','mg_left_blob'),(b'right','mg_right_blob')):
  p.a(('SBCLR',),*[('SBOUT',x) for x in key],('SBFIND',reg),('BLEN',reg.replace('blob','len'),reg))
 p.call('MG.equal').a(('OUTW','mg_equal')).goto('DONE')
 E.P('DONE').a(('ACCEPT',)).goto('DONE');E.g.finish();d={'start':'START','states':{n:[m,{str(k):v for k,v in r.items()}] for n,(m,r) in E.g.st.items()},'seqs':[list(map(list,s)) for s in E.g.seqs]};loaded=load(d)
 with tempfile.TemporaryDirectory(prefix='r10-canonical-') as name:
  t=pathlib.Path(name);model=t/'m.json';model.write_text(json.dumps(d));tbl=t/'m.tbl';net=t/'m.net'
  def cmd(*args):
   p=subprocess.run(list(map(str,args)),capture_output=True,timeout=20);assert p.returncode==0,(args,p.stderr);return p.stdout
  cmd(sys.executable,ROOT/'exec/c/tbl.py',model,tbl);cmd(sys.executable,ROOT/'exec/c/net.py',tbl,net);full=cmd(runtime,'--check-net',tbl,net).decode().strip()
  sys.path.insert(0,str(ROOT/'exec/c'));from pack import build
  mf=t/'routes';mf.write_text('test\tcanonical\tbytes\tbytes\tm.net\n');rd=t/'r';rd.mkdir();src=t/'empty';src.write_bytes(b'');pkg=t/'p'
  def check(left,right,expected=None):
   status,out,_=run(d,b'','fixture',files=GraphFiles(left,right),loaded=loaded,maxsteps=4000000)
   (rd/'left').write_bytes(left);(rd/'right').write_bytes(right);pkg.write_bytes(build([mf],[('',rd)],cache=False));cp=subprocess.run([runtime,'--bundle',str(pkg),'test',str(src)],capture_output=True,timeout=10)
   assert (cp.returncode==0)==(status=='accept'),(status,cp.returncode,cp.stderr)
   if status=='accept':assert cp.stdout==out
   if expected is not None:assert status=='accept' and out==bytes([expected]),(status,out,expected)
   return status,out
  leaf=definition(1,params=(I,D,F,PTR,PAIR,I,D,I,I),result=PAIR)
  shared=sig(params=(leaf,ref(1)),supported=0)
  copied=sig(params=(leaf,definition(2,params=(I,D,F,PTR,PAIR,I,D,I,I),result=PAIR)),supported=0)
  selfgraph=sig(params=(definition(1,params=(ref(1),),support=0),),supported=0)
  twocycle=sig(params=(definition(1,params=(definition(2,params=(ref(1),),support=0),),support=0),),supported=0)
  parserids=sig(params=(definition(1,params=(d2(base=234,shape=456),),base=678,shape=999),),supported=0)
  clean=sig(params=(definition(1,params=(I,)),),supported=0)
  support0=sig(params=(definition(1,params=(I,),support=0),),supported=0)
  bad9=sig(params=(definition(1,params=(I,D,F,PTR,PAIR,I,D,I,D),result=PAIR),ref(1)),supported=0)
  badresult=sig(params=(definition(1,params=(I,D,F,PTR,PAIR,I,D,I,I),result=I),ref(1)),supported=0)
  modefixed=sig(params=(definition(1,params=(I,),var=0,mode=0),),supported=0)
  modevar=sig(params=(definition(1,params=(I,),var=1,mode=1,support=0),),supported=0)
  offset=d2(5,16,8,U(2)+U(1)+U(0)+U(0)+U(8)+D+U(8)+U(0)+U(0)+U(4)+d2(1,4,4),1,base=7001)
  pairs=[(shared,copied,True),(selfgraph,twocycle,True),(parserids,clean,True),(clean,support0,True),(shared,bad9,False),(shared,badresult,False),(modefixed,modevar,False),(sig(params=(PAIR,)),sig(params=(offset,)),False),(sig(),sig(),True),(sig(params=(I,D,F,PTR,PAIR,I,D,I,I),result=PAIR),sig(params=(I,D,F,PTR,PAIR,I,D,I,I),result=PAIR),True),(sig(params=(I,)),sig(params=(D,)),False)]
  array=d2(5,16,8,U(2)+U(8)+I,3)
  otherarray=d2(5,16,8,U(1)+U(16)+PAIR,3)
  bitfield=d2(5,8,8,U(1)+U(0)+U(1)+U(3)+U(8)+I,1)
  bitfield2=d2(5,8,8,U(1)+U(0)+U(2)+U(3)+U(8)+I,1)
  union=d2(5,8,8,U(1)+bytes(24)+U(8)+I,2)
  structure=d2(5,8,8,U(1)+bytes(24)+U(8)+I,1)
  pairs += [(sig(params=(array,)),sig(params=(array,)),True),(sig(params=(array,)),sig(params=(otherarray,)),False),(sig(params=(bitfield,),supported=0),sig(params=(bitfield2,),supported=0),False),(sig(params=(union,),supported=0),sig(params=(structure,),supported=0),False)]
  for a,b,want in pairs:
   assert equal(a,b)==want
   check(a,b,want);check(b,a,want)
  for malformed in (shared[:-1],sig(params=(ref(1),),supported=0),sig(params=(definition(2),),supported=0)):
   assert check(malformed,shared)[0]=='reject';assert check(shared,malformed)[0]=='reject'
  # Boundary tests seed the same entry states, avoiding enormous source fixtures.
  # A full queue must reject a new pair, but an already visited pair still returns.
  for name,tail,visited,want in [('budget-new',65536,False,'reject'),('budget-repeat',65536,True,'accept')]:
   bs=importlib.util.spec_from_file_location(name,ROOT/'exec/build/parsebase.py');BE=importlib.util.module_from_spec(bs);bs.loader.exec_module(BE);install(BE)
   bp=BE.P('START').a(('LDI','mg_l',1),('LDI','mg_r',2),('LDI','mg_epoch',1),('LDI','mg_tail',tail))
   if visited:bp.a(('LDI','mg_key',65538),('STX','mg_key',436<<40,'mg_epoch'))
   bp.call('MG.enqueue').a(('ACCEPT',)).goto('DONE');BE.P('DONE').a(('ACCEPT',)).goto('DONE');BE.g.finish()
   bd={'start':'START','states':{n:[m,{str(k):v for k,v in row.items()}] for n,(m,row) in BE.g.st.items()},'seqs':[list(map(list,q)) for q in BE.g.seqs]}
   status,_,_=run(bd,b'','budget',maxsteps=10000);assert status==want
   bm=t/(name+'.json');bt=t/(name+'.tbl');bn=t/(name+'.net');bm.write_text(json.dumps(bd));cmd(sys.executable,ROOT/'exec/c/tbl.py',bm,bt);cmd(sys.executable,ROOT/'exec/c/net.py',bt,bn)
   br=t/(name+'-routes');br.write_text('test\tbudget\tbytes\tbytes\t'+bn.name+'\n');pkg.write_bytes(build([br],[],cache=False));cp=subprocess.run([runtime,'--bundle',str(pkg),'test',str(src)],capture_output=True,timeout=10);assert (cp.returncode==0)==(want=='accept')
  print(json.dumps({'states':len(d['states']),'pair65536_boundary_and_duplicate':True,'full_domain':full,'semantic_pairs_both_directions':len(pairs)*2,'shared_vs_duplicate':True,'self_vs_two_cycle':True,'ninth_result_mode_offset_inequality':True,'parser_ids_and_support_ignored':True,'malformed_both_inputs_rejected':6,'executors':'independent oracle + sim + C network','scope':'signature graph ABI equality only; callbacks unsupported'}))
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Independent recursive canonical-byte oracle; C network versus simulator."""
import importlib.util,json,pathlib,struct,sys,tempfile,subprocess
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'exec'),str(ROOT/'tests'),str(ROOT)]
from modeltypedcandidatescheck import U,d2,I,D,F,PTR,PAIR,sig,Files
import assemble
install=lambda E,fail='DEAD':assemble.run(assemble.FACTS.parent/'modelsignature-manifest.tsv',E,E.P,{},dict(fail=fail))  # modelsignature-manifest.tsv
from exec.pp.sim import run,load
spec=importlib.util.spec_from_file_location('canonicalbase',ROOT/'exec/build/parsebase.py');E=importlib.util.module_from_spec(spec);spec.loader.exec_module(E)
def canonical(blob):
 """Independent bounded parser, graph registration and canonical byte oracle."""
 p=0;ids={};nodes=0;callbacks=0
 def take(n,end):
  nonlocal p
  assert p+n<=end
  out=blob[p:p+n];p+=n;return out
 def word(end):return int.from_bytes(take(8,end),'little')
 def bit(end):
  v=take(1,end)[0];assert v in (0,1);return v
 def desc(end,level=1):
  nonlocal nodes,callbacks
  nodes+=1;assert level<=32 and nodes<=16384
  f=[word(end) for _ in range(7)];depth,base,shape,kind,width,unsigned,align=f
  assert kind<=6 and unsigned<=1
  assert width==0 if kind in (0,6) else width in (1,2,4,8) if kind==1 else width==8 if kind in (2,4) else width in (4,8) if kind==3 else 0<width<=16777216
  assert align==0 if kind in (0,6) else 0<align<=65536 and not align&(align-1)
  tag=take(1,end)[0];length=word(end);stop=p+length;assert stop<=end and length<=16777216
  f[1]=f[2]=0;q=struct.pack('<7Q',*f)+bytes([tag])+U(length)
  if kind==4:
   callbacks+=1;assert tag in (0,4)
   if length:
    assert tag==4 and depth==1 and align==8
    schema=take(1,stop)[0];form=take(1,stop)[0];ident=word(stop)
    assert schema==1 and form in (0,1) and 1<=ident<=1024
    if form==0:
     assert ident==len(ids)+1 and ident not in ids;ids[ident]=len(ids)+1
    else:assert ident in ids
    q+=bytes([schema,form])+U(ids[ident])
    if form==0:
     baseline=callbacks;var=bit(stop);mode=bit(stop);count=word(stop)
     assert count<=1024 and mode==int(var or count>6)
     q+=bytes([var,mode])+U(count)+desc(stop,level+1)
     assert word(stop)==count;q+=U(count)
     for _ in range(count):q+=desc(stop,level+1)
     support=bit(stop);assert callbacks==baseline or support==0;q+=bytes([support])
  elif kind==5:
   assert tag in (1,2,3)
   if tag in (1,2):
    count=word(stop);assert count<=64;q+=U(count)
    for _ in range(count):q+=take(32,stop)+desc(stop,level+1)
   else:
    count=word(stop);stride=word(stop);assert count<=16777216 and stride<=16777216 and count*stride==width
    q+=U(count)+U(stride)+desc(stop,level+1)
  else:assert tag==0
  assert p==stop;return q
 end=len(blob);assert end<=16777216 and take(8,end)==b'USLSIG2\n' and word(end)==1
 n=word(end);assert 0<n<=1024;name=take(n,end);assert name[:1] in b'_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'
 assert all(c in b'_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789' for c in name)
 assert bit(end)==0 and bit(end)==1;var=bit(end);mode=bit(end);count=word(end)
 assert count<=1024 and mode==int(var or count>6)
 out=bytes([var])+U(count)+desc(end);assert word(end)==count
 for _ in range(count):out+=desc(end)
 support=bit(end);assert callbacks==0 or support==0;assert p==end;return out

def ref(ident,**kw):return d2(4,8,8,bytes([1,1])+U(ident),4,depth=1,**kw)
def definition(ident,params=(),result=I,var=0,mode=None,support=1,**kw):
 if mode is None:mode=int(var or len(params)>6)
 payload=bytes([1,0])+U(ident)+bytes([var,mode])+U(len(params))+result+U(len(params))+b''.join(params)+bytes([support])
 return d2(4,8,8,payload,4,depth=1,**kw)

def main():
 runtime=sys.argv[1];install(E)
 E.P('START').a(('SBCLR',),*[('SBOUT',x) for x in b'input'],('SBFIND','ms_blob'),('BLEN','ms_len','ms_blob')).call('MS.canonical').a(('INPUSH','ms_canon'),('LDI','ms_zero',0),('SPAN2','ms_zero','ms_canonlen'),('INPOP',)).goto('DONE')
 E.P('DONE').a(('ACCEPT',)).goto('DONE');E.g.finish();d={'start':'START','states':{n:[m,{str(k):v for k,v in r.items()}] for n,(m,r) in E.g.st.items()},'seqs':[list(map(list,s)) for s in E.g.seqs]};loaded=load(d)
 with tempfile.TemporaryDirectory(prefix='r10-canonical-') as name:
  t=pathlib.Path(name);model=t/'m.json';model.write_text(json.dumps(d));tbl=t/'m.tbl';net=t/'m.net'
  def cmd(*args):
   p=subprocess.run(list(map(str,args)),capture_output=True,timeout=20);assert p.returncode==0,(args,p.stderr);return p.stdout
  cmd(sys.executable,ROOT/'exec/c/tbl.py',model,tbl);cmd(sys.executable,ROOT/'exec/c/net.py',tbl,net);full=cmd(runtime,'--check-net',tbl,net).decode().strip()
  sys.path.insert(0,str(ROOT/'exec/c'));from pack import build
  mf=t/'routes';mf.write_text('test\tcanonical\tbytes\tbytes\tm.net\n');rd=t/'r';rd.mkdir();src=t/'empty';src.write_bytes(b'');pkg=t/'p'
  def check(blob,expected=None):
   status,out,_=run(d,b'','fixture',files=Files(blob),loaded=loaded,maxsteps=2000000)
   (rd/'input').write_bytes(blob);pkg.write_bytes(build([mf],[('',rd)],cache=False));p=subprocess.run([runtime,'--bundle',str(pkg),'test',str(src)],capture_output=True,timeout=10)
   assert (p.returncode==0)==(status=='accept'),(p.returncode,status,p.stderr)
   if status=='accept':assert p.stdout==out
   if expected is not None:assert status=='accept' and out==expected
   return status,out
  leaf=definition(1,params=(I,D,F,PTR,PAIR,I,D,I,I),result=PAIR)
  shared=sig(params=(leaf,ref(1)),supported=0)
  selfgraph=sig(params=(definition(1,params=(ref(1),),support=0),),supported=0)
  mutual=sig(params=(definition(1,params=(definition(2,params=(ref(1),),support=0),ref(2)),support=0),),supported=0)
  variadic=sig(params=(definition(1,params=(F,I),var=1,support=0),),supported=0)
  old=[sig(),sig(params=(I,D,F,PTR,PAIR,I,D,I,I),result=PAIR),sig(params=(d2(4,8,8,depth=1),),supported=0),sig(params=(d2(4,8,8,tag=4,depth=1),),supported=0)]
  valid=old+[shared,selfgraph,mutual,variadic]
  for blob in valid:check(blob,canonical(blob))
  local=sig(params=(definition(1,params=(d2(1,8,8,base=999,shape=444),),base=222,shape=333),ref(1,base=444,shape=555)),supported=0)
  clean=sig(params=(definition(1,params=(I,)),ref(1)),supported=0)
  assert canonical(local)==canonical(clean);check(local,canonical(clean))
  # Shared versus copied signatures have different topology despite equal leaf ABI.
  copied=sig(params=(leaf,definition(2,params=(I,D,F,PTR,PAIR,I,D,I,I),result=PAIR)),supported=0)
  assert canonical(shared)!=canonical(copied);check(copied,canonical(copied))
  bad=[sig(params=(ref(1),),supported=0),sig(params=(definition(2),),supported=0),sig(params=(definition(1),definition(1)),supported=0),sig(params=(definition(1,params=(ref(2),),support=0),definition(2)),supported=0),sig(params=(definition(1,params=(ref(0),),support=0),),supported=0),sig(params=(ref(1025),),supported=0),sig(params=(definition(1,mode=1),),supported=0),sig(params=(definition(1,params=(ref(1),),support=1),),supported=0),sig(params=(leaf,),supported=1)]
  raw=bytes([1,1])+U(1)
  bad += [sig(params=(d2(4,8,8,payload,4,depth=depth),),supported=0) for payload,depth in [(bytes([2,1])+U(1),1),(bytes([1,2])+U(1),1),(raw+b'x',1),(raw,2)]]
  bad += [sig(params=(d2(4,8,4,raw,4,depth=1),),supported=0),sig(params=(definition(1,params=(I,))[:-1],),supported=0)]
  for blob in bad:
   try:canonical(blob)
   except (AssertionError,IndexError):pass
   else:raise AssertionError('oracle accepted malformed fixture')
   assert check(blob)[0]=='reject'
  trunc=sig(params=(definition(1),),supported=0)
  for n in range(len(trunc)):assert check(trunc[:n])[0]=='reject'
  # Prune must retain unsupported public callback roots and validate full graphs.
  sys.path.insert(0,str(ROOT/'exec/prune'))
  pm=t/'prune.json';cmd(sys.executable,ROOT/'exec/build/gen.py','prune',pm);pd=json.loads(pm.read_text());pl=load(pd);pt=t/'prune.tbl';pn=t/'prune.net'
  cmd(sys.executable,ROOT/'exec/c/tbl.py',pm,pt);cmd(sys.executable,ROOT/'exec/c/net.py',pt,pn);prunefull=cmd(runtime,'--check-net',pt,pn).decode().strip()
  routes=t/'prune-routes';routes.write_text('test\tprune\tbytes\tbytes\tprune.net\n')
  frame=b'  .frame 8\n  store64 [r7+0], r6\n  mov r6, r7\n  .frame 0\n  ret\n'
  tape=b'_start:\n  call main\n  .exit r0\nmain:\n'+frame+b'host:\n'+frame+b'dead:\n'+frame;src.write_bytes(tape)
  expected=tape[:tape.index(b'dead:')]
  class PublicFiles:
   def __init__(self,blob):self.blob=blob
   def get(self,key):return self.blob if key==b'\0library/signatures' else None
  def prunecheck(blob,okay):
   status,out,_=run(pd,tape,'fixture',files=PublicFiles(blob),loaded=pl,maxsteps=2000000)
   (rd/'signatures').write_bytes(blob);pkg.write_bytes(build([routes],[(b'\0library/'.hex(),rd)],cache=False));cp=subprocess.run([runtime,'--bundle',str(pkg),'test',str(src)],capture_output=True,timeout=10)
   assert (cp.returncode==0)==(status=='accept')==okay,(cp.returncode,status,cp.stderr)
   if okay:assert cp.stdout==out==expected,(out,expected)
  multi=b'USLSIG2\n'+U(2)+shared[16:]+sig(name=b'main',params=(definition(1),),supported=0)[16:]
  olddesc=struct.pack('<6Q',0,4,0,1,8,0)
  oldv1=b'USLSIG1\n'+U(1)+U(4)+b'host'+bytes([0,1,0])+U(1)+olddesc+U(1)+olddesc+bytes([1])
  for blob in (shared,selfgraph,mutual,variadic,multi,oldv1):prunecheck(blob,True)
  for blob in bad[:4]:prunecheck(blob,False)
  print(json.dumps({'states':len(d['states']),'full_domain':full,'prune_full_domain':prunefull,'prune_public_graph_roots':4,'prune_record_local_ids_and_v1':True,'prune_malformed_rejected':4,'exact_graph_canonical_shapes':len(valid)+2,'shared_self_mutual':True,'nested_variadic_mode':True,'malformed_controls':len(bad),'truncations':len(trunc),'old_fixed_variadic_framing':True,'executors':'sim+C constructed network','scope':'graph declaration only; callbacks unsupported'}))
if __name__=='__main__':main()

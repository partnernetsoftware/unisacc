#!/usr/bin/env python3
"""Independent hand-built SIG3 graphs: oracle, model simulator and C network.
Run each of the canonical/equal modes with tests/bound.py 55; private outputs.
No source serializer, host decoder, classifier or runtime ABI certification.
"""
import importlib.util,json,pathlib,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'exec'),str(ROOT/'tests'),str(ROOT)]
from exec.pp.sim import run,load
U=lambda n:struct.pack('<Q',n)
def descriptor(kind=1,width=8,align=8,payload=b'',tag=0,depth=0,base=0,shape=0,rank=0,format=0,natural=0,flags=0,known=0,origin=0):
 return struct.pack('<7Q',depth,base,shape,kind,width,0,align)+bytes([rank,format])+U(natural)+bytes([flags,known,origin,tag])+U(len(payload))+payload
I=descriptor();D=descriptor(3,8,8,rank=2,format=2,natural=8,known=3,origin=1)
def entry(child=I,ordinal=0,kind=0,align=8,offset=0,bitoffset=0,bitwidth=0,storage=None):
 if storage is None:storage=struct.unpack_from('<Q',child,32)[0]
 return U(ordinal)+bytes([kind])+U(align)+struct.pack('<4Q',offset,bitoffset,bitwidth,storage)+child
def aggregate(entries=(),width=8,tag=1,**kw):return descriptor(5,width,8,U(len(entries))+b''.join(entries),tag,**kw)
def signature(params=(),result=I):
 return b'USLSIG3\n'+U(1)+U(4)+b'host'+bytes([0,1,0,int(len(params)>6)])+U(len(params))+result+U(len(params))+b''.join(params)+b'\0'
def callback(ident=1,params=(),result=I,reference=False,**kw):
 payload=bytes([1,int(reference)])+U(ident)
 if not reference:payload+=bytes([0,int(len(params)>6)])+U(len(params))+result+U(len(params))+b''.join(params)+b'\0'
 return descriptor(4,8,8,payload,4,depth=1,**kw)
def oracle(blob):
 """Parse independently and keep canonical bytes and a cyclic semantic graph."""
 p=0;nodes=0;ids={}
 def take(n,end):
  nonlocal p
  assert 0<=n<=end-p
  b=blob[p:p+n];p+=n;return b
 def word(end):return int.from_bytes(take(8,end),'little')
 def byte(end):return take(1,end)[0]
 def bit(end):
  b=byte(end);assert b in (0,1);return b
 def pow2(v,zero=False):return (zero and v==0) or 0<v<=65536 and v&(v-1)==0
 def desc(end,level=1):
  nonlocal nodes
  nodes+=1;assert level<=32 and nodes<=16384
  fields=[word(end) for _ in range(7)];depth,_,_,kind,width,unsigned,align=fields
  assert kind<=6 and unsigned<=1
  assert width==0 if kind in (0,6) else width in (1,2,4,8) if kind==1 else width==8 if kind in (2,4) else width in (4,8,16) if kind==3 else 0<width<=16777216
  assert align==0 if kind in (0,6) else pow2(align)
  rank=byte(end);fmt=byte(end);natural=word(end);flags=byte(end);known=byte(end);origin=byte(end)
  assert rank<=3 and fmt<=4 and pow2(natural,True) and flags<=3 and known<=3 and flags&known==flags and origin<=2
  if origin==1:assert known==3 and (natural>0 or kind in (0,6))
  if kind!=3:assert rank==fmt==0
  else:
   if fmt==0:assert rank==0 and origin==0
   assert fmt==0 or width=={1:4,2:8,3:16,4:16}[fmt]
   assert rank==0 or rank==1 and fmt==1 or rank==2 and fmt==2 or rank==3 and fmt in (2,3,4)
  metadata=bytes([rank,fmt])+U(natural)+bytes([flags,known,origin]);tag=byte(end);length=word(end);stop=p+length;assert stop<=end and length<=16777216
  fields[1:3]=[0,0];canon=struct.pack('<7Q',*fields)+metadata+bytes([tag])+U(length)
  node={'fields':tuple(fields[:1]+fields[3:])+(rank,fmt,natural,flags,known,origin,tag),'edges':[],'entries':[]}
  if kind==5:
   assert tag in (1,2,3)
   if tag in (1,2):
    count=word(stop);assert count<=128;canon+=U(count)
    for index in range(count):
     ordinal=word(stop);ek=byte(stop);ea=word(stop);layout=tuple(word(stop) for _ in range(4));off,bo,bw,storage=layout
     assert ordinal==index and ek<=4 and pow2(ea)
     child,cc,cf,ct=desc(stop,level+1);cd,_,_,ck,cw,_,_=cf
     assert storage==cw and off<=width
     if ek in (0,4):
      assert bo==bw==0 and storage<=width-off
      if ek==4:assert ct in (1,2)
     else:
      assert ck==1 and cd==0 and ct==0
      if ek==3:assert bo==bw==0
      else:assert bw>0 and bo+bw<=storage*8 and storage<=width-off
     canon+=U(ordinal)+bytes([ek])+U(ea)+struct.pack('<4Q',*layout)+cc
     node['entries'].append((ordinal,ek,ea)+layout);node['edges'].append(child)
   else:
    count=word(stop);stride=word(stop);assert count<=16777216 and stride<=16777216 and count*stride==width
    child,cc,_,_=desc(stop,level+1);canon+=U(count)+U(stride)+cc;node['fields']+=(count,stride);node['edges'].append(child)
  elif kind==4:
   assert tag in (0,4)
   if length:
    assert tag==4 and depth==1 and align==8 and byte(stop)==1
    form=byte(stop);ident=word(stop);assert form in (0,1) and 1<=ident<=1024
    canon+=bytes([1,form])+U(ident)
    if form==0:
     assert ident==len(ids)+1;sig={'fields':None,'entries':[],'edges':[]};ids[ident]=sig
     var=bit(stop);mode=bit(stop);count=word(stop);assert count<=1024 and mode==int(var or count>6)
     sig['fields']=('sig',var,mode,count);canon+=bytes([var,mode])+U(count)
     child,cc,_,_=desc(stop,level+1);sig['edges'].append(child);canon+=cc;assert word(stop)==count;canon+=U(count)
     for _ in range(count):child,cc,_,_=desc(stop,level+1);sig['edges'].append(child);canon+=cc
     canon+=bytes([bit(stop)])
    assert ident in ids;node['edges'].append(ids[ident])
  else:assert tag==0
  assert p==stop;return node,canon,fields,tag
 end=len(blob);assert end<=16777216 and take(8,end)==b'USLSIG3\n' and word(end)==1
 n=word(end);assert 0<n<=1024;name=take(n,end);assert name[:1] in b'_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';assert all(c in b'_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789' for c in name)
 assert bit(end)==0 and bit(end)==1;var=bit(end);mode=bit(end);count=word(end);assert count<=1024 and mode==int(var or count>6)
 root={'fields':('root',3,var,count),'entries':[],'edges':[]};node,cc,_,_=desc(end);root['edges'].append(node);canon=b'USLSIG3\n'+bytes([var])+U(count)+cc;assert word(end)==count
 for _ in range(count):node,cc,_,_=desc(end);root['edges'].append(node);canon+=cc
 bit(end);assert p==end;return root,canon

def equal(a,b):
 queue=[(oracle(a)[0],oracle(b)[0])];seen=set()
 while queue:
  a,b=queue.pop();key=(id(a),id(b))
  if key in seen:continue
  seen.add(key)
  if a['fields']!=b['fields'] or a['entries']!=b['entries'] or len(a['edges'])!=len(b['edges']):return False
  queue.extend(zip(a['edges'],b['edges']))
 return True

class Files:
 def __init__(self,a,b):self.a=a;self.b=b
 def get(self,key):return self.a if key==b'left' else self.b if key==b'right' else None

def fixtures():
 barrier=aggregate((entry(kind=3,offset=8),));bits=aggregate((entry(kind=1,bitoffset=1,bitwidth=3),entry(kind=2,ordinal=1,bitoffset=4,bitwidth=2)))
 anonymous=aggregate((entry(aggregate((entry(),)),kind=4),))
 ordered=aggregate((entry(),entry(ordinal=1,offset=8)),width=16)
 array=descriptor(5,16,8,U(2)+U(8)+I,3)
 selfcycle=callback(params=(callback(reference=True),))
 twocycle=callback(params=(callback(2,params=(callback(reference=True),)),))
 shared=signature((callback(params=(I,D)),callback(reference=True)))
 copied=signature((callback(params=(I,D)),callback(2,params=(I,D))))
 valid=[signature(),signature((D,)),signature((descriptor(3,16,16,rank=3,format=3,natural=16,known=3,origin=1),)),signature((descriptor(3,16,16,rank=3,format=4),)),signature((descriptor(3,8,8,rank=3,format=2),)),signature((descriptor(3,8,8,format=2),)),signature((barrier,bits,anonymous,ordered,array)),shared,copied,signature((selfcycle,)),signature((twocycle,)),signature((aggregate(tuple(entry(ordinal=i,offset=i*8) for i in range(128)),width=1024),))]
 valid += [signature((descriptor(3,4,4,rank=1,format=1),)),signature((descriptor(3,16,16),)),signature((aggregate((entry(),entry(ordinal=1)),tag=2),)),signature(result=descriptor(0,0,0,known=3,origin=1)),signature((aggregate((entry(),),natural=8,flags=3,known=3,origin=2),))]
 badmeta=[dict(rank=1),dict(format=1),dict(natural=3),dict(flags=1),dict(known=4),dict(origin=3),dict(origin=1,known=0,natural=8),dict(origin=1,known=3,natural=0)]
 malformed=[signature((descriptor(**kw),)) for kw in badmeta]
 malformed += [signature((descriptor(3,8,8,rank=1,format=2),)),signature((descriptor(3,8,8,rank=2,format=0),)),signature((descriptor(3,8,8,rank=3,format=1),)),signature((descriptor(3,8,8,format=3),))]
 malformed += [signature((aggregate((entry(**kw),)),)) for kw in [dict(ordinal=1),dict(kind=5),dict(align=0),dict(align=3),dict(offset=1),dict(bitwidth=1),dict(storage=4),dict(kind=4),dict(kind=1,bitwidth=0),dict(kind=2,bitoffset=63,bitwidth=2),dict(kind=3,bitwidth=1),dict(kind=3,bitoffset=1),dict(kind=3,offset=9),dict(kind=1,bitoffset=1,bitwidth=(1<<64)-1),dict(kind=1,child=D,bitwidth=1)]]
 malformed += [signature((descriptor(3,16,16,origin=2),)),signature((descriptor(3,16,16,natural=16,known=3,origin=1),))]
 malformed += [shared[:cut] for cut in (0,7,15,24,40,95,96,97,105,108,109,117,127,137,145,180,220)]
 malformed += [signature((aggregate((entry(child=descriptor(depth=1),kind=1,bitwidth=1),)),)),signature((aggregate(tuple(entry(ordinal=i,offset=i*8) for i in range(129)),width=1032),)),shared[:-1],signature((callback(reference=True),))]
 pairs=[(a,a,True) for a in valid]+[(shared,copied,True),(signature((selfcycle,)),signature((twocycle,)),True),(signature((descriptor(base=222,shape=333),)),signature(),False),(signature((descriptor(base=222,shape=333),)),signature((I,)),True)]
 # Individually vary every descriptor metadata fact and each non-ordinal entry fact.
 for kw in [dict(natural=8),dict(flags=1,known=3),dict(known=3),dict(natural=8,known=3,origin=1),dict(origin=2)]:pairs.append((signature((I,)),signature((descriptor(**kw),)),False))
 pairs += [(signature((descriptor(3,8,8,format=2),)),signature((D,)),False),(signature((descriptor(3,16,16,rank=3,format=3),)),signature((descriptor(3,16,16,rank=3,format=4),)),False),(signature((aggregate((entry(kind=1,bitwidth=1),)),)),signature((aggregate((entry(kind=2,bitwidth=1),)),)),False),(signature((aggregate((entry(),)),)),signature((aggregate((entry(align=4),)),)),False)]
 pairs += [(signature((descriptor(known=3),)),signature((descriptor(known=3,flags=1),)),False),(signature((descriptor(3,8,8,format=2),)),signature((descriptor(3,8,8,format=2,rank=2),)),False),(signature((aggregate((entry(kind=1,bitoffset=1,bitwidth=2),)),)),signature((aggregate((entry(kind=1,bitoffset=2,bitwidth=2),)),)),False),(signature((aggregate((entry(kind=1,bitoffset=1,bitwidth=2),)),)),signature((aggregate((entry(kind=1,bitoffset=1,bitwidth=3),)),)),False)]
 return valid,malformed,pairs

def main():
 runtime,mode=sys.argv[1:3];assert mode in ('canonical','equal')
 spec=importlib.util.spec_from_file_location('sig3model',ROOT/'exec/parse/gen.py');E=importlib.util.module_from_spec(spec);spec.loader.exec_module(E)
 if mode=='canonical':
  from modelsignature import install
  install(E);E.P('START').a(('SBCLR',),*[('SBOUT',c) for c in b'left'],('SBFIND','ms_blob'),('BLEN','ms_len','ms_blob')).call('MS.canonical').a(('INPUSH','ms_canon'),('LDI','ms_zero',0),('SPAN2','ms_zero','ms_canonlen'),('INPOP',)).goto('DONE')
 else:
  from modelgraphequality import install
  install(E);p=E.P('START')
  for key,reg in ((b'left','mg_left_blob'),(b'right','mg_right_blob')):p.a(('SBCLR',),*[('SBOUT',c) for c in key],('SBFIND',reg),('BLEN',reg.replace('blob','len'),reg))
  p.call('MG.equal').a(('OUTW','mg_equal')).goto('DONE')
 E.P('DONE').a(('ACCEPT',)).goto('DONE');E.g.finish();d={'start':'START','states':{n:[m,{str(k):v for k,v in r.items()}] for n,(m,r) in E.g.st.items()},'seqs':[list(map(list,s)) for s in E.g.seqs]};loaded=load(d)
 with tempfile.TemporaryDirectory(prefix='r10-sig3-model-') as temp:
  t=pathlib.Path(temp);model=t/'m.json';model.write_text(json.dumps(d));tbl=t/'m.tbl';net=t/'m.net'
  def cmd(*args):
   cp=subprocess.run(list(map(str,args)),capture_output=True,timeout=20);assert cp.returncode==0,(args,cp.stderr);return cp.stdout
  cmd(sys.executable,ROOT/'exec/c/tbl.py',model,tbl);cmd(sys.executable,ROOT/'exec/c/net.py',tbl,net);full=cmd(runtime,'--check-net',tbl,net).decode().strip()
  sys.path.insert(0,str(ROOT/'exec/c'));from pack import build
  mf=t/'routes';mf.write_text('test\tsig3\tbytes\tbytes\tm.net\n');rd=t/'resources';rd.mkdir();src=t/'empty';src.write_bytes(b'');pkg=t/'p';checks=0
  def check(a,b=None,expected=None):
   nonlocal checks
   checks+=1;b=a if b is None else b;status,out,_=run(d,b'','fixture',files=Files(a,b),loaded=loaded,maxsteps=5000000)
   (rd/'left').write_bytes(a);(rd/'right').write_bytes(b);pkg.write_bytes(build([mf],[('',rd)],cache=False));cp=subprocess.run([runtime,'--bundle',str(pkg),'test',str(src)],capture_output=True,timeout=10)
   assert (cp.returncode==0)==(status=='accept'),(status,cp.returncode,cp.stderr)
   if status=='accept':assert cp.stdout==out
   if expected is not None:assert status=='accept' and out==expected,(checks,status,out,expected)
   return status
  valid,bad,pairs=fixtures()
  if mode=='canonical':
   for a in valid:check(a,expected=oracle(a)[1])
   for a in bad:
    try:oracle(a)
    except (AssertionError,IndexError,struct.error):pass
    else:raise AssertionError('independent oracle accepted malformed fixture')
    assert check(a)=='reject'
   # Control: V2 bytes remain exactly the pre-V3 independent oracle output.
   from modelcallbackgraphcheck import canonical
   from modeltypedcandidatescheck import sig
   for a in (sig(),sig(params=())):check(a,expected=canonical(a))
  else:
   for a,b,want in pairs:
    assert equal(a,b)==want;check(a,b,bytes([want]));check(b,a,bytes([want]))
   from modeltypedcandidatescheck import sig
   check(sig(),signature(),b'\0');check(signature(),sig(),b'\0')
   for a in bad[:8]+bad[-4:]:assert check(a,valid[0])=='reject';assert check(valid[0],a)=='reject'
  print(json.dumps({'mode':mode,'checks':checks,'states':len(d['states']),'full_domain':full,'executors':'independent manual wire oracle + sim + C network','valid_graphs':len(valid),'malformed_graphs':len(bad),'scope':'SIG3 framing/equality; no ABI certification'}))
if __name__=='__main__':main()

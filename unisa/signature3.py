"""Shared SIG3 observation oracle: canonical bytes and semantic graph.
Moved verbatim from modelsignature3check; reference/tool only, not runtime ABI.
"""
import struct
U=lambda n:struct.pack('<Q',n)
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


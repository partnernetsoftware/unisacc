#!/usr/bin/env python3
"""Source E2/E1/E3 provenance, including conservative multi-TU merging.

This checks source facts and framing, not native call ABI compatibility.
Models are explicit inputs so generation and verification remain bounded jobs.
"""
import argparse,json,pathlib,struct,sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT),str(ROOT/'tests')]
from exec.pp.sim import run,load
from exec.lex.sim import run as lexrun
from modelsourcefacts3check import decode,U
MAGIC=b'USLFACT1\n'
SOURCE=b'struct B { unsigned int x:3; unsigned int :2; unsigned int :0; unsigned int y:4; }; struct B flip(struct B v){return v;}\n'

class Files:
 def __init__(self,enabled=True):self.enabled=enabled
 def get(self,key):
  if key==b'\0library/sourcefacts':return U(1) if self.enabled else None
  if key in (b'\0library/module',b'\0library/symbols'):return U(1)
  if key==b'\0library/signatureversion':return U(3)
  return None

def unpack(data,stage):
 assert len(data)>=20 and data[:9]==MAGIC and data[9]==stage and data[10] in (0,1) and data[11]==1
 assert struct.unpack_from('<Q',data,12)[0]==len(data)-20
 return data[10],data[20:]

def metadata(records,clean):
 seen=set();count=0
 def desc(d):
  nonlocal count
  if id(d) in seen:return
  seen.add(id(d));count+=1
  rank,fmt,natural,flags,known,origin=d['facts'];kind=d['fields'][3]
  expected=clean and kind!=6
  assert (natural,flags,known,origin)==(d['fields'][6],0,3,1) if expected else (natural,flags,known,origin)==(0,0,0,0),d
  if d['tag'] in (1,2):
   for entry in d['child']:desc(entry['child'])
  elif d['tag']==3:desc(d['child'][2])
  elif d['tag']==4 and d['child']:
   g=d['child'];desc(g['result'])
   for parameter in g['params']:desc(parameter)
 for f in records.values():
  assert f['support']==0
  desc(f['result'])
  for parameter in f['params']:desc(parameter)
 assert count
 return count

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--e2',required=True,type=pathlib.Path);ap.add_argument('--e1',required=True,type=pathlib.Path);ap.add_argument('--e3',required=True,type=pathlib.Path);ap.add_argument('--units',required=True,type=pathlib.Path);ap.add_argument('--locations',action='store_true');ap.add_argument('--evidence',type=pathlib.Path);a=ap.parse_args()
 models=[json.loads(p.read_text()) for p in (a.e2,a.e1,a.e3,a.units)];loaded={i:load(models[i]) for i in (0,2,3)}
 f=Files();e={'schema':1,'scope':__doc__,'status':'failed','source_cases':[],'unit_cases':[]}
 def simulate(i,data,files=f):
  return lexrun(models[i],data,files=files,maxsteps=5000000) if i==1 else run(models[i],data,'source.c',files=files,loaded=loaded[i],maxsteps=5000000)
 def accept(i,data,files=f):
  status,out,n=simulate(i,data,files);assert status=='accept',(i,status,out);return out
 def tokens(source):return accept(1,accept(0,source))
 def parse(data,clean):
  out=accept(2,data);tape,meta,records=decode(out);return metadata(records,clean)
 try:
  probes=[('ordinary',SOURCE,1),('inactive-pragma',b'#if 0\n#pragma pack(1)\n#endif\n'+SOURCE,1),
    ('unused-macro',b'#define PACK _Pragma("pack(1)")\n'+SOURCE,1),
    ('active-pragma',b'#pragma pack(push,1)\n'+SOURCE+b'#pragma pack(pop)\n',0),
    ('direct-pragma-op',b'_Pragma("pack(1)")\n'+SOURCE,0),
    ('macro-pragma-op',b'#define PACK _Pragma("pack(1)")\nPACK\n'+SOURCE,0),
    ('attribute',SOURCE.replace(b'struct B {',b'struct __attribute__((packed)) B {'),0),
    ('macro-attribute',b'#define ATTR __attribute__((packed))\n'+SOURCE.replace(b'struct B {',b'struct ATTR B {'),0),
    ('conservative-unused-attribute',SOURCE.replace(b'struct B {',b'struct __attribute__((unused)) B {'),0),
    ('fake-comment',b'/* USLFACT1 clean policy1 */\n#pragma pack(1)\n'+SOURCE,0)]
  for name,source,clean in probes:
   pp=accept(0,source);tok=accept(1,pp);status,payload=unpack(tok,1);assert status==clean,(name,status)
   count=parse(tok,clean);e['source_cases'].append({'name':name,'status':status,'descriptors':count})
  tok=tokens(SOURCE);plain=unpack(tok,1)[1]
  # The same old input without chain evidence remains a projection.
  metadata(decode(accept(2,plain,Files(False)))[2],False)
  malformed=[tok[:i] for i in range(20)]+[tok[:-1],tok+b'x',b'/* USLFACT1 */\n'+plain]
  for position,value in ((9,2),(10,2),(11,2),(12,255),(19,128)):
   bad=bytearray(tok);bad[position]=value;malformed.append(bytes(bad))
  for bad in malformed:assert simulate(2,bad)[0]=='reject'
  e['malformed_rejected']=len(malformed)
  other=tokens(b'int other(int x){return x+1;}\n')
  unknown=tokens(b'#pragma pack(1)\nint other(int x){return x+1;}\n')
  def frame(b,name):
   payload=struct.pack('<I',len(name))+name+b if a.locations else b
   return struct.pack('<I',len(payload))+payload
  for name,second,clean in [('all-clean',other,True),('second-unknown',unknown,False)]:
   merged=accept(3,frame(tok,b'a.c')+frame(second,b'b.c'));status,payload=unpack(merged,1);assert status==int(clean)
   count=parse(merged,clean);e['unit_cases'].append({'name':name,'status':status,'descriptors':count})
  for bad in (b'',struct.pack('<I',len(plain))+plain,frame(tok,b'a.c')[:-1]):assert simulate(3,bad)[0]=='reject'
  # A short logical unit must not borrow its header from the next unit.
  for length in range(20):
   assert simulate(3,frame(tok[:length],b'a.c')+frame(other,b'b.c'))[0]=='reject',length
  e['short_unit_headers_rejected']=20
  e['status']='passed'
 finally:
  if a.evidence:a.evidence.write_text(json.dumps(e,indent=2)+'\n')
 print(json.dumps(e,ensure_ascii=False))

if __name__=='__main__':main()

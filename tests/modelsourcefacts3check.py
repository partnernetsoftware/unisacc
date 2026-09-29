#!/usr/bin/env python3
"""Independent source USLSIG3 oracle; no native ABI qualification claim."""
import argparse,hashlib,json,os,pathlib,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/c'),str(ROOT/'tests')]
from exec.pp.sim import run,load
from pack import build
from modellayoutfactscheck import SOURCE as LAYOUT_SOURCE
U=lambda n:struct.pack('<Q',n)
SOURCE=LAYOUT_SOURCE+"""struct Grid {int a[2][3];};
struct Grid grid(struct Grid x){return x;}
typedef struct Shared (*BFn)(struct Gaps);
BFn relay(BFn f){return f;}
struct Node {long (*f)(struct Node);};
long loop(struct Node n){return 0;}
"""
class Files:
 def __init__(self,version=None):self.version=version
 def get(self,k):
  if k in (b'\0library/module',b'\0library/symbols'):return U(1)
  if k==b'\0library/signatureversion':return self.version
  return None

def decode(data):
 assert data[:9]==b'USLTAPE1\n'
 tn,mn=struct.unpack_from('<2Q',data,9);meta=data[25+tn:];assert len(meta)==mn and meta[:8]==b'USLSIG3\n'
 at=8
 def word():
  nonlocal at
  assert at+8<=len(meta);v=struct.unpack_from('<Q',meta,at)[0];at+=8;return v
 def byte():
  nonlocal at
  assert at<len(meta);v=meta[at];at+=1;return v
 def desc():
  nonlocal at
  fields=tuple(word() for _ in range(7));rank,fmt=byte(),byte();natural=word();flags,known,origin=byte(),byte(),byte();tag=byte();n=word();end=at+n;assert end<=len(meta)
  d={'fields':fields,'facts':(rank,fmt,natural,flags,known,origin),'tag':tag,'child':None}
  if tag in (1,2):
   d['child']=[]
   for i in range(word()):
    ordinal,kind,align=word(),byte(),word();layout=tuple(word() for _ in range(4));assert ordinal==i
    d['child'].append({'ordinal':ordinal,'kind':kind,'align':align,'layout':layout,'child':desc()})
  elif tag==3:d['child']=(word(),word(),desc())
  elif tag==4 and n:
   assert byte()==1;form,ident=byte(),word()
   if form==0:
    assert ident==len(ids)+1;g={'id':ident};ids[ident]=g;d['child']=g
    g['var']=byte();g['mode']=byte();g['count']=word();g['result']=desc();assert word()==g['count'];g['params']=[desc() for _ in range(g['count'])];g['support']=byte()
   else:assert form==1 and ident in ids;d['child']=ids[ident]
  else:assert n==0
  assert at==end;return d
 records={}
 for _ in range(word()):
  n=word();name=meta[at:at+n].decode();at+=n;flags=tuple(byte() for _ in range(4));count=word();ids={};result=desc();assert word()==count;params=[desc() for _ in range(count)];support=byte()
  records[name]={'flags':flags,'count':count,'result':result,'params':params,'support':support,'graph':ids}
 assert at==len(meta) and records
 return data[25:25+tn],meta,records

def oracle(records):
 def facts(d):
  f=d['fields'];rank=1 if f[3]==3 and f[4]==4 else 2 if f[3]==3 and f[4]==8 else 0;assert d['facts']==(rank,rank,0,0,0,0)
 def primitive(d,kind,width,align,uns=0,depth=0):
  facts(d);assert d['fields'][0]==depth and d['fields'][3:]==(kind,width,uns,align) and d['tag']==0,d
 def record(d,width,align,tag,entries):
  facts(d);assert d['fields'][0]==0 and d['fields'][3:]==(5,width,0,align) and d['tag']==tag and len(d['child'])==len(entries),d
  for i,(e,want) in enumerate(zip(d['child'],entries)):
   assert (e['ordinal'],e['kind'],e['align'],e['layout'])==(i,*want),e
 def bf(d,width,bitoffset,kind=1,offset=0,uns=1):return (kind,4,(offset,bitoffset,width,4))
 shared=[bf(None,3,0),bf(None,5,3,uns=0),bf(None,6,8)]
 gaps=[bf(None,3,0,2),bf(None,5,3),bf(None,0,0,3,4),bf(None,4,0,1,4)]
 for side in ('result','params'):
  get=lambda name:records[name]['result'] if side=='result' else records[name]['params'][0]
  a=get('shared');record(a,4,4,1,shared)
  for e,uns in zip(a['child'],(1,0,1)):primitive(e['child'],1,4,4,uns)
  a=get('gaps');record(a,8,4,1,gaps)
  for e in a['child']:primitive(e['child'],1,4,4,1)
  a=get('ordinary');record(a,16,8,1,[(0,4,(0,0,0,4)),(0,8,(8,0,0,8))]);primitive(a['child'][0]['child'],1,4,4);primitive(a['child'][1]['child'],3,8,8)
  a=get('outer');record(a,16,8,1,[(4,4,(0,0,0,4)),(0,8,(8,0,0,8))]);record(a['child'][0]['child'],4,4,1,shared[:2]);primitive(a['child'][1]['child'],1,8,8)
  a=get('array');record(a,16,4,1,[(0,4,(0,0,0,12)),(0,2,(12,0,0,2))]);arr=a['child'][0]['child'];facts(arr);assert arr['tag']==3 and arr['fields'][3:]==(5,12,0,4) and arr['child'][:2]==(3,4);primitive(arr['child'][2],1,4,4);primitive(a['child'][1]['child'],1,2,2)
  a=get('choice');record(a,8,8,2,[(1,4,(0,0,3,4)),(0,8,(0,0,0,8))])
  a=get('reordered');record(a,16,8,1,[(0,8,(0,0,0,8)),(0,4,(8,0,0,4))])
  a=get('holder');record(a,8,8,1,[(0,8,(0,0,0,8))]);primitive(a['child'][0]['child'],2,8,8,depth=1)
  a=get('grid');record(a,24,4,1,[(0,4,(0,0,0,24))]);arr=a['child'][0]['child'];assert arr['tag']==3 and arr['child'][:2]==(2,12);arr=arr['child'][2];assert arr['tag']==3 and arr['child'][:2]==(3,4);primitive(arr['child'][2],1,4,4)
 relay=records['relay'];assert relay['result']['child'] is relay['params'][0]['child'];sig=relay['result']['child'];assert (sig['count'],sig['mode'],sig['var'],sig['support'])==(1,0,0,0);record(sig['result'],4,4,1,shared);record(sig['params'][0],8,4,1,gaps)
 cycle=records['loop']['params'][0]['child'][0]['child']['child'];assert cycle['params'][0]['child'][0]['child']['child'] is cycle and cycle['support']==0
 assert all(r['support']==0 for r in records.values())
 return {'records':len(records),'source_layout_groups':9,'recursive_callback':True,'two_dimensional_array':True,'unknown_source_facts_preserved':True}

HOST = r"""
#include "exec/c/libraryexports.h"
#include <assert.h>
static us_export *find(us_exports *s,const char*n){for(size_t i=0;i<s->count;i++)if(!strcmp(s->items[i].name,n))return s->items+i;assert(0);return NULL;}
int main(int argc,char **argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);rewind(f);unsigned char*b=malloc(n);assert(b&&fread(b,1,n,f)==(size_t)n);fclose(f);us_exports s={0};char error[200]={0};int rc=us_exports_load_bridge(&s,b,n,error,sizeof error);if(rc)fprintf(stderr,"%s\n",error);assert(!rc&&s.count==12);
 for(size_t i=0;i<s.count;i++){us_export*x=s.items+i;assert(x->version==3&&!x->supported&&!x->result.ffi&&!us_export_bridge_supported(x));assert(x->result.layout_origin==0&&x->result.layout_known_mask==0);}
 us_export_type*t=&find(&s,"shared")->result;assert(t->nmembers==3);for(size_t i=0;i<3;i++){assert(t->members[i].ordinal==i&&t->members[i].entry_kind==1&&t->members[i].effective_alignment==4);}assert(t->members[1].bit_offset==3&&t->members[2].bit_offset==8);
 t=&find(&s,"gaps")->result;assert(t->nmembers==4&&t->members[0].entry_kind==2&&t->members[2].entry_kind==3&&t->members[2].offset==4&&t->members[2].bit_width==0);
 t=&find(&s,"outer")->result;assert(t->members[0].entry_kind==4&&t->members[0].type->nmembers==2);
 t=find(&s,"grid")->result.members[0].type;assert(t->tag==3&&t->count==2&&t->stride==12);t=t->element;assert(t->tag==3&&t->count==3&&t->stride==4);
 us_export*x=find(&s,"relay");assert(x->result.signature==x->argtypes[0].signature&&!x->result.signature->supported);
 x=find(&s,"loop");us_export_signature*c=x->argtypes[0].members[0].type->signature;assert(c&&c->argtypes[0].members[0].type->signature==c);
 us_exports_clear(&s);us_exports_clear(&s);free(b);puts("12 source records decoded; ordered facts and cyclic ownership preserved");return 0;}
"""

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--model',type=pathlib.Path);ap.add_argument('--evidence',type=pathlib.Path);a=ap.parse_args();e={'schema':1,'status':'failed','scope':'V3 declaration facts, not native ABI certification','commands':[],'source':SOURCE}
 with tempfile.TemporaryDirectory(prefix='r10-sourcefacts3-') as name:
  t=pathlib.Path(name)
  def cmd(*args):
   c=[str(ROOT/'tests/bound'),'20',*map(str,args)];p=subprocess.run(c,capture_output=True,timeout=25,env=dict(os.environ,UA_TYPESPELL='1'));e['commands'].append({'command':c,'rc':p.returncode,'stdout':p.stdout.decode(errors='replace'),'stderr':p.stderr.decode(errors='replace')});assert p.returncode==0,e['commands'][-1];return p.stdout
  try:
   cmd(ROOT/'tests/build_ref.sh',t/'ref.c',t/'ref');s=(t/'ref.c').read_bytes();old=b'        if (tkind[i] == 2) { __write(1, "=", 1);';new=b'        if (L == 4 && TOKV[p] == 116 && TOKV[p+1] == 121 && TOKV[p+2] == 112 && TOKV[p+3] == 101 && getenv("UA_TYPESPELL")) { __write(1, "=", 1); __write(1, src + tpos[i], tlen[i]); }\n'+old;assert s.count(old)==1;(t/'dump.c').write_bytes(s.replace(old,new));cmd('cc','-w','-std=c99','-O2',t/'dump.c','-o',t/'dump');cmd('cc','-O2',ROOT/'exec/c/run.c','-o',t/'run')
   model=a.model or t/'e3.json'
   if not a.model:cmd(sys.executable,ROOT/'exec/parse2/gen2.py',model)
   e['model_sha256']=hashlib.sha256(model.read_bytes()).hexdigest();d=json.loads(model.read_text());loaded=load(d)
   cmd(sys.executable,ROOT/'exec/c/tbl.py',model,t/'e3.tbl');cmd(sys.executable,ROOT/'exec/c/net.py',t/'e3.tbl',t/'e3.net');e['full_domain']=cmd(t/'run','--check-net',t/'e3.tbl',t/'e3.net').decode().strip()
   (t/'source.c').write_text(SOURCE);raw=cmd(t/'dump','-dump-tokens',t/'source.c');(t/'tokens').write_bytes(raw)
   def simulate(ver):
    status,out,_=run(d,raw,'source',files=Files(ver),loaded=loaded,maxsteps=5000000);return status,out
   off,old=simulate(None);two,explicit=simulate(U(2));assert off==two=='accept' and old==explicit and old
   status,out=simulate(U(3));assert status=='accept',(status,out[:300]);tape,meta,records=decode(out);assert tape==old[25:25+struct.unpack_from('<Q',old,9)[0]];e.update(oracle(records))
   routes=t/'routes';routes.write_text('parse\te3\ttokens\ttape\te3.net\n');resources=t/'resources';resources.mkdir();(resources/'module').write_bytes(U(1));(resources/'symbols').write_bytes(U(1));(resources/'signatureversion').write_bytes(U(3));pkg=t/'bundle';pkg.write_bytes(build([routes],[('006c6962726172792f',resources)],cache=False));assert cmd(t/'run','--bundle',pkg,'parse',t/'tokens')==out
   (t/'source.sig').write_bytes(meta);(t/'host.c').write_text(HOST);cmd('cc','-std=c11','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I',ROOT,t/'host.c','-lffi','-o',t/'host');e['actual_source_host_decode']=cmd(t/'host',t/'source.sig').decode().strip()
   e['sim_C_network_bytes_equal']=True;e['V2_absent_explicit_equal']=True;e['V3_tape_unchanged']=True
   bad=[b'',U(1),U(4),U(3)+b'x']
   for val in bad:
    status,_=simulate(val);assert status=='reject',(val,status)
   e['bad_resource_rejects']=len(bad);e['status']='passed';print(json.dumps({k:v for k,v in e.items() if k not in ('commands','source')}))
  except BaseException as ex:e['error']=repr(ex);raise
  finally:
   if a.evidence:a.evidence.parent.mkdir(parents=True,exist_ok=True);a.evidence.write_text(json.dumps(e,indent=2)+'\n')
if __name__=='__main__':main()

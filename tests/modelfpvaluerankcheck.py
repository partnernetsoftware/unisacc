#!/usr/bin/env python3
"""Actual promoted call-site FP identity, not native wide-FP qualification."""
import argparse,hashlib,json,os,pathlib,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/c'),str(ROOT/'tests')]
from exec.pp.sim import run,load
from pack import build
from modelsourcefacts3check import decode,U
from modelcallablevarwirecheck import segments
SOURCE='''typedef long double LD;
typedef long (*V)(int,...);
struct Box {LD l;double d;float f;LD a[2];};
LD wide(LD x){return x;}
double plain(double x){return x;}
float small(float x){return x;}
long exercise(V fn,LD input){
 float x=1.0f;double y=2.0;LD z=input;LD *p=&z;
 struct Box b;b.l=input;b.d=y;b.f=x;b.a[0]=input;b.a[1]=input;
 return fn(1,input,z,*p,b.l,b.a[1],wide(input),(LD)y,1.0L,x,y,z+1.0,z+y,x+y,z?y:z,(int)z,plain(z),small(z));
}
LD global=1.0L;
long scoped(V fn){LD v=global;{double v=1.0;fn(2,v);}return fn(3,v,global);}
typedef long (*D)(double,...);
long converted(D fn,LD input){return fn(input,input);}
int main(void){return 0;}
'''
EXPECTED=[0,3,3,3,3,3,3,3,3,2,2,3,3,2,3,0,2,2]
EXPECTED_SITES=[EXPECTED,[0,2],[0,3,3],[2,3]]
class Files:
 def __init__(self,version):self.version=version
 def get(self,key):
  return {b'\0library/symbols':U(1),b'\0library/module':U(1),b'\0library/callables':U(1),b'\0library/callablemake':U(8192),b'\0library/callablecall':U(12288),b'\0library/signatureversion':self.version}.get(key)
def single(wire):
 _,_,r=decode(b'USLTAPE1\n'+U(0)+U(len(wire))+wire);assert len(r)==1;return next(iter(r.values()))
def oracle(out):
 tape,exports,calls,catalog=segments(out);assert calls==b'USCPLAN1'+U(0) and catalog[:9]==b'USLCALL2\n'
 at=9
 def word():
  nonlocal at
  n=struct.unpack_from('<Q',catalog,at)[0];at+=8;return n
 protos={}
 for _ in range(word()):
  key,n=word(),word();assert key not in protos;protos[key]=single(catalog[at:at+n]);at+=n
 sites=[]
 for _ in range(word()):
  n=word();end=at+n;site,key,fixed,size=word(),word(),word(),word();assert n==32+size and key in protos
  wire=catalog[at:at+size];concrete=single(wire);concrete['wire']=wire;at+=size;assert at==end;sites.append((site,fixed,concrete))
 assert at==len(catalog) and len(sites)==len(EXPECTED_SITES),sites
 rows=[];wires=[]
 for number,((site,fixed,c),expected) in enumerate(zip(sites,EXPECTED_SITES),1):
  assert site==number and fixed==1 and c['count']==len(expected)
  ranks=[d['facts'][0] for d in c['params']];assert ranks==expected,{'site':site,'actual_ranks':ranks,'expected_ranks':expected}
  for rank,d in zip(expected,c['params']):
   assert d['facts']==(rank,2 if rank else 0,0,0,0,0),d
   assert d['fields'][3:]==((3,8,0,8) if rank else (1,4,0,4)),d
  assert c['support']==0;rows.append(ranks);wires.append(c['wire'])
 assert all(p['support']==0 for p in protos.values())
 return {'sites':len(sites),'argument_ranks':rows,'fixed_double_conversion':True,'global_and_scope_restore':True,'default_float_promotion':True,'long_double_storage':'F64; native ABI not qualified','tape':tape,'wires':wires}
HOST=r"""
#include "exec/c/libraryexports.h"
#include <assert.h>
int main(int argc,char**argv){assert(argc==5);unsigned ranks[4][18]={{0,3,3,3,3,3,3,3,3,2,2,3,3,2,3,0,2,2},{0,2},{0,3,3},{2,3}};int counts[]={18,2,3,2};for(int k=0;k<4;k++){FILE*f=fopen(argv[k+1],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);unsigned char*b=malloc(n);assert(b&&fread(b,1,n,f)==(size_t)n);fclose(f);us_exports s={0};char e[200]={0};assert(!us_exports_load_bridge(&s,b,n,e,sizeof e)&&s.count==1);us_export*x=s.items;assert(x->version==3&&x->count==(unsigned)counts[k]&&!x->supported&&!x->variadic&&x->mode==1);for(int i=0;i<counts[k];i++){us_export_type*t=x->argtypes+i;assert(t->fp_rank==ranks[k][i]&&t->fp_format==(ranks[k][i]?2:0)&&!t->ffi&&!t->layout_known_mask&&!t->layout_origin);}us_exports_clear(&s);free(b);}puts("25 concrete promoted ranks decoded across four sites; native wide-FP remains unsupported");return 0;}
"""
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--model',type=pathlib.Path);ap.add_argument('--baseline-model',type=pathlib.Path);ap.add_argument('--evidence',type=pathlib.Path);a=ap.parse_args()
 e={'schema':1,'status':'failed','scope':__doc__,'source':SOURCE,'commands':[]}
 with tempfile.TemporaryDirectory(prefix='r10-fpvalue-') as name:
  t=pathlib.Path(name)
  def cmd(*args):
   c=[str(ROOT/'tests/bound'),'20',*map(str,args)];p=subprocess.run(c,capture_output=True,timeout=25,env=dict(os.environ,UA_TYPESPELL='1'));e['commands'].append({'command':c,'rc':p.returncode,'stdout':p.stdout.decode(errors='replace'),'stderr':p.stderr.decode(errors='replace')});assert p.returncode==0,e['commands'][-1];return p.stdout
  try:
   src=t/'source.c';src.write_text(SOURCE);cmd('cc','-std=c99','-pedantic-errors','-fsyntax-only',src);cmd(ROOT/'tests/build_ref.sh',t/'ref.c',t/'ref')
   s=(t/'ref.c').read_bytes();old=b'        if (tkind[i] == 2) { __write(1, "=", 1);';new=b'        if (L == 4 && TOKV[p] == 116 && TOKV[p+1] == 121 && TOKV[p+2] == 112 && TOKV[p+3] == 101 && getenv("UA_TYPESPELL")) { __write(1, "=", 1); __write(1, src + tpos[i], tlen[i]); }\n'+old;assert s.count(old)==1;(t/'dump.c').write_bytes(s.replace(old,new));cmd('cc','-O2','-w','-std=c99',t/'dump.c','-o',t/'dump');cmd('cc','-O2',ROOT/'exec/c/run.c','-o',t/'run')
   model=a.model or t/'e3.json'
   if not a.model:cmd(sys.executable,ROOT/'exec/parse2/gen2.py',model)
   e['model_sha256']=hashlib.sha256(model.read_bytes()).hexdigest();d=json.loads(model.read_text());loaded=load(d);raw=cmd(t/'dump','-dump-tokens',src);(t/'tokens').write_bytes(raw)
   def simulate(ver):
    status,out,_=run(d,raw,'value-ranks',files=Files(ver),loaded=loaded,maxsteps=5000000);assert status=='accept',(status,out);return out
   old=simulate(None);assert old==simulate(U(2));out=simulate(U(3));facts=oracle(out);tape=facts.pop('tape');wires=facts.pop('wires');e.update(facts);assert tape==segments(old)[0]
   if a.baseline_model:
    base=json.loads(a.baseline_model.read_text());status,before,_=run(base,raw,'baseline',files=Files(None),maxsteps=5000000);assert status=='accept' and before==old;e['old_V2_baseline_bytes_equal']=True
   cmd(sys.executable,ROOT/'exec/c/tbl.py',model,t/'e3.tbl');cmd(sys.executable,ROOT/'exec/c/net.py',t/'e3.tbl',t/'e3.net');e['full_domain']=cmd(t/'run','--check-net',t/'e3.tbl',t/'e3.net').decode().strip()
   routes=t/'routes';routes.write_text('parse\te3\ttokens\ttape\te3.net\n');resources=t/'resources';resources.mkdir()
   for key,value in [('module',1),('symbols',1),('callables',1),('callablemake',8192),('callablecall',12288),('signatureversion',3)]:(resources/key).write_bytes(U(value))
   pkg=t/'bundle';pkg.write_bytes(build([routes],[('006c6962726172792f',resources)],cache=False));assert cmd(t/'run','--bundle',pkg,'parse',t/'tokens')==out;e['sim_C_network_bytes_equal']=True
   paths=[t/('site'+str(i)+'.sig') for i in range(len(wires))]
   for path,wire in zip(paths,wires):path.write_bytes(wire)
   (t/'host.c').write_text(HOST);cmd('cc','-std=c11','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I',ROOT,t/'host.c','-lffi','-o',t/'host');e['host_actual_concrete_site']=cmd(t/'host',*paths).decode().strip();e['status']='passed';print(json.dumps({k:v for k,v in e.items() if k not in ('source','commands')}))
  except BaseException as ex:e['error']=repr(ex);raise
  finally:
   if a.evidence:a.evidence.parent.mkdir(parents=True,exist_ok=True);a.evidence.write_text(json.dumps(e,indent=2)+'\n')
if __name__=='__main__':main()

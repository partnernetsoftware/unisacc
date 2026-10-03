#!/usr/bin/env python3
"""Actual source semantic FP ranks; F64 long-double storage is not native ABI support."""
import argparse,hashlib,json,os,pathlib,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/c'),str(ROOT/'tests')]
from exec.pp.sim import run,load
from pack import build
from modelsourcefacts3check import Files,decode,U
SOURCE="""typedef long double LD;
typedef LD Alias;
typedef Alias Matrix[2][3];
struct RankSet {float f;double d;Alias l;Matrix a;Alias *p;};
struct Nested {struct RankSet inner;double tail;};
float rf(float);
double rd(double);
long double rl(long double);
Alias aliased(Alias);
float rf(float x){return x;}
double rd(double x){return x;}
long double rl(long double x){return x;}
Alias aliased(Alias x){return x;}
struct RankSet ranked(struct RankSet x){return x;}
struct Nested nested(struct Nested x){return x;}
typedef Alias (*Leaf)(Alias);
typedef Leaf (*Factory)(Leaf);
Leaf relay(Leaf);
Factory factory(Factory);
Leaf relay(Leaf f){return f;}
Factory factory(Factory f){return f;}
long double *opaque(long double *p){return p;}
int clear(int x){return x;}
int main(void){return 0;}
"""
def oracle(records):
 def facts(d,rank,kind,width):
  assert d['fields'][3]==kind and d['fields'][4]==width and d['facts']==(rank,1 if kind==3 and width==4 else 2 if kind==3 and width==8 else 0,0,0,0,0),d
 for name,rank,width in (('rf',1,4),('rd',2,8),('rl',3,8),('aliased',3,8)):
  f=records[name];facts(f['result'],rank,3,width);facts(f['params'][0],rank,3,width)
 for name in ('clear','main'):facts(records[name]['result'],0,1,4)
 def members(d):
  facts(d,0,5,80);assert d['tag']==1 and len(d['child'])==5
  for i,(e,off,storage) in enumerate(zip(d['child'],(0,8,16,24,72),(4,8,8,48,8))):assert e['ordinal']==i and e['kind']==0 and e['layout']==(off,0,0,storage),e
  f=d['child'];facts(f[0]['child'],1,3,4);facts(f[1]['child'],2,3,8);facts(f[2]['child'],3,3,8)
  a=f[3]['child'];facts(a,0,5,48);assert a['tag']==3 and a['child'][:2]==(2,24);a=a['child'][2];facts(a,0,5,24);assert a['tag']==3 and a['child'][:2]==(3,8);facts(a['child'][2],3,3,8)
  facts(f[4]['child'],0,2,8);assert f[4]['child']['fields'][0]==1
 for side in ('result','params'):
  get=lambda name:records[name]['result'] if side=='result' else records[name]['params'][0]
  members(get('ranked'));n=get('nested');facts(n,0,5,88);assert n['child'][1]['layout']==(80,0,0,8);members(n['child'][0]['child']);facts(n['child'][1]['child'],2,3,8)
  # long double * now spells its pointee (tag 5): rank 3 / F64 storage, so it differs from double *.
  o=get('opaque');facts(o,0,2,8);assert o['fields'][0]==1 and o['tag']==5;q=o['child'];facts(q,3,3,8);assert q['fields'][0]==0 and q['tag']==0
 def leaf(d):
  facts(d,0,4,8);g=d['child'];assert g and g['support']==0 and g['count']==1;facts(g['result'],3,3,8);facts(g['params'][0],3,3,8);return g
 f=records['relay'];assert leaf(f['result']) is leaf(f['params'][0])
 f=records['factory'];assert f['result']['child'] is f['params'][0]['child'];g=f['result']['child'];assert g['support']==0;assert leaf(g['result']) is leaf(g['params'][0])
 assert all(x['support']==0 for x in records.values())
 return {'records':len(records),'semantic_ranks':[1,2,3],'typedef_chain':True,'two_dimensional_alias_array':True,'nested_record':True,'callback_factory_shared_graph':True,'pointer_rank_zero':True,'packing_facts_unknown':True,'long_double_storage':'F64, not native long-double qualification'}
CONFLICTS = {
 'scalar-result-and-param': 'double conflict(double); long double conflict(long double); int main(void){return 0;}\n',
 'callback-param': 'typedef double (*D)(double); typedef long double (*L)(long double); int conflict(D); int conflict(L); int main(void){return 0;}\n',
 'callback-result': 'typedef double (*D)(double); typedef long double (*L)(long double); D conflict(void); L conflict(void); int main(void){return 0;}\n',
}
POSITIVES = {
 'returned-inline-callback': 'static int (*pick(int which,int (*fallback)(int)))(int); static int (*pick(int which,int (*fallback)(int)))(int){return fallback;} int main(void){return 0;}\n',
}
HOST=r"""
#include "exec/c/libraryexports.h"
#include <assert.h>
static us_export *find(us_exports*s,const char*n){for(size_t i=0;i<s->count;i++)if(!strcmp(s->items[i].name,n))return s->items+i;assert(0);return NULL;}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);unsigned char*b=malloc(n);assert(b&&fread(b,1,n,f)==(size_t)n);fclose(f);char e[200]={0};us_exports s={0};int rc=us_exports_load_bridge(&s,b,n,e,sizeof e);if(rc)fprintf(stderr,"%s\n",e);assert(!rc&&s.count==11);
 for(size_t i=0;i<s.count;i++)assert(s.items[i].version==3&&!s.items[i].supported&&!s.items[i].result.ffi);
 assert(find(&s,"rf")->result.fp_rank==1&&find(&s,"rd")->result.fp_rank==2);us_export*x=find(&s,"rl");assert(x->result.fp_rank==3&&x->result.fp_format==2&&x->result.width==8&&x->argtypes[0].fp_rank==3);
 x=find(&s,"ranked");assert(x->result.members[2].type->fp_rank==3);us_export_type*a=x->result.members[3].type;assert(a->tag==3&&a->element->tag==3&&a->element->element->fp_rank==3);assert(x->result.members[4].type->fp_rank==0);
 x=find(&s,"factory");us_export_signature*g=x->result.signature;assert(g==x->argtypes[0].signature&&g->result.signature==g->argtypes[0].signature&&g->result.signature->result.fp_rank==3);
 us_exports_clear(&s);free(b);puts("source ranks decoded with shared callback ownership; native ABI remains unsupported");return 0;}
"""
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--model',type=pathlib.Path);ap.add_argument('--baseline-model',type=pathlib.Path);ap.add_argument('--evidence',type=pathlib.Path);a=ap.parse_args();e={'schema':1,'status':'failed','scope':__doc__,'source':SOURCE,'commands':[]}
 with tempfile.TemporaryDirectory(prefix='r10-fprank-') as name:
  t=pathlib.Path(name)
  def cmd(*args):
   c=[str(ROOT/'tests/bound'),'20',*map(str,args)];p=subprocess.run(c,capture_output=True,timeout=25,env=dict(os.environ,UA_TYPESPELL='1'));e['commands'].append({'command':c,'rc':p.returncode,'stdout':p.stdout.decode(errors='replace'),'stderr':p.stderr.decode(errors='replace')});assert p.returncode==0,e['commands'][-1];return p.stdout
  try:
   (t/'source.c').write_text(SOURCE);cmd('cc','-std=c99','-pedantic-errors','-fsyntax-only',t/'source.c');cmd(ROOT/'tests/build_ref.sh',t/'ref.c',t/'ref');s=(t/'ref.c').read_bytes();old=b'        if (tkind[i] == 2) { __write(1, "=", 1);';new=b'        if (L == 4 && TOKV[p] == 116 && TOKV[p+1] == 121 && TOKV[p+2] == 112 && TOKV[p+3] == 101 && getenv("UA_TYPESPELL")) { __write(1, "=", 1); __write(1, src + tpos[i], tlen[i]); }\n'+old;assert s.count(old)==1;(t/'dump.c').write_bytes(s.replace(old,new));cmd('cc','-w','-std=c99','-O2',t/'dump.c','-o',t/'dump');cmd('cc','-O2',ROOT/'exec/c/run.c','-o',t/'run')
   model=a.model or t/'e3.json'
   if not a.model:cmd(sys.executable,ROOT/'exec/build/gen.py','parse2',model)
   e['model_sha256']=hashlib.sha256(model.read_bytes()).hexdigest();d=json.loads(model.read_text());loaded=load(d);raw=cmd(t/'dump','-dump-tokens',t/'source.c');(t/'tokens').write_bytes(raw)
   def simulate(ver):
    status,out,_=run(d,raw,'ranks',files=Files(ver),loaded=loaded,maxsteps=5000000);assert status=='accept',(status,out);return out
   old=simulate(None);assert old==simulate(U(2));out=simulate(U(3));tape,meta,records=decode(out);e.update(oracle(records));assert tape==old[25:25+struct.unpack_from('<Q',old,9)[0]]
   positives=[];positive_tokens=[]
   for label,text in POSITIVES.items():
    probe=t/(label+'.c');probe.write_text(text);cmd('cc','-std=c99','-pedantic-errors','-fsyntax-only',probe)
    tokens=cmd(t/'dump','-dump-tokens',probe);tokenfile=t/(label+'.tokens');tokenfile.write_bytes(tokens)
    status,output,_=run(d,tokens,label,files=Files(U(3)),loaded=loaded,maxsteps=5000000);assert status=='accept',(label,status,output)
    positive_tokens.append((label,tokenfile,output));positives.append(label)
   e['compatible_declaration_probes']=positives
   conflicts=[];negative_tokens=[]
   for label,text in CONFLICTS.items():
    probe=t/(label+'.c');probe.write_text(text)
    cc=subprocess.run([str(ROOT/'tests/bound'),'10','cc','-std=c99','-pedantic-errors','-fsyntax-only',str(probe)],capture_output=True,timeout=15);assert cc.returncode==1,(label,cc.returncode,cc.stderr)
    tokens=cmd(t/'dump','-dump-tokens',probe);tokenfile=t/(label+'.tokens');tokenfile.write_bytes(tokens);negative_tokens.append((label,tokenfile));status,reason,_=run(d,tokens,label,files=Files(U(3)),loaded=loaded,maxsteps=5000000);assert status=='reject',(label,status,reason);conflicts.append({'source':text,'host_cc_rc':cc.returncode,'model_status':status,'reason':str(reason),'cc_stderr':cc.stderr.decode(errors='replace')})
   e['constraint_rejections']=conflicts
   if a.baseline_model:
    base=json.loads(a.baseline_model.read_text());status,original,_=run(base,raw,'baseline',files=Files(),maxsteps=5000000);assert status=='accept' and original==old;e['old_V2_baseline_bytes_equal']=True;e['baseline_model_sha256']=hashlib.sha256(a.baseline_model.read_bytes()).hexdigest()
   cmd(sys.executable,ROOT/'exec/c/tbl.py',model,t/'e3.tbl');cmd(sys.executable,ROOT/'exec/c/net.py',t/'e3.tbl',t/'e3.net');e['full_domain']=cmd(t/'run','--check-net',t/'e3.tbl',t/'e3.net').decode().strip()
   routes=t/'routes';routes.write_text('parse\te3\ttokens\ttape\te3.net\n');resources=t/'resources';resources.mkdir()
   for key,value in [('module',1),('symbols',1),('signatureversion',3)]:(resources/key).write_bytes(U(value))
   pkg=t/'bundle';pkg.write_bytes(build([routes],[('006c6962726172792f',resources)],cache=False));assert cmd(t/'run','--bundle',pkg,'parse',t/'tokens')==out;e['sim_C_network_bytes_equal']=True
   for label,tokenfile,output in positive_tokens:
    assert cmd(t/'run','--bundle',pkg,'parse',tokenfile)==output,label
   e['compatible_C_network_accepts']=len(positive_tokens)
   for label,tokenfile in negative_tokens:
    c=[str(ROOT/'tests/bound'),'20',str(t/'run'),'--bundle',str(pkg),'parse',str(tokenfile)]
    p=subprocess.run(c,capture_output=True,timeout=25)
    assert p.returncode==1 and not p.stdout and b'incompatible semantic declaration type' in p.stderr,(label,p.returncode,p.stdout,p.stderr)
    e['commands'].append({'command':c,'rc':p.returncode,'stderr':p.stderr.decode(errors='replace')})
   e['constraint_C_network_rejections']=len(negative_tokens)
   (t/'source.sig').write_bytes(meta);(t/'host.c').write_text(HOST);cmd('cc','-std=c11','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I',ROOT,t/'host.c','-lffi','-o',t/'host');e['host_actual_source']=cmd(t/'host',t/'source.sig').decode().strip();e['status']='passed';print(json.dumps({k:v for k,v in e.items() if k not in ('source','commands')}))
  except BaseException as ex:e['error']=repr(ex);raise
  finally:
   if a.evidence:a.evidence.parent.mkdir(parents=True,exist_ok=True);a.evidence.write_text(json.dumps(e,indent=2)+'\n')
if __name__=='__main__':main()

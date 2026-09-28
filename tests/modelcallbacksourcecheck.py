#!/usr/bin/env python3
"""Actual source facts -> callback graph: independent fields, sim+C, host decode."""
import json,os,pathlib,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/c'),str(ROOT/'tests')]
from exec.pp.sim import run,load
from pack import build
U=lambda n:struct.pack('<Q',n)
SOURCE="""struct Pair { double d; int n; };
typedef struct Pair (*Leaf)(int,double,float,int*,struct Pair,int,double,int,int);
typedef struct Pair (*Relay)(Leaf);
struct Pair entry(Relay relay,Leaf leaf){struct Pair r;r.d=0.0;r.n=0;return r;}
long entry17(long (*f)(long,long,long,long,long,long,long,long,long,long,long,long,long,long,long,long,long)){return 0;}
struct Slots { Leaf leaf; Relay relay; };
struct Slots slots(struct Slots x){return x;}
long shared(Leaf a,Leaf b){return 0;}
long opaque(Leaf *p){return 0;}
Leaf returning(Leaf leaf){return leaf;}
struct Node { long (*f)(struct Node); };
long cyclic(struct Node n){return 0;}
long varleaf(long (*f)(long,...)){return 0;}
long fixed(int x,double d,float f,int *p){return x;}
int main(void){return 0;}
"""
class Files:
 def get(self,key):return U(1) if key in (b'\0library/symbols',b'\0library/module') else None

def decode(data):
 assert data[:9]==b'USLTAPE1\n';tn,mn=struct.unpack_from('<2Q',data,9);meta=data[25+tn:];assert len(meta)==mn and meta[:8]==b'USLSIG2\n'
 at=8
 def word():
  nonlocal at
  v=struct.unpack_from('<Q',meta,at)[0];at+=8;return v
 def byte():
  nonlocal at
  v=meta[at];at+=1;return v
 def desc():
  nonlocal at
  f=tuple(word() for _ in range(7));tag=byte();size=word();end=at+size;child=None
  if tag in (1,2):child=[(tuple(word() for _ in range(4)),desc()) for _ in range(word())]
  elif tag==3:child=(word(),word(),desc())
  elif tag==4 and size:
   assert byte()==1;form=byte();ident=word()
   if form==0:
    assert ident==len(ids)+1;child={'id':ident};ids[ident]=child
    child['var']=byte();child['mode']=byte();child['count']=word();child['result']=desc();assert word()==child['count'];child['params']=[desc() for _ in range(child['count'])];child['support']=byte()
   else:assert form==1 and ident in ids;child=ids[ident]
  else:assert size==0
  assert at==end;return {'fields':f,'tag':tag,'child':child}
 records={};n=word()
 for _ in range(n):
  start=at;nl=word();name=meta[at:at+nl].decode();at+=nl;link=byte();defined=byte();var=byte();mode=byte();count=word();ids={};result=desc();assert word()==count;params=[desc() for _ in range(count)];support=byte()
  records[name]={'result':result,'params':params,'count':count,'mode':mode,'var':var,'support':support,'graph':ids,'wire':b'USLSIG2\n'+U(1)+meta[start:at]}
 assert at==len(meta);return data[25:25+tn],records
HOST="""#include \"exec/c/libraryexports.h\"
static void emit(const us_export_type *t){printf(\" %llu:%llu:%llu:%llu:%llu\",(unsigned long long)t->kind,(unsigned long long)t->width,(unsigned long long)t->uns,(unsigned long long)t->alignment,(unsigned long long)t->depth);}
int main(int argc,char **argv){if(argc!=2)return 2;FILE *f=fopen(argv[1],\"rb\");if(!f)return 3;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);unsigned char *b=malloc(n);if(fread(b,1,n,f)!=(size_t)n)return 4;fclose(f);us_exports set={0};char err[200]={0};if(us_exports_load(&set,b,n,err,sizeof err)){fprintf(stderr,\"%s\\n\",err);return 5;}for(size_t i=0;i<set.count;i++){us_export *x=&set.items[i];printf(\"%s %u\\n\",x->name,x->supported);for(size_t j=0;j<x->graph.count;j++){us_export_signature *s=x->graph.signatures[j];printf(\"sig %llu %u %u %u\",(unsigned long long)s->count,s->variadic,s->mode,s->supported);emit(&s->result);for(size_t k=0;k<s->count;k++)emit(&s->argtypes[k]);printf(\"\\n\");}}us_exports_clear(&set);free(b);return 0;}
"""
def main():
 model,runtime,dumper=sys.argv[1:];d=json.loads(pathlib.Path(model).read_text());loaded=load(d)
 with tempfile.TemporaryDirectory(prefix='r10-callback-source-') as name:
  t=pathlib.Path(name)
  def cmd(*a):
   p=subprocess.run(list(map(str,a)),capture_output=True,timeout=25,env=dict(os.environ,UA_TYPESPELL='1'));assert p.returncode==0,(a,p.returncode,p.stderr);return p.stdout
  tbl=t/'m.tbl';net=t/'m.net';cmd(sys.executable,ROOT/'exec/c/tbl.py',model,tbl);cmd(sys.executable,ROOT/'exec/c/net.py',tbl,net);full=cmd(runtime,'--check-net',tbl,net).decode().strip()
  mf=t/'routes';mf.write_text('parse\tparse\ttokens\ttape\tm.net\n');rd=t/'r';rd.mkdir();(rd/'symbols').write_bytes(U(1));(rd/'module').write_bytes(U(1));pkg=t/'p';pkg.write_bytes(build([mf],[('006c6962726172792f',rd)],cache=False));src=t/'s.c';src.write_text(SOURCE);raw=cmd(dumper,'-dump-tokens',src);tok=t/'tokens';tok.write_bytes(raw)
  status,out,_=run(d,raw,'source',files=Files(),loaded=loaded,maxsteps=5000000);assert status=='accept',(status,out[:300]);native=cmd(runtime,'--bundle',pkg,'parse',tok);assert native==out
  tape,records=decode(out);entry=records['entry'];relay=entry['params'][0]['child'];leaf=relay['params'][0]['child']
  expect=[(1,4,0,4),(3,8,0,8),(3,4,0,4),(2,8,0,8),(5,16,0,8),(1,4,0,4),(3,8,0,8),(1,4,0,4),(1,4,0,4)]
  assert relay['count']==1 and relay['mode']==0 and relay['support']==0
  assert leaf['count']==9 and leaf['mode']==1 and leaf['var']==0 and leaf['support']==1
  assert [x['fields'][3:7] for x in leaf['params']]==expect,[x['fields'] for x in leaf['params']]
  assert leaf['result']['fields'][3:7]==(5,16,0,8) and entry['params'][1]['child'] is leaf
  leaf17=records['entry17']['params'][0]['child'];assert leaf17['count']==17 and leaf17['mode']==1 and len(leaf17['params'])==17 and all(x['fields'][3:7]==(1,8,0,8) for x in leaf17['params'])
  shared=records['shared'];assert shared['params'][0]['child'] is shared['params'][1]['child'] and len(shared['graph'])==1
  opaque=records['opaque']['params'][0];assert opaque['fields'][0]==2 and opaque['fields'][3:7]==(2,8,0,8) and opaque['tag']==0
  var=records['varleaf']['params'][0]['child'];assert (var['var'],var['mode'],var['count'],var['support'])==(1,1,1,0)
  returned=records['returning'];assert returned['result']['child'] is returned['params'][0]['child'] and returned['support']==0
  cycle=records['cyclic']['params'][0]['child'][0][1]['child'];assert cycle['params'][0]['child'][0][1]['child'] is cycle and cycle['support']==0
  assert all(records[n]['support']==0 for n in ('entry','entry17','shared','slots','varleaf','returning','cyclic'))
  fixed=records['fixed'];assert fixed['support']==1 and [x['fields'][3:7] for x in fixed['params']]==expect[:4]
  def primitive(depth,base,kind,width,alignment):return struct.pack('<7Q',depth,base,0,kind,width,0,alignment)+b'\0'+U(0)
  fixedwire=b'USLSIG2\n'+U(1)+U(5)+b'fixed'+bytes([0,1,0,0])+U(4)+primitive(0,8,1,8,8)+U(4)+primitive(0,4,1,4,4)+primitive(0,64,3,8,8)+primitive(0,65,3,4,4)+primitive(1,4,2,8,8)+b'\1'
  assert fixed['wire']==fixedwire,('legacy fixed bytes changed',fixed['wire'].hex(),fixedwire.hex())
  hostsrc=t/'host.c';hostsrc.write_text(HOST);host=t/'host';cmd('cc','-std=c99','-I',ROOT,hostsrc,'-lffi','-o',host)
  rows=0
  for name,rec in records.items():
   wire=t/(name+'.sig');wire.write_bytes(rec['wire']);actual=cmd(host,wire).decode().splitlines();expected=[name+' '+str(rec['support'])]
   for node in rec['graph'].values():
    prefix='sig %d %d %d %d'%(node['count'],node['var'],node['mode'],node['support'])
    def shape(x):
     f=x['fields'];return ' %d:%d:%d:%d:%d'%(f[3],f[4],f[5],f[6],f[0])
    expected.append(prefix+shape(node['result'])+''.join(shape(x) for x in node['params']));rows+=node['count']
   assert actual==expected,(name,actual,expected)
  print(json.dumps({'states':len(d['states']),'full_domain':full,'source_sim_network_byte_equal':True,'nine_and_seventeen_full_descriptors':True,'host_decoder_all_signature_fields':True,'host_parameter_rows':rows,'shared_signature_reference':True,'callback_return_and_source_cycle':True,'legacy_fixed_exact_bytes':True,'opaque_extra_indirection':True,'true_variadic_vs_stacked_mode':True,'scope':'declaration facts only; callback records support0'}))
if __name__=='__main__':main()

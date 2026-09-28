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
struct Pair script_leaf(int a,double b,float c,int *p,struct Pair s,int e,double f,int g,int h){return s;}
struct Pair script_relay(Leaf leaf){int n;struct Pair s;n=1;s.d=1.0;s.n=2;return leaf(2,1.5,2.5,&n,s,6,9.0,7,8);}
struct Pair entry(Leaf leaf){return script_relay(leaf);}
Leaf make_leaf(void){return script_leaf;}
long scalar(long (*f)(long),long x){return f(x);}
static long hidden(long x){return x;}
long address(void){long (*f)(long);f=&hidden;return f(4);}
"""

class Files:
 def get(self,key):return {b'\0library/symbols':U(1),b'\0library/module':U(1),b'\0library/callables':U(1),b'\0library/callablemake':U(8192),b'\0library/callablecall':U(12288)}.get(key)

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
  mf=t/'routes';mf.write_text('parse\tparse\ttokens\ttape\tm.net\n');rd=t/'r';rd.mkdir()
  for key,value in [('symbols',1),('module',1),('callables',1),('callablemake',8192),('callablecall',12288)]:(rd/key).write_bytes(U(value))
  pkg=t/'p';pkg.write_bytes(build([mf],[('006c6962726172792f',rd)],cache=False));src=t/'s.c';src.write_text(SOURCE);raw=cmd(dumper,'-dump-tokens',src);tok=t/'tokens';tok.write_bytes(raw)
  status,out,_=run(d,raw,'source',files=Files(),loaded=loaded,maxsteps=5000000);assert status=='accept',(status,out[:300]);native=cmd(runtime,'--bundle',pkg,'parse',tok);assert native==out
  assert out[:9]==b'USLTAPE3\n';lengths=struct.unpack_from('<4Q',out,9);at=41;segments=[]
  for n in lengths:segments.append(out[at:at+n]);at+=n
  assert at==len(out);tape,exports,calls,catalog=segments
  assert b'callr ' not in tape and tape.count(b'.librarycall ')>=4
  assert b'__us_callable_make_' in tape and b'  imm r1, 8192\n' in tape and b'  imm r1, 12288\n' in tape
  assert catalog[:9]==b'USLCALL1\n';n=struct.unpack_from('<Q',catalog,9)[0];p=17;keys=[];graphs=[]
  for i in range(n):
   key,size=struct.unpack_from('<2Q',catalog,p);p+=16;graph=catalog[p:p+size];p+=size;assert graph[:8]==b'USLSIG2\n';keys.append(key)
   envelope=b'USLTAPE1\n'+U(0)+U(len(graph))+graph;_,rec=decode(envelope);graphs.append(rec['callable'])
  assert p==len(catalog) and keys==list(range(1,n+1))
  assert any(g['count']==9 and [x['fields'][3:7] for x in g['params']]==[(1,4,0,4),(3,8,0,8),(3,4,0,4),(2,8,0,8),(5,16,0,8),(1,4,0,4),(3,8,0,8),(1,4,0,4),(1,4,0,4)] for g in graphs)
  # Disabled legacy framing and bytes remain exact for ordinary fixed sources.
  fixedsource='long fixed(long x){return x;}'
  src.write_text(fixedsource);raw=cmd(dumper,'-dump-tokens',src);tok.write_bytes(raw)
  class Disabled:
   def get(self,key):return U(1) if key in (b'\0library/symbols',b'\0library/module') else None
  status,oldout,_=run(d,raw,'disabled',files=Disabled(),loaded=loaded,maxsteps=5000000);assert status=='accept'
  (rd/'callables').write_bytes(U(0));pkg.write_bytes(build([mf],[('006c6962726172792f',rd)],cache=False));assert cmd(runtime,'--bundle',pkg,'parse',tok)==oldout
  _,fixed=decode(oldout)
  primitive=struct.pack('<7Q',0,8,0,1,8,0,8)+b'\0'+U(0)
  exact=b'USLSIG2\n'+U(1)+U(5)+b'fixed'+bytes([0,1,0,0])+U(1)+primitive+U(1)+primitive+b'\1'
  assert fixed['fixed']['wire']==exact
  # Final source definitions choose SCRIPT, while an imported prototype chooses NATIVE.
  (rd/'callables').write_bytes(U(1))
  from modeltypedcandidatescheck import sig,typed,wire3,I
  binding=wire3(typed(name=b'external',signature=sig(name=b'external',params=(I,)),count=1,address=16384,dispatcher=32768,plan=65536))
  (rd/'bindings').write_bytes(binding);pkg.write_bytes(build([mf],[('006c6962726172792f',rd)],cache=False))
  class Bound:
   def get(self,key):return binding if key==b'\0library/bindings' else Files().get(key)
  base='long external(long);long use(void){long (*p)(long);p=external;return p(7);}'
  for late in (False,True):
   src.write_text(base+('long external(long x){return x;}' if late else ''));raw=cmd(dumper,'-dump-tokens',src);tok.write_bytes(raw)
   status,result,_=run(d,raw,'origin',files=Bound(),loaded=loaded,maxsteps=5000000);assert status=='accept',(late,status,result)
   assert cmd(runtime,'--bundle',pkg,'parse',tok)==result
   size=struct.unpack_from('<Q',result,9)[0];body=result[41:41+size]
   if late:assert b'  imm r1, 1\n  store64 [r7+0], r1\n  .lea r1, external\n' in body
   else:assert b'  imm r1, 2\n  store64 [r7+0], r1\n  imm r1, 16384\n' in body
  # Explicit bridge declarations may prove support1 without changing graph ABI.
  # The candidate's actual nonzero plan remains required independently.
  from modeltypedcandidatescheck import D,F,PTR,PAIR,d2
  from modelcallbackgraphcheck import definition,ref
  leaf=definition(2,params=(d2(1,4,4),D,F,PTR,PAIR,d2(1,4,4),D,d2(1,4,4),d2(1,4,4)),result=PAIR,support=1)
  relay=definition(1,params=(leaf,),result=PAIR,support=1)
  graph=sig(name=b'host_drive',params=(relay,ref(2)),result=PAIR,supported=1)
  drivebinding=wire3(typed(name=b'host_drive',signature=graph,count=2,address=16384,dispatcher=32768,plan=65536,supported=1))
  driving='''struct Pair {double d;int n;};typedef struct Pair (*Leaf)(int,double,float,int*,struct Pair,int,double,int,int);typedef struct Pair (*Relay)(Leaf);struct Pair host_drive(Relay,Leaf);struct Pair script_relay(Leaf leaf){int n;struct Pair p;n=12;p.d=4.0;p.n=5;return leaf(2,1.5,2.5,&n,p,6,9.0,7,8);}struct Pair entry(Leaf leaf){return host_drive(script_relay,leaf);}'''
  src.write_text(driving);raw=cmd(dumper,'-dump-tokens',src);tok.write_bytes(raw)
  class Driving:
   def __init__(self,binding,enabled=True):self.binding=binding;self.enabled=enabled
   def get(self,key):
    if key==b'\0library/bindings':return self.binding
    if key==b'\0library/callables':return U(int(self.enabled))
    return Files().get(key)
  (rd/'bindings').write_bytes(drivebinding);pkg.write_bytes(build([mf],[('006c6962726172792f',rd)],cache=False))
  status,driven,_=run(d,raw,'five-segment-model',files=Driving(drivebinding),loaded=loaded,maxsteps=5000000);assert status=='accept',(status,driven)
  assert cmd(runtime,'--bundle',pkg,'parse',tok)==driven
  size=struct.unpack_from('<Q',driven,9)[0];body=driven[41:41+size]
  assert b'callr ' not in body and b'host_drive:' in body and b'__us_callable_make_1:' in body and b'.librarycall' in body
  # Capability absence retains the old support0 gate for both outer/nested graphs.
  (rd/'callables').write_bytes(U(0));pkg.write_bytes(build([mf],[('006c6962726172792f',rd)],cache=False))
  assert run(d,raw,'disabled-bridge',files=Driving(drivebinding,False),loaded=loaded,maxsteps=5000000)[0]=='reject'
  denied=subprocess.run([runtime,'--bundle',str(pkg),'parse',str(tok)],capture_output=True,timeout=15);assert denied.returncode!=0 and not denied.stdout
  # An unresolved empty graph cannot be a usable candidate with plan0.
  empty=sig(name=b'host_drive',params=(d2(4,8,8,tag=4,depth=1),),supported=1)
  badbinding=wire3(typed(name=b'host_drive',signature=empty,count=1,address=16384,dispatcher=32768,plan=0,supported=1))
  (rd/'callables').write_bytes(U(1));(rd/'bindings').write_bytes(badbinding);pkg.write_bytes(build([mf],[('006c6962726172792f',rd)],cache=False))
  assert run(d,raw,'missing-actual-plan',files=Driving(badbinding),loaded=loaded,maxsteps=5000000)[0]=='reject'
  denied=subprocess.run([runtime,'--bundle',str(pkg),'parse',str(tok)],capture_output=True,timeout=15);assert denied.returncode!=0 and not denied.stdout
  print(json.dumps({'states':len(d['states']),'injected_host_drive_full_nested_graph':True,'capability_off_and_plan0_rejected':True,'disabled_legacy_exact_bytes':True,'final_script_native_origin':True,'full_domain':full,'sim_network_actual_source':True,'catalog_keys':keys,'nine_full_descriptors':True,'make_call_tape':True,'no_unbridged_callr':True,'scope':'model operations and actual source graph; native execution verified by separate public API tests'}))
if __name__=='__main__':main()

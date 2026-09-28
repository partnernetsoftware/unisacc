#!/usr/bin/env python3
"""Source -> explicit promoted indirect variadic sites, independent wire oracle."""
import json,os,pathlib,re,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/c'),str(ROOT/'tests')]
from exec.pp.sim import run,load
from pack import build
from modelcallablewirecheck import decode,Files,U
SOURCE='''struct Pair {double d;int n;};
typedef long (*Leaf)(long);
typedef long (*V)(double,int,...);
typedef struct Pair (*PV)(int,...);
long named(int n,...){return n;}
long mix(V f,Leaf l){short a;unsigned char b;float c;_Bool d;int n;a=3;b=4;c=2.5;d=1;n=5;return f(1.0,2,a,b,c,d,l,l(7),&n);}
long nested(V outer,V inner){return outer(1.0,2,inner(2.0,3,4.5),6.5);}
struct Pair pair(PV f,Leaf l){struct Pair s;s.d=1.0;s.n=2;return f(2,s,l);}
long zero(V f){return f(1.0,2);}
long many(V f){return f(1.0,2,3,4.0,5,6.0,7,8.0,9,10.0,11,12.0,13,14.0,15,16.0,17);}
long named_nested(V f){return f(1.0,2,named(3,4.0),5.0);}
'''
def segments(out):
 assert out[:9]==b'USLTAPE3\n';lengths=struct.unpack_from('<4Q',out,9);at=41;values=[]
 for n in lengths:values.append(out[at:at+n]);at+=n
 assert at==len(out);return values

def single(wire):
 _,records=decode(b'USLTAPE1\n'+U(0)+U(len(wire))+wire)
 assert len(records)==1;return next(iter(records.values()))

def catalogue(catalog):
 assert catalog[:9]==b'USLCALL2\n';at=9
 def word():
  nonlocal at
  value=struct.unpack_from('<Q',catalog,at)[0];at+=8;return value
 protos={}
 for _ in range(word()):
  key,size=word(),word();assert key and key not in protos;protos[key]=single(catalog[at:at+size]);at+=size
 sites=[]
 for _ in range(word()):
  payload=word();end=at+payload;site,key,fixed,size=word(),word(),word(),word()
  assert payload==32+size and key in protos
  concrete=single(catalog[at:at+size]);at+=size;assert at==end
  proto=protos[key];assert proto['var']==1 and proto['count']==fixed
  assert proto['support']==0, 'variadic prototype is not a generic callable proof'
  assert concrete['var']==0 and concrete['mode']==1 and concrete['count']>=fixed
  # Parser base/shape IDs are not ABI facts. Nested signatures are checked below.
  facts=lambda d:(d['fields'][0],d['fields'][3:],d['tag'])
  assert facts(concrete['result'])==facts(proto['result'])
  assert [facts(x) for x in concrete['params'][:fixed]]==[facts(x) for x in proto['params']]
  sites.append((site,key,fixed,concrete))
 assert at==len(catalog) and [s[0] for s in sites]==list(range(1,len(sites)+1));return protos,sites

def main():
 model,runtime,dumper=sys.argv[1:];d=json.loads(pathlib.Path(model).read_text());loaded=load(d)
 with tempfile.TemporaryDirectory(prefix='r10-callable-var-wire-') as name:
  t=pathlib.Path(name)
  def cmd(*args):
   p=subprocess.run(list(map(str,args)),capture_output=True,timeout=25,env=dict(os.environ,UA_TYPESPELL='1'));assert p.returncode==0,(args,p.returncode,p.stderr);return p.stdout
  tbl,net=t/'m.tbl',t/'m.net';cmd(sys.executable,ROOT/'exec/c/tbl.py',model,tbl);cmd(sys.executable,ROOT/'exec/c/net.py',tbl,net)
  full=cmd(runtime,'--check-net',tbl,net).decode().strip()
  mf=t/'routes';mf.write_text('parse\tparse\ttokens\ttape\tm.net\n');rd=t/'r';rd.mkdir()
  for key,value in [('symbols',1),('module',1),('callables',1),('callablemake',8192),('callablecall',12288)]:(rd/key).write_bytes(U(value))
  pkg=t/'p';pkg.write_bytes(build([mf],[('006c6962726172792f',rd)],cache=False));src=t/'s.c';tok=t/'tokens'
  def compile(source,label='source'):
   src.write_text(source);raw=cmd(dumper,'-dump-tokens',src);tok.write_bytes(raw)
   status,out,_=run(d,raw,label,files=Files(),loaded=loaded,maxsteps=5000000);assert status=='accept',(label,status,out[:300])
   assert cmd(runtime,'--bundle',pkg,'parse',tok)==out;return out
  out=compile(SOURCE);tape,exports,calls,catalog=segments(out)
  assert calls==b'USCPLAN1'+U(0) and b'callr ' not in tape
  protos,sites=catalogue(catalog)
  expected_counts=[9,4,3,3,2,17,4];assert [x[3]['count'] for x in sites]==expected_counts
  # Numeric kind/width/unsigned/alignment independently assert every mixed type.
  abi=lambda x:x['fields'][3:]
  I=(1,4,0,4);L=(1,8,0,8);D=(3,8,0,8);PTR=(2,8,0,8);FP=(4,8,0,8);PAIR=(5,16,0,8)
  expected=[[D,I,I,I,D,I,FP,L,PTR],[D,I,L,D],[D,I,D],[I,PAIR,FP],[D,I],[D,I,I,D,I,D,I,D,I,D,I,D,I,D,I,D,I],[D,I,L,D]]
  assert [[abi(x) for x in s[3]['params']] for s in sites]==expected
  assert [s[2] for s in sites]==[2,2,2,1,2,2,2]
  for i in (0,3):
   callback=sites[i][3]['params'][6 if i==0 else 2]['child']
   assert callback['count']==1 and callback['var']==0 and callback['mode']==0
   assert abi(callback['result'])==L and [abi(x) for x in callback['params']]==[L]
  pair=sites[3][3]['result'];assert abi(pair)==PAIR and pair['tag']==1
  assert [m[0][0] for m in pair['child']]==[0,8]
  # Inner calls restore outer site: emission order differs from allocation order.
  emitted=[int(x) for x in re.findall(rb'store64 \[r7\+32\], r1\n  imm r1, ([0-9]+)\n  store64 \[r7\+40\]',tape)]
  assert emitted==[0,1,3,2,4,5,6,7],emitted
  # No var sites preserves CALL1 exact catalogue and fixed descriptor bytes.
  fixed=compile('long fixed(long x){return x;}long use(long (*f)(long)){return f(7);}','fixed')
  fixedtape,fixedexports,fixedcalls,fixedcatalog=segments(fixed)
  primitive=struct.pack('<7Q',0,8,0,1,8,0,8)+b'\0'+U(0)
  record=U(8)+b'callable'+bytes([0,1,0,0])+U(1)+primitive+U(1)+primitive+b'\1'
  signature=b'USLSIG2\n'+U(1)+record
  assert fixedcatalog==b'USLCALL1\n'+U(1)+U(1)+U(len(signature))+signature
  # Capability off retains complete fixed signature wire and old envelope.
  class Disabled:
   def get(self,key):return U(1) if key in (b'\0library/module',b'\0library/symbols') else None
  src.write_text('long fixed(long x){return x;}');raw=cmd(dumper,'-dump-tokens',src);tok.write_bytes(raw)
  status,disabled,_=run(d,raw,'disabled',files=Disabled(),loaded=loaded,maxsteps=5000000);assert status=='accept'
  (rd/'callables').write_bytes(U(0));pkg.write_bytes(build([mf],[('006c6962726172792f',rd)],cache=False));assert cmd(runtime,'--bundle',pkg,'parse',tok)==disabled
  _,records=decode(disabled)
  exact=b'USLSIG2\n'+U(1)+U(5)+b'fixed'+bytes([0,1,0,0])+U(1)+primitive+U(1)+primitive+b'\1'
  assert records['fixed']['wire']==exact
  # A short prefix rejects on both engines, with no partially accepted output.
  (rd/'callables').write_bytes(U(1));pkg.write_bytes(build([mf],[('006c6962726172792f',rd)],cache=False))
  src.write_text('typedef long (*V)(double,int,...);long bad(V f){return f(1.0);}')
  raw=cmd(dumper,'-dump-tokens',src);tok.write_bytes(raw);assert run(d,raw,'missing-prefix',files=Files(),loaded=loaded,maxsteps=5000000)[0]=='reject'
  bad=subprocess.run([runtime,'--bundle',str(pkg),'parse',str(tok)],capture_output=True,timeout=25);assert bad.returncode and not bad.stdout
  print(json.dumps({'states':len(d['states']),'full_domain':full,'sim_network_actual_source':True,'counts':expected_counts,'nested_site_emission':emitted,'full_mixed_promoted_descriptors':True,'nested_callback_graph':True,'fixed_call1_exact':True,'capoff_fixed_exact':True,'missing_prefix_rejected':True,'scope':'model wire only; no public native execution claim'}))
if __name__=='__main__':main()

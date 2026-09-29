#!/usr/bin/env python3
"""Actual E3 USBIND3 declarations/wrappers; not native ffi execution evidence."""
import json,os,struct,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'exec/pp'),str(ROOT/'exec/c')]
import sim
from pack import build
U=lambda x:struct.pack('<Q',x)
def descriptor(kind,width,align,depth=0,unsigned=0,tag=0,payload=b''):
 return b''.join(U(x) for x in (depth,999,777,kind,width,unsigned,align))+bytes([tag])+U(len(payload))+payload
I=descriptor(1,4,4);L=descriptor(1,8,8);D=descriptor(3,8,8);F=descriptor(3,4,4);PTR=descriptor(2,8,8,1)
PAIR=descriptor(5,16,8,tag=1,payload=U(2)+U(0)*3+U(8)+D+U(8)+U(0)*2+U(4)+I)
def signature(name,result,args,support=1):
 n=name.encode();return b'USLSIG2\n'+U(1)+U(len(n))+n+bytes((0,1,0,int(len(args)>6)))+U(len(args))+result+U(len(args))+b''.join(args)+bytes([support])
def binding(name,result,args,support=1):
 n=name.encode();sg=signature(name,result,args,support)
 payload=U(len(n))+n+bytes((0,0))+U(0)+bytes((0,0))+U(0x123456789abcd)+U(len(args))+b'\1'+U(0x123456789abce)+U(0x123456789abcf)+U(len(sg))+sg+bytes([support])
 return b'USBIND3\n'+U(1)+U(len(payload))+payload
SOURCE='''struct Pair { double d; int n; };
struct Pair host_exchange9(int,double,float,int*,struct Pair,int,double,int,int);
struct Pair script_exchange9(int a,double b,float c,int *p,struct Pair s,int e,double f,int g,int h){return host_exchange9(a,b,c,p,s,e,f,g,h);}
double host_double9(double,double,double,double,double,double,double,double,double);
float host_float9(float,float,float,float,float,float,float,float,float);
long host_integers17(long,long,long,long,long,long,long,long,long,long,long,long,long,long,long,long,long);
double double9(double a,double b,double c,double d,double e,double f,double g,double h,double i){return host_double9(a,b,c,d,e,f,g,h,i);}
float float9(float a,float b,float c,float d,float e,float f,float g,float h,float i){return host_float9(a,b,c,d,e,f,g,h,i);}
long integers17(long a,long b,long c,long d,long e,long f,long g,long h,long i,long j,long k,long l,long m,long n,long o,long p,long q){return host_integers17(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q);}
'''
def check(paths):
 js,tbl,core,dump=map(Path,paths);delta=json.loads(js.read_text());loaded=sim.load(delta)
 with tempfile.TemporaryDirectory(prefix='r10-typedimports-') as td:
  d=Path(td);src=d/'source.c';src.write_text(SOURCE)
  env=dict(os.environ,UA_TYPESPELL='1')
  def run(args):return subprocess.run(list(map(str,args)),capture_output=True,timeout=20,env=env,cwd=ROOT)
  p=run([dump,'-dump-tokens',src]);assert p.returncode==0,p.stderr;tokens=p.stdout
  inp=d/'tokens';inp.write_bytes(tokens);net=d/'parse.net'
  p=run([sys.executable,ROOT/'exec/c/net.py',tbl,net]);assert p.returncode==0,p.stderr
  p=run([core,'--check-net',tbl,net]);assert p.returncode==0,p.stderr
  manifest=d/'route.tsv';manifest.write_text('library\tparse\ttokens\ttape\tparse.net\n')
  resources=d/'resources';(resources/'library').mkdir(parents=True);(resources/'library/symbols').write_bytes(U(1));(resources/'library/module').write_bytes(U(1))
  def execute(raw,content=tokens):
   files=sim.Files();files.cache[b'\0library/symbols']=U(1);files.cache[b'\0library/module']=U(1)
   if raw is not None:files.cache[b'\0library/bindings']=raw;(resources/'library/bindings').write_bytes(raw)
   else:
    try:(resources/'library/bindings').unlink()
    except FileNotFoundError:pass
   inp.write_bytes(content);pkg=d/'pkg';pkg.write_bytes(build([manifest],[('00',resources)],cache=False))
   verdict,out,_=sim.run(delta,content,'typed-native-import',files,loaded=loaded,maxsteps=2000000)
   p=run([core,'--bundle',pkg,'library',inp]);assert (p.returncode==0)==(verdict=='accept'),(verdict,p.returncode,p.stderr)
   if verdict=='accept':assert p.stdout==out
   else:assert not p.stdout
   return verdict,out
  extras=[binding('host_double9',D,[D]*9),binding('host_float9',F,[F]*9),binding('host_integers17',L,[L]*17)]
  def combined(pair):return b'USBIND3\n'+U(4)+b''.join(x[16:] for x in [pair,*extras])
  raw=combined(binding('host_exchange9',PAIR,[I,D,F,PTR,PAIR,I,D,I,I]));v,out=execute(raw);assert v=='accept',(v,out)
  tn,mn=struct.unpack_from('<QQ',out,9);tape=out[25:25+tn]
  assert b'host_exchange9:\n  .frame 8\n  store64 [r7+0], r6\n  mov r6, r7\n' in tape,tape
  assert b'load64 r2, [r6+80]' in tape and b'store64 [r7+112], r2' in tape
  assert b'.bss __rv_host_exchange9 16\n' in tape and b'.lea r1, __rv_host_exchange9' in tape
  assert b'  imm r1, 320255973501902\n' in tape # dispatcher, not raw target
  floatbody=tape.split(b'float9:\n',1)[1].split(b'integers17:\n',1)[0]
  assert b'ftod' not in floatbody, 'fixed ninth float was default-promoted to double'
  assert execute(combined(binding('host_exchange9',PAIR,[I,D,F,PTR,PAIR,I,D,I,L])))[0]=='reject'
  assert execute(combined(binding('host_exchange9',I,[I,D,F,PTR,PAIR,I,D,I,I])))[0]=='reject'
  assert execute(None)[0]=='reject'
  # Fixed typed calls cannot omit or invent stack slots. These controls compile
  # only, so a missing slot never reaches a native target.
  for call in ('host_float9(a,b,c,d,e,f,g,h)', 'host_float9(a,b,c,d,e,f,g,h,i,i)'):
   src.write_text(SOURCE.replace('host_float9(a,b,c,d,e,f,g,h,i)',call))
   p=run([dump,'-dump-tokens',src]);assert p.returncode==0,p.stderr
   assert execute(raw,p.stdout)[0]=='reject',('wrong fixed typed argc accepted',call)
  # Source definitions win typed registry candidates, including true varargs.
  src.write_text('long host_integers17(long a,...){return a;} long caller(void){return host_integers17(42,1,2);}')
  p=run([dump,'-dump-tokens',src]);assert p.returncode==0,p.stderr
  vv,oo=execute(binding('host_integers17',L,[L]*17),p.stdout)
  assert vv=='accept', 'typed candidate overrode source-defined variadic signature'
  tt=struct.unpack_from('<Q',oo,9)[0];assert b'.librarycall' not in oo[25:25+tt]
  # A later prototype/definition must not replace another signature's epoch.
  src.write_text(SOURCE+'long unrelated(long q){return q;}\n')
  p=run([dump,'-dump-tokens',src]);assert p.returncode==0
  assert execute(raw,p.stdout)[0]=='accept'
  print('USBind3 typed E3: four unnamed prototypes: Pair9/FPdouble9/FPfloat9/GP17 wrappers; C network=sim; all-observation check-net; wrong result/tail and missing binding rejected; unrelated signature epoch preserved; native execution not claimed')
import sys as _sys
if len(_sys.argv)-1 < 4:
    _sys.stderr.write("usage: %s PARSE.json LOWER.json RUN DUMPER\n" % _sys.argv[0])
    _sys.exit(2)

if __name__=='__main__':check(sys.argv[1:])

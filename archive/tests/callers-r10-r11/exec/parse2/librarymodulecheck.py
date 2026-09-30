#!/usr/bin/env python3
"""Private E3/lower table/network module checks. No host execution claimed.
Usage: librarymodulecheck.py PARSE.json LOWER.json RUN DUMPER
"""
import json,os,pathlib,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'exec/pp'),str(ROOT/'exec/c'),str(ROOT)]
import sim
from pack import build
from libraryexportscheck import decode
class Files:
 def __init__(self,v):self.v=v
 def get(self,k):return self.v.get(k)
def command(args):
 p=subprocess.run(list(map(str,args)),capture_output=True,timeout=55,cwd=ROOT)
 assert p.returncode==0,(args,p.returncode,p.stderr)
 return p.stdout
def main():
 os.environ["UA_TYPESPELL"]="1"
 import sys as _sys
 if len(_sys.argv)-1 < 4:
     _sys.stderr.write("usage: %s PARSE.json LOWER.json RUN DUMPER\n" % _sys.argv[0])
     _sys.exit(2)

 parse,lower,runtime,dumper=sys.argv[1:];models={k:json.loads(pathlib.Path(f).read_text()) for k,f in [('parse',parse),('lower',lower)]}
 with tempfile.TemporaryDirectory(prefix='r10-modulecheck-') as tmp:
  t=pathlib.Path(tmp);manifest=t/'routes';manifest.write_text('parse\tparse\ttokens\ttape\tparse.net\nlower\tlower\ttape\ttins\tlower.net\n')
  for key,f in [('parse',parse),('lower',lower)]:
   command([sys.executable,ROOT/'exec/c/tbl.py',f,t/(key+'.tbl')]);command([sys.executable,ROOT/'exec/c/net.py',t/(key+'.tbl'),t/(key+'.net')]);command([runtime,'--check-net',t/(key+'.tbl'),t/(key+'.net')])
  resource=t/'resource';resource.mkdir();src=t/'source.c';inp=t/'input';n=0
  def execute(stage,raw,values):
   for p in resource.iterdir():p.unlink()
   for k,v in values.items():(resource/k.decode().split('/')[-1]).write_bytes(v)
   pkg=t/'pkg';pkg.write_bytes(build([manifest],[('006c6962726172792f',resource)] if values else []));inp.write_bytes(raw)
   verdict,out,_=sim.run(models[stage],raw,'probe',files=Files(values),maxsteps=3000000)
   p=subprocess.run([runtime,'--bundle',str(pkg),stage,str(inp)],capture_output=True,timeout=55)
   assert (p.returncode==0)==(verdict=='accept'),(stage,verdict,p.returncode,p.stderr)
   if verdict=='accept':assert p.stdout==out
   return verdict,out
  one=struct.pack('<Q',1);zero=struct.pack('<Q',0);symbols={b'\0library/symbols':one};module=dict(symbols) ;module[b'\0library/module']=one
  cases=['int f(int x){return x+1;}','int a=7; int f(void){return a;}','char *s="hello";int f(void){return s[0];}','int helper(int a){return a*2;} int f(int a){return helper(a);}','int f(void){static int s=4;return s;}','int main(int argc,char **argv){return argc+argv[0][0];}']
  for source in cases:
   src.write_text(source+'\n');tokens=command([dumper,'-dump-tokens',src])
   verdict,out=execute('parse',tokens,module);assert verdict=='accept',(source,verdict,out)
   tape,records=decode(out);assert b'__init:\n' in tape and b'_start:' not in tape and b'call main' not in tape and b'__main_ret:' not in tape and b'__argvv' not in tape
   assert 'f' in records or 'main' in records
   verdict,tins=execute('lower',tape,module);assert verdict=='accept',(source,verdict)
   assert b'spinit ' not in tins and b'argsave ' not in tins
   if 'main' in records:
    _,plain=execute('parse',tokens,symbols);_,plain0=execute('parse',tokens,{**symbols,b'\0library/module':zero});assert plain==plain0
    _,default=execute('parse',tokens,{});assert decode(plain)[0]==default
    assert command([dumper,src,'-S','-o','-'])==default
    _,ld=execute('lower',default,{});_,ld0=execute('lower',default,{b'\0library/module':zero});assert ld==ld0
   else:assert execute('parse',tokens,symbols)[0]=='reject'
   for bad in [b'',b'\1',struct.pack('<Q',2)]:assert execute('parse',tokens,{**symbols,b'\0library/module':bad})[0]=='reject'
   assert execute('parse',tokens,{b'\0library/module':one})[0]=='reject'
   n+=1
  other=t/'other.c';src.write_text('int g=8; int get(void){return g;}');other.write_text('int get(void);int public_f(void){return get()+2;}')
  first=command([dumper,'-dump-tokens',src]).split(b'eof\n',1)[0];second=command([dumper,'-dump-tokens',other]).split(b'eof\n',1)[0]+b'eof\n'
  tokens=b'@unit0\n'+first+b'@unit+\n'+second;verdict,out=execute('parse',tokens,module);assert verdict=='accept'
  tape,records=decode(out);assert {'get','public_f'}<=set(records);assert execute('lower',tape,module)[0]=='accept';n+=1
  src.write_text('int __init(void){return 0;}');tokens=command([dumper,'-dump-tokens',src]);assert execute('parse',tokens,module)[0]=='reject'
  assert execute('lower',b'f:\n  .argc r0\n  ret\n',module)[0]=='reject'
  print('librarymodule:',n,'source/init/export cases; default transparency; resources/reserved/process negatives; table/net equality; full-domain checks')
if __name__=='__main__':main()

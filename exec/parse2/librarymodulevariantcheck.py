#!/usr/bin/env python3
"""Located warnings/errors module probes; actual C network vs table outputs,
diagnostics and verdict. Prepared model inputs keep this check bounded.
Usage: CHECK RUNTIME PP.tbl LEX.tbl WARNINGS.json WARNINGS.net ERRORS.json ERRORS.net OUT.json
"""
import json,pathlib,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'exec/pp'),str(ROOT/'exec/c'),str(ROOT)]
import sim
from pack import build
from libraryexportscheck import decode
class Files:
 def __init__(self,v):self.v=v
 def get(self,k):return self.v.get(k)
def command(c):
 p=subprocess.run(list(map(str,c)),cwd=ROOT,capture_output=True,timeout=20)
 assert p.returncode==0,(c,p.returncode,p.stderr)
 return p.stdout

def main():
 import sys as _sys
 if len(_sys.argv)-1 < 4:
     _sys.stderr.write("usage: %s PARSE.json LOWER.json RUN DUMPER\n" % _sys.argv[0])
     _sys.exit(2)

 runtime,pp,lexer,wj,wn,ej,en,out=sys.argv[1:]
 models={'warnings':json.loads(pathlib.Path(wj).read_text()),'errors':json.loads(pathlib.Path(ej).read_text())}
 loaded={k:sim.load(v) for k,v in models.items()};result=[]
 cases=[('warnings','clean','int g=7;int f(int x){return g+x;}',False,True,None),
        ('warnings','unused','int f(void){int unused;return 4;}',False,True,b'unused variable'),
        ('warnings','werror','int f(void){int unused;return 4;}',True,False,b'unused variable'),
        ('errors','clean','int g=7;int f(int x){return g+x;}',False,True,None),
        ('errors','undeclared','int f(void){return missing;}',False,False,b'missing'),
        ('errors','syntax','int f(void){int a = ;return 1;}',False,False,None)]
 with tempfile.TemporaryDirectory(prefix='r10-modulevariants-') as tmp:
  d=pathlib.Path(tmp);manifest=d/'routes';manifest.write_text('warnings\tparse\ttokens\ttape\t'+str(pathlib.Path(wn).resolve())+'\nerrors\tparse\ttokens\ttape\t'+str(pathlib.Path(en).resolve())+'\n')
  resource=d/'resources';(resource/'library').mkdir(parents=True);(resource/'cli').mkdir()
  (resource/'library/module').write_bytes((1).to_bytes(8,'little'));(resource/'library/symbols').write_bytes((1).to_bytes(8,'little'))
  src=d/'probe.c';pre=d/'preprocessed';input=d/'tokens';pkg=d/'pkg';one=(1).to_bytes(8,'little')
  for mode,name,text,werror,accept,message in cases:
   src.write_text(text+'\n');pre.write_bytes(command([runtime,pp,src,src,ROOT/'include']));tokens=command([runtime,lexer,pre,src]);assert tokens.startswith(b'UNITOK1\0');input.write_bytes(tokens)
   values={b'\0library/module':one,b'\0library/symbols':one}
   flag=resource/'cli/werror'
   if werror:flag.write_bytes(one);values[b'\0cli/werror']=one
   else:
    try:flag.unlink()
    except FileNotFoundError:pass
   pkg.write_bytes(build([manifest],[('00',resource)]))
   diagnostics=bytearray();status,actual,steps=sim.run(models[mode],tokens,str(src),files=Files(values),loaded=loaded[mode],maxsteps=3000000,diagnostics=diagnostics)
   p=subprocess.run([runtime,'--bundle',str(pkg),mode,str(input),str(src)],capture_output=True,timeout=20)
   assert (p.returncode==0)==(status=='accept')==accept,(mode,name,status,p.returncode,p.stderr)
   if accept:
    assert p.stdout==actual;tape,records=decode(actual);assert 'f' in records and b'__init:\n' in tape and b'_start:' not in tape
   else:assert not p.stdout
   expected=bytes(diagnostics)
   if status=='reject' and actual[0]:expected+=b'reject: '+actual[0].encode()+b'\n'
   assert p.stderr==expected,(mode,name,p.stderr,expected)
   if message:assert message in p.stderr,(mode,name,p.stderr)
   result.append({'mode':mode,'case':name,'status':status,'rc':p.returncode,'stdout_bytes':len(p.stdout),'stderr':p.stderr.decode(),'steps':steps})
 pathlib.Path(out).write_text(json.dumps(result,indent=2));print('module variants:',len(result),'located probes; verdict/output/diagnostics network=table; valid no-main init/export; warning and error paths')
if __name__=='__main__':main()

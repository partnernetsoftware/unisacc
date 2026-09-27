#!/usr/bin/env python3
"""Located parser error/recovery results against the actual reference CLI."""
import os,pathlib,subprocess,sys,tempfile
R=pathlib.Path(__file__).resolve().parents[2]
warnings='--warnings' in sys.argv
ref_flags=['-Wall'] if warnings else []
sys.path.insert(0,str(R/'exec/c'));from pack import build

def run(args):return subprocess.run(list(map(str,args)),capture_output=True,timeout=60)
def call(args):
 p=run(args);assert p.returncode==0,(p.args,p.returncode,p.stderr[-2000:]);return p.stdout
with tempfile.TemporaryDirectory(prefix='parser-errors-') as td:
 t=pathlib.Path(td)
 call([os.environ.get('EXEC_CC','cc'),'-O2',R/'exec/c/run.c','-o',t/'run'])
 for name,script,flags in [('pp','pp/gen.py',['--locations']),('lex','lex/gen.py',['--locations']),('parse','parse2/gen2.py',['--errors']+(['--warnings'] if warnings else []))]:
  call([sys.executable,R/'exec'/script,t/(name+'.json'),*flags])
  call([sys.executable,R/'exec/c/tbl.py',t/(name+'.json'),t/(name+'.tbl')])
  call([sys.executable,R/'exec/c/net.py',t/(name+'.tbl'),t/(name+'.net')])
  call([t/'run','--check-net',t/(name+'.tbl'),t/(name+'.net')])
 route=t/'route.tsv';route.write_text('error\te2\tsource\tpp\tpp.net\nerror\te1\tpp\ttokens\tlex.net\nerror\te3\ttokens\ttape\tparse.net\n')
 resources=t/'cli';resources.mkdir();(resources/'error-limit').write_bytes(b'')
 pkg=t/'pkg'
 cases=[
  ('unknown','int main(void){return missing;}\n'),
  ('semi','int main(void){int n=3\nreturn n;}\n'),
  ('expression','int main(void){int x = ;}\n'),
  ('three','int one(int a){return a+nope1;}\nint two(int b){int c=b\nreturn c;}\nint three(void){return nope3;}\nint main(void){return one(1)+two(2)+three();}\n'),
  ('splice','#define TWO(a,b) \\\n((a)+(b))\nint main(void){return TWO(1,2)+gone;}\n'),
  ('header','#include <stdio.h>\nint main(void){\nprintf("hi");\nreturn missing;\n}\n'),
  ('tab','int main(void){\n\treturn unknown;\n}\n'),
  ('shadow','enum {A=7}; int f(void){int A=2;return missing;} int main(void){return A;}\n'),
 ('eof','int main(void){return (2+;}\n'),
 ('warning-before-error','int f(void){int *p=3;return missing;} int main(void){int unused;return 0;}\n'),
 ('printf-error','int main(void){printf("%d",missing);return 0;}\n'),
 ('clean','int main(void){return 3;}\n'),
 ]
 for name,src in cases:
  f=t/(name+'.c');f.write_text(src)
  pkg.write_bytes(build([route],[('00636c692f',resources),('006864722f',R/'include')]))
  got=run([t/'run','--bundle',pkg,'error',f,f,R/'include'])
  ref=run([os.environ.get('UA','/tmp/ua_ref'),*ref_flags,'-t','lnx/x86_64',f,'-o','-'])
  assert (got.returncode,got.stdout,got.stderr)==(ref.returncode,ref.stdout,ref.stderr),(name,got.returncode,ref.returncode,got.stderr,ref.stderr)
  print('parser errors',name,'full result identical',flush=True)
 for limit in ('1','2','0','3','20'):
  f=t/'limit.c';f.write_text(cases[3][1])
  (resources/'error-limit').write_text(limit)
  pkg.write_bytes(build([route],[('00636c692f',resources),('006864722f',R/'include')]))
  got=run([t/'run','--bundle',pkg,'error',f,f,R/'include'])
  ref=run([os.environ.get('UA','/tmp/ua_ref'),*ref_flags,'-ferror-limit='+limit,'-t','lnx/x86_64',f,'-o','-'])
  assert (got.returncode,got.stdout,got.stderr)==(ref.returncode,ref.stdout,ref.stderr),(limit,got.stderr,ref.stderr)
  print('error limit',limit,'full result identical',flush=True)
 f=t/'unsupported.c';f.write_text('int main(void){int arr[2];int *p; p=&arr[1]\n*p=0;return arr[1];}\n')
 (resources/'error-limit').write_bytes(b'')
 pkg.write_bytes(build([route],[('00636c692f',resources),('006864722f',R/'include')]))
 got=run([t/'run','--bundle',pkg,'error',f,f,R/'include'])
 assert got.returncode==1 and not got.stdout and b':2:3: error: not covered: pointer arithmetic' in got.stderr,got
 assert got.stderr.endswith(b'1 error generated.\n')
 print('prototype limitation retains its reason and gains a location; not reference-equivalence evidence')
 # Rejected token syntax must not be parsed again merely to locate its error.
 for name,src in [('wide-size','int main(void){return sizeof(L"ab");}\n'),
                  ('wide-global','int *p=L"ab"; int main(void){return 0;}\n')]:
  f=t/(name+'.c');f.write_text(src)
  got=run([t/'run','--bundle',pkg,'error',f,f,R/'include'])
  assert got.returncode==1 and not got.stdout,(name,got.returncode,got.stderr)
  assert b'error: not covered: string prefix' in got.stderr,(name,got.stderr)
  assert (str(f)+':1:'+str(src.index('L"')+1)+':').encode() in got.stderr,got.stderr
  assert got.stderr.endswith(b'1 error generated.\n'),got.stderr
 print('reader refusals: 2 located failures, no recursive token parsing')
 print('parser errors:',len(cases)+5,'complete result comparisons, one located prototype limitation')

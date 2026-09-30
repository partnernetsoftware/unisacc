import pathlib,re,subprocess,sys,tempfile
import sys as _sys, pathlib as _pl; _sys.path.insert(0, next(str(_p / 'tests') for _p in _pl.Path(__file__).resolve().parents if (_p / 'tests/checklib.py').is_file()))
from checklib import run
ua=sys.argv[1]
compiler=["/bin/sh",ua] if ua.endswith(".com") else [ua]
with tempfile.TemporaryDirectory(prefix='diag-units-') as td:
 p=pathlib.Path(td);a=p/'first.c';b=p/'second.c';h=p/'local.h'
 h.write_text('static int helper(void){return 3;}\n')
 b.write_text('int main(void){\n int unused;\n return 0;\n}\n')
 cc=run(['cc','-std=c99','-Wall','-fsyntax-only',b])
 assert cc.returncode==0 and (str(b)+':2:').encode() in cc.stderr and b'[-Wunused-variable]' in cc.stderr
 cases=['int helper(void){return \\\n3;}\n',
        '#include "local.h"\nint f(void){return helper();}\n',
        'int f(void){printf("ok");return 0;}\n']
 n=0
 for source in cases:
  a.write_text(source)
  for files in ([a,b],[b,a]):
   for level in [0,1,2]:
    r=run([*compiler,*files,'-Wall','-S','-O'+str(level),'-o','-'])
    assert r.returncode==0 and r.stdout,(r.returncode,r.stderr)
    lines=[line for line in r.stderr.splitlines() if b': warning:' in line]
    assert len(lines)==1 and lines[0].startswith((str(b)+':2:6: warning:').encode()) and b'[-Wunused-variable]' in lines[0],r.stderr
    assert r.stderr.endswith(b'1 warning generated.\n'),r.stderr
    n+=1
 assert n==18
 print('diagnostic units:',n,'fixed file/line/column checks; host cc confirms source line')

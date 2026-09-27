#!/usr/bin/env python3
"""Full multi-unit warning/tape/exit comparison with the reference front end."""
import pathlib,subprocess,sys
p=pathlib.Path(sys.argv[1]);target=sys.argv[2];ref=sys.argv[3]
def run(a):return subprocess.run(list(map(str,a)),capture_output=True,timeout=60)
def equal(a,b):
    assert (a.returncode,a.stdout,a.stderr)==(b.returncode,b.stdout,b.stderr),(a.args,a.returncode,len(a.stdout),a.stderr[-2500:],b.returncode,len(b.stdout),b.stderr[-2500:])
a=p/'first.c';b=p/'second.c';out=p/'sentinel';dep=p/'dependencies'
(p/'hidden.h').write_text('static int hidden(void){int unused;}\n')
cases=[
 ('basic','int f(void){int unused;}','int f(void); int main(void){int *p=3;return 0;}'),
 ('positions','#define X 3\nint f(void){\n\tint unused;\n}\n','int f(void);\n\nint main(void){int *p=\\\n3;return 0;}\n'),
 ('headers','#include "hidden.h"\nint f(void){int unused;}','int f(void); int main(void){int unused;return 0;}'),
 ('static','static int value; static int f(void){int value;return 0;} int other(void){return f();}','static int value; int other(void); int main(void){int value;return other();}'),
 ('contexts','int f(void){int x;x=1;return 0;}','int f(void); int main(void){int y;y=1;return y+f();}'),
 ('all-four','int f(void){int *p=3;printf("%ld",1);}','int f(void); int main(void){int unused;return 0;}'),
 ('strings','static const char *value="a"\n"b"; int f(void){int unused;return value[0];}','int f(void); int main(void){int unused;return f()-97;}'),
 ('empty','','int main(void){int unused;return 0;}'),
 ('clean','int f(void){return 3;}','int f(void); int main(void){return f()-3;}'),
]
count=0
for case,first,second in cases:
 a.write_text(first+'\n');b.write_text(second+'\n')
 for files in ([a,b],[b,a]):
  for flag in ['-Wall','-Werror']:
   args=[*files,'-t',target,flag,'-o','-'];want=run([ref,*args]);assert want.returncode==(0 if flag=='-Wall' or case=='clean' else 1),(case,want)
   for name in ['cc','ua','asm']:
    base=[p/('driver-'+name),'--models',p/'compiler.pkg']
    equal(run(base+args),want);count+=1
 print(case,'both orders, warning/error modes, three drivers same',flush=True)
# Optimisation and actual execution keep the accumulated single summary.
a.write_text('int f(void){int unused;return 3;}\n');b.write_text('int f(void); int main(void){return f()-3;}\n')
for name in ['cc','ua','asm']:
 base=[p/('driver-'+name),'--models',p/'compiler.pkg']
 for level in [1,2]:
  args=[a,b,'-t',target,'-Wall','-O'+str(level),'-o','-'];equal(run(base+args),run([ref,*args]));count+=1
 args=[a,b,'-Wall','-run'];equal(run(base+args),run([ref,*args]));count+=1
 out.write_bytes(b'keep');dep.write_bytes(b'dep')
 args=[a,b,'-Werror','-MF',dep,'-o',out,'-run'];equal(run(base+args),run([ref,*args]));count+=1
 assert out.read_bytes()==b'keep' and dep.read_bytes()==b'dep'
assert count==120,count
print('multi warnings:',count,'complete result comparisons')
# Recovery and the error limit span unit boundaries, including -Wall output
# before an error. Compare all bytes and exit status, not only the count.
a.write_text('int f(void){int *p=3;return missing;}\n')
b.write_text('int main(void){int n=3\nreturn n;}\n')
errors=0
for files in ([a,b],[b,a]):
 for limit in ('0','1','2'):
  args=[*files,'-Wall','-ferror-limit='+limit,'-t',target,'-o','-']
  want=run([ref,*args]);assert want.returncode==1 and not want.stdout
  for name in ['cc','ua','asm']:
   equal(run([p/('driver-'+name),'--models',p/'compiler.pkg',*args]),want);errors+=1
assert errors==18,errors
print('multi errors:',errors,'complete results; recovery, both orders, cross-unit limit')

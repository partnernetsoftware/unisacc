#!/usr/bin/env python3
"""Reference return-type or --int-conversion warnings on the model path.
This is not full -Wall parity. Cases avoid the remaining unmigrated kinds.
Both tape and complete diagnostic bytes must agree; every child is bounded.
"""
import os,pathlib,subprocess,sys,tempfile
R=pathlib.Path(__file__).resolve().parents[2]

def run(args):return subprocess.run(list(map(str,args)),capture_output=True,timeout=60)
def call(args):
    p=run(args);assert p.returncode==0,(p.args,p.returncode,p.stderr[-1200:]);return p.stdout

with tempfile.TemporaryDirectory(prefix='return-warnings-') as td:
    t=pathlib.Path(td)
    pieces=[R/'tests/refshim.h',R/'src/version.h',R/'kernel/unisa_model.inc',R/'kernel/unisa_headers.inc']
    source=''.join(p.read_text() for p in pieces)
    source+=''.join(l for l in (R/'kernel/unisa_core.c').read_text().splitlines(True) if not l.startswith('#include "unisa_'))
    source+=''.join((R/'src'/n).read_text() for n in ['front_pp.c','front_parse.c','opt.c','main.c','back_lower.c','back_encode.c','back_image.c'])
    source+=(R/'tests/reffoot.h').read_text()
    (t/'ref.c').write_text(source);call(['cc','-w','-O1',t/'ref.c','-o',t/'ref'])
    call([os.environ.get('EXEC_CC','cc'),'-O2',R/'exec/c/run.c','-o',t/'run'])
    for name,script,flag in [('pp','pp/gen.py','--locations'),('lex','lex/gen.py','--locations'),('parse','parse2/gen2.py','--warnings'),('plain','parse2/gen2.py','--locations')]:
        call([sys.executable,R/'exec'/script,t/(name+'.json'),flag])
        call([sys.executable,R/'exec/c/tbl.py',t/(name+'.json'),t/(name+'.tbl')])
        call([sys.executable,R/'exec/c/net.py',t/(name+'.tbl'),t/(name+'.net')])
    cases=[('empty','int f(void){}'),('void','void f(void){}'),('main',''),
           ('return','int f(void){return 1;}'),('if-only','int f(int x){if(x)return 1;}'),
           ('if-else','int f(int x){if(x)return 1;else return 2;}'),
           ('else-falls','int f(int x){if(x)return 1;else {}}'),
           ('nested','int f(int x){if(x){if(x==2)return 1;else return 3;}else return 2;}'),
           ('after-return','int f(void){return 1; ;}'),
           ('block-return','int f(void){{return 1;}}'),
           ('static-ordinals','int f(void){static int x=3;return x++;} int g(void){static int y=7;return f()+y;}'),
           ('while-one','int f(void){while(1){}}'),('while-zero','int f(void){while(0){}}'),
           ('while-variable','int f(int x){while(x){}}'),
           ('while-return','int f(int x){while(x){return 3;}}'),
           ('while-negative','int f(void){while(-1){}}'),
           ('while-char','int f(void){while(\'x\'){}}'),
           ('while-float','int f(void){while(1.0){}}'),
           ('while-fraction','int f(void){while(.5){}}'),
           ('while-hex','int f(void){while(0x10){}}'),
           ('for-ever','int f(void){for(;;){}}'),('for-break','int f(void){for(;;){break;}}'),
           ('for-zero','int f(void){for(;0;){}}'),
           ('do-return','int f(void){do{return 1;}while(0);}'),
           ('pointer','int *f(void){}'),('void-pointer','void *f(void){}'),
           ('struct','struct S{int x;}; struct S f(void){}'),
           ('two','int f(void){}\nint g(void){}'),
           ('macro','#define BODY {}\nint f(void)BODY'),
           ('splice','int f(void){\\\n;}'),
           ('header','#include "quiet.h"\nint f(void){}'),
           ('exit','void __exit(int x){} int f(void){__exit(0);}')]
    (t/'quiet.h').write_text('static int in_header(void){}\n')
    cases.append(('float-truth',(R/'exec/parse2/probes/float_truth.c').read_text()))
    category='return-type'
    if '--int-conversion' in sys.argv:
        category='int-conversion'
        cases=[
          ('init-nonzero','int f(void){int *p=7; return p!=0;}'),
          ('init-zero','int f(void){int *p=0; return p!=0;}'),
          ('init-zerohex','int f(void){int *p=0x0; return p!=0;}'),
          ('init-zeroexpr','int f(void){int *p=0+0; return p!=0;}'),
          ('init-parenzero','int f(void){int *p=(0); return p!=0;}'),
          ('init-castzero','int f(void){int *p=(int)0; return p!=0;}'),
          ('init-pointercast','int f(void){int *p=(int *)7; return p!=0;}'),
          ('init-enum','enum {Z=0}; int f(void){int *p=Z; return p!=0;}'),
          ('init-macro','#define Z 0\nint f(void){int *p=Z; return p!=0;}'),
          ('init-address','int f(void){int x=3; int *p=&x; return *p;}'),
          ('init-string','int f(void){char *p="ab"; return p[0];}'),
          ('init-call','int g(void){return 7;} int f(void){int *p=g(); return p!=0;}'),
          ('init-callplus','int g(void){return 7;} int f(void){int *p=g()+1; return p!=0;}'),
          ('init-fnvalue','int g(void){return 7;} int f(void){int *p=g; return p!=0;}'),
          ('init-fnptr','int f(int (*g)(void)){int *p=g(); return p!=0;}'),
          ('init-castcall','int g(void){return 7;} int f(void){int *p=((int (*)(void))g)(); return p!=0;}'),
          ('init-sizeof','int f(void){int *p=sizeof(int); return p!=0;}'),
          ('init-nested','int f(int *q){int *p=(q=7); return p==q;}'),
          ('assign','int f(int *p){p=7; return p!=0;}'),
          ('assign-zero','int f(int *p){p=0; return p!=0;}'),
          ('assign-parameter','int f(int *p,int k){p=k; return p!=0;}'),
          ('assign-nested','int f(int *p,int *q){p=q=7; return p==q;}'),
          ('assign-call','int g(void){return 7;} int f(int *p){p=g(); return p!=0;}'),
          ('assign-member','struct S{int *p;}; int f(struct S *s){s->p=7; return s->p!=0;}'),
          ('assign-index','int f(int **a){a[0]=7; return a[0]!=0;}'),
          ('assign-star','int f(int **a){*a=7; return *a!=0;}'),
          ('compound','int f(int *p){p+=7; return p!=0;}'),
          ('return-notchecked','int *f(void){return 7;}'),
          ('header-int','#include "quiet.h"\nint f(int *p){p=8; return p!=0;}'),
        ]
        (t/'quiet.h').write_text('static int in_header(int *p){p=7;return p!=0;}\n')
    positives=0
    for name,body in cases:
        f=t/'source.c';f.write_text(body if name=='float-truth' else body+'\nint main(void){return 0;}\n')
        if name=='float-truth':
            call(['cc','-O0',f,'-o',t/'truth'])
            assert call([t/'truth'])==b''
            assert call([t/'ref',f,'-run'])==b''
        ref=run([t/'ref',f,'-Wall','-S','-o','-']);assert ref.returncode==0,(name,ref.stderr)
        pp=call([t/'run',t/'pp.net',f,f,R/'include']);(t/'pp').write_bytes(pp)
        tok=call([t/'run',t/'lex.net',t/'pp']);(t/'tokens').write_bytes(tok)
        got=run([t/'run',t/'parse.net',t/'tokens',f])
        assert got.returncode==0,(name,got.stderr)
        baseline=run([t/'run',t/'plain.net',t/'tokens',f])
        assert baseline.returncode==0 and baseline.stdout==ref.stdout and not baseline.stderr,(name,'quiet tape')
        assert got.stdout==ref.stdout,(name,'warning tape')
        assert got.stderr==ref.stderr,(name,ref.stderr,got.stderr)
        if ('[-W'+category+']').encode() in ref.stderr: positives+=1
        print(category+' warning',name,'tape and diagnostics match',flush=True)
    assert positives>0
    print(category+' warning:',len(cases),'cases;',positives,'with warnings; full -Wall still pending')

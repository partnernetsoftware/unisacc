#!/usr/bin/env python3
"""Reference return-type, --int-conversion, --unused or --format model warnings.
This checks single-unit model rules, not compiler CLI -Wall parity.
Both tape and complete diagnostic bytes must agree; every child is bounded.
"""
import os,pathlib,subprocess,sys,tempfile
R=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(R/'exec/c'));from refsource import source_text,compile_command
from compilerpack import built_model
import sys as _sys, pathlib as _pl; _sys.path.insert(0, next(str(_p / 'tests') for _p in _pl.Path(__file__).resolve().parents if (_p / 'tests/checklib.py').is_file()))
from checklib import run

def call(args):
    p=run(args);assert p.returncode==0,(p.args,p.returncode,p.stderr[-1200:]);return p.stdout

with tempfile.TemporaryDirectory(prefix='return-warnings-') as td:
    t=pathlib.Path(td)
    source=source_text(R)
    (t/'ref.c').write_text(source);call(compile_command(R,t/'ref.c',t/'ref'))
    call([os.environ.get('EXEC_CC','cc'),'-O2',R/'exec/c/run.c','-o',t/'run'])
    for name,script,flag in [('pp','build/gen.py','pp --locations'),('lex','build/gen.py','lex --locations'),('parse','build/gen.py','parse2 --warnings'),('plain','build/gen.py','parse2 --locations')]:
        built_model(t,name,R/'exec'/script,flag.split())   # shared content-keyed cache (compilerpack), digest-verified on hit
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
    if '--unused' in sys.argv:
        category='unused-variable'
        cases=[
          ('unused','int f(void){int x;return 0;}'),
          ('initialized','int f(void){int x=3;return 0;}'),
          ('written','int f(void){int x;x=3;return 0;}'),
          ('read','int f(void){int x=3;return x;}'),
          ('discarded','int f(void){int x=3;x;return 0;}'),
          ('increment','int f(void){int x=3;x++;return 0;}'),
          ('preincrement','int f(void){int x=3;++x;return 0;}'),
          ('compound','int f(void){int x=3;x+=2;return 0;}'),
          ('address','int f(void){int x=3;int *p=&x;return *p;}'),
          ('parameter','int f(int x){return 0;}'),
          ('static','int f(void){static int x;return 0;}'),
          ('global','int x;int f(void){return 0;}'),
          ('array','int f(void){int x[3];return 0;}'),
          ('arraywrite','int f(void){int x[3];x[0]=3;return 0;}'),
          ('sizeof','int f(void){int x=3;return sizeof x;}'),
          ('sizeof-paren','int f(void){int x=3;return sizeof(x);}'),
          ('multiple','int f(void){int a,b=3,c;return b;}'),
          ('shadow','int f(void){int x=3;{int x=2;return x;}return 0;}'),
          ('restore','int f(void){int x=3;{int x=2;}return x;}'),
          ('nested-order','int f(void){int x=3;{int a,b;}{int c;}return 0;}'),
          ('static-shadow','int f(void){int x=3;{static int x=2;x++;}return 0;}'),
          ('static-restore','int f(void){int x=3;{static int x=2;x++;}return x;}'),
          ('member-name','struct S{int x;};int f(void){int x;struct S s;s.x=3;return s.x;}'),
          ('initializer-use','int f(void){int x=3;int y=x;return 0;}'),
          ('enum-shadow','enum {X=7};int f(void){int X=3;{int X=2;}return X;}'),
          ('two-functions','int f(void){int x;return 0;}int g(void){int y;return 0;}'),
          ('for-decl','int f(void){for(int x=0;0;){}return 0;}'),
          ('for-body','int f(void){for(int x=0;x<1;x++){int y;}return 0;}'),
          ('assigned-in-argument','int g(int n){return n;}int f(void){int x;return g(x=3);}'),
          ('nested-assignment','int f(void){int x,y;x=y=3;return 0;}'),
          ('if-write','int f(int k){int x;if(k)x=3;return 0;}'),
          ('member','struct S{int a;};int f(void){struct S s;s.a=3;return 0;}'),
          ('pointercall','int g(void){return 1;}int f(void){int (*p)(void)=g;return p();}'),
          ('unused-fp','int f(void){int (*p)(void);return 0;}'),
          ('long-name','int f(void){int abcdefghijklmnopqrstuvwxyz;return 0;}'),
          ('macro','#define DECL int x\nint f(void){DECL;return 0;}'),
          ('header-unused','#include "quiet.h"\nint f(void){int x;return 0;}'),
          ('splice-unused','int f(void){int '+chr(92)+'\n x;return 0;}'),
        ]
        (t/'quiet.h').write_text('static int in_header(void){int hidden;return 0;}\n')
    if '--format' in sys.argv:
        category='format'
        cases=[
          ('string-int','int f(void){printf("%s",7);return 0;}'),
          ('address-int','int f(void){printf("%p",7);return 0;}'),
          ('integer-pointer','int f(void){printf("%d","x");return 0;}'),
          ('integer-float','int f(void){printf("%d",1.0);return 0;}'),
          ('integer-long','int f(void){printf("%d",1L);return 0;}'),
          ('long-int','int f(void){printf("%ld",1);return 0;}'),
          ('real-int','int f(void){printf("%f",1);return 0;}'),
          ('okay','int f(void){printf("%s %p %d %ld %f","x","x",1,1L,1.0);return 0;}'),
          ('int-float-cast','int f(void){printf("%d",(float)1);return 0;}'),
          ('real-float-pointer','int f(void){printf("%f",(double*)0);return 0;}'),
          ('small-width','int f(void){char c=3;printf("%ld",c);return 0;}'),
          ('unsigned','int f(void){printf("%u %lu",1UL,1U);return 0;}'),
          ('lengths','int f(void){printf("%hd %lld %zd",1L,1,1L);return 0;}'),
          ('flags','int f(void){printf("%+-#08.4ld",1);return 0;}'),
          ('stars','int f(void){printf("%*.*ld",3,2,1);return 0;}'),
          ('percent','int f(void){printf("%% %ld",1);return 0;}'),
          ('few','int f(void){printf("%ld %s",1);return 0;}'),
          ('extra','int f(void){printf("plain",1);return 0;}'),
          ('trailing','int f(void){printf("%000.",1);return 0;}'),
          ('unknown','int f(void){printf("%q %ld",1,1);return 0;}'),
          ('nonliteral','int f(char *s){printf(s,1);return 0;}'),
          ('call-exempt','int g(void){return 1;}int f(void){printf("%s",g());return 0;}'),
          ('nested','int f(void){printf("%d",printf("%ld",1));return 0;}'),
          ('conditional','int f(int k){printf("%ld",k?1:2);return 0;}'),
          ('conditional-marker','int f(int k){printf("%d",k>1?k*2:k+1);printf("marker");return 0;}'),
          ('nested-format-order','int f(void){printf("%ld %d",1,printf("%ld",2));return 0;}'),
          ('struct-temp','struct S{int x;};struct S g(void){struct S s={3};return s;}int use(struct S s){return s.x;}int f(void){printf("%d",use(g()));return 0;}'),
          ('fnvalue','int g(void){return 1;}int f(void){printf("%s %d",g,g);return 0;}'),
          ('fnptr','int f(int (*p)(void)){printf("%d",p);return 0;}'),
          ('struct','struct S{int x;};int f(void){struct S s;s.x=1;printf("%d",s);return 0;}'),
          ('adjacent','int f(void){printf("%" "ld",1);return 0;}'),
          ('escaped','int f(void){printf("\\x25ld",1);return 0;}'),
          ('two-calls','int f(void){printf("%ld",1);printf("%s",3);return 0;}'),
          ('all-four','int f(void){int unused;int *p=7;printf("%ld",1);}'),
          ('star-assignment','int f(int *p){printf("%*ld",p=7,1);return p!=0;}'),
          ('many-conversions','int f(void){printf("%i %x %X %o %c",1L,1L,1L,1L,1L);return 0;}'),
          ('real-conversions','int f(void){printf("%e %g %E %G %F",1,1,1,1,1);return 0;}'),
          ('header-format','#include "quiet.h"\nint f(void){printf("%ld",1);return 0;}'),

        ]
    if category=='format': (t/'quiet.h').write_text('static int header_format(void){printf("%ld",1);return 0;}\n')
    if category=='format': cases.append(('vararg-float',(R/'exec/parse2/probes/vararg_float.c').read_text()))
    total=len(cases)
    shard='all'
    if '--shard' in sys.argv:
        try:
            shard=sys.argv[sys.argv.index('--shard')+1]
            part,parts=map(int,shard.split('/'))
            assert 0<=part<parts<=total
        except (ValueError,IndexError,AssertionError):
            raise SystemExit('bad --shard: use index/count, zero based')
        cases=cases[part::parts]
    assert cases
    positives=0
    for name,body in cases:
        f=t/'source.c';f.write_text(body if name in ('float-truth','vararg-float') else body+'\nint main(void){return 0;}\n')
        if name in ('float-truth','vararg-float'):
            expected=b'' if name=='float-truth' else b'1.5 2.5\n'
            call(['cc','-O0',f,'-o',t/'truth'])
            assert call([t/'truth'])==expected
            assert call([t/'ref',f,'-run'])==expected
        ref=run([t/'ref',f,'-Wall','-S','-o','-']);assert ref.returncode==0,(name,ref.stderr)
        pp=call([t/'run',t/'pp.net',f,f,R/'include']);(t/'pp').write_bytes(pp)
        tok=call([t/'run',t/'lex.net',t/'pp']);(t/'tokens').write_bytes(tok)
        got=run([t/'run',t/'parse.net',t/'tokens',f])
        assert got.returncode==0,(name,got.stderr)
        baseline=run([t/'run',t/'plain.net',t/'tokens',f])
        quiet=run([t/'ref',f,'-S','-o','-']);assert quiet.returncode==0,(name,quiet.stderr)
        assert baseline.returncode==0 and baseline.stdout==quiet.stdout and not baseline.stderr,(name,'quiet tape')
        assert got.stdout==ref.stdout,(name,'warning tape')
        assert ref.stdout==quiet.stdout,(name,'-Wall changed tape')
        assert got.stderr==ref.stderr,(name,ref.stderr,got.stderr)
        if ('[-W'+category+']').encode() in ref.stderr: positives+=1
        print(category+' warning',name,'tape and diagnostics match',flush=True)
    assert positives>0
    print(category+' warning:',len(cases),'cases;',positives,'with warnings; shard',shard,'of',total,'cases; single-unit rule check')

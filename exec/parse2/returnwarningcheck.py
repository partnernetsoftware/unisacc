#!/usr/bin/env python3
"""First actual warning kind on the model path: reference laststmt semantics.
This is not full -Wall parity. Cases avoid the three unmigrated warning kinds.
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
        if b'[-Wreturn-type]' in ref.stderr: positives+=1
        print('return warning',name,'tape and diagnostics match',flush=True)
    assert positives>0
    print('return warning:',len(cases),'cases;',positives,'with warnings; full -Wall still pending')

#!/usr/bin/env python3
"""Compare full driver rc/stdout/stderr; Werror must never touch outputs."""
import pathlib,subprocess,sys
p=pathlib.Path(sys.argv[1]);target=sys.argv[2];ref=sys.argv[3]
def run(a):return subprocess.run(list(map(str,a)),capture_output=True,timeout=60)
def equal(a,b):
    assert (a.returncode,a.stdout,a.stderr)==(b.returncode,b.stdout,b.stderr),(a.args,a.returncode,len(a.stdout),a.stderr[-1800:],b.returncode,len(b.stdout),b.stderr[-1800:])
f=p/'warn.c';clean=p/'clean.c';hdr=p/'inside.h'
hdr.write_text('static int header_function(void){int unused;return 0;}\n')
f.write_text('#include "inside.h"\nint f(void){int unused; int *p=3;printf("%ld",1); }\nint main(void){return 0;}\n')
clean.write_text('int main(void){return 0;}\n')
count=0
for name in ['cc','ua','asm']:
    base=[p/('driver-'+name),'--models',p/'compiler.pkg']
    for flag in ['-Wall','-Wextra','-Werror']:
        for level in range(3):
            args=[f,'-b',target,'-S','-O'+str(level),flag,'-o','-']
            a,b=run(base+args),run([ref,*args]);equal(a,b)
            assert b.stderr and b.returncode==(1 if flag=='-Werror' else 0)
            count+=1
        for mode in ['-E','-dump-tokens']:
            args=[f,mode,flag] if mode=='-dump-tokens' else [f,'-b',target,mode,flag]
            equal(run(base+args),run([ref,*args]));count+=1
    # Rejection precedes any output/dependency opening and any user execution.
    for mode in ['-S','-c','image','-run']:
        out=p/'sentinel';dep=p/'dependency';out.write_bytes(b'keep');dep.write_bytes(b'dep')
        args=[f,'-b',target,'-Werror','-MF',dep,'-o',out]+([] if mode=='image' else [mode])
        a,b=run(base+args),run([ref,*args]);equal(a,b)
        assert a.returncode==1 and not a.stdout
        assert out.read_bytes()==b'keep' and dep.read_bytes()==b'dep'
        count+=1
    args=[clean,'-Werror','-run'];equal(run(base+args),run([ref,*args]));count+=1
    # Located multi-unit framing is not implemented yet: named refusal.
    a=run(base+[f,clean,'-Wall','-S'])
    assert a.returncode==1 and not a.stdout and b'multi-unit warning locations not migrated' in a.stderr
    print(name+': warning modes, preprocessing, Werror output/run barrier pass',flush=True)
assert count==60,count
print('warning driver:',count,'full-result comparisons; single-unit only')

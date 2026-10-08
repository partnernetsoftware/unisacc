#!/usr/bin/env python3
"""Multi-unit tape, native behaviour, preprocessing isolation and framing."""
import json,os,pathlib,struct,subprocess,sys
p=pathlib.Path(sys.argv[1]);target=sys.argv[2];ua=sys.argv[3]
def run(cmd,**kw):return subprocess.run(list(map(str,cmd)),capture_output=True,timeout=60,**kw)
def ok(cmd,**kw):
    r=run(cmd,**kw);assert r.returncode==0,(r.args,r.returncode,r.stderr);return r.stdout
kind=os.environ.get('DRIVER_KIND','all'); assert kind in ('all','cc','ua','asm')
part=os.environ.get('MULTI_PART','all'); assert part in ('all','tapes-m-forward','tapes-m-reverse','tapes-n-forward','tapes-n-reverse','tapes-pool-forward','tapes-pool-reverse','isolation','static','frame')
assert part!='frame' or kind in ('all','cc')
drivers=[p/('driver-'+k) for k in ('cc','ua','asm') if kind in ('all',k)];count=0
if part in ('all','tapes-m-forward','tapes-m-reverse','tapes-n-forward','tapes-n-reverse','tapes-pool-forward','tapes-pool-reverse'):
    pairs=[['tests/multi/m1.c','tests/multi/m2.c'],['tests/multi/n1.c','tests/multi/n2.c'],
           ['tests/multi/printfpool1.c','tests/multi/printfpool2.c']]
    if part.startswith('tapes-m'): pairs=pairs[:1]
    if part.startswith('tapes-n'): pairs=pairs[1:2]
    if part.startswith('tapes-pool'): pairs=pairs[2:3]
    for pair in pairs:
        orders=(pair,pair[::-1])
        if part.endswith('-forward'): orders=orders[:1]
        if part.endswith('-reverse'): orders=orders[1:]
        for files in orders:
            for level in (0,1,2):
                flags=['-t',target,'-O'+str(level)]
                want=ok([ua,*files,*flags])
                for driver in drivers:
                    got=ok([driver,'--models',p/'compiler.pkg',*files,*flags])
                    assert got==want,(files,level,driver);count+=1
            want=ok([ua,'-run',*files,'-O2'])
            for driver in drivers:
                assert ok([driver,'--models',p/'compiler.pkg','-run',*files,'-O2'])==want
        print('multi-unit O0/O1/O2 tapes and native memory:',pair,'orders:',len(orders),flush=True)
# Each input starts preprocessing afresh, including macros and include guards.
# Static pointer declarators and global initialisers use distinct unit names.
if part in ('all','isolation'):
    (p/'local.h').write_text('#ifndef LOCAL_H\n#define LOCAL_H\n#define VALUE PICK\n#endif\n')
    a=p/'a.c';b=p/'b.c'
    a.write_text('#define PICK 2\n#include "local.h"\nstatic int value=VALUE; static int helper(void){return value;} int other(void); int main(void){return helper()*10+other();}\n')
    b.write_text('#define PICK 7\n#include "local.h"\nstatic int value=VALUE; static int helper(void){return value;} static int (*choose)(void)=helper; int other(void){return choose();}\n')
    ref=p/'reference';ok(['cc',a,b,'-o',ref]);r=run([ref]);assert r.returncode==27 and not r.stdout and not r.stderr
    for files in ([a,b],[b,a]):
        want=ok([ua,*files,'-t',target])
        for driver in drivers:
            base=[driver,'--models',p/'compiler.pkg']
            assert ok([*base,*files,'-t',target])==want
            r=run([*base,'-run',*files]);assert (r.returncode,r.stdout,r.stderr)==(27,b'',b''),r
            # Failure in a later unit must not truncate an existing destination.
            out=p/'sentinel';out.write_bytes(b'preserve')
            r=run([*base,a,p/'absent.c','-o',out]);assert r.returncode and out.read_bytes()==b'preserve'
    print('unit-local macros/guards/static function-pointer/global init: system cc exit 27, both orders',flush=True)
    # The multi route parses all units in one E3 invocation.  Its diagnostics
    # must detect duplicate external definitions before the backend sees tape.
    d1=p/'duplicate-object-a.c';d2=p/'duplicate-object-b.c'
    d1.write_text('int shared=1; int main(void){return shared;}\n')
    d2.write_text('int shared=2;\n')
    m1=p/'duplicate-main-a.c';m2=p/'duplicate-main-b.c'
    m1.write_text('int main(void){return 1;}\n')
    m2.write_text('int main(void){return 2;}\n')
    for files,reason in (([d1,d2],b'multiple definitions of this object across units'),
                         ([m1,m2],b'multiple definitions of this function')):
        ref=run([ua,*files,'-t',target])
        assert ref.returncode==1 and reason in ref.stderr,(files,ref)
        for driver in drivers:
            got=run([driver,'--models',p/'compiler.pkg',*files,'-t',target])
            assert got.returncode==1 and reason in got.stderr and not got.stdout,(files,driver,got)
    print('duplicate initialized object and duplicate main: named multi-unit rejection',flush=True)
if part in ('all','static'):
    for files in (['tests/multi/static1.c','tests/multi/static2.c'],['tests/multi/static2.c','tests/multi/static1.c']):
        want=ok([ua,*files,'-t',target])
        for driver in drivers:
            base=[driver,'--models',p/'compiler.pkg']
            assert ok([*base,*files,'-t',target])==want,files
            r=run([*base,'-run',*files]);assert (r.returncode,r.stdout,r.stderr)==(19,b'',b''),r
    print('block-static unit namespaces: both orders, reference tapes and exit 19',flush=True)
    pair=['tests/multi/qualifiedstatic1.c','tests/multi/qualifiedstatic2.c']
    oracle=p/'qualified-static-cc';ok(['cc',*pair,'-o',oracle])
    expected=run([oracle]);assert (expected.returncode,expected.stdout,expected.stderr)==(27,b'',b'')
    for files in (pair,pair[::-1]):
        want=ok([ua,*files,'-t',target])
        for driver in drivers:
            base=[driver,'--models',p/'compiler.pkg']
            assert ok([*base,*files,'-t',target])==want,('qualified static',files)
            r=run([*base,'-run',*files]);assert (r.returncode,r.stdout,r.stderr)==(27,b'',b''),r
            out=p/'qualified-static-image';ok([*base,*files,'-o',out])
            r=run([out]);assert (r.returncode,r.stdout,r.stderr)==(27,b'',b''),r
    print('const/volatile/restrict static pointers, typedefs, arrays and function pointers: both unit orders, system cc exit 27',flush=True)

if part in ('all','frame') and kind in ('all','cc'):
    sys.path.insert(0,str(pathlib.Path('exec/pp').resolve()));import sim
    d=json.loads((p/'units.json').read_text());loaded=sim.load(d)
    tokens=b'type=int\nid=main\n(\n)\n{\nreturn\nnum=0\n;\n}\neof\n10 tokens\n'
    framed=struct.pack('<I',len(tokens))+tokens
    path=p/'framed';path.write_bytes(framed)
    expected=b'@unit0\n'+tokens.split(b'eof\n')[0]+b'eof\n'
    assert ok([p/'run',p/'units.net',path])==expected
    r,out,_=sim.run(d,framed,'frame',loaded=loaded);assert r=='accept' and out==expected
    for bad in [b'',b'\1',b'\0'*4,framed[:-1],framed+b'\1',struct.pack('<I',len(tokens)+1)+tokens,framed[:-10]+b'x'*10]:
        path.write_bytes(bad);r=run([p/'run',p/'units.net',path]);assert r.returncode!=0 and not r.stdout
        verdict,_,_=sim.run(d,bad,'bad-frame',loaded=loaded);assert verdict!='accept'
    print('multi-unit:',count,'tape comparisons; bounded framing accepts/rejects on both executors')
if part.startswith('tapes-'): assert count > 0, ('no tape comparisons', part)
print('multi-unit backend:',kind,'part:',part,count,'tape comparisons')

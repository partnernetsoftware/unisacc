#!/usr/bin/env python3
"""Driver contract checks; compiler semantics stay in freshly built models."""
import json,os,pathlib,resource,signal,subprocess,sys
p=pathlib.Path(sys.argv[1]);target=sys.argv[2];ua=sys.argv[3]
def run(cmd, **kw):
    return subprocess.run(list(map(str,cmd)),capture_output=True,timeout=60,**kw)
def ok(cmd, **kw):
    r=run(cmd,**kw);assert r.returncode==0,(r.args,r.returncode,r.stderr);return r.stdout
src=pathlib.Path('examples/hello.c').resolve();n=0
drivers=[p/'driver-cc',p/'driver-ua',p/'driver-asm']
# The migrated pipeline builds the driver itself, not just its input programs.
netdriver=p/'driver-net'
netdriver.write_bytes(ok([p/'run','--bundle',p/'models.pkg',target,'exec/c/compiler.c']))
assert netdriver.read_bytes()==(p/'driver-ua').read_bytes(), 'network-built driver differs'
netdriver.chmod(0o755)
assert ok([netdriver,'--models',p/'compiler.pkg',src,'-b',target,'-O2'])==ok([ua,src,'-b',target,'-O2'])
for exe in drivers:
    base=[exe,'--models',p/'compiler.pkg']
    for level in range(3):
        for mode in ['-E','-S','-b']:
            flags=['-b',target,mode]+([target] if mode=='-b' else [])+['-O'+str(level)]
            got=ok([*base,src,*flags]);want=ok([ua,src,*flags]);assert got==want,(exe,flags)
            n+=1
    # stdin reaches the first network unchanged.
    flags=['-b',target,'-S','-O1']
    assert ok([*base,'-',*flags],input=src.read_bytes())==ok([ua,'-',*flags],input=src.read_bytes())
    # Parser/source failure cannot truncate an existing output file.
    bad=p/'bad.c';bad.write_text('int main( { this is invalid; }')
    out=p/'sentinel';out.write_bytes(b'preserve')
    r=run([*base,bad,'-o',out]);assert r.returncode!=0 and out.read_bytes()==b'preserve'
    for args in [['-nostdinc',src],['-E',src,src],['-o'],['-b'],['-D'],['-U'],['-include']]:
        r=run([*base,*args]);assert r.returncode==1 and not r.stdout
    r=run([*base,src,'-b','unknown/target','-o',out]);assert r.returncode!=0 and out.read_bytes()==b'preserve'
    r=run([*base,src,'-o',p]);assert r.returncode==1 and b'cannot open output' in r.stderr
    def limit():
        resource.setrlimit(resource.RLIMIT_FSIZE,(1024,1024));signal.signal(signal.SIGXFSZ,signal.SIG_IGN)
    r=run([*base,src,'-b',target,'-o',p/'limited'],preexec_fn=limit)
    assert r.returncode==1 and b'short output write' in r.stderr,(r.returncode,r.stderr)
# Raw CLI resources are interpreted by E2, in both the Python action oracle
# and the actual threshold-network runtime. No C-side macro parser is used.
sys.path.insert(0,str(pathlib.Path('exec/pp').resolve()))
import sim
delta=json.loads((p/'e2.json').read_text());loaded=sim.load(delta)
probe=p/'cli.c';probe.write_text('#ifdef X\nX\n#else\n17\n#endif\n__UNISA__\n')
def cli(opts,defs=(),undefs=(),includes=(),incdir=''):
    flags=['-b',target,'-E',*opts]
    want=ok([ua,probe,*flags])
    for exe in drivers:
        assert ok([exe,'--models',p/'compiler.pkg',probe,*flags])==want,opts
    files=sim.Files()
    for key,values in [('defines',defs),('undefines',undefs),('includes',includes)]:
        files.cache[('\0cli/'+key).encode()]=b''.join(str(v).encode()+b'\0' for v in values)
    files.cache[b'\0cli/include-dir']=str(incdir).encode()
    verdict,out,_=sim.run(delta,probe.read_bytes(),str(probe),files,maxsteps=50000000,loaded=loaded)
    assert verdict=='accept' and out==want,(opts,verdict,out,want)
for opts,ds,us in [(['-DX=3'],['X=3'],[]),(['-D','X'],['X'],[]),
    (['-DX='],['X='],[]),(['-DX=1+2'],['X=1+2'],[]),(['-DX=-3'],['X=-3'],[]),
    (['-DX=1','-DX=2'],['X=1','X=2'],[]),(['-DX=1','-UX'],['X=1'],['X']),
    (['-U','X','-DX=1'],['X=1'],['X']),(['-D__UNISA__=4'],['__UNISA__=4'],[]),
    (['-U__UNISA__'],[],['__UNISA__'])]: cli(opts,ds,us)
h1=p/'first.h';h2=p/'second.h'
h1.write_text('#define X 7\n');h2.write_text('#undef X\n#define X 9\n')
cli(['-include',h1,'-include',h2,'-DX=4'],['X=4'],includes=[h1,h2])
# Quoted source-relative headers outrank -I; -I outranks carried headers.
idir=p/'headers';idir.mkdir();(idir/'stdio.h').write_text('#define PICK 29\n')
(p/'local.h').write_text('#define LOCAL 31\n');(idir/'local.h').write_text('#define LOCAL 99\n')
probe.write_text('#include <stdio.h>\n#include "local.h"\nPICK LOCAL\n')
cli(['-I',idir],incdir=idir);cli(['-I'+str(idir)],incdir=idir)
probe.write_text('int main(void) { return VALUE; }\n')
flags=['-DVALUE=7','-b',target,'-O2']
for exe in drivers:
    assert ok([exe,'--models',p/'compiler.pkg',probe,*flags])==ok([ua,probe,*flags])
# Assembly driver consumes the same explicit package from an isolated cwd;
# this is the real compiler CLI, not a standalone core_run harness.
asmdir=p/'asm-isolated';asmdir.mkdir()
asm=asmdir/'compiler';asm.write_bytes((p/'driver-asm').read_bytes());asm.chmod(0o755)
(asmdir/'models.pkg').write_bytes((p/'compiler.pkg').read_bytes())
(asmdir/'hello.c').write_bytes(src.read_bytes())
base=[asm,'--models','models.pkg']
for mode in ['-E','-S']:
    flags=['-b',target,mode,'-O2']
    assert ok([*base,'hello.c',*flags],cwd=asmdir)==ok([ua,src,*flags])
ok([*base,'hello.c','-O2'],cwd=asmdir)
assert (asmdir/'a.out').read_bytes()==ok([ua,src,'-b',target,'-O2'])
assert ok([asmdir/'a.out'])==b'hello from C99\n'
before=set(asmdir.iterdir())
assert ok([*base,'-run','hello.c'],cwd=asmdir)==b'hello from C99\n'
assert set(asmdir.iterdir())==before
print('assembly compiler driver: isolated package, native output and memory run pass')
# Public-shaped commands operate with only the container and source in cwd.
isolated=p/'isolated';isolated.mkdir()
com=isolated/'compiler.com';com.write_bytes((p/'driver.com').read_bytes())
(isolated/'hello.c').write_bytes(src.read_bytes())
base=['sh',com]
for mode in ['-E','-S']:
    flags=['-b',target,mode,'-O2']
    assert ok([*base,'hello.c',*flags],cwd=isolated)==ok([ua,src,*flags])
ok([*base,'hello.c','-O2'],cwd=isolated)
assert (isolated/'a.out').read_bytes()==ok([ua,src,'-b',target,'-O2'])
assert ok([isolated/'a.out'])==b'hello from C99\n'
before=set(isolated.iterdir())
assert ok([*base,'-run','hello.c'],cwd=isolated)==b'hello from C99\n'
assert set(isolated.iterdir())==before, 'memory run created a file'
print(f'compiler driver: {n} mode/level matches, CLI resource/model checks, stdin, fail-before-output, IO failures, embedded default image ran')

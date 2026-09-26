#!/usr/bin/env python3
"""Driver contract checks; compiler semantics stay in freshly built models."""
import os,pathlib,resource,signal,subprocess,sys
p=pathlib.Path(sys.argv[1]);target=sys.argv[2];ua=sys.argv[3]
def run(cmd, **kw):
    return subprocess.run(list(map(str,cmd)),capture_output=True,timeout=60,**kw)
def ok(cmd, **kw):
    r=run(cmd,**kw);assert r.returncode==0,(r.args,r.returncode,r.stderr);return r.stdout
src=pathlib.Path('examples/hello.c').resolve();n=0
# The migrated pipeline builds the driver itself, not just its input programs.
netdriver=p/'driver-net'
netdriver.write_bytes(ok([p/'run','--bundle',p/'models.pkg',target,'exec/c/compiler.c']))
assert netdriver.read_bytes()==(p/'driver-ua').read_bytes(), 'network-built driver differs'
netdriver.chmod(0o755)
assert ok([netdriver,'--models',p/'compiler.pkg',src,'-b',target,'-O2'])==ok([ua,src,'-b',target,'-O2'])
for exe in [p/'driver-cc',p/'driver-ua']:
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
    for args in [['-run',src],['-DNAME=1',src],[src,src],['-o'],['-b']]:
        r=run([*base,*args]);assert r.returncode==1 and not r.stdout
    r=run([*base,src,'-b','unknown/target','-o',out]);assert r.returncode!=0 and out.read_bytes()==b'preserve'
    r=run([*base,src,'-o',p]);assert r.returncode==1 and b'cannot open output' in r.stderr
    def limit():
        resource.setrlimit(resource.RLIMIT_FSIZE,(1024,1024));signal.signal(signal.SIGXFSZ,signal.SIG_IGN)
    r=run([*base,src,'-b',target,'-o',p/'limited'],preexec_fn=limit)
    assert r.returncode==1 and b'short output write' in r.stderr,(r.returncode,r.stderr)
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
print(f'compiler driver: {n} mode/level matches, stdin, fail-before-output, IO failures, embedded default image ran')

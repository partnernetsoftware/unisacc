#!/usr/bin/env python3
"""Check bound bytes independently, then output/status of real in-memory runs."""
import json,os,pathlib,struct,subprocess,sys
p=pathlib.Path(sys.argv[1]);target=sys.argv[2];ua=sys.argv[3]
def run(cmd,**kw):return subprocess.run(list(map(str,cmd)),capture_output=True,timeout=60,**kw)
def ok(cmd,**kw):
    r=run(cmd,**kw);assert r.returncode==0,(r.args,r.returncode,r.stderr);return r.stdout
sys.path.insert(0,str(pathlib.Path('exec/pp').resolve()));import sim
sys.path.insert(0,str(pathlib.Path('exec/c').resolve()));from pack import build
low=json.loads((p/'lower.json').read_text());enc=json.loads((p/'elf.json').read_text())
ll=sim.load(low);el=sim.load(enc)
resources=p/'resources';(resources/'process').mkdir(parents=True);(resources/'memory').mkdir()
manifest=p/'memory.tsv';manifest.write_text('memory\tencode\ttarget.text\tmemory-v1\telf.net\n')
for f in ['examples/hello.c','tests/c/b_funcptr.c']:
    tape=ok([ua,f,'-t',target,'-O2']);(p/'tape').write_bytes(tape)
    ref=ok([p/'ref-memory',p/'tape']);line,body=ref.split(b'\n',1)
    tb,db,nt,nd,entry=map(int,line.split());assert len(body)==nt+nd
    files=sim.Files()
    vals={'process/argc':3,'process/argv':0x123456780,'memory/text':tb,'memory/data':db}
    for key,v in vals.items():
        raw=struct.pack('<Q',v);files.cache[b'\0'+key.encode()]=raw;(resources/key).write_bytes(raw)
    r,lowered,_=sim.run(low,tape,f,files,maxsteps=50000000,loaded=ll);assert r=='accept',(f,r,lowered)
    r,image,_=sim.run(enc,lowered,f,files,maxsteps=50000000,loaded=el);assert r=='accept',(f,r,image)
    assert image[:8]==b'UNIMEM1\n';text,extent,stored,off=struct.unpack('<4Q',image[8:40])
    assert (text,extent,off)==(nt,nd,entry),(f,(text,extent,off),(nt,nd,entry))
    assert image[40:40+text]==body[:nt],f+' text'
    assert image[40+text:]+bytes(extent-stored)==body[nt:],f+' data'
    (p/'lowered').write_bytes(lowered);(p/'bound.pkg').write_bytes(build([manifest],[('00',resources)]))
    assert ok([p/'run','--bundle',p/'bound.pkg','memory',p/'lowered'])==image
    print('bound native bytes:',f,'reference = action oracle = network')
# Binding context is atomic: a half-specified base or absent process context
# must not turn run headers into an ordinary executable image.
for missing in [('memory/text',),('memory/data',),('process/argc','process/argv')]:
    files=sim.Files()
    for key,v in vals.items():
        if key not in missing: files.cache[b'\0'+key.encode()]=struct.pack('<Q',v)
    r,_,_=sim.run(enc,lowered,'bad-context',files,maxsteps=50000000,loaded=el)
    assert r!='accept',missing
    for key in missing: (resources/key).unlink()
    (p/'bound.pkg').write_bytes(build([manifest],[('00',resources)]))
    rejected=run([p/'run','--bundle',p/'bound.pkg','memory',p/'lowered'])
    assert rejected.returncode!=0 and not rejected.stdout,(missing,rejected.returncode)
    for key in missing: (resources/key).write_bytes(struct.pack('<Q',vals[key]))
print('memory context: missing paired bases or process context rejected by both executors')
# Behaviour includes real stdio, arguments, pointers, static storage, all levels.
probes=['examples/hello.c','examples/fib.c','examples/struct.c','tests/c/b_argv.c','tests/c/b_printf.c','tests/c/b_static.c']
drivers=[p/'driver-cc',p/'driver-ua',p/'driver-asm']
for exe in drivers:
    for f in probes:
        for level in (0,2):
            flags=['-O'+str(level),'-run',f,'one','two']
            ref=run([ua,*flags]);got=run([exe,'--models',p/'compiler.pkg',*flags])
            assert ref.returncode>=0 and ref.returncode<128
            assert (got.returncode,got.stdout,got.stderr)==(ref.returncode,ref.stdout,ref.stderr),(exe,f,level,ref.returncode,got.returncode,got.stderr)
    app=p/'app.c';app.write_text('#include <stdio.h>\n#include <stdlib.h>\nint main(int n,char **v) { printf("%d %s %s\\n",n,v[1],getenv("MEMORY_PROBE")); return 7; }\n')
    env=dict(os.environ,MEMORY_PROBE='present');args=['-O1','-run',app,'--','-argument']
    # Exact intended argv contract, independent of the old driver's -- quirk.
    got=run([exe,'--models',p/'compiler.pkg',*args],env=env)
    assert got.returncode==7 and got.stdout==b'2 -argument present\n',(exe,got.returncode,got.stdout,got.stderr)
print(f'memory run: {len(drivers)*len(probes)*2} native runs match (C/UA/ASM), argv/env/O1 and exit status pass; no executable file written')
# The decoder is the production loader's, not a second format parser.
root=pathlib.Path.cwd()
check=p/'bounds.c';check.write_text('#define UNISA_RUNTIME_LIBRARY\n#include "run.c"\n#include "memory.c"\nint main(int n,char **v){ Buf b={0}; MemoryImage m; if(n!=2)return 1; b.b=readfile(v[1],&b.n,0); memory_image(&b,&m); free(b.b); return 0; }\n')
for compiler,name in [('cc','bounds-cc'),(ua,'bounds-ua')]:
    ok([compiler,'-O2','-I'+str(root/'exec/c'),check,'-o',p/name])
    bads=[]
    bad=bytearray(image);struct.pack_into('<Q',bad,32,text);bads.append(bad)
    bad=bytearray(image);struct.pack_into('<Q',bad,8,text+1);bads.append(bad)
    bad=bytearray(image);struct.pack_into('<Q',bad,16,1<<31);bads.append(bad)
    (p/'valid.mem').write_bytes(image);ok([p/name,p/'valid.mem'])
    for bad in bads:
        (p/'invalid.mem').write_bytes(bad);r=run([p/name,p/'invalid.mem'])
        assert r.returncode==2 and b'memory image' in r.stderr,(name,r.returncode,r.stderr)
print('memory loader: both builds accept valid bytes and reject entry/length/extent violations')

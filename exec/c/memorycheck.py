#!/usr/bin/env python3
"""Check bound bytes independently, then output/status of real in-memory runs."""
import json,os,pathlib,struct,subprocess,sys,signal
p=pathlib.Path(sys.argv[1]);target=sys.argv[2];ua=sys.argv[3]
def run(cmd,**kw):
    process=subprocess.Popen(list(map(str,cmd)),stdout=subprocess.PIPE,stderr=subprocess.PIPE,
                             start_new_session=True,**kw)
    try:
        out,err=process.communicate(timeout=55)
    except subprocess.TimeoutExpired:
        try: os.killpg(process.pid,signal.SIGKILL)
        except ProcessLookupError: pass
        process.communicate()
        raise
    return subprocess.CompletedProcess(process.args,process.returncode,out,err)
def ok(cmd,**kw):
    r=run(cmd,**kw);assert r.returncode==0,(r.args,r.returncode,r.stderr);return r.stdout
kind=os.environ.get('DRIVER_KIND','all'); assert kind in ('all','cc','ua','asm')
shard=os.environ.get('MEMORY_SHARD','all'); assert shard in ('all','1/3','2/3','3/3')
if kind in ('all','cc') and shard in ('all','1/3'):
    sys.path.insert(0,str(pathlib.Path('exec/pp').resolve()));import sim
    sys.path.insert(0,str(pathlib.Path('exec/c').resolve()));from pack import build
    low=json.loads((p/'lower.json').read_text());enc=json.loads((p/'elf.json').read_text())
    prune=json.loads((p/'prune.json').read_text())
    ll=sim.load(low);el=sim.load(enc);pl=sim.load(prune)
    resources=p/'resources';(resources/'process').mkdir(parents=True);(resources/'memory').mkdir()
    manifest=p/'memory.tsv';manifest.write_text('memory\tencode\ttarget.text\tmemory-v1\telf.net\n')
    prefix=32 if target.startswith('osx/') else 0
    for f in ['examples/hello.c','tests/c/b_funcptr.c']:
        tape=ok([ua,f,'-t',target,'-O2']);(p/'tape').write_bytes(tape)
        ref=ok([p/'ref-memory',p/'tape']);line,body=ref.split(b'\n',1)
        tb,db,nt,nd,entry=map(int,line.split());assert len(body)==nt+nd
        files=sim.Files()
        vals={'process/argc':3,'process/argv':0x123456780,'memory/text':tb,'memory/data':db}
        if prefix:
            assert nd>=prefix and db>=prefix,(f,'physical dl prefix bounds')
            slots=struct.unpack('<4Q',body[nt:nt+prefix]);assert all(slots),(f,'zero loader symbol')
            vals.update({'process/dl/'+str(i):v for i,v in enumerate(slots)})
            (resources/'process/dl').mkdir(exist_ok=True)
        for key,v in vals.items():
            raw=struct.pack('<Q',v);files.cache[b'\0'+key.encode()]=raw;(resources/key).write_bytes(raw)
        r,pruned,_=sim.run(prune,tape,f,files,maxsteps=50000000,loaded=pl);assert r=='accept',(f,'prune',r,pruned)
        r,lowered,_=sim.run(low,pruned,f,files,maxsteps=50000000,loaded=ll);assert r=='accept',(f,r,lowered)
        r,image,_=sim.run(enc,lowered,f,files,maxsteps=50000000,loaded=el);assert r=='accept',(f,r,image)
        assert image[:8]==b'UNIMEM1\n';text,extent,stored,off=struct.unpack('<4Q',image[8:40])
        expected_stored=prefix+len(body[nt+prefix:].rstrip(b'\0'))
        assert (text,extent,stored,off)==(nt,nd,expected_stored,entry),(f,(text,extent,stored,off),(nt,nd,expected_stored,entry))
        assert len(image)==40+text+stored,(f,'physical image byte length')
        assert image[40:40+text]==body[:nt],f+' text'
        assert image[40+text:]+bytes(extent-stored)==body[nt:],f+' data'
        (p/'lowered').write_bytes(lowered);(p/'bound.pkg').write_bytes(build([manifest],[('00',resources)]))
        assert ok([p/'run','--bundle',p/'bound.pkg','memory',p/'lowered'])==image
        # Same actual base, one-pass data placement must match the retained
        # explicit text/data binding byte for byte, not merely run hello.
        reserve=2147467264
        (resources/'memory/data').unlink()
        (resources/'memory/reserve').write_bytes(struct.pack('<Q',reserve))
        one=sim.Files()
        for key,v in vals.items():
            if key!='memory/data': one.cache[b'\0'+key.encode()]=struct.pack('<Q',v)
        one.cache[b'\0memory/reserve']=struct.pack('<Q',reserve)
        verdict,once,_=sim.run(enc,lowered,f,one,maxsteps=50000000,loaded=el)
        # The reference maps data wherever the OS puts it (ASLR); one-pass places
        # it at the 16 KiB-aligned end of text plus the dl prefix
        # (exec/enc/memorylayout-result.tsv ML.reserve-pages). Compare against
        # the explicit binding at that derived base, not at the live one.
        d1=((tb+nt+16383)&~16383)+prefix
        if d1==db: expect=image
        else:
            two=sim.Files()
            for key,v in vals.items(): two.cache[b'\0'+key.encode()]=struct.pack('<Q',d1 if key=='memory/data' else v)
            r,expect,_=sim.run(enc,lowered,f,two,maxsteps=50000000,loaded=el);assert r=='accept',(f,'explicit at derived base',r)
        assert verdict=='accept' and once==expect,(f,'one-pass binding differs',hex(tb),hex(db),hex(d1))
        (p/'bound.pkg').write_bytes(build([manifest],[('00',resources)]))
        assert ok([p/'run','--bundle',p/'bound.pkg','memory',p/'lowered'])==once,(f,'reserved network differs from one-pass')
        (resources/'memory/reserve').unlink()
        (resources/'memory/data').write_bytes(struct.pack('<Q',db))
        print('bound native bytes:',f,'reference = explicit binding = reserved binding = network')
    # Binding context is atomic: a half-specified base or absent process context
    # must not turn run headers into an ordinary executable image.
    missing_context=[('memory/text',),('memory/data',),('process/argc','process/argv')]
    if prefix: missing_context += [('process/dl/'+str(i),) for i in range(4)]
    for missing in missing_context:
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
    # The final valid program supplies all required process/Darwin resources.
    # A malformed reserve binding must reject before producing any bytes.
    for capacity,drop,mixed in [(2147467264,True,False),(2147467264,False,True),
                                 (0,False,False),(-1,False,False),(1<<31,False,False)]:
        bad=sim.Files()
        for key,v in vals.items():
            if key=='memory/data' and not mixed: continue
            if key=='memory/text' and drop: continue
            bad.cache[b'\0'+key.encode()]=struct.pack('<Q',v)
        bad.cache[b'\0memory/reserve']=struct.pack('<Q',capacity & ((1<<64)-1))
        verdict,_,_=sim.run(enc,lowered,'bad-reserve',bad,maxsteps=50000000,loaded=el)
        assert verdict!='accept',(capacity,drop,mixed)
        if not mixed: (resources/'memory/data').unlink()
        if drop: (resources/'memory/text').unlink()
        (resources/'memory/reserve').write_bytes(bad.cache[b'\0memory/reserve'])
        (p/'bound.pkg').write_bytes(build([manifest],[('00',resources)]))
        rejected=run([p/'run','--bundle',p/'bound.pkg','memory',p/'lowered'])
        assert rejected.returncode!=0 and not rejected.stdout,(capacity,drop,mixed,rejected.returncode)
        (resources/'memory/reserve').unlink()
        for key in ('memory/text','memory/data'):
            (resources/key).write_bytes(struct.pack('<Q',vals[key]))
    print('memory context: missing bases/process/loader slots and five malformed reserve bindings rejected by both executors')
# Behaviour includes real stdio, arguments, pointers, static storage, all levels.
probes=['examples/hello.c','examples/fib.c','examples/struct.c','tests/c/b_argv.c','tests/c/b_printf.c','tests/c/b_static.c']
if shard != 'all': probes=probes[int(shard[0])-1::3]
drivers=[p/('driver-'+k) for k in ('cc','ua','asm') if kind in ('all',k)]
for exe in drivers:
    for f in probes:
        for level in (0,2):
            flags=['-O'+str(level),'-run',f,'one','two']
            ref=run([ua,*flags]);got=run([exe,'--models',p/'compiler.pkg',*flags])
            assert ref.returncode>=0 and ref.returncode<128,(f,level,ref.returncode,ref.stderr)
            assert (got.returncode,got.stdout,got.stderr)==(ref.returncode,ref.stdout,ref.stderr),(exe,f,level,ref.returncode,got.returncode,got.stderr)
    app=p/'app.c';app.write_text('#include <stdio.h>\n#include <stdlib.h>\nint main(int n,char **v) { printf("%d %s %s\\n",n,v[1],getenv("MEMORY_PROBE")); return 7; }\n')
    env=dict(os.environ,MEMORY_PROBE='present');args=['-O1','-run',app,'--','-argument']
    # Exact intended argv contract, independent of the old driver's -- quirk.
    got=run([exe,'--models',p/'compiler.pkg',*args],env=env)
    assert got.returncode==7 and got.stdout==b'2 -argument present\n',(exe,got.returncode,got.stdout,got.stderr)
print(f'memory run [{kind}]: {len(drivers)*len(probes)*2} native runs match, argv/env/O1 and exit status pass; no executable file written')
if kind in ('all','cc') and shard in ('all','1/3'):
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
    # Compile the production loader with a Windows API-contract double.
    # This checks reserve/commit arguments and failures, not a Windows guest.
    mock=p/'reserve.c';mock.write_text((root/'exec/c/memory-contract.c').read_text())
    ok(['cc','-O2','-I'+str(root/'exec/c'),mock,'-o',p/'reserve'])
    got=ok([p/'reserve','0']);assert got==b'reserve and exact-address commit\n',got
    for mode,message in [(1,b'cannot reserve'),(2,b'cannot commit'),
                          (3,b'cannot commit'),(4,b'exceeds reserved')]:
        rejected=run([p/'reserve',str(mode)])
        assert rejected.returncode==2 and message in rejected.stderr and not rejected.stdout,(mode,rejected)
    print('memory allocation: Windows contract success and four failures; mock only')
    # Large virtual extent, only two actual pages touched; not a huge C source.
    lazy=p/'lazy.c'
    lazy.write_text('#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n'
                    'typedef struct { unsigned char *b; int n; } Buf;\n'
                    'static void die(const char *s){fprintf(stderr,"%s\\n",s);exit(2); }\n'
                    '#include "memory.c"\n'
                    'int main(void){MemoryMap x;MemoryImage m={1,610000000,0,0};'
                    'memory_reserve(&x);memory_commit(&m,&x);unsigned char *p=x.base+x.dataoff;'
                    'if(p[0] || p[609999999])return 8;p[609999999]=7;'
                    'printf("%d %d\\n",p[0],p[609999999]);munmap(x.base,x.reserved);return 0;}\n')
    ok(['cc','-O2','-I'+str(root/'exec/c'),lazy,'-o',p/'lazy'])
    got=ok([p/'lazy']);assert got==b'0 7\n',got
    print('memory extent: 610MB virtual range, zero initial pages and final page write pass; host loader only')

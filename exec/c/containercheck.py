#!/usr/bin/env python3
"""Actual compiler container: all six output routes, both local host ISAs.
The isolated runtime has only its container and source; Python is the referee.
"""
import os,pathlib,shutil,struct,subprocess,sys
p=pathlib.Path(sys.argv[1]);ua=sys.argv[2];root=pathlib.Path.cwd()
def run(args,**kw):
    return subprocess.run(list(map(str,args)),capture_output=True,timeout=60,**kw)
def ok(args,**kw):
    r=run(args,**kw);assert r.returncode==0,(r.args,r.returncode,r.stderr);return r.stdout
explicit='MODEL_COM' in os.environ
artifact=pathlib.Path(os.environ['MODEL_COM']).resolve(strict=True) if explicit else p/'build/unisacc-next.com'
assert artifact.stat().st_mode & 0o111, 'compiler artifact is not executable'
raw=artifact.read_bytes()
assert len(raw)>=64 and raw[-16:-8]==b'UNIPKG1\n', 'missing package footer'
length=int.from_bytes(raw[-8:],'little')
assert 0<length<=len(raw)-16-64, 'invalid package extent'
payload=raw[-16-length:-16]
if not explicit: assert payload==(p/'build/compiler.pkg').read_bytes()
peoff=struct.unpack_from('<I',raw,0x3C)[0]
assert peoff%8==0 and peoff+4<=len(raw)-16-length and raw[peoff:peoff+4]==b'PE\0\0'
assert raw[-16:-8]==b'UNIPKG1\n' and int.from_bytes(raw[-8:],'little')==len(payload)
assert raw[-16-len(payload):-16]==payload and raw.count(payload)==1
# Decode the package directory and raw spans, proving that neither models nor
# per-ISA cores were accidentally repeated while constructing six routes.
pos=0
def line():
    global pos
    end=payload.index(b'\n',pos);v=payload[pos:end].split();pos=end+1;return v
head=line();assert head[:2]==[b'P',b'2'];nm,ns,nr=map(int,head[2:])
assert nm>0 and ns>0 and nr>=2
routes=set();directory=[]
for _ in range(ns):
    row=line();assert len(row)==6 and row[0]==b'D'
    assert 0<=int(row[5])<nm
    routes.add(row[1]);directory.append(row)
models=[]
for _ in range(nm):
    tag,n=line();n=int(n);assert tag==b'M' and n>0 and pos+n<=len(payload)
    models.append(payload[pos:pos+n]);pos+=n
assert len(set(models))==len(models)
resources={}
for _ in range(nr):
    tag,n,length=line();n=int(n);length=int(length);assert tag==b'F'
    assert n>0 and length>=0 and pos+n+length<=len(payload)
    key=payload[pos:pos+n];assert key not in resources
    resources[key]=payload[pos+n:pos+n+length];pos+=n+length
assert pos==len(payload)
assert {int(row[5]) for row in directory}==set(range(nm)), 'unreferenced model'
for isa,arch in enumerate(('arm64','x86_64'),1):
    core=resources[b'\0kernel/'+arch.encode()]
    assert len(core)>=40 and core[:8]==b'UNIKERN1'
    kind,entry,slot,length=struct.unpack_from('<4Q',core,8)
    assert kind==isa and length==len(core)-40 and entry<length
    assert slot+8<=length and slot%8==0 and not any(core[40+slot:48+slot])
    if not explicit: assert core==(p/'build/kernels'/arch).read_bytes()
assert len([k for k in resources if k.startswith(b'\0kernel/')])==2
isolated=p/'isolated';isolated.mkdir()
com=isolated/'compiler.com';com.write_bytes(raw)
src=isolated/'hello.c';src.write_bytes((root/'examples/hello.c').read_bytes())
env=dict(os.environ);env.pop('UNISA_KERNEL',None);env.pop('UNISA_CONTAINER',None)
# Make the loose package and kernel paths disappear, so neither the bridge nor
# shell launcher can accidentally obtain data outside the final container.
if not explicit: shutil.rmtree(p/'build')
targets=[os_+'/'+arch for os_ in ('lnx','osx','win') for arch in ('arm64','x86_64')]
for target in targets:
    assert (target+'/image/O2').encode() in routes
    got=ok(['sh',com,'hello.c','-O2','-b',target],cwd=isolated,env=env)
    assert got==ok([ua,src,'-O2','-b',target]),target
for arch in ('arm64','x86_64'):
    prefix=['arch','-'+arch,'sh',com]
    assert ok([*prefix,'-run','hello.c'],cwd=isolated,env=env)==b'hello from C99\n'
    out=isolated/('hello-'+arch)
    ok([*prefix,'hello.c','-O2','-o',out],cwd=isolated,env=env)
    assert out.read_bytes()==ok([ua,src,'-O2','-b','osx/'+arch])
    assert ok([out])==b'hello from C99\n'
print(f'assembly compiler container: six targets equal; arm64/x86 memory/native execution; {nm} unique models, two cores, one package; no loose kernel/model inputs')

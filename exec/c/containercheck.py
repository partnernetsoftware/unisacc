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
artifact=p/'build/unisacc-next.com'
assert artifact.stat().st_mode & 0o111, 'compiler artifact is not executable'
raw=artifact.read_bytes();payload=(p/'build/compiler.pkg').read_bytes()
peoff=struct.unpack_from('<I',raw,0x3C)[0]
assert peoff%8==0 and raw[peoff:peoff+4]==b'PE\0\0'
assert raw[-16:-8]==b'UNIPKG1\n' and int.from_bytes(raw[-8:],'little')==len(payload)
assert raw[-16-len(payload):-16]==payload and raw.count(payload)==1
# Decode the package directory and raw spans, proving that neither models nor
# per-ISA cores were accidentally repeated while constructing six routes.
pos=0
def line():
    global pos
    end=payload.index(b'\n',pos);v=payload[pos:end].split();pos=end+1;return v
head=line();assert head[:2]==[b'P',b'2'];nm,ns,nr=map(int,head[2:])
routes=set()
for _ in range(ns):
    row=line();assert row[0]==b'D';routes.add(row[1])
models=[]
for _ in range(nm):
    tag,n=line();n=int(n);assert tag==b'M';models.append(payload[pos:pos+n]);pos+=n
assert len(set(models))==len(models)
resources={}
for _ in range(nr):
    tag,n,length=line();n=int(n);length=int(length);assert tag==b'F'
    key=payload[pos:pos+n];assert key not in resources
    resources[key]=payload[pos+n:pos+n+length];pos+=n+length
assert pos==len(payload)
for arch in ('arm64','x86_64'):
    assert resources[b'\0kernel/'+arch.encode()]==(p/'build/kernels'/arch).read_bytes()
assert len([k for k in resources if k.startswith(b'\0kernel/')])==2
isolated=p/'isolated';isolated.mkdir()
com=isolated/'compiler.com';com.write_bytes(raw)
src=isolated/'hello.c';src.write_bytes((root/'examples/hello.c').read_bytes())
env=dict(os.environ);env.pop('UNISA_KERNEL',None);env.pop('UNISA_CONTAINER',None)
# Make the loose package and kernel paths disappear, so neither the bridge nor
# shell launcher can accidentally obtain data outside the final container.
shutil.rmtree(p/'build')
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

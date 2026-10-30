#!/usr/bin/env python3
"""Guest-side execution of the carried assembly compiler, with cc as referee.
hello/fib use the compiler's implicit printf extension: the cc referee receives
an explicit stdio declaration. Other program bytes remain unchanged.
"""
import hashlib,os,pathlib,subprocess,sys
p=pathlib.Path(sys.argv[1]);root=pathlib.Path(sys.argv[2]);target=sys.argv[3];com=p/'compiler.com'
assert target in ('lnx/arm64','lnx/x86_64')
env=dict(os.environ);env.pop('UNISA_KERNEL',None);env.pop('UNISA_CONTAINER',None)
def run(args,**kw):return subprocess.run(list(map(str,args)),capture_output=True,timeout=60,cwd=p,env=env,**kw)
def ok(args,**kw):
 r=run(args,**kw);assert r.returncode==0,(r.args,r.returncode,r.stderr);return r.stdout
raw=com.read_bytes();assert raw[-16:-8]==b'UNIPKG1\n';n=int.from_bytes(raw[-8:],'little');footer=raw[-16-n:]
for source in ['examples/hello.c','examples/fib.c','tests/c/b_strderef.c','tests/c/b_argv.c']:
 src=root/source;expected=p/'cc-program';ok(['cc','-include','stdio.h','-O2',src,'-o',expected]);q=run([expected,'one','two'])
 assert 0<=q.returncode<128
 for level in (0,1,2):
  r=run(['sh',com,'-O'+str(level),'-run',src,'one','two'])
  assert (r.returncode,r.stdout,r.stderr)==(q.returncode,q.stdout,q.stderr),(source,level,r,q)
 image=p/'compiled';ok(['sh',com,'-O2',src,'-o',image]);r=run([image,'one','two'])
 assert (r.returncode,r.stdout,r.stderr)==(q.returncode,q.stdout,q.stderr),(source,r,q)
 print('Linux assembly container execution:',source,flush=True)
for os_ in ('lnx','osx','win'):
 for arch in ('arm64','x86_64'):
  route=os_+'/'+arch;got=ok(['sh',com,root/'examples/hello.c','-O2','-b',route])
  assert got==(p/(os_+'-'+arch+'.ref')).read_bytes(),route
print('Linux host: six cross-target images equal host-produced references',flush=True)
source=root/'exec/c/asmcompiler.c'
n1=ok(['sh',com,'-O2','-b',target,source]);assert n1==(p/'seed.ref').read_bytes()
for number in (1,2):
 exe=p/('n'+str(number));exe.write_bytes(n1+footer);exe.chmod(0o755)
 got=ok([exe,'-O2','-b',target,source]);assert got==n1
 assert ok([exe,'-run',root/'examples/hello.c'])==b'hello from C99\n'
print(target+': N1=N2=N3 bare image',len(n1),hashlib.sha256(n1).hexdigest(),flush=True)
print('container',len(raw),hashlib.sha256(raw).hexdigest(),flush=True)

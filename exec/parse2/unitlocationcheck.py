#!/usr/bin/env python3
"""Located unit framing: independent bytes, reference diagnostics, bad extents."""
import os,pathlib,struct,subprocess,sys,tempfile
import sys as _sys, pathlib as _pl; _sys.path.insert(0, next(str(_p / 'tests') for _p in _pl.Path(__file__).resolve().parents if (_p / 'tests/checklib.py').is_file()))
from checklib import run
R=pathlib.Path(__file__).resolve().parents[2]
def call(a):
 r=run(a);assert r.returncode==0,(a,r.returncode,r.stderr[-1200:]);return r.stdout
u32=lambda x:struct.pack('<I',x)
with tempfile.TemporaryDirectory(prefix='unit-locations-') as td:
 p=pathlib.Path(td)
 call([os.environ.get('EXEC_CC','cc'),'-O2',R/'exec/c/run.c','-o',p/'run'])
 for name,script,args in [('pp','build/gen.py',['pp','--osx','--arm64','--locations']),('lex','build/gen.py',['lex','--locations']),('units','parse2/units.py',['--locations']),('parse','parse2/gen2.py',['--warnings'])]:
  call([sys.executable,R/'exec'/script,p/(name+'.json'),*args]);call([sys.executable,R/'exec/c/tbl.py',p/(name+'.json'),p/(name+'.tbl')]);call([sys.executable,R/'exec/c/net.py',p/(name+'.tbl'),p/(name+'.net')])
 files=[p/'first-文件.c',p/'second.c'];files[0].write_text('static inline int h(int * restrict p){return *p;}\nint f(void){int unused;int n=0;return h(&n); }\n');files[1].write_text('int f(void);int main(void){return f();}\n')
 frames=[];maps=[];stream=b''
 for i,f in enumerate(files):
  q=p/'pp';pp=call([p/'run',p/'pp.net',f,f,R/'include']);q.write_bytes(pp)
  tok=call([p/'run',p/'lex.net',q]);assert tok[:8]==b'UNITOK1\0'
  n=struct.unpack_from('<I',tok,8)[0];assert tok[12:12+n]==pp
  name=str(f).encode();payload=u32(len(name))+name+tok;frames.append(u32(len(payload))+payload)
  maps.append(u32(len(name))+name+u32(n)+pp)
  stream+=b'@'+u32(i)+u32(0)+b'\n@unit'+(b'0' if i==0 else b'+')+b'\n'
  at=12+n
  while True:
   assert tok[at]==64 and tok[at+5]==10
   pos=struct.unpack_from('<I',tok,at+1)[0];end=tok.index(b'\n',at+6);line=tok[at+6:end]
   if line==b'eof':break
   stream+=b'@'+u32(i)+tok[at+1:end+1];at=end+1
  if i==len(files)-1:stream+=b'@'+u32(i)+u32(pos)+b'\neof\n'
 framed=p/'framed';framed.write_bytes(b''.join(frames))
 joined=call([p/'run',p/'units.net',framed]);expected=b'UNITOK2\0'+u32(2)+b''.join(maps)+stream
 assert joined==expected,'located unit bytes differ from independent serializer'
 dest=p/'joined';dest.write_bytes(joined)
 r=run([p/'run',p/'parse.net',dest,files[0]])
 ref=run([os.environ.get('UA','/tmp/ua_ref'),*files,'-Wall','-t','osx/arm64','-o','-'])
 assert r.returncode==ref.returncode==0 and r.stdout==ref.stdout and r.stderr==ref.stderr,(r.stderr,ref.stderr)
 start=12+sum(map(len,maps))
 badmaps=[b'',joined[:7],joined[:11],joined[:8]+u32(0)+joined[12:],joined[:8]+u32(65)+joined[12:],
          joined[:8]+u32(3)+joined[12:],joined[:12]+u32(0)+joined[16:],
          joined[:start+1]+u32(2)+joined[start+5:],joined[:start+5]+u32(2**31)+joined[start+9:],joined[:-2]]
 for data in badmaps:
  dest.write_bytes(data);r=run([p/'run',p/'parse.net',dest,files[0]])
  assert r.returncode==1 and not r.stdout,(r.returncode,r.stderr)
 badframes=[b'',b'\1',u32(0),frames[0][:-1],b''.join(frames)+b'\1',u32(5)+u32(0)+b'x',frames[0]*65]
 for data in badframes:
  framed.write_bytes(data);r=run([p/'run',p/'units.net',framed])
  assert r.returncode==1 and not r.stdout,(r.returncode,r.stderr)
 print('located units: independent serialized bytes, UTF-8 filenames, reference tape/diagnostics; 10 map and 7 input-frame rejects')

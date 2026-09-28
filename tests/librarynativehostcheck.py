#!/usr/bin/env python3
import os,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
u=lambda n:struct.pack('<Q',n)
def desc(kind,width,align,depth=0,tag=0,payload=b''):
 return b''.join(u(n)for n in(depth,0,0,kind,width,0,align))+bytes([tag])+u(len(payload))+payload
I=desc(1,4,4);D=desc(3,8,8);F=desc(3,4,4);P=desc(2,8,8,1)
PAIR=desc(5,16,8,tag=1,payload=u(2)+u(0)+u(0)+u(0)+u(8)+D+u(8)+u(0)+u(0)+u(4)+I)
def signature(result,args):return b'USLSIG2\n'+u(1)+u(8)+b'exchange'+bytes([0,1,0,1])+u(len(args))+result+u(len(args))+b''.join(args)+bytes([1])
def run(cmd,env=None):subprocess.run([str(ROOT/'tests/bound'),'20',*map(str,cmd)],check=True,env=env)
with tempfile.TemporaryDirectory(prefix='r10-nativehost-')as tmp:
 t=Path(tmp);exe=t/'probe'
 for sanitize in (False,True):
  run([os.environ.get('CC','cc'),'-std=c99','-I',ROOT,'-Wall','-Wextra','-Wno-unused-function',ROOT/'tests/librarynativehostcheck.c','-lffi','-o',exe,*(['-fsanitize=address,undefined','-g']if sanitize else[])])
  for name,result,args in [('pair',PAIR,[I,D,F,P,PAIR,I,D,I,I]),('fp',D,[D]*9),('int17',I,[I]*17)]:
   file=t/(name+'.sig');decl=signature(result,args);file.write_bytes(decl);wire=t/'wire';run([exe,file,name],dict(os.environ,US_NATIVE_WIRE=str(wire)))
   data=wire.read_bytes();assert data[:8]==b'USBIND3\n';at=8
   def q():
    global at
    v=struct.unpack_from('<Q',data,at)[0];at+=8;return v
   count=q();assert count==2
   seen=set()
   for _ in range(count):
    size=q();end=at+size;n=q();nm=data[at:at+n];at+=n;seen.add(nm)
    kind,origin=data[at:at+2];at+=2;ordinal=q();abi,var=data[at:at+2];at+=2;addr=q();argc=q();fmt=data[at];at+=1
    assert (kind,origin,ordinal,abi,var)==(0,0,0,0,0) and addr
    if fmt==1:
     dispatcher=q();handle=q();length=q();assert dispatcher==123 and handle and argc==len(args) and data[at:at+length]==decl;at+=length
    else:
     assert fmt==0 and nm==b'legacy' and argc==0;at+=48
    assert data[at]==1;at+=1;assert at==end
   assert at==len(data) and seen=={b'exchange',b'legacy'}
print('native typed host: normal and ASan/UBSan passed')

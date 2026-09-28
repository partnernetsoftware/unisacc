#!/usr/bin/env python3
"""Own synthetic V2 declarations and native libffi closure oracle; no shared builds."""
import os
from pathlib import Path
import struct
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
def u(n): return struct.pack('<Q',n)
def desc(kind,width,align,uns=0,depth=0,tag=0,payload=b''):
    return b''.join(u(x) for x in (depth,0,0,kind,width,uns,align))+bytes([tag])+u(len(payload))+payload
I=desc(1,4,4); D=desc(3,8,8); F=desc(3,4,4); P=desc(2,8,8,depth=1)
pair_payload=u(2)+u(0)+u(0)+u(0)+u(8)+D+u(8)+u(0)+u(0)+u(4)+I
PAIR=desc(5,16,8,tag=1,payload=pair_payload)
args=[I,D,F,P,PAIR,I,D,I,I]
def pack(result=PAIR,arglist=args,support=1):
    name=b'exchange';return b'USLSIG2\n'+u(1)+u(len(name))+name+bytes([0,1,0,1])+u(len(arglist))+result+u(len(arglist))+b''.join(arglist)+bytes([support])
def run(cmd,**kw):
    return subprocess.run([str(ROOT/'tests/bound'), '20',*map(str,cmd)],check=True,**kw)
with tempfile.TemporaryDirectory(prefix='r10-sig2host-') as tmp:
    t=Path(tmp);valid=t/'valid.sig';valid.write_bytes(pack());exe=t/'probe'
    flags=['-std=c99','-I',ROOT,'-Wall','-Wextra','-Wno-unused-function']
    if os.uname().sysname=='Darwin': flags+=['-lffi']
    else: flags+=['-lffi']
    for sanitize in (False,True):
        run([os.environ.get('CC','cc'),ROOT/'tests/librarysig2hostcheck.c','-o',exe,*flags,*(['-fsanitize=address,undefined','-g'] if sanitize else [])])
        run([exe,valid])
        many=t/'many.sig';many.write_bytes(pack(I,[I]*1024));run([exe,many,'many'])
        old=t/'v1.sig';old.write_bytes(b'USLSIG1\n'+u(1)+u(8)+b'exchange'+bytes([0,1,0])+u(1)+I[:48]+u(1)+I[:48]+bytes([1]))
        run([exe,old,'many']);run([exe,old,'legacy'])
        array=desc(5,8,4,tag=3,payload=u(2)+u(4)+I)
        nested=desc(5,16,8,tag=1,payload=u(2)+u(0)+u(0)+u(0)+u(8)+D+u(8)+u(0)+u(0)+u(8)+array)
        arr=t/'array.sig';arr.write_bytes(pack(nested,[array]));run([exe,arr,'decode'])
        bad_layout=desc(5,16,8,tag=1,payload=u(2)+u(0)+u(0)+u(0)+u(8)+D+u(9)+u(0)+u(0)+u(4)+I)
        malformed=[pack(bad_layout),pack(desc(3,8,4)),pack(PAIR,[I]*1025),pack(desc(5,16,8,tag=4,payload=b'x')),
                   pack(desc(5,16,8,tag=3,payload=u(2)+u(8)+I))]
        for index,data in enumerate(malformed):
            f=t/f'bad{index}.sig';f.write_bytes(data)
            p=subprocess.run([str(ROOT/'tests/bound'),'20',str(exe),str(f)],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
            assert p.returncode==1,(index,p.returncode,p.stderr)
            assert b'malformed library signature declaration' in p.stderr,(index,p.stderr)
        print('sig2 malformed layout/extent/parameters/payload controls rejected')
    print('sig2 host: native + ASan/UBSan passed')

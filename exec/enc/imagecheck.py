#!/usr/bin/env python3
"""Verify delta ELF output. Reference assembly/image writing is test-only.
No Linux execution is claimed on a macOS host. Synthetic relocation/layout
assertions are independent of the reference writer; both executors run them.
"""
import os
import pathlib
import struct
import subprocess
import sys
import tempfile
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[2]))
from unisa.__main__ import _oracle
from unisa.driver import compile_file
from unisa.lower import lower, TargetProgram
from unisa.assemble import assemble
from unisa import image
from unisa.tape import DATA_BASE
from tins import dump


def main():
    if len(sys.argv) not in (4,5) or (len(sys.argv)==5 and sys.argv[4]!='arm64'):
        raise SystemExit('usage: imagecheck.py RUN DELTA.tbl DELTA.json [arm64]')
    arch='arm64' if len(sys.argv)==5 else 'x86_64'
    target='lnx/'+arch
    with tempfile.TemporaryDirectory() as d:
        p=pathlib.Path(d)/'in.txt'
        def verify(tp, name, both=False, sparse=False):
            text=dump(tp,full=True)
            if sparse:
                # Oracle retains full data; only the tested declaration omits zeros.
                tp.data_len=len(tp.data)
                text=dump(tp,full=True)
                data_line = '@data ' + tp.data.hex()
                assert text.splitlines().count(data_line) == 1, 'expected exactly one data line'
                text=text.replace(data_line + '\n', '@data '+(tp.data.rstrip(b'\x00').hex() or '-') + '\n', 1)
            p.write_text(text)
            code,st=assemble(tp)
            assert st['encoded']==st['insns']
            want=image.build(tp,code,image.relocate(tp,tp.data,st['data_va']-DATA_BASE),st['entry'])
            commands=[[sys.argv[1],sys.argv[2],str(p)]]
            if both: commands.append([sys.executable,'exec/pp/sim.py',sys.argv[3],str(p)])
            for cmd in commands:
                r=subprocess.run(cmd,capture_output=True,timeout=60)
                if r.returncode or r.stdout!=want:
                    raise RuntimeError('%s rc %d bytes %d/%d %r' % (name,r.returncode,len(r.stdout),len(want),r.stderr))
            print('ELF',name,len(want),'bytes equal; executors',len(commands),flush=True)
            return want
        for length in (16,8192):
            tp=TargetProgram(target,(DATA_BASE+8).to_bytes(8,'little')+b'q'+bytes(length-9),{'item':DATA_BASE+8})
            tp.relocs=[0];tp.labels={'_start':1,'end':3}
            tp.emit('nop');tp.emit('jump','end',reloc='arm26' if arch=='arm64' else 'rel32');tp.emit('.lea','x0' if arch=='arm64' else 'rax','item');tp.emit('ret')
            b=verify(tp,'relocated-pointer-'+str(length),True)
            entry=struct.unpack_from('<Q',b,24)[0]
            rx=struct.unpack_from('<IIQQQQQQ',b,64)
            rw=struct.unpack_from('<IIQQQQQQ',b,120)
            assert entry==0x400000+176+(4 if arch=="arm64" else 1)
            assert struct.unpack_from("<H",b,18)[0]==(183 if arch=="arm64" else 62)
            tend=204 if arch=="arm64" else 187
            assert rx==(1,5,0,0x400000,0x400000,tend,tend,4096)
            assert rw==(1,6,4096,0x401000,0x401000,9,length,4096)
            assert len(b)==4105 and b[4096:4104]==(0x401008).to_bytes(8,'little') and b[-1:]==b'q'
        tp=TargetProgram(target,b'x'+bytes(63),{})
        tp.emit('ret');tp.relocs=[16]
        verify(tp,'relocation-in-omitted-zero-tail',True,sparse=True)
        for at,name in ((16,'straddling-stored-tail'),(56,'last-valid-relocation')):
            raw=bytearray(64);raw[at:at+8]=(DATA_BASE+8).to_bytes(8,'little')
            tp=TargetProgram(target,bytes(raw),{});tp.relocs=[at];tp.emit('ret')
            verify(tp,name,True,sparse=True)
        # Real lowering, no op filtering, including stdio. All image bytes match.
        oracle=_oracle('built')
        for f in ('examples/hello.c','examples/fib.c'):
            tp=lower(compile_file([f],oracle,target),target,oracle,drive='built')
            result=verify(tp,f)
            if os.environ.get('ELF_KEEP'):
                keep=pathlib.Path(os.environ['ELF_KEEP']);keep.mkdir(parents=True,exist_ok=True)
                (keep/(pathlib.Path(f).stem+'.elf')).write_bytes(result)
        # An out-of-data relocation must fail, not read/write a default zero cell.
        tp=TargetProgram(target,b'12345678',{});tp.relocs=[1];tp.emit('ret')
        p.write_text(dump(tp,full=True))
        for cmd in ([sys.argv[1],sys.argv[2],str(p)],[sys.executable,'exec/pp/sim.py',sys.argv[3],str(p)]):
            r=subprocess.run(cmd,capture_output=True,timeout=60)
            assert r.returncode==1 and not r.stdout and b'ELF input or relocation' in r.stderr
        p.write_text('@target '+target+'\n@data -\n@data_len 64\n@relocs 57\nret\n')
        for cmd in ([sys.argv[1],sys.argv[2],str(p)],[sys.executable,'exec/pp/sim.py',sys.argv[3],str(p)]):
            r=subprocess.run(cmd,capture_output=True,timeout=60)
            assert r.returncode==1 and not r.stdout and b'ELF input or relocation' in r.stderr
        for size in ('7','-1','2147483648','abc'):
            p.write_text('@target '+target+'\n@data 3132333435363738\n@data_len '+size+'\nret\n')
            for cmd in ([sys.argv[1],sys.argv[2],str(p)],[sys.executable,'exec/pp/sim.py',sys.argv[3],str(p)]):
                r=subprocess.run(cmd,capture_output=True,timeout=60)
                assert r.returncode==1 and not r.stdout and b'ELF input or relocation' in r.stderr
        for value in ('2147483648','18446744073709551616','999999999999999999999999999999999999'):
            p.write_text('@target '+target+'\n@data -\n@data_len 64\n@relocs '+value+'\nret\n')
            for cmd in ([sys.argv[1],sys.argv[2],str(p)],[sys.executable,'exec/pp/sim.py',sys.argv[3],str(p)]):
                r=subprocess.run(cmd,capture_output=True,timeout=60)
                assert r.returncode==1 and not r.stdout and b'ELF input or relocation' in r.stderr
        print('ELF relocation/extent bounds: both reject; no Linux execution on this host claimed',flush=True)

if __name__=='__main__':main()

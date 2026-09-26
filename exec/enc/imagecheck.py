#!/usr/bin/env python3
"""Verify delta ELF output. Reference assembly/image writing is test-only.
No Linux execution is claimed on a macOS host. Synthetic relocation/layout
assertions are independent of the reference writer; both executors run them.
"""
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
    if len(sys.argv)!=4:
        raise SystemExit('usage: imagecheck.py RUN DELTA.tbl DELTA.json')
    with tempfile.TemporaryDirectory() as d:
        p=pathlib.Path(d)/'in.txt'
        def verify(tp, name, both=False, sparse=False):
            text=dump(tp,full=True)
            if sparse:
                # Oracle retains full data; only the tested declaration omits zeros.
                tp.data_len=len(tp.data)
                text=dump(tp,full=True)
                text=text.replace('@data '+tp.data.hex(), '@data '+(tp.data.rstrip(b'\x00').hex() or '-'))
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
            tp=TargetProgram('lnx/x86_64',(DATA_BASE+8).to_bytes(8,'little')+b'q'+bytes(length-9),{'item':DATA_BASE+8})
            tp.relocs=[0];tp.labels={'_start':1,'end':3}
            tp.emit('nop');tp.emit('jump','end',reloc='rel32');tp.emit('.lea','rax','item');tp.emit('ret')
            b=verify(tp,'relocated-pointer-'+str(length),True)
            entry=struct.unpack_from('<Q',b,24)[0]
            rx=struct.unpack_from('<IIQQQQQQ',b,64)
            rw=struct.unpack_from('<IIQQQQQQ',b,120)
            assert entry==0x400000+176+1
            assert rx==(1,5,0,0x400000,0x400000,187,187,4096)
            assert rw==(1,6,4096,0x401000,0x401000,9,length,4096)
            assert len(b)==4105 and b[4096:4104]==(0x401008).to_bytes(8,'little') and b[-1:]==b'q'
        tp=TargetProgram('lnx/x86_64',b'x'+bytes(63),{})
        tp.emit('ret');tp.relocs=[16]
        verify(tp,'relocation-in-omitted-zero-tail',True,sparse=True)
        for at,name in ((16,'straddling-stored-tail'),(56,'last-valid-relocation')):
            raw=bytearray(64);raw[at:at+8]=(DATA_BASE+8).to_bytes(8,'little')
            tp=TargetProgram('lnx/x86_64',bytes(raw),{});tp.relocs=[at];tp.emit('ret')
            verify(tp,name,True,sparse=True)
        # Real lowering, no op filtering, including stdio. All image bytes match.
        oracle=_oracle('built')
        for f in ('examples/hello.c','examples/fib.c'):
            tp=lower(compile_file([f],oracle,'lnx/x86_64'),'lnx/x86_64',oracle,drive='built')
            verify(tp,f)
        # An out-of-data relocation must fail, not read/write a default zero cell.
        tp=TargetProgram('lnx/x86_64',b'12345678',{});tp.relocs=[1];tp.emit('ret')
        p.write_text(dump(tp,full=True))
        for cmd in ([sys.argv[1],sys.argv[2],str(p)],[sys.executable,'exec/pp/sim.py',sys.argv[3],str(p)]):
            r=subprocess.run(cmd,capture_output=True,timeout=60)
            assert r.returncode==1 and not r.stdout and b'ELF input or relocation' in r.stderr
        p.write_text('@target lnx/x86_64\n@data -\n@data_len 64\n@relocs 57\nret\n')
        for cmd in ([sys.argv[1],sys.argv[2],str(p)],[sys.executable,'exec/pp/sim.py',sys.argv[3],str(p)]):
            r=subprocess.run(cmd,capture_output=True,timeout=60)
            assert r.returncode==1 and not r.stdout and b'ELF input or relocation' in r.stderr
        for size in ('7','-1','2147483648','abc'):
            p.write_text('@target lnx/x86_64\n@data 3132333435363738\n@data_len '+size+'\nret\n')
            for cmd in ([sys.argv[1],sys.argv[2],str(p)],[sys.executable,'exec/pp/sim.py',sys.argv[3],str(p)]):
                r=subprocess.run(cmd,capture_output=True,timeout=60)
                assert r.returncode==1 and not r.stdout and b'ELF input or relocation' in r.stderr
        print('ELF relocation/extent bounds: both reject; no Linux execution on this host claimed',flush=True)

if __name__=='__main__':main()

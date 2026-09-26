"""Full payload declarations -> code; both runtimes, page crossings and rejects."""
import pathlib
import subprocess
import sys
import tempfile
from tins import parse
from unisa.assemble import assemble


def check(args,native):
    cmds=[[args[0],args[1]],[sys.executable,'exec/pp/sim.py',args[2]]]
    with tempfile.TemporaryDirectory() as tmp:
        f=pathlib.Path(tmp)/'input'
        for target in ('lnx/arm64','osx/arm64','win/arm64'):
            for pad in (0,1025):
                source='@target '+target+'\n@data 7879000000000000\n@data_len 8192\n@bss 0\n@relocs -\n@sym value 256\n@sym far 4352\nstart:\nvalue:\n'
                source+='setreg x0, addr 256\nsetreg x1, mem 256\nsetmem 4352, x2\nargsave 264, 272, true\nargsave 264, 272, false\nargvget x0, x1, 280\n.lea x3, far\n.lea x6, value\n'
                source+='nop\n'*pad+'.lea x4, start\n.lea x5, end\nend:\n'
                source+='spinit x7, 4352\nwinsave 256\nwinrest 256, x0\nwinrest 264, x8\n'
                if target=='win/arm64':source+='winstdh 256\n'
                tp=parse(source);want,stats=assemble(tp);assert stats['encoded']==stats['insns']
                f.write_text(source)
                for cmd in cmds:
                    r=subprocess.run(cmd+[str(f)],capture_output=True,timeout=60)
                    assert r.returncode==0 and r.stdout==want,(target,pad,r.returncode,r.stderr)
                    w0=int.from_bytes(r.stdout[:4],'little');w1=int.from_bytes(r.stdout[4:8],'little')
                    pages=((w0>>29)&3)|(((w0>>5)&0x7ffff)<<2)
                    if pages&(1<<20):pages-=1<<21
                    decoded=(stats['text_va']&~4095)+pages*4096+((w1>>10)&4095)
                    assert decoded==stats['data_va'],(target,pad,decoded,stats['data_va'])
        print('ARM64 address layout: three OS layouts, page crossings, data/code/end names; both executors')
        bad=['@target bad/arm64\nret','@target lnx/arm64\n@target osx/arm64\nret',
             '@sym a 256\n@sym a 256\nret','@sym a 2147483648\nret','@sym a -1\nret','@sym a \nret',
             '.lea x0, absent','setmem 256, x17','argvget x0, x17, 256','setreg x0, addr 2147483648',
             'label:\n@target lnx/arm64\nret','nop\n@data -\nret','@unknown value\nret']
        for source in bad:
            f.write_text(source+'\n')
            for cmd in cmds:
                r=subprocess.run(cmd+[str(f)],capture_output=True,timeout=60)
                assert r.returncode==1 and not r.stdout and b'not covered' in r.stderr,(source,r.returncode,r.stderr)
        print('ARM64 payload/address: 13 invalid-target/symbol/header/scratch cases reject on both')

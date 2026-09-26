"""TIns tagged operands and metadata; syscall words checked, not executed."""
import pathlib
import subprocess
import sys
import tempfile
from tins import parse
from unisa.assemble import assemble


def check(args,native):
    with tempfile.TemporaryDirectory() as tmp:
        d=pathlib.Path(tmp);f=d/'in';cmds=[[args[0],args[1]],[sys.executable,'exec/pp/sim.py',args[2]]]
        cases=[
            'setreg x0, imm 18446744073709551615 role=arg0\nsetreg x1, reg x2 role=arg1\nspinit x7\n',
            'gate form=svc gate=svc0 carry=false catop=write sysno=64 ret=x0 retconv=none winimp=none winapi=WriteFile scr0=33352 scr1=33360 written=33544 hstd=33520\n',
            'gate gate=svc80 carry=true form=svc\n',
            'jumpz x0, end reloc=arm19 form=arm\ncall end reloc=arm26 form=arm\njump end reloc=arm26\nend:\n',
            'mov x0, x1 role=arg0 form=arm\nmov x0, x1 role=arg1 form=arm\n',
        ]
        for source in cases:
            tp=parse(source,target='lnx/arm64');want,st=assemble(tp);assert st['encoded']==st['insns']
            f.write_text(source)
            for cmd in cmds:
                r=subprocess.run(cmd+[str(f)],capture_output=True,timeout=60)
                assert r.returncode==0 and r.stdout==want,(source,r.returncode,r.stderr,r.stdout.hex(),want.hex())
        for source,expected in [('gate gate=svc0\n','010000d4'),('gate gate=svc80 carry=true\n','011000d443000054e00300cb'),('spinit x7\n','e7030091')]:
            f.write_text(source);r=subprocess.run(cmds[0]+[str(f)],capture_output=True,timeout=60)
            assert r.returncode==0 and r.stdout.hex()==expected
        bad=['setreg x0, mem x1','setreg x0, addr x1','setreg x0, reg 5','setreg x0, imm x1','setreg x0, 5',
             'spinit x7, 256','gate gate=winapi','gate gate=svc0 carry=1','gate form=winapi',
             'mov x0, x1 carry=true','mov x0, x1 role=a role=b','jump x reloc=arm19\nx:',
             'jumpz x0, x reloc=arm26\nx:','jump x reloc=arm26 reloc=arm26\nx:',
             'gate gate=svc0 gate=svc0','gate unknown=true','gate carry=']
        for source in bad:
            f.write_text(source+'\n')
            for cmd in cmds:
                r=subprocess.run(cmd+[str(f)],capture_output=True,timeout=60)
                assert r.returncode==1 and not r.stdout and b'not covered' in r.stderr,(source,r.returncode,r.stderr)
        print('ARM64 input: 5 whole fixtures, 3 worked setup/syscall encodings, 17 rejects; both runtimes')

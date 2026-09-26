"""FP byte checks on both runtimes, shared native C numerical referee."""
import pathlib
import subprocess
import sys
import tempfile
from tins import parse
from unisa.emit_arm import FP_OPS, FARITH, FCMP_INV, encode


def check(args,native):
    def run(cmd):return subprocess.run(cmd,capture_output=True,timeout=60)
    with tempfile.TemporaryDirectory() as tmp:
        f=pathlib.Path(tmp)/'input';lines=[]
        for op in FP_OPS:
            binary=op[:4] in FARITH or op[:3] in FCMP_INV
            for regs in ((0,1,2),(0,0,0),(16,17,30)):
                lines.append(op+' x%d, x%d'%regs[:2]+(', x%d'%regs[2] if binary else ''))
        f.write_text('\n'.join(lines)+'\n');tp=parse(f.read_text());parts=[encode(i,0,{}) for i in tp.code];assert parts and all(p is not None for p in parts)
        want=b''.join(parts)
        for cmd in ([args[0],args[1]],[sys.executable,'exec/pp/sim.py',args[2]]):
            r=run(cmd+[str(f)]);assert r.returncode==0 and r.stdout==want,(r.returncode,r.stderr)
        print('ARM64 FP:',len(FP_OPS),'ops,',len(lines),'register fixtures,',len(want),'bytes match on both executors')
        if not native:
            print('ARM64 FP native: SKIPPED');return
        r=run([sys.executable,'exec/enc/fpcheck.py',args[0],args[1],'arm64'])
        assert r.returncode==0,(r.returncode,r.stdout,r.stderr)
        print(r.stdout.decode().strip())

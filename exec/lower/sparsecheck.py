#!/usr/bin/env python3
"""Large virtual zero region: arithmetic expectations, no large reference array."""
import pathlib, struct, subprocess, sys, tempfile
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[2]))
from exec.enc.tins import parse

if len(sys.argv)!=6:raise SystemExit('RUN LOWER.tbl LOWER.json ELF.tbl ELF.json')
run,lt,lj,et,ej=sys.argv[1:]
with tempfile.TemporaryDirectory() as d:
    p=pathlib.Path(d)/'raw'; q=pathlib.Path(d)/'lower'
    p.write_text('.bss huge 610000000\n.str text "x\\x00"\n.bss tail 17\n.str final ""\n_start:\n  ret\n')
    outputs=[]
    for cmd in ([run,lt,str(p)],[sys.executable,'exec/pp/sim.py',lj,str(p)]):
        r=subprocess.run(cmd,capture_output=True,timeout=60)
        assert r.returncode==0,(r.returncode,r.stderr)
        outputs.append(r.stdout);tp=parse(r.stdout.decode())
        assert tp.data==b'x' and tp.data_len==610000200 and tp.bss==0
        assert tp.syms=={'huge':264,'text':256,'tail':610000264,'final':610000281},tp.syms
        assert len(r.stdout)<2000
    assert outputs[0]==outputs[1];q.write_bytes(outputs[0])
    images=[]
    for cmd in ([run,et,str(q)],[sys.executable,'exec/pp/sim.py',ej,str(q)]):
        r=subprocess.run(cmd,capture_output=True,timeout=60)
        assert r.returncode==0,(r.returncode,r.stderr)
        rw=struct.unpack_from('<IIQQQQQQ',r.stdout,120)
        assert rw==(1,6,4096,0x401000,0x401000,1,610000200,4096),rw
        assert len(r.stdout)==4097 and r.stdout[-1:]==b'x'
        images.append(r.stdout)
    assert images[0]==images[1]
    for text in ('.bss a 2147483648\nret\n', '.bss a 2000000000\n.bss b 1000000000\nret\n'):
        p.write_text(text)
        for cmd in ([run,lt,str(p)],[sys.executable,'exec/pp/sim.py',lj,str(p)]):
            r=subprocess.run(cmd,capture_output=True,timeout=60)
            assert r.returncode==1 and not r.stdout and b'tape data directive' in r.stderr
    print('sparse data: 610000200 virtual bytes, one stored byte, 4097-byte ELF; both executors; extent overflow rejected')

"""Independent ELF symbol-plan fixture, exercising real encoder headers."""
import importlib.util
import struct
import json
import subprocess
import tempfile
import sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/enc')]
from exec.pp.sim import run
with tempfile.TemporaryDirectory() as _d:
    subprocess.run([sys.executable,ROOT/'exec/build/gen.py','enc',Path(_d)/'m.json','--object'],check=True,stderr=subprocess.DEVNULL)
    model=json.loads((Path(_d)/'m.json').read_text())
text=(b'@target lnx/x86_64\n@data 610000\n@data_len 24\n@bss 0\n'
      b'@sym g_export 256\n@sym local 272\n@obj_nzend 3\n'
      b'@obj_name local 0 0\n@obj_name g_export 1 0\n@obj_name _start 0 0\n'
      b'@obj_tape 7265740a\n_start:\nret\n')
def parse(data):
    status,out,steps=run(model,data,'objectplan',maxsteps=1000000)
    assert status=='accept',(status,out)
    shoff=struct.unpack_from('<Q',out,40)[0]
    rows=[struct.unpack_from('<IIQQQQIIQQ',out,shoff+64*i) for i in range(9)]
    syms=[struct.unpack_from('<IBBHQQ',out,rows[5][4]+24*i) for i in range(rows[5][5]//24)]
    names=out[rows[6][4]:rows[6][4]+rows[6][5]]
    assert names==b'\0local\0export\0_start\0',names
    assert syms[4][1:5]==(0,0,3,13),syms[4]
    assert syms[5][1:5]==(17,0,2,0),syms[5]
    assert syms[6][1:5]==(18,0,1,0),syms[6]
    assert rows[5][7]==5,rows[5]
    assert out[rows[8][4]:rows[8][4]+rows[8][5]]==b'UNISATAPE1 lnx/x86_64 4\nret\n'
parse(text)
status,notape,_=run(model,text.replace(b'@obj_nzend',b'@obj_unit 0\n@obj_nzend'),'objectplan-no-tape',maxsteps=1000000)
assert status=='accept',(status,notape)
assert struct.unpack_from('<H',notape,60)[0]==8
assert b'UNISATAPE1' not in notape
with tempfile.TemporaryDirectory(prefix='objectplan-check-') as tmp:
    tmp=Path(tmp);modelpath=tmp/'model.json';table=tmp/'model.tbl';net=tmp/'model.net';runtime=tmp/'run';source=tmp/'input'
    modelpath.write_text(json.dumps(model));source.write_bytes(text)
    def cmd(args):
        q=subprocess.run(args,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30)
        assert q.returncode==0,(args,q.returncode,q.stderr)
        return q.stdout
    cmd(['cc','-O2','-std=c99','-w','-o',str(runtime),str(ROOT/'exec/c/run.c')])
    cmd([sys.executable,str(ROOT/'exec/c/tbl.py'),str(modelpath),str(table)])
    cmd([sys.executable,str(ROOT/'exec/c/net.py'),str(table),str(net)])
    cmd([str(runtime),'--check-net',str(table),str(net)])
    expected=run(model,text,'objectplan',maxsteps=1000000)[1]
    for path in (table,net):assert cmd([str(runtime),str(path),str(source)])==expected
for bad in [text.replace(b'@obj_nzend 3\n',b''), text.replace(b'@obj_tape 7265740a',b'@obj_tape 7'),text.replace(b'@obj_name local 0 0\n',b'@obj_name local 0 0\n@obj_name local 0 0\n'),text.replace(b'local 0 0',b'local 2 0')]:
    status,out,_=run(model,bad,'bad-objectplan',maxsteps=1000000)
    assert status=='reject',(status,out)
print('object plan: ELF ordered symbols, linkage names, BSS offsets, raw tape; 4 rejects')

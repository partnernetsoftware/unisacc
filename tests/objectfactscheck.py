#!/usr/bin/env python3
"""Independent object-fact assertions on simulator and constructed network."""
import json,pathlib,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from exec.pp.sim import run
CASES=[
 (b'.global f\n.extern g_x\nf:\n.lea r0,g_x\nret\n.str s "a\\0"\n.bss z 4\n',
  [b'f 1 0',b'g_x 0 1',b's 0 0',b'z 0 0'],8),
 (b'jump later\n.global later\nlater:\nret\n.str empty "\\0"\n.str tail "b\\0"\n',
  [b'later 1 0',b'empty 0 0',b'tail 0 0'],3),
 (b'.bss z 8\n.str s "x\\0"\n.str s "ignored"\n.bss z 8\nret\n',
  [b'z 0 0',b's 0 0'],2),
]
def main():
 with tempfile.TemporaryDirectory(prefix='object-facts-') as name:
  t=pathlib.Path(name)
  def cmd(args):
   r=subprocess.run(list(map(str,args)),capture_output=True,timeout=30);assert r.returncode==0,(args,r.stderr[-2000:]);return r.stdout
  model=t/'m.json';tbl=t/'m.tbl';net=t/'m.net';runtime=t/'run';src=t/'tape'
  cmd([sys.executable,ROOT/'exec/lower/gen.py',model,'--object'])
  d=json.loads(model.read_bytes())
  cmd(['cc','-O2','-o',runtime,ROOT/'exec/c/run.c'])
  cmd([sys.executable,ROOT/'exec/c/tbl.py',model,tbl]);cmd([sys.executable,ROOT/'exec/c/net.py',tbl,net]);cmd([runtime,'--check-net',tbl,net])
  for raw,names,nzend in CASES:
   src.write_bytes(raw);status,out,_=run(d,raw,'probe',maxsteps=100000)
   assert status=='accept',(status,out)
   assert out==cmd([runtime,net,src])
   lines=out.splitlines()
   assert [x[len(b'@obj_name '):] for x in lines if x.startswith(b'@obj_name ')]==names,out
   assert b'@obj_nzend '+str(nzend).encode() in lines,out
   assert b'@obj_tape '+raw.hex().encode() in lines,out
   assert b'.global ' not in out and b'.extern ' not in out
  print('object facts: 3 independent encounter/linkage/data-boundary/raw-tape cases; simulator/network equal')
if __name__=='__main__':main()

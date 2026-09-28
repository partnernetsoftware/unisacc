#!/usr/bin/env python3
"""Mapping callback routing: table/network, source spill ABI, return register.
Does not claim actual host mapping execution.
Usage: librarymemorycheck.py RUN JSON TARGET
"""
import json,pathlib,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/c')]
from exec.pp.sim import run as simulate
from pack import build
from unisa.lower import SYSA
from unisa.catalog import REGMAP

def command(args):
 p=subprocess.run(args,capture_output=True,timeout=55)
 if p.returncode:raise RuntimeError((args,p.returncode,p.stderr.decode()))
 return p.stdout
class Resources:
 def __init__(self,op,value):self.op=op;self.value=value
 def get(self,key):
  if isinstance(self.value,dict):return self.value.get(key)
  return self.value if key==('\0library/'+self.op).encode() else None
def main():
 if len(sys.argv)!=4:raise SystemExit(__doc__)
 runtime,model,target=sys.argv[1:];d=json.loads(pathlib.Path(model).read_text());reg=REGMAP[target.split('/')[1]]
 with tempfile.TemporaryDirectory(prefix='unisa-librarymemory-') as name:
  t=pathlib.Path(name);tbl=t/'m.tbl';net=t/'m.net';source=t/'input';pkg=t/'m.pkg'
  command([sys.executable,'exec/c/tbl.py',model,str(tbl)]);command([sys.executable,'exec/c/net.py',str(tbl),str(net)])
  command([runtime,'--check-net',str(tbl),str(net)])
  manifest=t/'routes.tsv';manifest.write_text('test\tlower\ttape\ttins\tm.net\n');resource=t/'resources';resource.mkdir();n=0
  cases=[('mmap',1),('munmap',1),('munmap',0),('mmap',0)]
  for op,mode in cases:
   raw=('_start:\n'+('.sys6 ' if mode else '.sys ')+op+', '+', '.join('r'+str(i) for i in range(6 if mode else 3))+'\n').encode();source.write_bytes(raw);previous=None
   for value in (None,bytes(8),(0x123456789abcd).to_bytes(8,'little'),b'bad'):
    for p in resource.iterdir():p.unlink()
    status,out,_=simulate(d,raw,'input',files=Resources(op,value),maxsteps=1000000)
    (resource/op).write_bytes(value or bytes(8));pkg.write_bytes(build([manifest],[] if value is None else [('006c6962726172792f',resource)]))
    got=subprocess.run([runtime,'--bundle',str(pkg),'test',str(source)],capture_output=True,timeout=55)
    supported=target.split('/')[0] in ('osx','lnx')
    valid=value in (None,bytes(8)) or (len(value)==8 and supported and (op=='munmap' or mode==1))
    assert (status=='accept')==valid,(target,op,mode,value,status)
    assert got.returncode==(0 if valid else 1),(target,op,mode,value,got.returncode,got.stderr)
    if valid:
     assert got.stdout==out,'table/network differ'
     if value is None:previous=out;assert command([runtime,str(tbl),str(source)])==out,'C table differs'
     elif value==bytes(8):assert out==previous,'zero resource changes ordinary path'
     else:
      text=out.decode();base=256+(SYSA if mode else 0)
      assert '320255973501901' in text and 'hostcall '+reg[1]+', '+reg[0] in text
      assert 'setreg '+reg[0]+', addr '+str(base)+'\n' in text
      assert 'mov '+reg[0]+', '+reg[0]+'\n' in text,'result no longer copied from host return'
      for i in range(6 if op=='mmap' else 2):assert 'setmem '+str(base+8*i)+', '+reg[i]+'\n' in text,'syscall operand spill differs'
      if op=='munmap':
       scratch='x16' if target.endswith('/arm64') else 'r11'
       assert 'setreg '+scratch+', imm 0\n' in text,'zero scratch not initialized'
       for i in range(2,6):assert 'setmem '+str(base+8*i)+', '+scratch+'\n' in text,'unused callback argument not zero register'
      assert 'gate ' not in text,'mapping still uses untracked process syscall'
    n+=1
  raw=b'_start:\n.sys6 mmap, r0, r1, r2, r3, r4, r5\n.sys munmap, r0, r1, r2\n.exit r0\n';source.write_bytes(raw)
  values={('\0library/'+op).encode():(0x123456789abcd+i).to_bytes(8,'little') for i,op in enumerate(('exit','mmap','munmap'))}
  for p in resource.iterdir():p.unlink()
  for key,value in values.items():(resource/key.decode().split('/')[-1]).write_bytes(value)
  status,out,_=simulate(d,raw,'input',files=Resources('',values),maxsteps=1000000)
  pkg.write_bytes(build([manifest],[('006c6962726172792f',resource)]))
  got=subprocess.run([runtime,'--bundle',str(pkg),'test',str(source)],capture_output=True,timeout=55)
  if target.split('/')[0] in ('osx','lnx'):
   assert status=='accept' and got.returncode==0 and got.stdout==out,'combined callback routes differ'
   assert out.count(b'hostcall ')==3 and b'gate ' not in out,'combined callback route lost'
   for value in values.values():assert str(int.from_bytes(value,'little')).encode() in out,'callback address lost'
  else:assert status=='reject' and got.returncode==1,'unsupported combined library route accepted'
  n+=1
  print('librarymemory:',target,n,'table/network/ABI/return verdicts; check-net full domain')
if __name__=='__main__':main()

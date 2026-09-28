#!/usr/bin/env python3
"""Complete mapping-callback TIns encode: table, network chain and byte oracle.
No native guest execution is claimed; native library harness checks that next.
Usage: librarymemorychaincheck.py RUN LOWER.json ENC.json TARGET
"""
import json,pathlib,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/c')]
from exec.pp.sim import run as simulate
from exec.enc.tins import parse
from unisa.assemble import assemble
from pack import build

def command(args):
 p=subprocess.run(args,capture_output=True,timeout=55)
 if p.returncode:raise RuntimeError((args,p.returncode,p.stderr.decode()))
 return p.stdout
class Resources:
 def __init__(self,values):self.values=values
 def get(self,key):return self.values.get(key)
def main():
 if len(sys.argv)!=5:raise SystemExit(__doc__)
 runtime,lower,encoder,target=sys.argv[1:];ld=json.loads(pathlib.Path(lower).read_text());ed=json.loads(pathlib.Path(encoder).read_text())
 with tempfile.TemporaryDirectory(prefix='unisa-librarymemchain-') as name:
  t=pathlib.Path(name);source=t/'input';pkg=t/'pkg';resource=t/'resources';resource.mkdir()
  for label,model in [('lower',lower),('enc',encoder)]:
   command([sys.executable,'exec/c/tbl.py',model,str(t/(label+'.tbl'))]);command([sys.executable,'exec/c/net.py',str(t/(label+'.tbl')),str(t/(label+'.net'))])
   command([runtime,'--check-net',str(t/(label+'.tbl')),str(t/(label+'.net'))])
  manifest=t/'routes.tsv';manifest.write_text('test\tlower\ttape\ttins\tlower.net\ntest\tenc\ttins\tbytes\tenc.net\nenc\tenc\ttins\tbytes\tenc.net\n')
  cases=['.sys6 mmap, r0, r1, r2, r3, r4, r5','.sys munmap, r0, r1, r2','.sys6 munmap, r0, r1, r2, r3, r4, r5','.exit r0']
  n=bad=0
  for enabled in (False,True):
   values={('\0library/'+op).encode():(0x123456789abcd+i).to_bytes(8,'little') for i,op in enumerate(('exit','mmap','munmap'))} if enabled else {}
   for p in resource.iterdir():p.unlink()
   for k,v in values.items():(resource/k.decode().split('/')[-1]).write_bytes(v)
   pkg.write_bytes(build([manifest],[('006c6962726172792f',resource)] if enabled else []))
   files=Resources(values)
   for op in cases:
    raw=('_start:\n'+op+'\n').encode();source.write_bytes(raw)
    status,tins,_=simulate(ld,raw,'input',files=files,maxsteps=1000000);assert status=='accept',(target,op,status)
    status,encoded,_=simulate(ed,tins,'input',files=files,maxsteps=2000000);assert status=='accept',(target,op,status,tins)
    got=command([runtime,'--bundle',str(pkg),'test',str(source)]);assert got==encoded,'lower/network encoder chain differs'
    tp=parse(tins.decode());ref,stats=assemble(tp);assert stats['encoded']==stats['insns'],'oracle left unencoded instructions'
    assert encoded==ref,'full TIns encoded bytes differ from reference'
    if enabled and 'munmap' in op:
     scratch='x16' if target.endswith('/arm64') else 'r11'
     assert 'setreg '+scratch+', imm 0\n' in tins.decode()
     line=next(x for x in tins.decode().splitlines(keepends=True) if x.startswith('setmem ') and x.endswith(', '+scratch+'\n'))
     broken=tins.replace(line.encode(),(line.rsplit(', ',1)[0]+', 0\n').encode(),1)
     status,_,_=simulate(ed,broken,'input',files=files,maxsteps=2000000);assert status=='reject','literal setmem source unexpectedly accepted'
     source.write_bytes(broken);p=subprocess.run([runtime,'--bundle',str(pkg),'enc',str(source)],capture_output=True,timeout=55)
     assert p.returncode==1,'network did not reject literal setmem source';bad+=1
    n+=1
  print('librarymemorychain:',target,n,'complete lower/encode/oracle cases;',bad,'literal-source controls rejected; full-domain check-net')
if __name__=='__main__':main()

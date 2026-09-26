"""Windows x86 setup: final RIP addresses after a shortening branch."""
import pathlib,subprocess,sys,tempfile
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[2]))
from tins import parse
from unisa.assemble import assemble
assert len(sys.argv)==4
source='''@target win/x86_64
@data -
@data_len 1024
_start:
jump setup reloc=rel32
nop
setup:
winstdh 256
winargs 280, 288, 320
winsave 832
winrest 832, rax
ret
'''
want,stats=assemble(parse(source));assert stats['encoded']==stats['insns'] and want[0]==0xeb
commands=[[sys.argv[1],sys.argv[2]],[sys.executable,'exec/pp/sim.py',sys.argv[3]]]
with tempfile.TemporaryDirectory() as d:
 f=pathlib.Path(d)/'in'
 f.write_text(source)
 for cmd in commands:
  r=subprocess.run(cmd+[str(f)],capture_output=True,timeout=60)
  at=next((i for i,(a,b) in enumerate(zip(r.stdout,want)) if a!=b),min(len(r.stdout),len(want)))
  assert r.returncode==0 and r.stdout==want,(cmd,r.returncode,r.stderr,at,len(r.stdout),len(want))
 print('Windows x86 setup:',len(want),'bytes equal after relaxation, both executors',flush=True)
 for src in (source.replace('832, rax','832, r8'),source.replace('win/x86_64','lnx/x86_64'),source.replace('winsave 832','winsave')):
  f.write_text(src)
  for cmd in commands:
   r=subprocess.run(cmd+[str(f)],capture_output=True,timeout=60)
   assert r.returncode==1 and not r.stdout and b'not covered' in r.stderr,(cmd,r.returncode,r.stderr)
 print('Windows x86 setup: invalid return register, target and arity rejected, both executors')

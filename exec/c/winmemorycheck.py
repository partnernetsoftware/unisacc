#!/usr/bin/env python3
"""Bound Windows memory bytes against the existing assembler, on both runtimes.
Fake process addresses are labelled data inputs, not native execution evidence.
"""
import json,pathlib,struct,subprocess,sys
root=pathlib.Path(__file__).resolve().parents[2]
sys.path[:0]=[str(root/'exec/pp'),str(root/'exec/c'),str(root/'exec/enc'),str(root)]
import sim
from pack import build
from tins import parse
from unisa import image,assemble
from unisa.image.pe import IMPORTS,DLL
p=pathlib.Path(sys.argv[1]);target=sys.argv[2];ua=sys.argv[3]
def run(cmd):return subprocess.run(list(map(str,cmd)),capture_output=True,timeout=60)
def ok(cmd):
 r=run(cmd);assert r.returncode==0,(r.args,r.returncode,r.stderr);return r.stdout
low=json.loads((p/'lower.json').read_text());enc=json.loads((p/'elf.json').read_text())
ll=sim.load(low);el=sim.load(enc)
resources=p/'winresources';resources.mkdir(exist_ok=True)
manifest=p/'winmemory.tsv';manifest.write_text('memory\tencode\ttarget.text\tmemory-v1\telf.net\nlower\tlower\ttape\ttarget.text\tlower.net\n')
tb,db=0x140000000,0x140200000
values={'process/argc':3,'process/argv':0x123456780,'memory/text':tb,'memory/data':db}
values.update({'process/import/'+DLL.decode().lower()+'/'+n:0x7fff00000000+i*16 for i,n in enumerate(IMPORTS)})
for k,v in values.items():
 q=resources/k;q.parent.mkdir(parents=True,exist_ok=True);q.write_bytes(struct.pack('<Q',v))
def context(missing=None):
 f=sim.Files();f.cache.update({b'\0'+k.encode():struct.pack('<Q',v) for k,v in values.items() if k!=missing});return f
for source in ['examples/hello.c','tests/c/b_funcptr.c']:
 tape=ok([ua,'-O2','-t',target,source])
 r,lowered,_=sim.run(low,tape,source,context(),maxsteps=50000000,loaded=ll);assert r=='accept',(r,lowered)
 r,mem,_=sim.run(enc,lowered,source,context(),maxsteps=50000000,loaded=el);assert r=='accept',(r,mem)
 lines=lowered.decode().splitlines();headers={x.split()[0]:x.split()[1] for x in lines if x.startswith(('@argc ','@argv '))}
 tp=parse('\n'.join(x for x in lines if not x.startswith(('@argc ','@argv ')))+'\n')
 oldlayout,oldimports=image.layout,image.imports
 image.layout=lambda *args:(tb,db)
 image.imports=lambda os,arch,n:{'__imp_'+name:tb+((n+7)&-8)+8*i for i,name in enumerate(IMPORTS)}
 try: code,stats=assemble.assemble(tp)
 finally:image.layout,image.imports=oldlayout,oldimports
 assert stats['encoded']==stats['insns']
 expected=code+bytes((-len(code))%8)+b''.join(struct.pack('<Q',values['process/import/'+DLL.decode().lower()+'/'+n]) for n in IMPORTS)
 assert mem[:8]==b'UNIMEM1\n';nt,extent,stored,entry=struct.unpack('<4Q',mem[8:40])
 assert (nt,extent,entry)==(len(expected),tp.data_len+tp.bss,stats['entry'])
 assert mem[40:40+nt]==expected,(source,'bound code/import slots')
 data=bytearray(tp.data)+bytes(tp.data_len-len(tp.data));data=bytearray(image.relocate(tp,data,db-256))
 for name in ['argc','argv']:
  at=int(headers['@'+name])-256;data[at:at+8]=struct.pack('<Q',values['process/'+name])
 assert mem[40+nt:]+bytes(extent-stored)==data+bytes(tp.bss)
 (p/'winlowered').write_bytes(lowered);(p/'winmemory.pkg').write_bytes(build([manifest],[('00',resources)]))
 (p/'wintape').write_bytes(tape)
 assert ok([p/'run','--bundle',p/'winmemory.pkg','lower',p/'wintape'])==lowered
 assert ok([p/'run','--bundle',p/'winmemory.pkg','memory',p/'winlowered'])==mem
 print(target,source,'bound code/imports/data/entry: assembler = action oracle = network',flush=True)
missing='process/import/'+DLL.decode().lower()+'/'+IMPORTS[0]
r,_,_=sim.run(enc,lowered,'missing-import',context(missing),maxsteps=50000000,loaded=el);assert r!='accept'
(resources/missing).unlink();(p/'winmemory.pkg').write_bytes(build([manifest],[('00',resources)]))
r=run([p/'run','--bundle',p/'winmemory.pkg','memory',p/'winlowered']);assert r.returncode==1 and not r.stdout
print(target,'missing process import refused by both executors',flush=True)

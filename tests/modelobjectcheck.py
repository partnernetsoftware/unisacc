#!/usr/bin/env python3
"""Real tape -> two ELF object deltas, byte-exact against the C reference."""
import os,pathlib,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/c')]
from pack import build
CASES={
 'function':'int twice(int x){return x*2;}\n',
 'global':'int count=3; static int hidden; int get(void){return count+hidden;}\n',
 'external':'extern int count; int twice(int); int get(void){return twice(count); }\n',
 'main':'int main(void){return 7;}\n',
 'pointer':'int f(void){return 3;} int (*p)(void)=f; int main(void){return p();}\n',
 'bytes':'char a[5]="x"; int z[3]; int main(void){return a[0]+z[2];}\n',
}
def main():
 with tempfile.TemporaryDirectory(prefix='model-object-') as name:
  t=pathlib.Path(name)
  def cmd(args):
   r=subprocess.run(list(map(str,args)),capture_output=True,timeout=30)
   assert r.returncode==0,(args,r.returncode,r.stderr[-2000:]);return r.stdout
  ref=t/'ref';runtime=t/'run'
  cmd([ROOT/'tests/build_ref.sh',t/'ref.c',ref]);cmd(['cc','-O2','-o',runtime,ROOT/'exec/c/run.c'])
  routes=t/'routes';rows=[]
  for arch in ['x86_64','arm64']:
   for stage,script,flags in [('prune','exec/prune/gen.py',[]),('lower','exec/lower/gen.py',['--full','--object']+(['--arm64'] if arch=='arm64' else [])),('elf','exec/enc/'+('gen.py' if arch=='x86_64' else 'arm.py'),['--object'])]:
    base=t/(arch+'-'+stage);model=base.with_suffix('.json');table=base.with_suffix('.tbl');net=base.with_suffix('.net')
    cmd([sys.executable,ROOT/script,model,*flags]);cmd([sys.executable,ROOT/'exec/c/tbl.py',model,table]);cmd([sys.executable,ROOT/'exec/c/net.py',table,net]);cmd([runtime,'--check-net',table,net])
    rows.append('\t'.join(['lnx/'+arch+'/object/O0',stage,'target.text' if stage=='elf' else 'tape.text','tape.text' if stage=='prune' else 'target.text' if stage=='lower' else 'object.bytes',str(net)]))
  routes.write_text('\n'.join(rows)+'\n');resources=t/'resources';resources.mkdir();(resources/'funit').write_bytes(b'1')
  packages=[]
  for unit in [False,True]:
   pkg=t/('unit.pkg' if unit else 'whole.pkg');pkg.write_bytes(build([routes],[('00636c692f',resources)] if unit else [],cache=False));packages.append(pkg)
  count=0
  for arch in ['x86_64','arm64']:
   for unit in [False,True]:
    for name,source in CASES.items():
     # A non-unit cannot leave declarations unresolved.
     if name=='external' and not unit:continue
     src=t/(name+'.c');src.write_text(source+('int main(void){return 0;}\n' if not unit and name in ('function','global') else ''));tape=t/(name+'.tape');expected=t/'expected.o'
     tape.write_bytes(cmd([ref,src,'-S',*(['-funit'] if unit else []),'-o','-']))
     cmd([ref,tape,'-c','-b','lnx/'+arch,*(['-funit'] if unit else []),'-o',expected])
     actual=cmd([runtime,'--bundle',packages[int(unit)],'lnx/'+arch+'/object/O0',tape])
     want=expected.read_bytes()
     if actual!=want and os.environ.get('UNISACC_OBJECT_EVIDENCE'):
      ev=pathlib.Path(os.environ['UNISACC_OBJECT_EVIDENCE']);ev.mkdir(parents=True,exist_ok=True)
      (ev/'actual.o').write_bytes(actual);(ev/'expected.o').write_bytes(want);(ev/'input.tape').write_bytes(tape.read_bytes())
     assert actual==want,(arch,unit,name,len(actual),len(want),next((i for i,(a,b) in enumerate(zip(actual,want)) if a!=b),None))
     count+=1
  print('model object: %d real ELF tapes byte-identical, two architectures, unit/whole; all-domain net=table'%count)
if __name__=='__main__':main()

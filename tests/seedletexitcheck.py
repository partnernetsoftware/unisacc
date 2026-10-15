#!/usr/bin/env python3
"""Generic let exit C/P parity: ASCII/raw bytes, existing container repr; no Unicode parser claim."""
import hashlib, json, pathlib, shutil, subprocess, sys, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
INPUTS=('exec/build/gen.py','exec/build/graph.py','exec/assemble.py','exec/finite_rules.py','exec/facts/load.py')
VALUES=('unsupported layout word_width','',None,False,True,0,7,-1,256,{'why':'bad','values':[1,True,None]},['bad',2],'line1\nline2','a\0b',['a\0b'],{'k':'a\0b'})
def bounded(argv,cwd):
 return subprocess.run([sys.executable,str(ROOT/'tests/bound.py'),'55',*map(str,argv)],cwd=cwd,capture_output=True,timeout=58)
def main():
 with tempfile.TemporaryDirectory(prefix='seed-let-exit-') as tmp:
  tree=pathlib.Path(tmp)
  for rel in INPUTS:
   target=tree/rel;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/rel,target)
  stage=tree/'exec/letexit';shutil.copytree(ROOT/'tests/fixtures/seed-let-exit',stage)
  gen=tree/'gen';q=bounded(['cc','-std=c99','-O2','-I'+str(ROOT/'seed'),str(ROOT/'seed/gen.c'),'-o',gen],tree)
  if q.returncode:raise AssertionError(('compile',q.returncode,q.stderr))
  original=(stage/'gen-manifest.tsv').read_text();count=0
  for value in VALUES:
   lines=original.splitlines();row=lines[4].split('\t');assert len(row)==9 and row[0]=='let';row[8]=json.dumps({'exit':value});lines[4]='\t'.join(row);(stage/'gen-manifest.tsv').write_text('\n'.join(lines)+'\n')
   for hit in (False,True):
    result=[];data=[]
    for backend,cmd in [('c',[gen]),('p',[sys.executable,tree/'exec/build/gen.py'])]:
     out=tree/(backend+'.json');out.write_bytes(b'SENTINEL');q=bounded([*cmd,'letexit',out,*(['--stop'] if hit else [])],tree);result.append(q);data.append(out.read_bytes())
    a,b=result
    assert a.returncode==b.returncode and a.stdout==b.stdout,(value,hit,'rc/stdout',a.returncode,b.returncode,a.stderr,b.stderr)
    assert data[0]==data[1],(value,hit,'artifact')
    if hit:
     assert a.stderr==b.stderr and data[0]==b'SENTINEL',(value,hit,'diagnostic/exit-before-write',a.stderr,b.stderr)
    else:assert a.returncode==0 and data[0]!=b'SENTINEL',(value,hit,'miss')
    count+=1
  print('seedgen-let-exit: C/P exact',count,'hit/miss; container NUL repr exact; Unicode excluded')
if __name__=='__main__':main()

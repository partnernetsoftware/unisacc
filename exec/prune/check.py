#!/usr/bin/env python3
"""Bounded private qualification, no default compiler/cache/output directory.
python3 exec/prune/check.py --output PRIVATE --probes PRIVATE_PROBES --calc CALC --ua PRIVATE_UA
Without --probes, fixed source/hand fixtures are sampled into the private output.
PRUNE_PART=0..3 selects bounded groups; --ua or PRUNE_UA is required.
"""
import argparse, hashlib, json, os, subprocess, sys, time
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT))
from unisa.prune import prune_text

def sha(x):return hashlib.sha256(x).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path);ap.add_argument('--probes',type=Path);ap.add_argument('--calc',type=Path);ap.add_argument('--ua',type=Path,default=os.environ.get('PRUNE_UA'));ap.add_argument('--start',type=int,default=0);ap.add_argument('--count',type=int,default=10);ap.add_argument('--build',action='store_true');ns=ap.parse_args()
 if ns.output is None:
  import tempfile
  ns.output=Path(tempfile.mkdtemp(prefix='unisacc-prune-check-'))
 assert ns.ua is not None, '--ua or PRUNE_UA required; no default UA'
 part=os.environ.get('PRUNE_PART')
 if part is not None:
  assert part in ('0','1','2','3');ns.start=int(part)*10;ns.count=10;ns.build=True
 assert ns.output.is_absolute() and str(ns.output).startswith(('/tmp/','/private/tmp/'))
 assert ns.ua.is_absolute(), 'explicit absolute compiler path required'
 assert ns.ua.is_file(), 'missing explicit compiler'
 ns.output.mkdir(parents=True,exist_ok=True);deadline=time.monotonic()+53;commands=[]
 def run(cmd,limit=10):
  assert time.monotonic()+limit<deadline, 'bounded slice exhausted'
  started=time.monotonic();r=subprocess.run(list(map(str,cmd)),cwd=ROOT,capture_output=True,timeout=limit)
  commands.append({'argv':list(map(str,cmd)),'rc':r.returncode,'seconds':round(time.monotonic()-started,4),'stdout_sha256':sha(r.stdout),'stderr':r.stderr.decode(errors='replace')});assert r.returncode==0,(cmd,r.returncode,r.stderr);return r
 d=ns.output/'prune.json';tbl=ns.output/'prune.tbl';net=ns.output/'prune.net';exe=ns.output/'executor';uexe=ns.output/'executor-unisacc'
 if ns.build:
  run([sys.executable,ROOT/'exec/prune/gen.py',d]);run([sys.executable,ROOT/'exec/c/tbl.py',d,tbl]);run([sys.executable,ROOT/'exec/c/net.py',tbl,net]);run(['cc','-O2',ROOT/'exec/c/run.c','-o',exe],15)
  run(([Path('/bin/sh'),ns.ua] if ns.ua.suffix=='.com' else [ns.ua])+['-O0',ROOT/'exec/c/run.c','-o',uexe],15);run([exe,'--check-net',tbl,net]);run([uexe,'--check-net',tbl,net])
 if ns.probes is None:
  ns.probes=ns.output/'samples';ns.probes.mkdir(exist_ok=True)
  fixtures=json.loads((ROOT/'exec/prune/fixtures.json').read_text())
  compiler=([Path('/bin/sh'),ns.ua] if ns.ua.suffix=='.com' else [ns.ua])
  for case in fixtures:
   folder=ns.probes/case['group'];folder.mkdir(exist_ok=True);sources=[]
   for name,raw in case['sources'].items():
    src=folder/name;src.write_text(raw);sources.append(src)
   for level in range(3):
    r=run(compiler+['-S','-O'+str(level),*sources],5);assert r.stdout,'empty source sample';(folder/('O%d.tape'%level)).write_bytes(r.stdout)
   for name,raw in case['manual'].items():(folder/name).write_text(raw)
  ns.calc=ns.output/'calc.tape';r=run(compiler+['-S','-O0',ROOT/'examples/apps/calc.c'],5);assert r.stdout,'empty calc';ns.calc.write_bytes(r.stdout)
 assert ns.probes.is_dir() and ns.calc is not None and ns.calc.is_file(),'missing required qualification input'
 samples=sorted(ns.probes.glob('*/O[012].tape'))+sorted(p for p in ns.probes.glob('*/*.tape') if not p.name.startswith('O'))+[ns.calc]
 assert len(samples)==30,len(samples)
 # Common-domain controls: never execute fixture tape as a program.
 base=b'_start:\n  call main\n  .exit r0\ndead:\n  .frame 8\n  store64 [r7+0], r6\n  mov r6, r7\n  .frame 0\n  ret\nmain:\n  .frame 8\n  store64 [r7+0], r6\n  mov r6, r7\n  .frame 0\n  ret\n'
 controls={'cr':base.replace(b'\n',b'\r'),'underscores':base.replace(b'.frame 0',b'.frame 0_0'),'wide':base.replace(b'.frame 0',b'.frame 9223372036854775808'),'host-slot':base.replace(b'  ret\nmain:',b'  .hostaddr r0, 4\n  ret\nmain:'),'bad-width':base.replace(b'  ret\nmain:',b'  .ld r0, r1, 3\n  ret\nmain:'),'too-large':b';'*(2097152+1),'too-many-rows':b'.bss z 0\n'*32769,'too-many-names':b''.join(('name%d:\n'%i).encode() for i in range(8193))}
 for name,raw in controls.items():
  p=ns.output/(name+'.tape');p.write_bytes(raw);samples.append(p)
 leading=ns.output/'bss-leadingzeros.tape';leading.write_bytes(base.replace(b'dead:\n',b'.bss data 001024\ndead:\n'));samples.append(leading)
 records=[]
 assert ns.start>=0 and ns.count>0 and ns.start<len(samples),'empty qualification slice'
 # Missing input must fail explicitly, never count as a successful comparison.
 missing=ns.output/'missing-required.tape'
 try:missing.stat()
 except FileNotFoundError:pass
 else:raise AssertionError('missing-input fixture unexpectedly present')
 r=subprocess.run([str(exe),str(net),str(missing)],capture_output=True,timeout=5);assert r.returncode!=0 and not r.stdout,'missing input accepted'
 for i,p in enumerate(samples[ns.start:ns.start+ns.count],ns.start):
  raw=p.read_bytes();assert raw,('empty input sample',p);expected=prune_text(raw);record={'index':i,'input':str(p),'input_bytes':len(raw),'output_bytes':len(expected),'input_sha256':sha(raw),'output_sha256':sha(expected),'runs':[]}
  assert expected,('empty reference sample',p)
  if p.name in {name+'.tape' for name in controls}:assert expected==raw,('control was pruned',p)
  for runner,model in [(exe,tbl),(exe,net),(uexe,net)]:
   r=run([runner,model,p],5);assert r.stdout==expected,(p,runner,model,len(r.stdout),len(expected));assert r.stdout or not expected
   record['runs'].append({'executor':str(runner),'model':str(model),'rc':r.returncode,'bytes_equal':True,'sha256':sha(r.stdout)})
  records.append(record)
 receipt={'not_routed_or_shipped':True,'missing_input_rejected':True,'reference_sha256':sha((ROOT/'unisa/prune.py').read_bytes()),'schema_sha256':sha((ROOT/'unisa/tape.py').read_bytes()),'generator_sha256':sha((ROOT/'exec/prune/gen.py').read_bytes()),'ua_sha256':sha(ns.ua.read_bytes()),'models':{str(p):sha(p.read_bytes()) for p in [d,tbl,net]},'commands':commands,'cases':records}
 out=ns.output/('receipt-%d.json'%ns.start);out.write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({'receipt':str(out),'cases':len(records),'actual_runs':sum(len(x['runs']) for x in records)}))
if __name__=='__main__':main()

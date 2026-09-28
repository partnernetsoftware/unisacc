"""Real SDK two-ISA cross build; --native executes on Windows. No VM lifecycle."""
import argparse,hashlib,json,pathlib,shutil,subprocess,tempfile,time
ROOT=pathlib.Path(__file__).resolve().parents[1]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--output');ap.add_argument('--native',action='store_true');ap.add_argument('--target',choices=['aarch64-windows-gnu','x86_64-windows-gnu']);a=ap.parse_args()
 td=pathlib.Path(a.output) if a.output else pathlib.Path(tempfile.mkdtemp(prefix='r10-windows-resolver-host-'))
 td.mkdir(parents=True,exist_ok=True);(td/'exec/c').mkdir(parents=True,exist_ok=True);(td/'tests').mkdir(exist_ok=True)
 files=['exec/c/librarybindings.h','exec/c/libraryresolver.h','tests/windowsresolverhost.c']
 for f in files:shutil.copy2(ROOT/f,td/f)
 zig=shutil.which('zig');assert zig,'real Windows SDK cross compiler zig required';runs=[];artifacts={}
 def run(cmd):
  start=time.monotonic();p=subprocess.run(['python3',str(ROOT/'tests/bound.py'),'45',*map(str,cmd)],capture_output=True,timeout=50)
  row=dict(command=list(map(str,cmd)),rc=p.returncode,seconds=time.monotonic()-start,stdout=p.stdout.decode(errors='replace'),stderr=p.stderr.decode(errors='replace'));runs.append(row)
  (td/'runs.json').write_text(json.dumps(runs,indent=2)+'\n');assert p.returncode==0,row
 for target in ([a.target] if a.target else ['aarch64-windows-gnu','x86_64-windows-gnu']):
  stem=target.split('-')[0];exe=td/(stem+'-resolver.exe');dlls=[]
  for val in [33,44]:
   dll=td/(stem+'-fixture-'+str(val)+'.dll');dlls.append(dll)
   run([zig,'cc','-target',target,'-std=c11','-O1','-Wall','-Wextra','-Werror','-shared','-DUS_RESOLVER_FIXTURE='+str(val),td/'tests/windowsresolverhost.c','-o',dll])
  run([zig,'cc','-target',target,'-std=c11','-O1','-Wall','-Wextra','-Werror',td/'tests/windowsresolverhost.c','-o',exe])
  if a.native:run([exe,*dlls])
  for p in [exe,*dlls]:artifacts[p.name]=dict(path=str(p),bytes=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest())
 result=dict(scope='mechanical host candidates only; native runtime required separately unless --native',runtime_executed=a.native,output=str(td),source_sha256={f:hashlib.sha256((td/f).read_bytes()).hexdigest() for f in files},artifacts=artifacts,runs=runs)
 (td/'evidence.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':main()

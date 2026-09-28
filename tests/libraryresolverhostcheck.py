"""Private native candidate enumeration, not a δ/native script proof."""
import hashlib,json,os,pathlib,platform,shutil,subprocess,tempfile,time
ROOT=pathlib.Path(__file__).resolve().parents[1]
def command(args,cwd,env=None):
 start=time.monotonic();p=subprocess.run([str(a) for a in args],cwd=cwd,env=env,capture_output=True,timeout=20)
 assert p.returncode==0,(args,p.returncode,p.stdout.decode(errors='replace'),p.stderr.decode(errors='replace'))
 return {'command':list(map(str,args)),'rc':p.returncode,'seconds':time.monotonic()-start,'stdout':p.stdout.decode(),'stderr':p.stderr.decode()}
def main():
 assert platform.system()!='Windows','Windows helper unimplemented'
 with tempfile.TemporaryDirectory(prefix='r10-resolver-host-') as name:
  td=pathlib.Path(name);(td/'exec/c').mkdir(parents=True);(td/'tests').mkdir()
  for f in ('librarybindings.h','libraryresolver.h'):shutil.copy2(ROOT/'exec/c'/f,td/'exec/c'/f)
  shutil.copy2(ROOT/'tests/libraryresolverhostcheck.c',td/'tests/libraryresolverhostcheck.c')
  results=[];darwin=platform.system()=='Darwin';suffix='.dylib' if darwin else '.so';libs=[]
  for i,(value,data) in enumerate(((33,34),(44,45))):
   c=td/f'lib{i}.c';lib=td/f'lib{i}{suffix}';libs.append(lib)
   c.write_text('int resolver_shared(void){return %d;}\nint resolver_data=%d;\n#include <stdio.h>\n#include <stdlib.h>\n__attribute__((destructor)) static void finished(void){const char*p=getenv("US_RESOLVER_CLOSE_LOG");if(p){FILE*f=fopen(p,"a");if(f){fputs("%d\\n",f);fclose(f);}}}\n'%(value,data,i))
   results.append(command(['cc','-shared','-fPIC',c,'-o',lib],td))
  for flags,stem in (([],'plain'),(['-fsanitize=address,undefined'],'sanitised')):
   exe=td/stem;close=td/(stem+'.closed');export=['-Wl,-export_dynamic'] if darwin else ['-rdynamic'];dl=[] if darwin else ['-ldl']
   results.append(command(['cc','-std=c11','-O1','-Wall','-Wextra',*flags,*export,td/'tests/libraryresolverhostcheck.c',*dl,'-o',exe],td))
   results.append(command([exe,*libs],td,dict(os.environ,US_RESOLVER_CLOSE_LOG=str(close))))
   assert close.read_text().splitlines()==['1','0'],'owned handles must close exactly once, in reverse order'
  print(json.dumps({'scope':'host C candidate IO only; no model/script test; current native platform only','platform':platform.platform(),'source_sha256':{f:hashlib.sha256((ROOT/f).read_bytes()).hexdigest() for f in ('exec/c/libraryresolver.h','exec/c/librarybindings.h','tests/libraryresolverhostcheck.c')},'all_candidates':8,'runs':results}))
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Consume built static and dynamic libraries through the public C API.
Uses private outputs only. Every compiler/process has a <=30 second timeout.
Evidence applies only to the current POSIX host, not all six targets.
"""
import argparse,hashlib,json,os,pathlib,shlex,subprocess,tempfile,time
SOURCE=r'''#include "libunisacc.h"
#include <stdio.h>
#include <string.h>
static const char *program=
"#ifndef READY\nint main( {\n#else\n"
"long sum(long a,long b){return a+b;}\n"
"int main(void){return 17;}\n#endif\n";
int main(int argc,char **argv){
 if(argc!=3)return 2;
 us_context *c=us_new(argv[1]);if(!c)return 3;
 if(us_add_source(c,"consumer.c",program))return 4;
 if(us_compile(c,argv[2],0)==0 || !*us_error(c))return 5;
 if(us_define(c,"READY=1"))return 6;
 for(int level=0;level<3;level++){
  if(us_compile(c,argv[2],level) || us_relocate(c)){fprintf(stderr,"%s\n",us_error(c));return 7;}
  long (*sum)(long,long)=(long(*)(long,long))us_sym(c,"sum");
  if(!sum || sum(11,31)!=42)return 8;
  int status=-1;const char *args[]={"consumer"};
  if(us_run_main(c,1,args,&status) || status!=17)return 9;
  if(sum(20,22)!=42)return 10;
  int exit_status=-1;if(us_call_status(c,&exit_status))return 11;
 }
 us_free(c);puts("public C API: parse failure recovery, O0/O1/O2 compile/map/main, native us_sym: ok");return 0;
}
'''
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def check(folder,cc):
 folder=pathlib.Path(folder).resolve();manifest=json.loads((folder/'manifest.json').read_text());target=manifest['target']
 import platform
 host_os={'Darwin':'osx','Linux':'lnx'}.get(platform.system());arch='arm64' if platform.machine().lower() in ('arm64','aarch64') else 'x86_64'
 if target!=host_os+'/'+arch:raise ValueError('consumer requires library matching actual host')
 for name,facts in manifest['artifacts'].items():
  p=folder/name;assert p.stat().st_size==facts['bytes'] and sha(p)==facts['sha256'],('artifact changed',name)
 evidence={'scope':'actual host-native static and dynamic public C API consumption','target':target,'manifest_sha256':sha(folder/'manifest.json'),'package_sha256':manifest['package_sha256'],'results':[]}
 with tempfile.TemporaryDirectory(prefix='unisacc-library-consumer-') as name:
  d=pathlib.Path(name);source=d/'consumer.c';source.write_text(SOURCE)
  for kind,library in [('static','libunisacc.a'),('dynamic','libunisacc.dylib' if host_os=='osx' else 'libunisacc.so')]:
   exe=d/kind;cmd=cc+['-std=c11','-O2','-I',str(folder),str(source),str(folder/library)]+manifest['dependencies']['ffi_link_flags']+['-Wl,-rpath,'+str(folder),'-o',str(exe)]
   steps=[]
   for args in [cmd,[str(exe),str(folder/'compiler.pkg'),target]]:
    started=time.monotonic();r=subprocess.run(args,capture_output=True,timeout=30)
    step={'argv':args,'rc':r.returncode,'seconds':time.monotonic()-started,'stdout':r.stdout.decode(errors='replace'),'stderr':r.stderr.decode(errors='replace')};steps.append(step)
    assert r.returncode==0,step
   assert steps[-1]['stdout']=='public C API: parse failure recovery, O0/O1/O2 compile/map/main, native us_sym: ok\n',steps[-1]
   evidence['results'].append({'linkage':kind,'library_sha256':sha(folder/library),'consumer_sha256':sha(exe),'commands':steps})
 return evidence
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);g=ap.add_mutually_exclusive_group(required=True);g.add_argument('--library-dir',type=pathlib.Path);g.add_argument('--package',type=pathlib.Path);ap.add_argument('--cc',default=os.environ.get('CC','cc'));ap.add_argument('--evidence',type=pathlib.Path);a=ap.parse_args()
 try:
  if a.package:
   import sys
   sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'exec/c'))
   from buildlibrary import build
   with tempfile.TemporaryDirectory(prefix='unisacc-library-artifact-gate-') as name:
    build(a.package,pathlib.Path(name),shlex.split(a.cc),shlex.split(os.environ.get('AR','ar')),[],['-lffi'])
    evidence=check(pathlib.Path(name),shlex.split(a.cc))
  else:evidence=check(a.library_dir,shlex.split(a.cc))
  if a.evidence:a.evidence.write_text(json.dumps(evidence,indent=2)+'\n')
  print(json.dumps(evidence,sort_keys=True))
 except (OSError,ValueError,AssertionError,subprocess.SubprocessError) as e:ap.exit(1,'libraryartifactcheck: '+str(e)+'\n')

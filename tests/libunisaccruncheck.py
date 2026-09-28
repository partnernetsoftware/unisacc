"""Actual in-process library execution; consumes an already constructed package.
This is host-native evidence, not a six-platform or full-signature claim.
"""
import argparse,ctypes,pathlib,shutil,subprocess,tempfile,time,json,concurrent.futures
ROOT=pathlib.Path(__file__).resolve().parents[1]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--package',required=True);ap.add_argument('--iterations',type=int,default=100);a=ap.parse_args()
 assert 1<=a.iterations<=1000
 with tempfile.TemporaryDirectory(prefix='unisacc-library-run-') as name:
  td=pathlib.Path(name);runtime=td/'exec/c';runtime.mkdir(parents=True);(td/'src').mkdir()
  for f in (ROOT/'exec/c').iterdir():
   if f.is_file() and f.suffix in ('.h','.c','.S'):shutil.copy2(f,runtime/f.name)
  shutil.copy2(ROOT/'src/host_dl.h',td/'src/host_dl.h')
  out=td/'library.dylib'
  wrapper=td/'test-library.c'
  wrapper.write_text('#include "'+str(runtime/'libunisacc.c')+'"\n'+r'''
__attribute__((visibility("default"))) long test_guest_count(us_context*c){long n=0;for(GuestMap*m=c->guest_maps;m;m=m->next)n++;return n;}
__attribute__((visibility("default"))) long test_guest_base(us_context*c){return c->guest_maps?(long)c->guest_maps->base:0;}
#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_vm.h>
__attribute__((visibility("default"))) int test_mapped(long p){
 mach_vm_address_t a=p;mach_vm_size_t n=0;vm_region_basic_info_data_64_t i;
 mach_msg_type_number_t c=VM_REGION_BASIC_INFO_COUNT_64;mach_port_t o=0;
 int r=mach_vm_region(mach_task_self(),&a,&n,VM_REGION_BASIC_INFO_64,(vm_region_info_t)&i,&c,&o);
 if(o)mach_port_deallocate(mach_task_self(),o);return !r && a<=(unsigned long)p && (unsigned long)p<a+n;
}
#else
__attribute__((visibility("default"))) int test_mapped(long p){unsigned char v;return !mincore((void*)p,(size_t)sysconf(_SC_PAGESIZE),&v);}
#endif
''')
  subprocess.run(['cc','-std=c11','-O2','-fvisibility=hidden','-shared','-fPIC',str(wrapper),str(runtime/('librarycall_'+('arm64' if __import__('platform').machine() in ('arm64','aarch64') else 'x86_64')+'.S')),'-lffi','-o',str(out)],check=True,timeout=30)
  L=ctypes.CDLL(str(out));L.us_new.argtypes=[ctypes.c_char_p];L.us_new.restype=ctypes.c_void_p
  for n,args in [('us_add_source',[ctypes.c_void_p,ctypes.c_char_p,ctypes.c_char_p]),('us_compile',[ctypes.c_void_p,ctypes.c_char_p,ctypes.c_int]),('us_run_main',[ctypes.c_void_p,ctypes.c_int,ctypes.POINTER(ctypes.c_char_p),ctypes.POINTER(ctypes.c_int)])]:getattr(L,n).argtypes=args
  L.us_error.argtypes=[ctypes.c_void_p];L.us_error.restype=ctypes.c_char_p;L.us_free.argtypes=[ctypes.c_void_p]
  import platform
  target=('osx/' if platform.system()=='Darwin' else 'lnx/')+('arm64' if platform.machine() in ('arm64','aarch64') else 'x86_64')
  pkg=str(pathlib.Path(a.package).resolve()).encode();args=(ctypes.c_char_p*2)(b'hosted',b'arg')
  def run(source,expected,iterations):
   c=L.us_new(pkg);assert c
   try:
    assert L.us_add_source(c,b'hosted.c',source)==0
    for _ in range(iterations):
     assert L.us_compile(c,target.encode(),0)==0,L.us_error(c)
     status=ctypes.c_int();rc=L.us_run_main(c,2,args,ctypes.byref(status))
     assert rc==0,(rc,L.us_error(c));assert status.value==expected,status.value
   finally:L.us_free(c)
  L.test_guest_count.argtypes=[ctypes.c_void_p];L.test_guest_count.restype=ctypes.c_long
  L.test_guest_base.argtypes=[ctypes.c_void_p];L.test_guest_base.restype=ctypes.c_long
  L.test_mapped.argtypes=[ctypes.c_long]
  started=time.monotonic()
  with concurrent.futures.ThreadPoolExecutor(max_workers=4) as ex:
   list(ex.map(lambda n:run(('int main(int argc,char **argv){return argc+'+str(n)+';}').encode(),n+2,3),range(4)))
  run(b'#include <stdlib.h>\nint main(void){exit(51);return 9;}',51,1)
  run(b'int main(int argc,char **argv){return argc+37;}',39,a.iterations)
  run(b'int main(int argc,char **argv){return argc==2 && argv[2]==0 && argv[1][0]==97 ? 61 : 3;}',61,1)
  run(b'#include <stdlib.h>\nint main(void){exit(0);return 9;}',0,1)
  c=L.us_new(pkg)
  try:
   assert not L.us_add_source(c,b'alloc.c',b'#include <stdlib.h>\nint main(void){char *p=malloc(32);if(!p)return 9;p[0]=41;int n=p[0];free(p);return n;}')
   for k in range(20):
    assert not L.us_compile(c,target.encode(),0),L.us_error(c)
    assert L.test_guest_count(c)==0,'old guest mappings survive next compile'
    status=ctypes.c_int();rc=L.us_run_main(c,2,args,ctypes.byref(status))
    assert rc==0,(rc,L.us_error(c));assert status.value==41,status.value
    assert L.test_guest_count(c)==1,('mapping accumulation',k,L.test_guest_count(c))
   base=L.test_guest_base(c);assert base and L.test_mapped(base)==1
  finally:L.us_free(c)
  assert L.test_mapped(base)==0,'guest mapping survives context free'
  print(json.dumps({'scope':target+' actual host execution','concurrent_contexts':4,'explicit_exit_host_survived':True,'guest_pool_released':True,'same_context_compile_relocate_run_iterations':a.iterations,'seconds':time.monotonic()-started}))
if __name__=='__main__':main()

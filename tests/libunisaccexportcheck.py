"""Actual E3 declarations -> native callable exports, host-native scoped."""
import argparse,ctypes,concurrent.futures,pathlib,platform,shutil,subprocess,tempfile,json
ROOT=pathlib.Path(__file__).resolve().parents[1]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--package',required=True);ap.add_argument('--no-main',action='store_true');a=ap.parse_args()
 arch='arm64' if platform.machine() in ('arm64','aarch64') else 'x86_64'
 target=('osx/' if platform.system()=='Darwin' else 'lnx/')+arch
 with tempfile.TemporaryDirectory(prefix='r10-native-exports-context-') as folder:
  td=pathlib.Path(folder);runtime=td/'exec/c';runtime.mkdir(parents=True);(td/'src').mkdir()
  for p in (ROOT/'exec/c').iterdir():
   if p.is_file() and p.suffix in ('.h','.c','.S'):shutil.copy2(p,runtime/p.name)
  shutil.copy2(ROOT/'src/host_dl.h',td/'src/host_dl.h')
  lib=td/'library.dylib'
  subprocess.run(['cc','-std=c11','-O2','-shared','-fPIC','-fvisibility=hidden',str(runtime/'libunisacc.c'),str(runtime/('librarycall_'+arch+'.S')),'-lffi','-o',str(lib)],check=True,timeout=20)
  L=ctypes.CDLL(str(lib));P=ctypes.c_void_p
  L.us_new.argtypes=[ctypes.c_char_p];L.us_new.restype=P
  L.us_add_source.argtypes=[P,ctypes.c_char_p,ctypes.c_char_p];L.us_compile.argtypes=[P,ctypes.c_char_p,ctypes.c_int]
  L.us_relocate.argtypes=[P];L.us_free.argtypes=[P];L.us_sym.argtypes=[P,ctypes.c_char_p];L.us_sym.restype=P
  L.us_error.argtypes=[P];L.us_error.restype=ctypes.c_char_p
  L.us_call_status.argtypes=[P,ctypes.POINTER(ctypes.c_int)]
  L.us_run_main.argtypes=[P,ctypes.c_int,ctypes.POINTER(ctypes.c_char_p),ctypes.POINTER(ctypes.c_int)]
  source=b"""#include <stdlib.h>
int state=5;
static int hidden(void){return 99;}
int bump(int n){state+=n;return state;}
long mixed(signed char a,unsigned short b,int c,unsigned long d,int *p){return a+b+c+d+*p;}
void store(int *p,int n){*p=n;}
int *echo(int *p){return p;}
int die(void){exit(37);return 9;}
int process_count(void){return __argc();}
char *process_arg(int n){return __argv(n);}
int main(int argc,char **argv){return state+argc+(argv[1][0]==97);}
"""
  if a.no_main:source=source.replace(b'int main(int argc,char **argv){return state+argc+(argv[1][0]==97);}',b'')
  def probe(index):
   c=L.us_new(str(pathlib.Path(a.package).resolve()).encode());assert c
   try:
    assert not L.us_add_source(c,b'exports.c',source)
    assert not L.us_compile(c,target.encode(),index%3),L.us_error(c)
    assert not L.us_sym(c,b'bump') and L.us_error(c)
    assert not L.us_relocate(c),L.us_error(c)
    def fn(name,result,*args):
     address=L.us_sym(c,name.encode());assert address,(name,L.us_error(c))
     assert address==L.us_sym(c,name.encode()),'unstable same-generation pointer'
     return ctypes.CFUNCTYPE(result,*args)(address)
    bump=fn('bump',ctypes.c_int,ctypes.c_int)
    count=fn('process_count',ctypes.c_int);assert count()==0
    assert bump(2)==7 and bump(2)==9,'initialisation repeated or state lost'
    cell=ctypes.c_int(11)
    mixed=fn('mixed',ctypes.c_long,ctypes.c_byte,ctypes.c_ushort,ctypes.c_int,ctypes.c_ulong,ctypes.POINTER(ctypes.c_int))
    assert mixed(-3,65000,-20,100000,ctypes.byref(cell))==164988
    store=fn('store',None,ctypes.POINTER(ctypes.c_int),ctypes.c_int);store(ctypes.byref(cell),43);assert cell.value==43
    echo=fn('echo',P,ctypes.POINTER(ctypes.c_int));assert echo(ctypes.byref(cell))==ctypes.addressof(cell)
    assert not L.us_sym(c,b'hidden') and not L.us_sym(c,b'absent')
    die=fn('die',ctypes.c_int);assert die()==0
    status=ctypes.c_int();assert L.us_call_status(c,ctypes.byref(status))==1 and status.value==37
    assert b'37' in L.us_error(c)
    assert bump(1)==10 and L.us_call_status(c,ctypes.byref(status))==0,'exit recovery failed'
    args=(ctypes.c_char_p*2)(b'hosted',b'arg');status=ctypes.c_int()
    rc=L.us_run_main(c,2,args,ctypes.byref(status))
    if a.no_main:assert rc and b'no main' in L.us_error(c),'missing main was not refused'
    else:
     assert not rc,L.us_error(c)
     assert status.value==13,'main reinitialised the existing mapping'
     assert count()==2,'dynamic argc remains at relocation-time value'
     arg=fn('process_arg',ctypes.c_char_p,ctypes.c_int);assert arg(1)==b'arg'
     nextargs=(ctypes.c_char_p*3)(b'hosted',b'other',b'tail')
     assert not L.us_run_main(c,3,nextargs,ctypes.byref(status)),L.us_error(c)
     assert count()==3 and arg(1)==b'other','same mapping argv did not update'
     assert status.value==13,'main reset state on second invocation'
    assert bump(1)==11,'main invalidated a callable export'
    return index
   finally:L.us_free(c)
  with concurrent.futures.ThreadPoolExecutor(max_workers=4) as ex:assert list(ex.map(probe,range(4)))==list(range(4))
  print(json.dumps({'platform':target,'contexts':4,'E3_declared_native_exports':True,'initialisation_once':True,'exit_recovery':True,'no_main_module':a.no_main,'main_preserves_export_pointers':not a.no_main,'dynamic_process_slots':True}))
if __name__=='__main__':main()

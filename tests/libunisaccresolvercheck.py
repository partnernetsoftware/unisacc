"""Native POSIX resolver through public API and actual product network routes.
Host C builds only the library adaptation and real shared-library fixtures.
"""
import argparse,ctypes,hashlib,json,pathlib,platform,shutil,subprocess,tempfile
from libunisaccbindingscheck import Type,Signature,integer
ROOT=pathlib.Path(__file__).resolve().parents[1]
P=ctypes.c_void_p

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--package',required=True);a=ap.parse_args()
 pkg=pathlib.Path(a.package).resolve();arch='arm64' if platform.machine() in ('arm64','aarch64') else 'x86_64'
 target=('osx/' if platform.system()=='Darwin' else 'lnx/')+arch
 with tempfile.TemporaryDirectory(prefix='r10-resolver-native-') as folder:
  td=pathlib.Path(folder);runtime=td/'exec/c';runtime.mkdir(parents=True);(td/'src').mkdir()
  for f in (ROOT/'exec/c').iterdir():
   if f.is_file() and f.suffix in ('.h','.c','.S'):shutil.copy2(f,runtime/f.name)
  shutil.copy2(ROOT/'src/host_dl.h',td/'src/host_dl.h')
  def shared(name,body):
   src=td/(name+'.c');out=td/(name+'.dylib');src.write_text(body)
   subprocess.run(['cc','-std=c11','-O2','-shared','-fPIC',str(src),'-o',str(out)],check=True,timeout=15);return out
  first=shared('first','long owned_only(long n){return n+10;} int owned_value=101; long layered(long n){return n+11;}')
  second=shared('second','long owned_only(long n){return n+20;} int owned_value=202; long layered(long n){return n+22;}')
  process=shared('process','long layered(long n){return n+3;}')
  process_handle=ctypes.CDLL(str(process),mode=ctypes.RTLD_GLOBAL)
  lib=td/'library.dylib';command=['cc','-std=c11','-O2','-shared','-fPIC','-fvisibility=hidden',str(runtime/'libunisacc.c'),str(runtime/('librarycall_'+arch+'.S')),'-lffi','-o',str(lib)]
  if platform.system()=='Linux':command.append('-ldl')
  subprocess.run(command,check=True,timeout=25)
  L=ctypes.CDLL(str(lib));L.us_new.argtypes=[ctypes.c_char_p];L.us_new.restype=P
  L.us_free.argtypes=[P];L.us_add_source.argtypes=[P,ctypes.c_char_p,ctypes.c_char_p]
  L.us_declare_import.argtypes=[P,ctypes.c_char_p,ctypes.POINTER(Signature)]
  L.us_load_library.argtypes=[P,ctypes.c_char_p];L.us_add_symbol.argtypes=[P,ctypes.c_char_p,P,ctypes.POINTER(Signature)]
  L.us_compile.argtypes=[P,ctypes.c_char_p,ctypes.c_int];L.us_relocate.argtypes=[P]
  L.us_sym.argtypes=[P,ctypes.c_char_p];L.us_sym.restype=P;L.us_error.argtypes=[P];L.us_error.restype=ctypes.c_char_p
  params=(Type*1)(integer());fn_sig=Signature(0,integer(),params,1,0,0,0)
  data_sig=Signature(1,integer(4),None,0,0,4,1)
  callback=ctypes.CFUNCTYPE(ctypes.c_long,ctypes.c_long)(lambda n:n+40)
  checks=[]
  def ok(rc,c):assert rc==0,L.us_error(c)
  def compile_(c,source,level):
   ok(L.us_add_source(c,b'resolve.c',source),c);ok(L.us_compile(c,target.encode(),level),c);ok(L.us_relocate(c),c)
   address=L.us_sym(c,b'check');assert address,L.us_error(c);return ctypes.CFUNCTYPE(ctypes.c_long,ctypes.c_long)(address)
  for level in range(3):
   for mode in ('owned','process','injected','source'):
    c=L.us_new(str(pkg).encode());assert c
    try:
     name=b'owned_only' if mode=='owned' else b'layered'
     ok(L.us_declare_import(c,name,ctypes.byref(fn_sig)),c)
     ok(L.us_load_library(c,str(first).encode()),c);ok(L.us_load_library(c,str(second).encode()),c)
     if mode=='injected':ok(L.us_add_symbol(c,name,ctypes.cast(callback,P),ctypes.byref(fn_sig)),c)
     source=b'long '+name+b'(long n);long check(long n){return '+name+b'(n);}'
     if mode=='source':source+=b'long layered(long n){return n+7;}'
     fn=compile_(c,source,level);expected={'owned':15,'process':8,'injected':45,'source':12}[mode]
     assert fn(5)==expected,(level,mode,fn(5))
     # Failed mutations preserve existing compiled code and export pointers.
     assert L.us_load_library(c,str(td/'missing').encode())!=0
     assert fn(5)==expected
     assert L.us_load_library(c,str(first).encode())!=0
     assert fn(5)==expected
     assert L.us_declare_import(c,name,ctypes.byref(fn_sig))!=0
     assert fn(5)==expected
     checks.append([level,mode,expected])
    finally:L.us_free(c)
   c=L.us_new(str(pkg).encode());assert c
   try:
    ok(L.us_declare_import(c,b'owned_value',ctypes.byref(data_sig)),c);ok(L.us_load_library(c,str(first).encode()),c);ok(L.us_load_library(c,str(second).encode()),c)
    fn=compile_(c,b'extern int owned_value;long check(long n){owned_value=owned_value+n;return owned_value;}',level)
    assert fn(1)==102;assert fn(2)==104
    # Each iteration opens the same shared fixture; restore for next context.
    probe=ctypes.CDLL(str(first));assert ctypes.c_int.in_dll(probe,'owned_value').value==104
    ctypes.c_int.in_dll(probe,'owned_value').value=101
    checks.append([level,'owned-data',104])
   finally:L.us_free(c)
  # Missing declared data must not silently receive fabricated .bss storage.
  for source in (b'extern int absent_object;long check(long n){return absent_object+n;}',b'long absent_function(long n);long check(long n){return absent_function(n);}'):
   c=L.us_new(str(pkg).encode());assert c
   try:
    data=source.startswith(b'extern');ok(L.us_declare_import(c,b'absent_object' if data else b'absent_function',ctypes.byref(data_sig if data else fn_sig)),c)
    ok(L.us_add_source(c,b'missing.c',source),c)
    rc=L.us_compile(c,target.encode(),0)
    if rc==0:rc=L.us_relocate(c) # lower resolves data during mapping, not tape generation
    assert rc!=0;assert L.us_error(c);assert not L.us_sym(c,b'check')
   finally:L.us_free(c)
  # Registration succeeds only when the two trusted declarations agree.
  c=L.us_new(str(pkg).encode());assert c
  try:
   ok(L.us_declare_import(c,b'layered',ctypes.byref(fn_sig)),c)
   wrong=Signature(0,integer(4),params,1,0,0,0)
   assert L.us_add_symbol(c,b'layered',ctypes.cast(callback,P),ctypes.byref(wrong))!=0
   ok(L.us_add_symbol(c,b'layered',ctypes.cast(callback,P),ctypes.byref(fn_sig)),c)
   fn=compile_(c,b'long layered(long n);long check(long n){return layered(n);}',0);assert fn(5)==45
   # A successful mutation invalidates the old generation; never call stale fn.
   ok(L.us_load_library(c,str(first).encode()),c);assert not L.us_sym(c,b'check')
   ok(L.us_compile(c,target.encode(),0),c);ok(L.us_relocate(c),c)
   address=L.us_sym(c,b'check');assert address;assert ctypes.CFUNCTYPE(ctypes.c_long,ctypes.c_long)(address)(5)==45
  finally:L.us_free(c)
  print(json.dumps({'target':target,'checks':checks,'missing_rejected':2,'failed_mutation_preserves_exports':True,'successful_mutation_invalidates_exports':True,'trusted_abi_conflict_rejected':True,'package_sha256':hashlib.sha256(pkg.read_bytes()).hexdigest(),'process_library':str(process),'scope':'native POSIX fixed integer function and writable scalar data; not Windows/FP/aggregate/variadic'}))
if __name__=='__main__':main()

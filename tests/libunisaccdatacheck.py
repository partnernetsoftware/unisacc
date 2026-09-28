"""Real borrowed host objects read/written through E3/lower model decisions.
Consumes a built candidate; C compiler only builds the host API library.
"""
import argparse,ctypes,concurrent.futures,pathlib,platform,shutil,subprocess,tempfile,json,hashlib
from libunisaccbindingscheck import Type,Signature,integer,pointer
ROOT=pathlib.Path(__file__).resolve().parents[1]
P=ctypes.c_void_p
SOURCE=b"extern int hostvalue;extern signed char tiny;extern unsigned short wide;extern int *hostptr;long hostplus(long n);long check(long d){int *alias=&hostvalue;hostvalue=hostvalue+d;tiny=tiny-1;wide=wide+1;*hostptr=*hostptr+2;return hostplus(*alias)+tiny+wide+*hostptr;}"
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--package',required=True);a=ap.parse_args()
 arch='arm64' if platform.machine() in ('arm64','aarch64') else 'x86_64';target=('osx/' if platform.system()=='Darwin' else 'lnx/')+arch
 pkg=str(pathlib.Path(a.package).resolve()).encode()
 with tempfile.TemporaryDirectory(prefix='r10-data-injection-') as folder:
  td=pathlib.Path(folder);runtime=td/'exec/c';runtime.mkdir(parents=True);(td/'src').mkdir()
  for f in (ROOT/'exec/c').iterdir():
   if f.is_file() and f.suffix in ('.h','.c','.S'):shutil.copy2(f,runtime/f.name)
  shutil.copy2(ROOT/'src/host_dl.h',td/'src/host_dl.h')
  library=td/'library.dylib';subprocess.run(['cc','-std=c11','-O2','-shared','-fPIC','-fvisibility=hidden',str(runtime/'libunisacc.c'),str(runtime/('librarycall_'+arch+'.S')),'-lffi','-o',str(library)],check=True,timeout=25)
  L=ctypes.CDLL(str(library));L.us_new.argtypes=[ctypes.c_char_p];L.us_new.restype=P
  L.us_add_source.argtypes=[P,ctypes.c_char_p,ctypes.c_char_p];L.us_add_symbol.argtypes=[P,ctypes.c_char_p,P,ctypes.POINTER(Signature)]
  L.us_compile.argtypes=[P,ctypes.c_char_p,ctypes.c_int];L.us_relocate.argtypes=[P];L.us_free.argtypes=[P]
  L.us_sym.argtypes=[P,ctypes.c_char_p];L.us_sym.restype=P;L.us_error.argtypes=[P];L.us_error.restype=ctypes.c_char_p
  def data(c,name,obj,t,writable=1):
   sig=Signature(1,t,None,0,0,ctypes.sizeof(obj),writable)
   assert L.us_add_symbol(c,name.encode(),ctypes.cast(ctypes.pointer(obj),P),ctypes.byref(sig))==0,L.us_error(c)
   return sig
  def compiled(c,source,level):
   assert L.us_add_source(c,b'data.c',source)==0
   assert L.us_compile(c,target.encode(),level)==0,L.us_error(c)
   assert L.us_relocate(c)==0,L.us_error(c)
  def probe(index):
   c=L.us_new(pkg);assert c
   value=ctypes.c_int(11+index);tiny=ctypes.c_byte(-3);wide=ctypes.c_ushort(65000);pointee=ctypes.c_int(91);ptr=ctypes.pointer(pointee)
   calls=[];cb=ctypes.CFUNCTYPE(ctypes.c_long,ctypes.c_long)(lambda n:(calls.append(n),n+7)[1]);types=(Type*1)(integer());fsig=Signature(0,integer(),types,1,0,0,0)
   try:
    signatures=[data(c,'hostvalue',value,integer(4)),data(c,'tiny',tiny,integer(1)),data(c,'wide',wide,integer(2,1)),data(c,'hostptr',ptr,pointer())]
    assert not L.us_add_symbol(c,b'hostplus',ctypes.cast(cb,P),ctypes.byref(fsig))
    compiled(c,SOURCE,index%3)
    fn=L.us_sym(c,b'check');assert fn,L.us_error(c);fn=ctypes.CFUNCTYPE(ctypes.c_long,ctypes.c_long)(fn)
    assert fn(5)==65113+index
    assert (value.value,tiny.value,wide.value,pointee.value)==(16+index,-4,65001,93)
    assert fn(1)==65116+index
    assert (value.value,tiny.value,wide.value,pointee.value)==(17+index,-5,65002,95)
    assert calls==[16+index,17+index]
   finally:L.us_free(c)
  with concurrent.futures.ThreadPoolExecutor(max_workers=4) as ex:assert list(ex.map(probe,range(4)))==[None]*4
  # A real source definition supersedes an earlier extern and injected address.
  host=ctypes.c_int(99);c=L.us_new(pkg);assert c
  try:
   data(c,'hostvalue',host,integer(4));compiled(c,b'extern int hostvalue;int read(void){return hostvalue;}int hostvalue=7;',0)
   fn=L.us_sym(c,b'read');assert fn,L.us_error(c);assert ctypes.CFUNCTYPE(ctypes.c_int)(fn)()==7;assert host.value==99
  finally:L.us_free(c)
  high_depth=pointer();high_depth.depth=(1<<32)+1
  for tag,t,writable in [('wrong-width',integer(8),1),('wrong-kind',pointer(),1),('readonly',integer(4),0),('high-depth',high_depth,1)]:
   c=L.us_new(pkg);assert c
   try:
    obj=ctypes.c_ulonglong(99);data(c,'hostvalue',obj,t,writable)
    source=b'extern int *hostvalue;int read(void){return *hostvalue;}' if tag=='high-depth' else b'extern int hostvalue;int read(void){return hostvalue;}'
    assert not L.us_add_source(c,b'bad.c',source)
    assert L.us_compile(c,target.encode(),0)!=0,(tag,'invalid binding accepted');assert L.us_error(c)
   finally:L.us_free(c)
  print(json.dumps({'scope':target+' borrowed scalar/data-pointer objects','contexts':4,'optimisation_levels':[0,1,2],'actual_host_mutation':True,'mixed_function_and_data':True,'address_alias':True,'source_definition_precedes_injection':True,'invalid_abi_and_readonly_rejected':True,'package_sha256':hashlib.sha256(pathlib.Path(a.package).read_bytes()).hexdigest(),'source_sha256':hashlib.sha256(SOURCE).hexdigest()}))
if __name__=='__main__':main()

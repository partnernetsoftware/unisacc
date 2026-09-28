"""Real source -> model-declared imports -> host ABI calls, private contexts."""
import argparse,ctypes,concurrent.futures,pathlib,platform,shutil,subprocess,tempfile,json,hashlib
ROOT=pathlib.Path(__file__).resolve().parents[1]
P=ctypes.c_void_p;Q=ctypes.c_uint64
class Type(ctypes.Structure):
 _fields_=[(n,Q) for n in ('depth','base','shape','kind','width','uns')]
class Signature(ctypes.Structure):
 _fields_=[('kind',ctypes.c_uint),('result',Type),('args',ctypes.POINTER(Type)),('count',ctypes.c_size_t),('variadic',ctypes.c_uint),('extent',Q),('writable',ctypes.c_uint)]
def integer(width=8,uns=0):return Type(0,0,0,1,width,uns)
def pointer():return Type(1,0,0,2,8,0)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--package',required=True);a=ap.parse_args()
 arch='arm64' if platform.machine() in ('arm64','aarch64') else 'x86_64';target=('osx/' if platform.system()=='Darwin' else 'lnx/')+arch
 with tempfile.TemporaryDirectory(prefix='r10-host-injection-') as folder:
  td=pathlib.Path(folder);runtime=td/'exec/c';runtime.mkdir(parents=True);(td/'src').mkdir()
  for p in (ROOT/'exec/c').iterdir():
   if p.is_file() and p.suffix in ('.h','.c','.S'):shutil.copy2(p,runtime/p.name)
  shutil.copy2(ROOT/'src/host_dl.h',td/'src/host_dl.h')
  lib=td/'library.dylib';subprocess.run(['cc','-std=c11','-O2','-shared','-fPIC','-fvisibility=hidden',str(runtime/'libunisacc.c'),str(runtime/('librarycall_'+arch+'.S')),'-lffi','-o',str(lib)],check=True,timeout=20)
  L=ctypes.CDLL(str(lib));L.us_new.argtypes=[ctypes.c_char_p];L.us_new.restype=P
  L.us_add_symbol.argtypes=[P,ctypes.c_char_p,P,ctypes.POINTER(Signature)]
  L.us_add_source.argtypes=[P,ctypes.c_char_p,ctypes.c_char_p];L.us_compile.argtypes=[P,ctypes.c_char_p,ctypes.c_int]
  L.us_relocate.argtypes=[P];L.us_free.argtypes=[P];L.us_sym.argtypes=[P,ctypes.c_char_p];L.us_sym.restype=P
  L.us_error.argtypes=[P];L.us_error.restype=ctypes.c_char_p
  L.us_call_status.argtypes=[P,ctypes.POINTER(ctypes.c_int)]
  source=b'long hostadd(long a,long b); int hostmixed(signed char a,unsigned short b,int c,int *p); void hostnote(int n);long hostapply(long n);long hostrecover(void);long transform(long n){return n*3+1;}int die(void){__exit(37);return 9;}long answer(void){int p=9;hostnote(17);return hostadd(10,20)+hostmixed(-3,65000,-20,&p)+hostapply(7)+hostrecover();}'
  def probe(index):
   c=L.us_new(str(pathlib.Path(a.package).resolve()).encode());assert c;seen=[];nested={};callback_errors=[];fail_nested=[False]
   cbadd=ctypes.CFUNCTYPE(ctypes.c_long,ctypes.c_long,ctypes.c_long)(lambda x,y:x+y+index)
   cbmixed=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_byte,ctypes.c_ushort,ctypes.c_int,ctypes.POINTER(ctypes.c_int))(lambda x,y,z,p:x+y+z+p[0])
   cbnote=ctypes.CFUNCTYPE(None,ctypes.c_int)(lambda n:seen.append(n))
   def apply(n):
    try:return nested['transform'](n)
    except BaseException as e:callback_errors.append(str(e));return -999
   def recover():
    try:
     if fail_nested[0]:assert nested['die']()==0
     # A later successful callback cannot clear the first failure. Public call
     # status is published only when the outer invocation returns.
     assert nested['transform'](5)==16
     return 17
    except BaseException as e:callback_errors.append(str(e));return -999
   cbapply=ctypes.CFUNCTYPE(ctypes.c_long,ctypes.c_long)(apply)
   cbrecover=ctypes.CFUNCTYPE(ctypes.c_long)(recover)

   def bind(name,cb,result,args):
    types=(Type*len(args))(*args);sig=Signature(0,result,types,len(args),0,0,0)
    rc=L.us_add_symbol(c,name.encode(),ctypes.cast(cb,P),ctypes.byref(sig));assert not rc,L.us_error(c)
    return types,sig
   try:
    declarations=[bind('hostadd',cbadd,integer(),[integer(),integer()]),bind('hostmixed',cbmixed,integer(4),[integer(1),integer(2,1),integer(4),pointer()]),bind('hostnote',cbnote,Type(),[integer(4)]),bind('hostapply',cbapply,integer(),[integer()]),bind('hostrecover',cbrecover,integer(),[])]
    assert L.us_add_symbol(c,b'hostadd',ctypes.cast(cbadd,P),ctypes.byref(declarations[0][1])) and b'duplicate' in L.us_error(c)
    assert not L.us_add_source(c,b'import.c',source)
    assert not L.us_compile(c,target.encode(),index%3),L.us_error(c)
    assert not L.us_relocate(c),L.us_error(c)
    addr=L.us_sym(c,b'answer');assert addr,L.us_error(c)
    answer=ctypes.CFUNCTYPE(ctypes.c_long)(addr)
    nested['transform']=ctypes.CFUNCTYPE(ctypes.c_long,ctypes.c_long)(L.us_sym(c,b'transform'))
    nested['die']=ctypes.CFUNCTYPE(ctypes.c_int)(L.us_sym(c,b'die'))
    assert answer()==65055+index and answer()==65055+index
    fail_nested[0]=True
    assert answer()==0,'failed nested call committed a normal result'
    status=ctypes.c_int()
    assert L.us_call_status(c,ctypes.byref(status))==1 and status.value==37,L.us_error(c)
    fail_nested[0]=False
    assert answer()==65055+index,'new invocation did not recover'
    assert L.us_call_status(c,ctypes.byref(status))==0 and status.value==0 and not L.us_error(c)
    assert not callback_errors,callback_errors
    assert seen==[17,17,17,17],'script did not actually execute native host callback'
    return index
   finally:L.us_free(c)
  with concurrent.futures.ThreadPoolExecutor(max_workers=4) as ex:assert list(ex.map(probe,range(4)))==list(range(4))
  print(json.dumps({'platform':target,'contexts':4,'source_to_native_imports':True,'integer_widths_pointer_void':True,'calls_observed_per_context':4,'nested_native_script_callbacks':True,'nested_exit_sticky_until_outer_return':True,'new_invocation_recovery':True,'scope':'explicit fixed native GP declarations and callbacks via us_sym; no data/FP/variadic imports claimed','package_sha256':hashlib.sha256(pathlib.Path(a.package).read_bytes()).hexdigest(),'native_library_sha256':hashlib.sha256(lib.read_bytes()).hexdigest(),'source_sha256':hashlib.sha256(source).hexdigest()}))
if __name__=='__main__':main()

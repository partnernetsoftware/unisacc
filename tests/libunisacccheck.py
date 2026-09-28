import ctypes, concurrent.futures, subprocess, tempfile, pathlib, argparse, shutil, sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
def main():
 global L,pkg
 ap=argparse.ArgumentParser();ap.add_argument('--package',required=True);a=ap.parse_args()
 with tempfile.TemporaryDirectory(prefix='unisacc-lib-context-') as name:
  td=pathlib.Path(name);runtime=td/'runtime';runtime.mkdir()
  for p in (ROOT/'exec/c').iterdir():
   if p.is_file() and p.suffix in ('.c','.h'):shutil.copyfile(p,runtime/p.name)
  lib=td/'library.dylib';candidate=td/'candidate.com';shutil.copy2(a.package,candidate)
  subprocess.run(['cc','-std=c11','-O2','-fvisibility=hidden','-shared','-fPIC',str(runtime/'libunisacc.c'),'-o',str(lib)],check=True,timeout=30)
  L=ctypes.CDLL(str(lib));pkg=str(candidate).encode()
  check(candidate,td)
def check(candidate,td):
 L.us_new.argtypes=[ctypes.c_char_p];L.us_new.restype=ctypes.c_void_p
 for n,args in [('us_add_source',[ctypes.c_void_p,ctypes.c_char_p,ctypes.c_char_p]),('us_compile',[ctypes.c_void_p,ctypes.c_char_p,ctypes.c_int]),('us_define',[ctypes.c_void_p,ctypes.c_char_p])]:getattr(L,n).argtypes=args
 L.us_free.argtypes=[ctypes.c_void_p];L.us_error.argtypes=[ctypes.c_void_p];L.us_error.restype=ctypes.c_char_p
 L.us_tape.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_size_t)];L.us_tape.restype=ctypes.c_void_p
 def probe(k):
  c=L.us_new(pkg);assert c
  try:
   assert not L.us_define(c,('K='+str(k)).encode())
   assert not L.us_add_source(c,b'probe.c',b'int main(void){return K;}')
   for _ in range(3):
    rc=L.us_compile(c,b'osx/arm64',0);assert not rc,L.us_error(c)
    n=ctypes.c_size_t();p=L.us_tape(c,ctypes.byref(n));out=ctypes.string_at(p,n.value)
    assert out
   return out
  finally:L.us_free(c)
 with concurrent.futures.ThreadPoolExecutor(max_workers=4) as ex: outs=list(ex.map(probe,range(4)))
 assert len(set(outs))==4
 for k,out in enumerate(outs):
  src=td/('reference'+str(k)+'.c');src.write_text('int main(void){return K;}')
  r=subprocess.run(['sh',str(candidate),'-b','osx/arm64','-D','K='+str(k),'-S',str(src),'-o','-'],capture_output=True,timeout=10)
  assert r.returncode==0 and r.stdout==out,('reference',k,r.returncode,r.stderr)
 c=L.us_new(pkg);assert not L.us_add_source(c,b'bad.c',b'int main( {')
 assert L.us_compile(c,b'osx/arm64',0)!=0 and L.us_error(c)
 L.us_free(c)
 c=L.us_new(b'/tmp/absent-r10-library-package');assert not L.us_add_source(c,b'probe.c',b'int main(void){return 0;}')
 assert L.us_compile(c,b'osx/arm64',0)!=0 and L.us_error(c)
 L.us_free(c)
 assert probe(0)==outs[0]
 print('lib prototype: 4 concurrent contexts, 12 compilations, reject and IO error recovery: ok')

if __name__=="__main__": main()

"""Actual fixed-width resource encoding plus native and Windows COFF memory ABI.
Cross compilation is not Windows execution or full library qualification.
"""
import argparse,hashlib,json,pathlib,shutil,subprocess,tempfile,time
ROOT=pathlib.Path(__file__).resolve().parents[1]
BODY=r'''
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
typedef struct {unsigned char *b;int n;} Buf;
static void die(const char*s){fprintf(stderr,"%s\n",s);exit(2);}
#include "memory.c"
typedef char map_word_must_be_64[sizeof(((MemoryMap*)0)->size)==8?1:-1];
int main(void){
 uint64_t values[]={0,1,UINT64_C(0x80000000),UINT64_C(0xffffffff),UINT64_C(0x100000001),UINT64_C(0x123456789abcdef0),UINT64_MAX};
 unsigned char bytes[8];
 for(unsigned i=0;i<sizeof(values)/sizeof(values[0]);i++){
  resource_u64(bytes,values[i]);uint64_t got=0;
  for(int j=7;j>=0;j--)got=(got<<8)|bytes[j];
  if(got!=values[i])return 10+i;
 }
 MemoryMap mapping={0};MemoryImage image={1,8,0,0};
 memory_reserve(&mapping);memory_commit(&image,&mapping);
 if(mapping.size!=32768 || mapping.dataoff!=16384 || !mapping.base)return 30;
 if(memory_protect_code(&mapping,1))return 31;
#ifdef _WIN32
 if(!VirtualFree(mapping.base,0,MEM_RELEASE))return 32;
#else
 if(munmap(mapping.base,(size_t)mapping.reserved))return 32;
#endif
 puts("word64 and host reserve/commit/protect/release: ok");return 0;
}
'''
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--zig',default=shutil.which('zig'));ap.add_argument('--evidence');ap.add_argument('--native-only',action='store_true');a=ap.parse_args();logs=[]
 with tempfile.TemporaryDirectory(prefix='r10-word64-') as folder:
  td=pathlib.Path(folder);shutil.copy2(ROOT/'exec/c/memory.c',td/'memory.c');src=td/'check.c';src.write_text(BODY)
  def cmd(args):
   t=time.monotonic();r=subprocess.run(list(map(str,args)),capture_output=True,timeout=25)
   rec={'argv':list(map(str,args)),'rc':r.returncode,'seconds':time.monotonic()-t,'stdout':r.stdout.decode(errors='replace'),'stderr':r.stderr.decode(errors='replace')};logs.append(rec);assert r.returncode==0,rec
  cmd(['cc','-std=c99','-O2',src,'-o',td/'native']);cmd([td/'native'])
  for target in (() if a.native_only else ('x86_64-windows-gnu','aarch64-windows-gnu')):
   if not a.zig:raise RuntimeError('zig unavailable: Windows compile evidence required')
   cmd([a.zig,'cc','-target',target,'-O2','-c',src,'-o',td/(target+'.obj')]);assert (td/(target+'.obj')).stat().st_size>0
  data={'scope':'native memory mechanism and fixed64 wire; two Windows COFF only, no guest execution','memory_source_sha256':hashlib.sha256((ROOT/'exec/c/memory.c').read_bytes()).hexdigest(),'resource_vectors':7,'windows_cross_compiled':not a.native_only,'commands':logs}
  raw=json.dumps(data,indent=2)+'\n'
  if a.evidence:pathlib.Path(a.evidence).write_text(raw)
  print(raw)
if __name__=='__main__':main()

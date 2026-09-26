#!/usr/bin/env python3
"""Exercise the production PE import enumerator on a mapped-image fixture.
The addresses are simulated; this test does not execute Windows instructions.
"""
import pathlib,struct,subprocess,sys
p=pathlib.Path(sys.argv[1]);exe=pathlib.Path(sys.argv[2]);ua=sys.argv[3]
root=pathlib.Path(__file__).resolve().parents[2]
raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];opt=pe+24
assert raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,opt)[0]==0x20b
size,headers=struct.unpack_from('<II',raw,opt+56);data=bytearray(size);data[:headers]=raw[:headers]
ns=struct.unpack_from('<H',raw,pe+6)[0];at=opt+struct.unpack_from('<H',raw,pe+20)[0]
for i in range(ns):
 vsize,rva,n,off=struct.unpack_from('<IIII',raw,at+i*40+8);data[rva:rva+n]=raw[off:off+n]
def u32(at):return struct.unpack_from('<I',data,at)[0]
def z(at):return data[at:data.index(0,at)].decode()
d=u32(opt+120);wanted=[];first=d
while u32(d+12):
 dll=z(u32(d+12)).lower();ilt,iat=u32(d),u32(d+16);i=0
 while struct.unpack_from('<Q',data,ilt+i*8)[0]:
  name=z(u32(ilt+i*8)+2);v=0x7fff12340000+len(wanted)*16
  struct.pack_into('<Q',data,iat+i*8,v);wanted.append(f'process/import/{dll}/{name} {v}\n');i+=1
 d+=20
assert wanted
(p/'process.mem').write_bytes(data)
src=p/'process.c';src.write_text('''#define UNISA_RUNTIME_LIBRARY
#include "run.c"
#include "winprocess.c"
int main(int n,char **v){ int size; if(n!=2)return 1;
 unsigned char *p=readfile(v[1],&size,0); ResourceInput items[256];
 int count=process_imports(items,256,p,size);
 for(int i=0;i<count;i++){ fwrite(items[i].name+1,1,items[i].n-1,stdout);
 long value=process_u32(items[i].data)|(process_u32(items[i].data+4)<<32);
 printf(" %ld\\n",value); } return 0; }
''')
for compiler,name in [('cc','process-cc'),(ua,'process-ua')]:
 r=subprocess.run([compiler,'-O2','-I'+str(root/'exec/c'),str(src),'-o',str(p/name)],capture_output=True,timeout=60);assert r.returncode==0,r.stderr
 def check(path):return subprocess.run([str(p/name),str(path)],capture_output=True,timeout=60)
 r=check(p/'process.mem');assert r.returncode==0 and r.stdout==''.join(wanted).encode(),(name,r.returncode,r.stdout,r.stderr)
 bad=bytearray(data);struct.pack_into('<I',bad,first+12,size);(p/'bad-process.mem').write_bytes(bad)
 r=check(p/'bad-process.mem');assert r.returncode==2 and b'process import outside image' in r.stderr,(name,r.returncode,r.stderr)
 print(name,len(wanted),'named imports exact; out-of-image name refused (SIMULATED)',flush=True)

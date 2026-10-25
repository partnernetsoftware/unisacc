#!/usr/bin/env python3
"""Independent USBIND1 decoding and bounded native registry guard controls.
No C-source parser or model import support is claimed by this host format check.
"""
import hashlib,json,pathlib,struct,subprocess,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
SOURCE=r'''
#include "librarybindings.h"
static long demo(long x,long *p){return x+*p;}
static void nothing(void){}
#define OK(E) do{if(!(E)){fprintf(stderr,"failed line %d: %s\n",__LINE__,error);return 1;}}while(0)
int main(void){us_bindings b={0};char error[256];us_binding_type i={0,999,456,1,8,0},p={1,777,0,2,8,0},v={0,0,0,0,0,0},f={0,8,0,3,8,0},a={0,9,123,5,16,0};int data=7;
 us_binding_type args[]={i,p};
 OK(!us_bindings_add_function(&b,"demo",(uintptr_t)&demo,&i,args,2,0,error,sizeof error));
 OK(!us_bindings_add_function(&b,"voidfn",(uintptr_t)&nothing,&v,NULL,0,0,error,sizeof error));
 OK(!us_bindings_add_function(&b,"floating",(uintptr_t)&demo,&f,&f,1,0,error,sizeof error));
 OK(!us_bindings_add_function(&b,"aggregate",(uintptr_t)&demo,&a,&a,1,0,error,sizeof error));
 OK(!us_bindings_add_function(&b,"variadic",(uintptr_t)&demo,&i,&i,1,1,error,sizeof error));
 us_binding_type many[7];for(unsigned j=0;j<7;j++)many[j]=i;
 OK(!us_bindings_add_function(&b,"many",(uintptr_t)&demo,&i,many,7,0,error,sizeof error));
 us_binding_type it={0,4,0,1,4,0};OK(!us_bindings_add_data(&b,"data",(uintptr_t)&data,&it,sizeof data,1,error,sizeof error));
 size_t count=b.count;OK(us_bindings_add_function(&b,"demo",(uintptr_t)&demo,&i,args,2,0,error,sizeof error));OK(b.count==count);
 OK(us_bindings_add_data(&b,"demo",(uintptr_t)&data,&it,sizeof data,1,error,sizeof error));OK(b.count==count);
 OK(us_bindings_add_function(&b,"bad-name",(uintptr_t)&demo,&i,args,2,0,error,sizeof error));
 OK(us_bindings_add_function(&b,"nil",0,&i,NULL,0,0,error,sizeof error));
 OK(us_bindings_add_function(&b,"voidarg",(uintptr_t)&demo,&i,&v,1,0,error,sizeof error));
 OK(us_bindings_add_function(&b,"badvar",(uintptr_t)&demo,&i,NULL,0,2,error,sizeof error));
 OK(us_bindings_add_function(&b,"huge",(uintptr_t)&demo,&i,NULL,1025,0,error,sizeof error));
 us_binding_type bad=i;bad.width=3;OK(us_bindings_add_function(&b,"width",(uintptr_t)&demo,&bad,NULL,0,0,error,sizeof error));bad=i;bad.uns=2;OK(us_bindings_add_function(&b,"uns",(uintptr_t)&demo,&bad,NULL,0,0,error,sizeof error));bad=i;bad.kind=7;OK(us_bindings_add_function(&b,"class",(uintptr_t)&demo,&bad,NULL,0,0,error,sizeof error));
 OK(us_bindings_add_data(&b,"shortdata",(uintptr_t)&data,&it,3,1,error,sizeof error));
 OK(us_bindings_add_data(&b,"overflow",UINTPTR_MAX-2,&it,4,1,error,sizeof error));
 OK(us_bindings_add_data(&b,"badwrite",(uintptr_t)&data,&it,4,2,error,sizeof error));
 OK(b.count==count);
 unsigned char *wire;size_t n;OK(!us_bindings_serialize(&b,&wire,&n,error,sizeof error));OK(fwrite(wire,1,n,stdout)==n);free(wire);
 us_bindings_clear(&b);OK(!b.items&&!b.count);
 OK(!us_bindings_serialize(&b,&wire,&n,error,sizeof error));OK(n==16);free(wire);return 0;
}
'''
def decode(data):
 assert data[:8]==b'USBIND1\n';at=8
 def q():
  nonlocal at
  assert at<=len(data)-8;n=struct.unpack_from('<Q',data,at)[0];at+=8;return n
 def desc():return tuple(q() for _ in range(6))
 count=q();records={}
 for _ in range(count):
  rn=q();end=at+rn;assert end<=len(data);nn=q();name=data[at:at+nn].decode('ascii');at+=nn
  kind,origin,abi,var=data[at:at+4];at+=4;address=q();nargs=q();result=desc();params=[desc() for _ in range(nargs)]
  extra=None
  if kind==1:extra=(q(),data[at]);at+=1
  supported=data[at];at+=1;assert at==end and name not in records
  records[name]={'kind':kind,'origin':origin,'abi':abi,'variadic':var,'address':address,'result':result,'params':params,'data':extra,'supported':supported}
 assert at==len(data);return records
def main():
 with tempfile.TemporaryDirectory(prefix='r10-bindings-check-') as folder:
  d=pathlib.Path(folder);(d/'librarybindings.h').write_bytes((ROOT/'exec/c/librarybindings.h').read_bytes());(d/'check.c').write_text(SOURCE);exe=d/'check'
  subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-I',d,d/'check.c','-o',exe],check=True,capture_output=True,timeout=20)
  r=subprocess.run([exe],check=True,capture_output=True,timeout=10);records=decode(r.stdout)
  assert set(records)=={'demo','voidfn','floating','aggregate','variadic','many','data'}
  assert all(x['address'] and x['origin']==0 and x['abi']==0 for x in records.values())
  assert records['demo']['supported']==records['voidfn']['supported']==1
  assert records['demo']['result']==(0,999,456,1,8,0) and records['demo']['params'][1]==(1,777,0,2,8,0)
  assert all(records[n]['supported']==0 for n in ('floating','aggregate','variadic','many'))
  assert records['variadic']['variadic']==1 and len(records['many']['params'])==7
  assert records['aggregate']['result']==(0,9,123,5,16,0)
  assert records['data']['kind']==1 and records['data']['data']==(4,1)
  for bad in [r.stdout[:-1],r.stdout+b'X']:
   try:decode(bad)
   except (AssertionError,IndexError,struct.error):pass
   else:raise AssertionError('damaged fixture decoded')
  print(json.dumps({'registry_records':len(records),'wire_bytes':len(r.stdout),'unsupported_descriptors_preserved':True,'duplicates_invalid_abi_null_and_data_bounds_rejected':True,'clear_and_empty_wire':True,'header_sha256':hashlib.sha256((d/'librarybindings.h').read_bytes()).hexdigest()}))
if __name__=='__main__':main()

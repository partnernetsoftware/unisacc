/* Mechanical explicit carrier certificates, not model/public acceptance. */
#define US_CALLABLES_IMPLEMENTATION
#include "librarycallables.h"
#include <assert.h>
typedef union Mixed { double d; uint64_t u; } Mixed;
typedef union Floating { double d; } Floating;
typedef struct Owner { int native_fail,script_fail,failures,calls; } Owner;
static Mixed mixed(Mixed x){x.u+=17;return x;}
static Mixed exhausted(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h,uint64_t i,Mixed x){x.u+=a+b+c+d+e+f+g+h+i;return x;}
static Floating floating(Floating x){x.d+=1.25;return x;}
static int native(void *owner,ffi_cif *cif,uintptr_t target,void *result,void **args){Owner *o=owner;ffi_call(cif,FFI_FN(target),result,args);return o->native_fail;}
static void failed(void *owner,const char *message){((Owner*)owner)->failures++;assert(message&&*message);}
static int script(void *owner,const void *raw,const us_export_signature *sig,const us_export_frame *f){
 Owner *o=owner;o->calls++;assert(sig->result.kind==5&&f->result_kind==5&&f->result_bytes==8&&f->count==sig->count);
 Mixed x;memcpy(&x,(void*)(uintptr_t)f->slots[f->count-1],8);
 if((uintptr_t)raw==2)x.d+=1.25;else{x.u+=17;for(size_t i=0;i+1<f->count;i++)x.u+=f->slots[i];}
 memset((void*)(uintptr_t)f->slots[f->count-1],0x55,8); /* own incoming copy */
 memcpy(f->result,&x,8);return o->script_fail;
}
static us_export_type scalar(unsigned kind){us_export_type t={0};t.kind=kind;t.width=t.alignment=8;t.uns=kind==1;return t;}
static us_export_signature sig(us_export_type result,us_export_type *args,size_t n){us_export_signature s={0};s.result=result;s.argtypes=args;s.count=s.stored=n;s.mode=1;return s;}
int main(void){
 Owner owner={0};us_callables r;us_callables_init(&r,&owner,9,NULL,native,failed);r.script_call=script;char error[256]={0};
 us_export_type I=scalar(1),D=scalar(3),U={0};us_export_member members[2]={{0}};
 U.kind=5;U.width=U.alignment=8;U.tag=2;U.nmembers=2;U.members=members;members[0].type=&D;members[1].type=&I;members[0].storage=members[1].storage=8;
 us_export_type F=U;F.nmembers=1;
 us_export_signature original=sig(U,&U,1),carrier=sig(I,&I,1),float_original=sig(F,&F,1),float_carrier=sig(D,&D,1);
 uint64_t hn=0,hs=0,hf=0,hfs=0,h2=0;assert(!us_callable_make_carrier(&r,US_CALLABLE_NATIVE,&original,&carrier,(uintptr_t)mixed,&hn,error,sizeof error));
 assert(!us_callable_make_carrier(&r,US_CALLABLE_SCRIPT,&original,&carrier,1,&hs,error,sizeof error));
 assert(!us_callable_make_carrier(&r,US_CALLABLE_NATIVE,&float_original,&float_carrier,(uintptr_t)floating,&hf,error,sizeof error));
 assert(!us_callable_make_carrier(&r,US_CALLABLE_SCRIPT,&float_original,&float_carrier,2,&hfs,error,sizeof error));
 assert(!us_callable_make_carrier(&r,US_CALLABLE_NATIVE,&original,&carrier,(uintptr_t)mixed,&h2,error,sizeof error)&&h2==hn);
 /* Distinct explicit certificates remain distinct; this is not ABI approval. */
 assert(!us_callable_make_carrier(&r,US_CALLABLE_NATIVE,&original,&float_carrier,(uintptr_t)mixed,&h2,error,sizeof error)&&h2!=hn);
 uint64_t different_original=0;assert(!us_callable_make_carrier(&r,US_CALLABLE_NATIVE,&float_original,&carrier,(uintptr_t)mixed,&different_original,error,sizeof error)&&different_original!=hn);
 /* Deliberately unmatched fixture is never invoked; C does not certify ABI. */
 assert(us_callable_find(&r,hn)->cif.arg_types[0]==&ffi_type_uint64);
 assert(us_callable_find(&r,hn)->opaque_result&&us_callable_find(&r,hn)->opaque_args[0]);
 assert(us_callable_find(&r,hf)->cif.arg_types[0]==&ffi_type_double);
 assert(us_callable_find(&r,h2)->cif.arg_types[0]==&ffi_type_double);
 void *code=NULL;assert(!us_callable_pointer(&r,hs,&original,&code,error,sizeof error));Mixed (*cb)(Mixed)=(void*)code;
 void *float_code=NULL;assert(!us_callable_pointer(&r,hfs,&float_original,&float_code,error,sizeof error));Floating (*fcb)(Floating)=(void*)float_code;
 us_export_type oa[10],ca[10];for(int i=0;i<9;i++)oa[i]=ca[i]=I;oa[9]=U;ca[9]=I;
 us_export_signature many=sig(U,oa,10),many_carrier=sig(I,ca,10);uint64_t hg=0,hgs=0;
 assert(!us_callable_make_carrier(&r,US_CALLABLE_NATIVE,&many,&many_carrier,(uintptr_t)exhausted,&hg,error,sizeof error));
 assert(!us_callable_make_carrier(&r,US_CALLABLE_SCRIPT,&many,&many_carrier,1,&hgs,error,sizeof error));
 void *gcode=NULL;assert(!us_callable_pointer(&r,hgs,&many,&gcode,error,sizeof error));
 Mixed (*gcb)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,Mixed)=(void*)gcode;
 for(int n=0;n<100;n++){
  struct {uint64_t left;Mixed value;uint64_t right;} in={0xaabbccdd,{.u=0x10203040u+(unsigned)n},0xeeff0011},out={0x11223344,{.u=0},0x55667788};
  uint64_t slot=(uintptr_t)&in.value;assert(!us_callable_call(&r,hn,&original,&slot,&out.value,1,error,sizeof error));assert(out.value.u==in.value.u+17);
  Mixed back=cb(in.value);assert(back.u==in.value.u+17);
  uint64_t slots[10];for(int i=0;i<9;i++)slots[i]=i+1;slots[9]=(uintptr_t)&in.value;
  assert(!us_callable_call(&r,hg,&many,slots,&out.value,10,error,sizeof error));assert(out.value.u==in.value.u+45);
  back=gcb(1,2,3,4,5,6,7,8,9,in.value);assert(back.u==in.value.u+62);
  assert(in.value.u==0x10203040u+(unsigned)n&&in.left==0xaabbccdd&&in.right==0xeeff0011&&out.left==0x11223344&&out.right==0x55667788);
  Floating fi={.d=2.5},fo={0};slot=(uintptr_t)&fi;assert(!us_callable_call(&r,hf,&float_original,&slot,&fo,1,error,sizeof error)&&fo.d==3.75);assert(fcb(fi).d==3.75&&fi.d==2.5);
 }
 /* Failed calls do not commit native results; callback failures return ABI zero. */
 Mixed x={.u=91},y={.u=0xdeadbeef};uint64_t slot=(uintptr_t)&x;owner.native_fail=1;
 assert(us_callable_call(&r,hn,&original,&slot,&y,1,error,sizeof error)&&y.u==0xdeadbeef);owner.native_fail=0;
 owner.script_fail=1;assert(cb(x).u==0&&owner.failures==0);owner.script_fail=0;assert(cb(x).u==108);
 us_callable *head=r.head;uint64_t token=atomic_load(&us_callable_global_token);us_export_type bad=I;bad.width=4;us_export_signature invalid=sig(bad,&I,1);h2=99;
 assert(us_callable_make_carrier(&r,US_CALLABLE_NATIVE,&original,&invalid,(uintptr_t)mixed,&h2,error,sizeof error)&&!h2&&r.head==head&&atomic_load(&us_callable_global_token)==token);
 invalid=carrier;invalid.variadic=1;assert(us_callable_make_carrier(&r,US_CALLABLE_NATIVE,&original,&invalid,(uintptr_t)mixed,&h2,error,sizeof error)&&r.head==head);
 invalid=carrier;invalid.count=invalid.stored=0;assert(us_callable_make_carrier(&r,US_CALLABLE_NATIVE,&original,&invalid,(uintptr_t)mixed,&h2,error,sizeof error)&&r.head==head);
 assert(us_callable_make_carrier(&r,US_CALLABLE_NATIVE,&carrier,&float_carrier,(uintptr_t)mixed,&h2,error,sizeof error)&&r.head==head&&atomic_load(&us_callable_global_token)==token); /* scalar reclassification forbidden */
 invalid=carrier;invalid.mode=0;assert(us_callable_make_carrier(&r,US_CALLABLE_NATIVE,&original,&invalid,(uintptr_t)mixed,&h2,error,sizeof error)&&r.head==head);
 invalid=carrier;bad=I;bad.alignment=4;invalid.result=bad;assert(us_callable_make_carrier(&r,US_CALLABLE_NATIVE,&original,&invalid,(uintptr_t)mixed,&h2,error,sizeof error)&&r.head==head);
 assert(us_callable_make_carrier(&r,US_CALLABLE_NATIVE,&original,&original,(uintptr_t)mixed,&h2,error,sizeof error)&&r.head==head&&atomic_load(&us_callable_global_token)==token);
 us_export_type pointer={0},opaque=U;us_export_member unsafe_members[2];memcpy(unsafe_members,members,sizeof members);
 pointer.kind=4;pointer.depth=1;pointer.width=pointer.alignment=8;pointer.tag=4;pointer.signature=&carrier;unsafe_members[1].type=&pointer;opaque.members=unsafe_members;
 us_export_signature unsafe=sig(opaque,&opaque,1);assert(us_callable_make_carrier(&r,US_CALLABLE_NATIVE,&unsafe,&carrier,(uintptr_t)mixed,&h2,error,sizeof error)&&r.head==head&&atomic_load(&us_callable_global_token)==token);
 assert(us_callable_call(&r,hf,&original,&slot,&y,1,error,sizeof error)&&y.u==0xdeadbeef); /* floating-union identity differs */
 assert(us_callable_make(&r,US_CALLABLE_NATIVE,&original,(uintptr_t)mixed,&h2,error,sizeof error)&&r.head==head);
 uint64_t plain=0;assert(!us_callable_make(&r,US_CALLABLE_NATIVE,&carrier,(uintptr_t)mixed,&plain,error,sizeof error));
 uint64_t primitive=10,primitive_result=0;assert(!us_callable_call(&r,plain,&carrier,&primitive,&primitive_result,1,error,sizeof error)&&primitive_result==27);
 uint64_t roundtrip=0;assert(!us_callable_from_native(&r,&original,code,&roundtrip,error,sizeof error)&&roundtrip==hs);
 /* Owned graphs are independent of subsequent borrowed descriptor changes. */
 U.width=99;I.width=99;assert(us_callable_find(&r,hn)->graph.signatures[0]->result.width==8&&us_callable_find(&r,hn)->carrier_graph.signatures[0]->result.width==8);
 us_callables_clear(&r);assert(!r.head&&!r.generation);
 puts("carrier mechanical: true native mixed/floating unions, script closures, GP exhaustion, canaries, ownership and atomic rejection passed");return 0;
}

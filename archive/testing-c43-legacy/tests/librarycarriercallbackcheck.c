/* Explicit fixture graph pair only: not model/public callback acceptance. */
#define US_CALLABLES_IMPLEMENTATION
#include "librarycallables.h"
#include <assert.h>
typedef union Mixed{double d;uint64_t u;} Mixed;
typedef Mixed (*Leaf)(Mixed,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
typedef struct Box{Leaf fn;Mixed x;} Box;
typedef struct ScriptBox{uint64_t fn;Mixed x;} ScriptBox;
static Leaf retained;
static Mixed leaf_native(Mixed x,uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h){x.u+=a+b+c+d+e+f+g+h;return x;}
static Leaf factory(void){return leaf_native;}
static Leaf echo(Leaf fn){retained=fn;return fn;}
static Mixed apply(Leaf fn,Mixed x){retained=fn;return fn(x,1,2,3,4,5,6,7,8);}
static Box box_native(Box b){b.x=apply(b.fn,b.x);return b;}
typedef struct Owner{us_callables *registry;us_export_signature *leaf;uint64_t handle;int failed,status,calls;} Owner;
static int native(void *owner,ffi_cif *cif,uintptr_t raw,void *result,void **args){ffi_call(cif,FFI_FN(raw),result,args);return ((Owner*)owner)->failed;}
static void failure(void *owner,const char *message){Owner *o=owner;if(!o->status)o->status=1;assert(message);}
static int script(void *owner,const void *raw,const us_export_signature *sig,const us_export_frame *f){
 Owner *o=owner;o->calls++;if(o->failed){o->status=23;return 1;}
 if((uintptr_t)raw==2){assert(sig->result.kind==4&&f->result_kind==4);memcpy(f->result,&o->handle,8);return 0;}
 if((uintptr_t)raw==3){assert(sig->result.kind==5&&f->result_bytes==16);ScriptBox b;memcpy(&b,(void*)(uintptr_t)f->slots[0],16);uint64_t slots[9]={(uintptr_t)&b.x,1,2,3,4,5,6,7,8};char error[128]={0};Mixed out;
  if(us_callable_call(o->registry,b.fn,o->leaf,slots,&out,9,error,sizeof error))return 1;b.x=out;memcpy(f->result,&b,16);return 0;}
 assert(sig->count==9&&sig->result.kind==5&&f->result_kind==5&&f->result_bytes==8);Mixed x;memcpy(&x,(void*)(uintptr_t)f->slots[0],8);x.u+=17;for(int i=1;i<9;i++)x.u+=f->slots[i];memcpy(f->result,&x,8);return 0;
}
static us_export_type integer(void){us_export_type t={0};t.kind=1;t.uns=1;t.width=t.alignment=8;return t;}
static us_export_type callback(us_export_signature *s){us_export_type t={0};t.kind=4;t.depth=1;t.width=t.alignment=8;t.tag=4;t.signature=s;return t;}
static us_export_signature signature(us_export_type result,us_export_type *args,size_t n){us_export_signature s={0};s.mode=1;s.count=s.stored=n;s.result=result;s.argtypes=args;return s;}
int main(void){
 us_export_type I=integer(),D=I,U={0};D.kind=3;D.uns=0;us_export_member um[2]={{0}};um[0].type=&D;um[1].type=&I;um[0].storage=um[1].storage=8;U.kind=5;U.tag=2;U.width=U.alignment=8;U.nmembers=2;U.members=um;
 us_export_type oa[9],ca[9];oa[0]=U;ca[0]=I;for(int i=1;i<9;i++)oa[i]=ca[i]=I;
 us_export_signature leaf=signature(U,oa,9),leaf_carrier=signature(I,ca,9);us_export_type F=callback(&leaf),FC=callback(&leaf_carrier);
 us_export_signature factory_sig=signature(F,NULL,0),factory_carrier=signature(FC,NULL,0),echo_sig=signature(F,&F,1),echo_carrier=signature(FC,&FC,1);
 us_export_type aa[2]={F,U},ac[2]={FC,I};us_export_signature apply_sig=signature(U,aa,2),apply_carrier=signature(I,ac,2);
 us_export_type B={0},BC={0};us_export_member bm[2]={{0}},cm[2]={{0}};B.kind=BC.kind=5;B.tag=BC.tag=1;B.width=BC.width=16;B.alignment=BC.alignment=8;B.nmembers=BC.nmembers=2;B.members=bm;BC.members=cm;
 bm[0].type=&F;cm[0].type=&FC;bm[1].type=&U;cm[1].type=&I;bm[1].offset=cm[1].offset=8;for(int i=0;i<2;i++)bm[i].storage=cm[i].storage=8;
 us_export_signature box_sig=signature(B,&B,1),box_carrier=signature(BC,&BC,1);
 Owner owner={0};us_callables registry;us_callables_init(&registry,&owner,9,NULL,native,failure);registry.script_call=script;owner.registry=&registry;owner.leaf=&leaf;char error[256]={0};uint64_t hf=0,he=0,ha=0,hb=0,hsb=0,hfs=0;
 assert(!us_callable_make_carrier(&registry,US_CALLABLE_NATIVE,&factory_sig,&factory_carrier,(uintptr_t)factory,&hf,error,sizeof error));
 assert(!us_callable_make_carrier(&registry,US_CALLABLE_NATIVE,&echo_sig,&echo_carrier,(uintptr_t)echo,&he,error,sizeof error));
 assert(!us_callable_make_carrier(&registry,US_CALLABLE_NATIVE,&apply_sig,&apply_carrier,(uintptr_t)apply,&ha,error,sizeof error));
 assert(!us_callable_make_carrier(&registry,US_CALLABLE_NATIVE,&box_sig,&box_carrier,(uintptr_t)box_native,&hb,error,sizeof error));
 assert(!us_callable_make_carrier(&registry,US_CALLABLE_SCRIPT,&box_sig,&box_carrier,3,&hsb,error,sizeof error));
 assert(!us_callable_make_carrier(&registry,US_CALLABLE_SCRIPT,&leaf,&leaf_carrier,1,&owner.handle,error,sizeof error));
 assert(!us_callable_make_carrier(&registry,US_CALLABLE_SCRIPT,&factory_sig,&factory_carrier,2,&hfs,error,sizeof error));
 void *factory_code=NULL,*box_code=NULL;assert(!us_callable_pointer(&registry,hfs,&factory_sig,&factory_code,error,sizeof error));Leaf (*script_factory)(void)=(void*)factory_code;
 assert(!us_callable_pointer(&registry,hsb,&box_sig,&box_code,error,sizeof error));Box (*script_box)(Box)=(void*)box_code;
 uint64_t native_leaf=0;assert(!us_callable_call(&registry,hf,&factory_sig,NULL,&native_leaf,0,error,sizeof error));assert(native_leaf&&us_callable_find(&registry,native_leaf)->origin==US_CALLABLE_NATIVE&&us_callable_find(&registry,native_leaf)->carrier_graph.count);
 for(int i=0;i<100;i++){
  struct{uint64_t a;Mixed x;uint64_t b;} in={0x12345678,{.u=91+(unsigned)i},0xabcdef01},out={0x10203040,{.u=0},0x50607080};uint64_t slots[9]={(uintptr_t)&in.x,1,2,3,4,5,6,7,8};
  assert(!us_callable_call(&registry,native_leaf,&leaf,slots,&out.x,9,error,sizeof error)&&out.x.u==in.x.u+36);
  uint64_t app[2]={owner.handle,(uintptr_t)&in.x};assert(!us_callable_call(&registry,ha,&apply_sig,app,&out.x,2,error,sizeof error)&&out.x.u==in.x.u+53);
  uint64_t roundtrip=0;assert(!us_callable_call(&registry,he,&echo_sig,&owner.handle,&roundtrip,1,error,sizeof error)&&roundtrip==owner.handle);
  assert(retained(in.x,1,2,3,4,5,6,7,8).u==in.x.u+53);Leaf cb=script_factory();assert(cb(in.x,1,2,3,4,5,6,7,8).u==in.x.u+53);
  ScriptBox input={owner.handle,in.x},result={0};uint64_t bp=(uintptr_t)&input;assert(!us_callable_call(&registry,hb,&box_sig,&bp,&result,1,error,sizeof error)&&result.fn==owner.handle&&result.x.u==in.x.u+53&&input.x.u==in.x.u);
  Box bx={factory(),in.x},back=script_box(bx);assert(back.fn==bx.fn&&back.x.u==in.x.u+36);
  assert(in.x.u==91+(unsigned)i&&in.a==0x12345678&&in.b==0xabcdef01&&out.a==0x10203040&&out.b==0x50607080);
 }
 void *leaf_code=NULL;assert(!us_callable_pointer_carrier(&registry,owner.handle,&leaf,&leaf_carrier,&leaf_code,error,sizeof error));
 Owner other_owner={0};us_callables other;us_callables_init(&other,&other_owner,10,NULL,native,failure);uint64_t h=77;
 assert(us_callable_from_native_carrier(&other,&leaf,&leaf_carrier,leaf_code,&h,error,sizeof error)&&!h&&!other.head);us_callables_clear(&other);
 us_export_signature wrong=leaf_carrier;wrong.result=D;assert(us_callable_pointer_carrier(&registry,owner.handle,&leaf,&wrong,&leaf_code,error,sizeof error)&&!leaf_code);
 Mixed x={.u=50},out={.u=0xdeadbeef};uint64_t app[2]={owner.handle,(uintptr_t)&x};owner.failed=1;assert(us_callable_call(&registry,ha,&apply_sig,app,&out,2,error,sizeof error)&&out.u==0xdeadbeef&&owner.status==23);owner.failed=0;
 assert(!us_callable_call(&registry,ha,&apply_sig,app,&out,2,error,sizeof error)&&out.u==103&&owner.status==23); /* later success does not erase owner failure */
 us_callable *head=registry.head;uint64_t token=atomic_load(&us_callable_global_token);cm[1].offset=0;
 assert(us_callable_make_carrier(&registry,US_CALLABLE_NATIVE,&box_sig,&box_carrier,(uintptr_t)box_native,&h,error,sizeof error)&&registry.head==head&&atomic_load(&us_callable_global_token)==token);cm[1].offset=8;
 /* Carrier pairing cycles are coinductive, and copied graphs retain both edges. */
 us_export_signature recursive=signature(I,NULL,0),recursive_carrier=recursive;us_export_type self=callback(&recursive),self_carrier=callback(&recursive_carrier);recursive.argtypes=&self;recursive_carrier.argtypes=&self_carrier;recursive.count=recursive.stored=recursive_carrier.count=recursive_carrier.stored=1;
 assert(us_callable_carrier_valid(&recursive,&recursive_carrier));assert(!us_callable_make_carrier(&registry,US_CALLABLE_NATIVE,&recursive,&recursive_carrier,(uintptr_t)echo,&h,error,sizeof error));
 assert(us_callable_find(&registry,h)->graph.signatures[0]->argtypes[0].signature==us_callable_find(&registry,h)->graph.signatures[0]);
 assert(us_callable_find(&registry,h)->carrier_graph.signatures[0]->argtypes[0].signature==us_callable_find(&registry,h)->carrier_graph.signatures[0]);
 us_callables_clear(&registry);assert(!registry.head&&!registry.generation);puts("paired carrier callbacks: native factory/Leaf9, SCRIPT factory+closure, retained roundtrip, struct fields, foreign identity, cycles, canaries and failure passed");return 0;
}

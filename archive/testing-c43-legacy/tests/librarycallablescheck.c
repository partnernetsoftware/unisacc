/* Mechanical bridge only: invoke_frame is a mock, native functions are real. */
#define US_CALLABLES_IMPLEMENTATION
#include "librarycallables.h"
#include <assert.h>
#include <stddef.h>
struct Pair {double d;int n;};
typedef struct Pair (*Leaf)(int,double,float,int*,struct Pair,int,double,int,int);
typedef struct Pair (*Relay)(Leaf);
struct Box {struct Pair p;Leaf leaf;Leaf list[2];};
static us_export_type I,D,F,P,PAIR,LEAF,RELAY,ARRAY,BOX;
static us_export_member pm[2],bm[3];
static us_export_type leafargs[9],relayargs[1],driveargs[2],boxargs[1];
static us_export_signature leafsig,relaysig,drivesig,boxsig;
static int leafcalls,drivereturns,scriptleaf,scriptrelay,scriptfloat;
static Relay retained;
typedef struct Owner {us_callables registry;int fail,sticky,failures,native_depth,declared_calls;unsigned mode;} Owner;
static uint64_t bits(const void *p,size_t n){uint64_t v=0;memcpy(&v,p,n);return v;}
static us_export_type scalar(unsigned kind,unsigned width,unsigned depth){us_export_type t={0};t.kind=kind;t.width=width;t.depth=depth;t.alignment=width;return t;}
static us_export_type fp(us_export_signature *s){us_export_type t=scalar(4,8,1);t.tag=4;t.signature=s;return t;}
static void signature(us_export_signature *s,us_export_type ret,us_export_type *args,unsigned n){memset(s,0,sizeof *s);s->result=ret;s->argtypes=args;s->count=s->stored=n;s->mode=n>6;}
static void fixtures(void){
 I=scalar(1,4,0);D=scalar(3,8,0);F=scalar(3,4,0);P=scalar(2,8,1);
 PAIR=scalar(5,sizeof(struct Pair),0);PAIR.alignment=_Alignof(struct Pair);PAIR.tag=1;PAIR.nmembers=2;PAIR.members=pm;
 pm[0]=(us_export_member){offsetof(struct Pair,d),0,0,8,&D};pm[1]=(us_export_member){offsetof(struct Pair,n),0,0,4,&I};
 us_export_type types[]={I,D,F,P,PAIR,I,D,I,I};memcpy(leafargs,types,sizeof types);signature(&leafsig,PAIR,leafargs,9);LEAF=fp(&leafsig);
 relayargs[0]=LEAF;signature(&relaysig,PAIR,relayargs,1);RELAY=fp(&relaysig);
 driveargs[0]=RELAY;driveargs[1]=LEAF;signature(&drivesig,PAIR,driveargs,2);
 ARRAY=scalar(5,2*sizeof(Leaf),0);ARRAY.alignment=8;ARRAY.tag=3;ARRAY.element=&LEAF;ARRAY.count=2;ARRAY.stride=8;
 BOX=scalar(5,sizeof(struct Box),0);BOX.alignment=_Alignof(struct Box);BOX.tag=1;BOX.nmembers=3;BOX.members=bm;
 bm[0]=(us_export_member){offsetof(struct Box,p),0,0,sizeof(struct Pair),&PAIR};bm[1]=(us_export_member){offsetof(struct Box,leaf),0,0,8,&LEAF};bm[2]=(us_export_member){offsetof(struct Box,list),0,0,16,&ARRAY};
 boxargs[0]=BOX;signature(&boxsig,BOX,boxargs,1);
}
static struct Pair native_leaf(int a,double b,float c,int *p,struct Pair s,int e,double f,int g,int h){leafcalls++;return (struct Pair){b+c+s.d+f,a+*p+s.n+e+g+h};}
static struct Pair native_other(int a,double b,float c,int *p,struct Pair s,int e,double f,int g,int h){struct Pair r=native_leaf(a,b,c,p,s,e,f,g,h);r.d++;r.n+=10;return r;}
static struct Pair drive(Relay r,Leaf l){retained=r;struct Pair p=r(l);drivereturns++;return p;}
static struct Box boxcall(struct Box b){b.p.d+=1;b.list[1]=native_other;return b;}
static int negative(int n){return -n;}
/* This is the boundary owner hook; production must register cleanup and merge
   TLS sticky outcomes here, then return so registry copies can be destroyed. */
static int native_call(void *raw,ffi_cif *cif,uintptr_t target,void *result,void **values){
 Owner *o=raw;o->native_depth++;ffi_call(cif,FFI_FN((void*)target),result,values);o->native_depth--;return o->sticky;
}
static void failure(void *raw,const char *message){Owner *o=raw;o->sticky=1;o->failures++;assert(message&&*message);}
static int invoke(void *raw,const void *target,const us_export_frame *frame){
 Owner *o=raw;char err[256]={0};assert(frame->result_kind==5?frame->result_bytes==sizeof(struct Pair):frame->result_kind?frame->result_bytes==8:frame->result_bytes==0);if(o->fail){o->sticky=23;return 23;}o->mode=frame->mode;
 if(target==&scriptfloat){float f;memcpy(&f,frame->slots,4);f+=1.25f;uint64_t v=bits(&f,4);memcpy(frame->result,&v,8);return 0;}
 if(target==&scriptrelay){
  assert(frame->count==1);int n=12;struct Pair s={4,5};double b=1.5,f=9;float c=2.5;
  uint64_t slots[]={2,bits(&b,8),bits(&c,4),(uintptr_t)&n,(uintptr_t)&s,6,bits(&f,8),7,8};
  return us_callable_call(&o->registry,frame->slots[0],&leafsig,slots,frame->result,9,err,sizeof err);
 }
 assert(target==&scriptleaf && frame->count==9);double b,f;float c;memcpy(&b,frame->slots+1,8);memcpy(&c,frame->slots+2,4);memcpy(&f,frame->slots+6,8);
 struct Pair s=*(struct Pair*)(uintptr_t)frame->slots[4];int n=*(int*)(uintptr_t)frame->slots[3];
 struct Pair r={b+c+s.d+f,(int)frame->slots[0]+n+s.n+(int)frame->slots[5]+(int)frame->slots[7]+(int)frame->slots[8]};memcpy(frame->result,&r,sizeof r);return 0;
}
/* Optional owner hook receives exact owned declarations, so production can
   register incoming by-value copies without accepting arbitrary heap ranges. */
static int declared_invoke(void *raw,const void *target,const us_export_signature *sig,const us_export_frame *frame){
 Owner *o=raw;assert(sig&&sig->count==9&&sig->stored==9&&sig->mode==1&&frame->count==9);
 assert(sig->result.kind==5&&sig->result.width==sizeof(struct Pair));
 assert(sig->argtypes[1].kind==3&&sig->argtypes[1].width==8&&sig->argtypes[2].width==4);
 assert(sig->argtypes[4].kind==5&&sig->argtypes[4].width==sizeof(struct Pair));
 assert(us_callable_signature_equal(sig,&leafsig));o->declared_calls++;return invoke(raw,target,frame);
}
static void init(Owner *o,uint64_t generation){memset(o,0,sizeof *o);us_callables_init(&o->registry,o,generation,invoke,native_call,failure);}
int main(void){
 fixtures();Owner o,other;init(&o,1);init(&other,1);char error[256]={0};uint64_t relay=0,leaf=0,host=0,script=0;
 assert(!us_callable_make(&o.registry,US_CALLABLE_SCRIPT,&relaysig,(uintptr_t)&scriptrelay,&relay,error,sizeof error));
 assert(!us_callable_make(&o.registry,US_CALLABLE_NATIVE,&leafsig,(uintptr_t)native_leaf,&leaf,error,sizeof error));
 assert(!us_callable_make(&o.registry,US_CALLABLE_NATIVE,&drivesig,(uintptr_t)drive,&host,error,sizeof error));
 assert(!us_callable_make(&o.registry,US_CALLABLE_SCRIPT,&leafsig,(uintptr_t)&scriptleaf,&script,error,sizeof error));
 assert(relay&&leaf&&host&&relay!=leaf);assert(PAIR.ffi==NULL&&PAIR.elements==NULL&&PAIR.native.size==0);
 for(int i=0;i<100;i++){
  uint64_t h;assert(!us_callable_make(&o.registry,US_CALLABLE_NATIVE,&leafsig,(uintptr_t)((i&1)?native_other:native_leaf),&h,error,sizeof error));
  uint64_t slots[]={relay,h};struct {uint64_t a;struct Pair p;uint64_t b;} canary={123,{0,0},456};
  assert(!us_callable_call(&o.registry,host,&drivesig,slots,&canary.p,2,error,sizeof error));assert(canary.a==123&&canary.b==456);
  assert(canary.p.d==17+(i&1)&&canary.p.n==40+10*(i&1));assert(retained);
  struct Pair p=retained((i&1)?native_other:native_leaf);assert(p.d==canary.p.d&&p.n==canary.p.n);
 }
 assert(drivereturns==100&&leafcalls==200&&o.native_depth==0);
 void *raw=NULL;assert(!us_callable_pointer(&o.registry,script,&leafsig,&raw,error,sizeof error));Leaf callback=(Leaf)raw;
 int n=12;struct Pair s={4,5};struct Pair p=callback(2,1.5,2.5f,&n,s,6,9,7,8);assert(p.d==17&&p.n==40&&o.mode==1);
 uint64_t round=0;assert(!us_callable_from_native(&o.registry,&leafsig,raw,&round,error,sizeof error)&&round==script);
 assert(us_callable_from_native(&other.registry,&leafsig,raw,&round,error,sizeof error));
 assert(us_callable_call(&other.registry,leaf,&leafsig,NULL,&p,9,error,sizeof error));
 assert(!us_callable_pointer(&o.registry,0,&leafsig,&raw,error,sizeof error)&&!raw);
 assert(!us_callable_from_native(&o.registry,&leafsig,NULL,&round,error,sizeof error)&&!round);
 /* By-value fields and arrays: native receives closures, caller stays handles. */
 uint64_t bh;assert(!us_callable_make(&o.registry,US_CALLABLE_NATIVE,&boxsig,(uintptr_t)boxcall,&bh,error,sizeof error));
 struct Box box={{4,5},(Leaf)(uintptr_t)script,{(Leaf)(uintptr_t)script,(Leaf)(uintptr_t)leaf}},saved=box;
 struct {uint64_t a;struct Box b;uint64_t z;} got={777,{0},888};uint64_t bs=(uintptr_t)&box;
 assert(!us_callable_call(&o.registry,bh,&boxsig,&bs,&got.b,1,error,sizeof error));assert(!memcmp(&box,&saved,sizeof box));
 assert(got.a==777&&got.z==888&&got.b.p.d==5&&(uint64_t)(uintptr_t)got.b.leaf==script&&(uint64_t)(uintptr_t)got.b.list[0]==script);
 assert(!us_callable_pointer(&o.registry,(uintptr_t)got.b.list[1],&leafsig,&raw,error,sizeof error)&&raw==(void*)native_other);
 /* Native callback returns normally, but failed call commits no Pair bytes. */
 o.fail=1;o.sticky=0;uint64_t ds[]={relay,leaf};struct Pair untouched={1234,5678};int returned=drivereturns,failures=o.failures;
 assert(us_callable_call(&o.registry,host,&drivesig,ds,&untouched,2,error,sizeof error));assert(drivereturns==returned+1&&untouched.d==1234&&untouched.n==5678&&o.sticky==23&&o.failures==failures);
 p=retained(native_leaf);assert(p.d==0&&p.n==0&&o.sticky==23&&o.failures==failures);
 o.fail=o.sticky=0;assert(!us_callable_call(&o.registry,host,&drivesig,ds,&p,2,error,sizeof error)&&p.d==17);
 us_export_signature bad=leafsig;us_export_type ba[9];memcpy(ba,leafargs,sizeof ba);bad.argtypes=ba;ba[8]=D;
 assert(us_callable_pointer(&o.registry,script,&bad,&raw,error,sizeof error));
 bad=leafsig;bad.variadic=1;
 assert(!us_callable_make(&o.registry,US_CALLABLE_NATIVE,&bad,(uintptr_t)native_leaf,&round,error,sizeof error));
 assert(us_callable_call(&o.registry,round,&bad,NULL,&p,9,error,sizeof error));
 assert(!us_callable_make(&o.registry,US_CALLABLE_SCRIPT,&bad,(uintptr_t)&scriptleaf,&round,error,sizeof error));
 assert(us_callable_pointer(&o.registry,round,&bad,&raw,error,sizeof error));
 bad=leafsig;bad.mode=0;assert(us_callable_pointer(&o.registry,script,&bad,&raw,error,sizeof error));
 /* Layout disagreement and missing native boundary hooks explicitly refuse. */
 bad=leafsig;us_export_type wrong=PAIR;us_export_member wm[2];memcpy(wm,pm,sizeof wm);wm[1].offset=4;wrong.members=wm;bad.result=wrong;
 assert(us_callable_make(&o.registry,US_CALLABLE_NATIVE,&bad,(uintptr_t)native_leaf,&round,error,sizeof error));
 bad=leafsig;bad.result=D;bad.result.alignment=4;assert(us_callable_make(&o.registry,US_CALLABLE_NATIVE,&bad,(uintptr_t)native_leaf,&round,error,sizeof error));
 Owner missing;init(&missing,1);missing.registry.native_call=NULL;
 assert(us_callable_make(&missing.registry,US_CALLABLE_NATIVE,&leafsig,(uintptr_t)native_leaf,&round,error,sizeof error));us_callables_clear(&missing.registry);
 /* Signature cycles/sharing are cloned once; object conversion never follows them. */
 us_export_signature cycle={0};us_export_type cp=fp(&cycle);signature(&cycle,P,&cp,1);cp=fp(&cycle);
 uint64_t cyc;assert(!us_callable_make(&o.registry,US_CALLABLE_SCRIPT,&cycle,(uintptr_t)&scriptleaf,&cyc,error,sizeof error));
 assert(us_callable_find(&o.registry,cyc)->graph.count==1);
 us_export_signature duplicate=leafsig;duplicate.id=999;assert(!us_callable_make(&o.registry,US_CALLABLE_NATIVE,&duplicate,(uintptr_t)native_leaf,&round,error,sizeof error)&&round==leaf);
 us_export_type one[1]={I};us_export_signature ns;signature(&ns,I,one,1);uint64_t nh;assert(!us_callable_make(&o.registry,US_CALLABLE_NATIVE,&ns,(uintptr_t)negative,&nh,error,sizeof error));
 uint64_t in=7,out=0xaaaaaaaaaaaaaaaaULL;assert(!us_callable_call(&o.registry,nh,&ns,&in,&out,1,error,sizeof error)&&out==(uint64_t)(int64_t)-7);
 /* Production frame contract: scalar results always occupy eight bytes. */
 us_export_type fa[1]={F};us_export_signature fs;signature(&fs,F,fa,1);uint64_t fh;
 assert(!us_callable_make(&o.registry,US_CALLABLE_SCRIPT,&fs,(uintptr_t)&scriptfloat,&fh,error,sizeof error));
 assert(!us_callable_pointer(&o.registry,fh,&fs,&raw,error,sizeof error));float (*fc)(float)=(float(*)(float))raw;
 assert(fc(2.5f)==3.75f);float inputf=2.5f;uint64_t fslot=bits(&inputf,4),fresult=UINT64_MAX;
 assert(!us_callable_call(&o.registry,fh,&fs,&fslot,&fresult,1,error,sizeof error));float value;memcpy(&value,&fresult,4);assert(value==3.75f&&fresult>>32==0);
 /* Optional declaration hook works with invoke_frame absent, both ways. */
 Owner scoped;init(&scoped,1);scoped.registry.invoke_frame=NULL;scoped.registry.script_call=declared_invoke;
 uint64_t sh;assert(!us_callable_make(&scoped.registry,US_CALLABLE_SCRIPT,&leafsig,(uintptr_t)&scriptleaf,&sh,error,sizeof error));
 assert(!us_callable_pointer(&scoped.registry,sh,&leafsig,&raw,error,sizeof error));Leaf declared=(Leaf)raw;
 p=declared(2,1.5,2.5f,&n,s,6,9,7,8);assert(p.d==17&&p.n==40);
 double qb=1.5,qf=9;float qc=2.5;uint64_t declared_slots[]={2,bits(&qb,8),bits(&qc,4),(uintptr_t)&n,(uintptr_t)&s,6,bits(&qf,8),7,8};
 assert(!us_callable_call(&scoped.registry,sh,&leafsig,declared_slots,&p,9,error,sizeof error)&&p.d==17&&p.n==40&&scoped.declared_calls==2);
 us_callables_clear(&scoped.registry);init(&scoped,2);scoped.registry.invoke_frame=NULL;
 assert(us_callable_make(&scoped.registry,US_CALLABLE_SCRIPT,&leafsig,(uintptr_t)&scriptleaf,&sh,error,sizeof error));us_callables_clear(&scoped.registry);
 /* Borrowed declarations remain untouched and stale tokens never recycle. */
 assert(PAIR.ffi==NULL&&BOX.ffi==NULL&&ARRAY.ffi==NULL&&leafsig.id==0);
 us_callables_clear(&o.registry);init(&o,2);assert(us_callable_call(&o.registry,leaf,&leafsig,NULL,&p,9,error,sizeof error));
 assert(!us_callable_make(&o.registry,US_CALLABLE_NATIVE,&leafsig,(uintptr_t)native_leaf,&round,error,sizeof error)&&round!=leaf);
 us_callables_clear(&o.registry);us_callables_clear(&other.registry);retained=NULL;
 puts("callables: real ffi Leaf9/Relay/retained/fields/arrays/failure/ownership/cycles: ok; mock script only");return 0;
}

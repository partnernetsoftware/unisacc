/* Actual libffi mechanics; script entry is a mock, no public API claim. */
#define US_CALLABLES_IMPLEMENTATION
#include "librarycallables.h"
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
struct Pair {double d;int n;};
typedef struct Pair (*Callback)(double,int,struct Pair);
typedef struct Pair (*VarFn)(double,int,...);
static us_export_type I,D,F,PAIR,CB,VAR;
static us_export_member members[2];
static us_export_type prefix[2],cbargs[3],all[13],acceptargs[1],transportargs[1];
static us_export_signature proto,concrete,cbsig,acceptsig,transportsig;
static int scriptcb,scriptaccept,scriptvar,native_returns;
typedef struct Owner {us_callables r;int calls,fail,sticky,callbacks;} Owner;
static uint64_t bits(double d){uint64_t v;memcpy(&v,&d,8);return v;}
static us_export_type scalar(unsigned k,unsigned w){us_export_type t={0};t.kind=k;t.width=t.alignment=w;return t;}
static us_export_type pointer(us_export_signature *s){us_export_type t=scalar(4,8);t.depth=1;t.tag=4;t.signature=s;return t;}
static us_export_signature sig(us_export_type result,us_export_type *args,unsigned count){us_export_signature s={0};s.result=result;s.argtypes=args;s.count=s.stored=count;s.mode=1;return s;}
static void fixtures(void){
 I=scalar(1,4);D=scalar(3,8);F=scalar(3,4);PAIR=scalar(5,sizeof(struct Pair));PAIR.alignment=_Alignof(struct Pair);PAIR.tag=1;PAIR.nmembers=2;PAIR.members=members;
 members[0]=(us_export_member){offsetof(struct Pair,d),0,0,8,&D};members[1]=(us_export_member){offsetof(struct Pair,n),0,0,4,&I};
 prefix[0]=D;prefix[1]=I;proto=sig(PAIR,prefix,2);proto.variadic=1;
 cbargs[0]=D;cbargs[1]=I;cbargs[2]=PAIR;cbsig=sig(PAIR,cbargs,3);CB=pointer(&cbsig);VAR=pointer(&proto);
 all[0]=D;all[1]=I;all[2]=D;all[3]=I;all[4]=PAIR;all[5]=CB;for(int i=6;i<13;i++)all[i]=D;concrete=sig(PAIR,all,13);
 acceptargs[0]=VAR;acceptsig=sig(PAIR,acceptargs,1);transportargs[0]=pointer(&acceptsig);transportsig=sig(PAIR,transportargs,1);
}
static struct Pair cb(double x,int k,struct Pair p){return (struct Pair){x+p.d,k+p.n};}
static struct Pair mix(double x,int mode,...){
 if(!mode){native_returns++;return (struct Pair){x,0};}
 va_list ap;va_start(ap,mode);double y=va_arg(ap,double);int k=va_arg(ap,int);struct Pair p=va_arg(ap,struct Pair);Callback callback=va_arg(ap,Callback);
 for(int i=0;i<7;i++)y+=va_arg(ap,double);va_end(ap);struct Pair r=callback(x+y,k,p);native_returns++;return r;
}
static struct Pair host_accept(VarFn f){struct Pair p={4,5};return f(1.5,1,2.5,7,p,cb,1.,2.,3.,4.,5.,6.,7.);}
static struct Pair transport(struct Pair (*accept)(VarFn)){struct Pair r=accept(mix);native_returns++;return r;}
static int native(void *raw,ffi_cif *cif,uintptr_t target,void *result,void **values){Owner *o=raw;o->calls++;ffi_call(cif,FFI_FN((void*)target),result,values);if(o->fail)o->sticky=23;return o->sticky;}
static int invoke(void *raw,const void *target,const us_export_frame *f){
 Owner *o=raw;o->callbacks++;assert(f->result_bytes==sizeof(struct Pair)&&f->result_kind==5);
 if(target==&scriptvar){assert(f->count==13&&f->mode==1);double x,y,z;memcpy(&x,f->slots,8);memcpy(&y,f->slots+2,8);for(int i=6;i<13;i++){memcpy(&z,f->slots+i,8);y+=z;}struct Pair r=cb(x+y,(int)f->slots[3],*(struct Pair*)(uintptr_t)f->slots[4]);memcpy(f->result,&r,sizeof r);return 0;}
 if(target==&scriptcb){assert(f->count==3);double d;memcpy(&d,f->slots,8);struct Pair r=cb(d,(int)f->slots[1],*(struct Pair*)(uintptr_t)f->slots[2]);memcpy(f->result,&r,sizeof r);return 0;}
 assert(target==&scriptaccept&&f->count==1);char e[256]={0};struct Pair p={4,5};uint64_t h;
 assert(!us_callable_make(&o->r,US_CALLABLE_NATIVE,&cbsig,(uintptr_t)cb,&h,e,sizeof e));
 uint64_t slots[]={bits(1.5),1,bits(2.5),7,(uintptr_t)&p,h,bits(1),bits(2),bits(3),bits(4),bits(5),bits(6),bits(7)};
 return us_callable_call_concrete(&o->r,f->slots[0],&proto,&concrete,slots,f->result,13,e,sizeof e);
}
static void init(Owner *o){memset(o,0,sizeof *o);us_callables_init(&o->r,o,1,invoke,native,NULL);}
static void expect(struct Pair p){assert(p.d==36&&p.n==12);}
int main(void){
 fixtures();Owner o,other;init(&o);init(&other);char e[256]={0};uint64_t h,callback,script;
 assert(us_callable_concrete_valid(&proto,&concrete));
 assert(!us_callable_make(&o.r,US_CALLABLE_NATIVE,&proto,(uintptr_t)mix,&h,e,sizeof e));
 assert(us_callable_find(&o.r,h)->graph.signatures[0]->variadic==1);
 assert(us_callable_find(&o.r,h)->cif.nargs==0); /* Prototype has no prepared CIF. */
 assert(!us_callable_make(&o.r,US_CALLABLE_NATIVE,&cbsig,(uintptr_t)cb,&callback,e,sizeof e));
 assert(!us_callable_make(&o.r,US_CALLABLE_SCRIPT,&cbsig,(uintptr_t)&scriptcb,&script,e,sizeof e));
 struct Pair p={4,5};uint64_t slots[]={bits(1.5),1,bits(2.5),7,(uintptr_t)&p,callback,bits(1),bits(2),bits(3),bits(4),bits(5),bits(6),bits(7)};
 struct {uint64_t before;struct Pair result;uint64_t after;} out={123,{0,0},456};
 assert(!us_callable_call_concrete(&o.r,h,&proto,&concrete,slots,&out.result,13,e,sizeof e));expect(out.result);
 slots[5]=script;assert(!us_callable_call_concrete(&o.r,h,&proto,&concrete,slots,&out.result,13,e,sizeof e));expect(out.result);assert(o.callbacks==1);
 assert(out.before==123&&out.after==456&&p.d==4&&p.n==5&&PAIR.ffi==NULL&&PAIR.elements==NULL);
 us_export_signature zero=sig(PAIR,prefix,2);uint64_t zs[]={bits(1.5),0};
 assert(!us_callable_call_concrete(&o.r,h,&proto,&zero,zs,&out.result,2,e,sizeof e));assert(out.result.d==1.5&&out.result.n==0);
 void *raw=NULL;uint64_t round=0;assert(!us_callable_pointer(&o.r,h,&proto,&raw,e,sizeof e)&&raw==(void*)mix);
 assert(!us_callable_from_native(&o.r,&proto,raw,&round,e,sizeof e)&&round==h);
 uint64_t accept,transporth;assert(!us_callable_make(&o.r,US_CALLABLE_NATIVE,&acceptsig,(uintptr_t)host_accept,&accept,e,sizeof e));
 assert(!us_callable_call(&o.r,accept,&acceptsig,&h,&out.result,1,e,sizeof e));expect(out.result);
 assert(!us_callable_make(&o.r,US_CALLABLE_SCRIPT,&acceptsig,(uintptr_t)&scriptaccept,&accept,e,sizeof e));
 assert(!us_callable_make(&o.r,US_CALLABLE_NATIVE,&transportsig,(uintptr_t)transport,&transporth,e,sizeof e));
 assert(!us_callable_call(&o.r,transporth,&transportsig,&accept,&out.result,1,e,sizeof e));expect(out.result);
 int calls=o.calls;out.result=(struct Pair){99,88};
 #define BAD(P,C,N) assert(us_callable_call_concrete(&o.r,h,(P),(C),slots,&out.result,(N),e,sizeof e))
 assert(us_callable_call(&o.r,h,&proto,slots,&out.result,2,e,sizeof e));
 assert(us_callable_call_concrete(&other.r,h,&proto,&concrete,slots,&out.result,13,e,sizeof e));
 assert(!us_callable_make(&o.r,US_CALLABLE_SCRIPT,&proto,(uintptr_t)&scriptvar,&round,e,sizeof e));
 assert(us_callable_pointer(&o.r,round,&proto,&raw,e,sizeof e));
 assert(!us_callable_call_concrete(&o.r,round,&proto,&concrete,slots,&out.result,13,e,sizeof e));expect(out.result);out.result=(struct Pair){99,88};
 BAD(&proto,&concrete,12);us_export_signature bad=concrete;bad.count=bad.stored=1;BAD(&proto,&bad,1);
 bad=concrete;bad.mode=0;BAD(&proto,&bad,13);bad=concrete;bad.variadic=1;BAD(&proto,&bad,13);
 bad=concrete;bad.result=D;BAD(&proto,&bad,13);bad=proto;bad.result=D;BAD(&bad,&concrete,13);
 bad=concrete;bad.count=bad.stored=1025;BAD(&proto,&bad,1025);
 us_export_type saved=all[0];all[0]=I;BAD(&proto,&concrete,13);all[0]=saved;
 saved=all[2];all[2]=F;BAD(&proto,&concrete,13);all[2]=scalar(1,1);BAD(&proto,&concrete,13);all[2]=scalar(1,2);BAD(&proto,&concrete,13);all[2]=saved;
 us_export_signature malformed=proto;malformed.mode=0;assert(us_callable_make(&o.r,US_CALLABLE_NATIVE,&malformed,(uintptr_t)mix,&round,e,sizeof e));
 malformed=proto;malformed.count=malformed.stored=0;assert(us_callable_make(&o.r,US_CALLABLE_NATIVE,&malformed,(uintptr_t)mix,&round,e,sizeof e));
 assert(o.calls==calls&&out.result.d==99&&out.result.n==88);
 o.fail=1;int returned=native_returns;BAD(&proto,&concrete,13);assert(native_returns==returned+1&&o.sticky==23&&out.result.d==99&&out.result.n==88&&out.before==123&&out.after==456);
 us_callables_clear(&o.r);BAD(&proto,&concrete,13);us_callables_clear(&other.r);
 puts("variadic concrete native calls, callback transport, rejection and cleanup passed");return 0;
}

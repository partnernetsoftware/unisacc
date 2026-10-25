/* Explicit fixture certificates; no target ABI classification occurs here. */
#define US_CALLABLES_IMPLEMENTATION
#include "librarycarrierplan.h"
#include <assert.h>
typedef union Mixed {double d;uint64_t u;} Mixed;
static Mixed round_native(Mixed x){x.u+=17;return x;}
static unsigned char *read_fixture(const char *name,size_t *n){FILE *f=fopen(name,"rb");assert(f);assert(!fseek(f,0,SEEK_END));long z=ftell(f);assert(z>=0);rewind(f);unsigned char *b=malloc(z?z:1);assert(b);assert(fread(b,1,z,f)==(size_t)z);fclose(f);*n=z;return b;}
typedef struct Provider {const char *target;unsigned char *blob,*original;size_t len,original_len;int calls;} Provider;
static int classify(void *owner,us_native_plans *plans,uintptr_t raw,const void *wire,size_t len,uint64_t *handle,char *error,size_t cap){
 Provider *p=owner;p->calls++;assert(raw==(uintptr_t)round_native);
 if(len!=p->original_len||memcmp(wire,p->original,len))return 0; /* fixture classifier */
 us_carrier_certificate cert={0};if(us_carrier_certificate_load(&cert,p->target,p->blob,p->len,error,cap))return 1;
 int rc=us_carrier_certificate_native_add(plans,raw,&cert,handle,error,cap);us_carrier_certificate_clear(&cert);return rc;
}
static void exercise(us_native_plan *plan){
 for(int i=0;i<100;i++){
  struct {uint64_t a;Mixed x;uint64_t b;} in={0x10203040,{.u=51+(unsigned)i},0x50607080},out={0x11223344,{.u=0xabcdef},0x55667788};
  uint64_t slot=(uintptr_t)&in.x;us_native_arena *arena=NULL;char error[128]={0};assert(!us_native_prepare(plan,&slot,1,&out.x,&arena,error,sizeof error));
  assert(arena->values[0]!=&in.x&&arena->owned[0]==arena->values[0]);assert(!us_native_call(arena));assert(out.x.u==0xabcdef);
  arena->boundary.outcome.exited=1;arena->boundary.outcome.exit_status=23;assert(us_native_commit(arena)&&out.x.u==0xabcdef);
  memset(&arena->boundary.outcome,0,sizeof arena->boundary.outcome);assert(!us_native_commit(arena)&&out.x.u==in.x.u+17);
  assert(in.a==0x10203040&&in.b==0x50607080&&out.a==0x11223344&&out.b==0x55667788&&in.x.u==51+(unsigned)i);us_native_arena_free(arena);
 }
 Mixed x={.u=9},out={.u=0};uint64_t slot=(uintptr_t)&x;us_native_arena *a=NULL;char error[128]={0};
 assert(us_native_prepare(plan,&slot,0,&out,&a,error,sizeof error)&&!a);assert(us_native_prepare(plan,&slot,1,NULL,&a,error,sizeof error)&&!a);
 assert(!us_native_prepare(plan,&slot,1,&out,&a,error,sizeof error));assert(!us_native_invoke(a)&&out.u==26);us_native_arena_free(a);
}
int main(int argc,char **argv){
 assert(argc==9);size_t n[7];unsigned char *b[7];for(int i=0;i<7;i++)b[i]=read_fixture(argv[i+2],n+i);
 char error[256]={0};us_native_plans plans={0};uint64_t h=99;
 assert(!us_native_plan_add(&plans,(uintptr_t)round_native,b[1],n[1],&h,error,sizeof error)&&!h&&!plans.head);
 us_carrier_certificate cert={0};assert(!us_carrier_certificate_load(&cert,argv[1],b[0],n[0],error,sizeof error));
 us_exports oldoriginal=cert.original,oldcarrier=cert.carrier;uint64_t oldtoken=atomic_load(&us_callable_global_token);
 cert.carrier.items->result.width=4;
 assert(us_carrier_certificate_native_add(&plans,(uintptr_t)round_native,&cert,&h,error,sizeof error)&&!h&&!plans.head&&cert.original.items==oldoriginal.items&&cert.carrier.items==oldcarrier.items&&atomic_load(&us_callable_global_token)==oldtoken);
 cert.carrier.items->result.width=8;
 assert(!us_carrier_certificate_native_add(&plans,(uintptr_t)round_native,&cert,&h,error,sizeof error)&&h&&!cert.original.items&&!cert.carrier.items&&!cert.target[0]);
 us_native_plan *plan=us_native_plan_find(&plans,h);assert(plan&&plan->graph.items->result.kind==5&&plan->carrier.items->result.kind==1&&!plan->bridge_required&&plan->signature.result.kind==5&&plan->cif.rtype==&ffi_type_uint64);exercise(plan);us_carrier_certificate_clear(&cert);us_native_plans_clear(&plans);
 Provider provider={argv[1],b[0],b[1],n[0],n[1],0};plans.carrier_provider=classify;plans.carrier_owner=&provider;
 assert(!us_native_plan_add(&plans,(uintptr_t)round_native,b[2],n[2],&h,error,sizeof error)&&h&&provider.calls==0); /* supported plain */
 assert(!us_native_plan_add(&plans,(uintptr_t)round_native,b[3],n[3],&h,error,sizeof error)&&!h&&provider.calls==1); /* plain support0 attempt */
 assert(!us_native_plan_add(&plans,(uintptr_t)round_native,b[4],n[4],&h,error,sizeof error)&&!h&&provider.calls==1); /* variadic prototype */
 assert(!us_native_plan_add_variadic(&plans,(uintptr_t)round_native,b[1],n[1],1,&h,error,sizeof error)&&!h&&provider.calls==1); /* no widening */
 assert(!us_native_plan_add(&plans,(uintptr_t)round_native,b[5],n[5],&h,error,sizeof error)&&!h&&provider.calls==2); /* unsupported callback requires a model certificate */
 assert(!us_native_plan_add_bridge(&plans,(uintptr_t)round_native,b[5],n[5],&h,error,sizeof error)&&h&&us_native_plan_find(&plans,h)->bridge_required&&provider.calls==2);
 assert(us_native_plan_add(&plans,(uintptr_t)round_native,b[6],n[6],&h,error,sizeof error)&&!h&&provider.calls==2); /* strict wire fails before provider */
 assert(!us_native_plan_add(&plans,(uintptr_t)round_native,b[1],n[1],&h,error,sizeof error)&&h&&provider.calls==3);exercise(us_native_plan_find(&plans,h));
 us_native_plans_clear(&plans);assert(!plans.head&&!plans.carrier_provider&&!plans.carrier_owner);for(int i=0;i<7;i++)free(b[i]);
 puts("carrier native plans: true Mixed call/commit, canaries, original arena, sticky discard, transactional consume and provider gates passed");return 0;
}

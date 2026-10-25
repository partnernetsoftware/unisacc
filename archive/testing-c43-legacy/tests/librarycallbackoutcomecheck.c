/* Native call/commit split: actual ffi_call must leave target untouched. */
#ifdef US_OUTCOME_ARENA_PROBE
#include "exec/c/librarynative.h"
struct Pair {double d;int n;};
static struct Pair target(int n){struct Pair r={17,n};return r;}
int main(int argc,char **argv){
 if(argc!=2)return 2;unsigned char sig[4096];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
 size_t n=fread(sig,1,sizeof sig,f);fclose(f);char error[160];uint64_t handle=0,slots[1]={40};
 us_native_plans plans={0};us_native_arena *arena=NULL;
 struct{uint64_t before;struct Pair value;uint64_t after;}r={11,{91,92},22};
 if(us_native_plan_add(&plans,(uintptr_t)target,sig,n,&handle,error,sizeof error)||!handle||
    us_native_prepare(us_native_plan_find(&plans,handle),slots,1,&r.value,&arena,error,sizeof error)||us_native_call(arena))return 1;
 if(r.value.d!=91||r.value.n!=92||r.before!=11||r.after!=22)return 1;
 arena->boundary.outcome.exited=1;arena->boundary.outcome.exit_status=23;
 if(!us_native_commit(arena)||r.value.d!=91||r.value.n!=92)return 1;
 memset(&arena->boundary.outcome,0,sizeof arena->boundary.outcome);
 if(us_native_commit(arena)||r.value.d!=17||r.value.n!=40||r.before!=11||r.after!=22)return 1;
 us_native_arena_free(arena);us_native_plans_clear(&plans);puts("actual ffi arena: deferred commit, poisoned result unchanged, recovered commit passed");return 0;
}
#else
/* Public API sticky-error mechanism only; globals from us_sym deliberately do
   not claim function-pointer parameter ABI conversion. */
#include "libunisacc.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct Pair {double d;int n;};
static int (*bad)(int),(*good)(void),(*foreign)(int);
static int returns,calls,zeroes;
static struct Pair host_drive(int mode){
 calls++;int v=(mode==3 ? foreign:bad)(mode ? 23:0);
 if(mode && !v)zeroes++;
 if(mode==2){v=bad(31);if(!v)zeroes++;}
 if(good()!=42){struct Pair fail={-1,-1};return fail;}
 returns++;struct Pair r={17,40};return r;
}
static const char source[]="#include <stdlib.h>\nstruct Pair{double d;int n;};struct Pair host_drive(int);struct Pair outer(int mode){return host_drive(mode);}int bad(int n){if(n)exit(n);return 42;}int good(void){return 42;}";
static int bind(us_context *c,const char *file){
 unsigned char b[4096];FILE *f=fopen(file,"rb");if(!f)return 1;
 size_t n=fread(b,1,sizeof b,f);int e=ferror(f)||!feof(f);fclose(f);
 return e||us_add_symbol_typed(c,"host_drive",(void*)host_drive,b,n);
}
static int fail(us_context *c,const char *step){fprintf(stderr,"%s: %s\n",step,us_error(c));return 1;}
int main(int argc,char **argv){
 if(argc!=4)return 2;
 for(int opt=0;opt<3;opt++){
  us_context *a=us_new(argv[1]),*b=us_new(argv[1]);if(!a||!b)return 2;
  if(bind(a,argv[3])||us_add_source(a,"outcome.c",source)||us_compile(a,argv[2],opt)||us_relocate(a))return fail(a,"setup A");
  if(us_add_source(b,"foreign.c","#include <stdlib.h>\nint foreign(int n){exit(n);return 0;}")||us_compile(b,argv[2],opt)||us_relocate(b))return fail(b,"setup B");
  struct Pair (*outer)(int)=(struct Pair(*)(int))us_sym(a,"outer");
  bad=(int(*)(int))us_sym(a,"bad");good=(int(*)(void))us_sym(a,"good");foreign=(int(*)(int))us_sym(b,"foreign");
  if(!outer||!bad||!good||!foreign)return fail(a,"symbols");
  for(int mode=0;mode<4;mode++){
   int before=returns,z=zeroes;
   struct{uint64_t lo;struct Pair value;uint64_t hi;}guard={UINT64_C(0x123456789abcdef),{91,92},UINT64_C(0xfedcba9876543210)};
   guard.value=outer(mode);int status=-1,failed=us_call_status(a,&status);
   if(returns!=before+1||guard.lo!=UINT64_C(0x123456789abcdef)||guard.hi!=UINT64_C(0xfedcba9876543210))return fail(a,"native normal return/canary");
   if(mode==1||mode==2){
    if(!failed||status!=23||guard.value.d!=0||guard.value.n!=0||!strstr(us_error(a),"23")||zeroes-z!=(mode==2?2:1))return fail(a,"sticky first exit23 after later success");
   }else if(failed||status||guard.value.d!=17||guard.value.n!=40)return fail(a,"success/cross-owner isolation");
   if(mode==3){int bs=-1;if(!us_call_status(b,&bs)||bs!=23)return fail(b,"foreign owner status");}
   struct Pair recovered=outer(0);if(recovered.d!=17||recovered.n!=40||us_call_status(a,&status)||status||*us_error(a))return fail(a,"success after failure");
  }
  bad=NULL;good=NULL;foreign=NULL;us_free(a);us_free(b);
  printf("O%d: sticky exit23/first failure/later success, native normal return, Pair zero/canary, owner isolation and recovery passed\n",opt);
 }
 return 0;
}

#endif

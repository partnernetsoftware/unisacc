/* Real public native -> script -> variadic native ABI, not a wire-only probe. */
#include "libunisacc.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
struct Pair { double d; int n; };
static int mixed_calls, empty_calls, native_bad;
static struct Pair host_exchange(double seed,int mode,...){
 struct Pair r={seed,mode};va_list ap;va_start(ap,mode);
 if(mode==7){int c=va_arg(ap,int),u=va_arg(ap,int),b=va_arg(ap,int);double f=va_arg(ap,double);int *p=va_arg(ap,int*);struct Pair q=va_arg(ap,struct Pair);
  if(c!=-3||u!=24||b!=1||f!=2.5||!p||*p!=9||q.d!=4.0||q.n!=11)native_bad++;
  r.d+=f+*p+q.d;r.n+=c+u+b+q.n;
  for(int i=0;i<6;i++)if(va_arg(ap,double)!=0.0)native_bad++;
  mixed_calls++;
 }else if(mode==0)empty_calls++;else native_bad++;
 va_end(ap);return r;
}
static int bind(us_context *c,const char *dir){char path[2048];unsigned char bytes[8192];snprintf(path,sizeof path,"%s/host_exchange.sig",dir);FILE *f=fopen(path,"rb");if(!f)return 1;size_t n=fread(bytes,1,sizeof bytes,f);int bad=ferror(f)||!feof(f);fclose(f);return bad||us_add_symbol_typed(c,"host_exchange",(void*)host_exchange,bytes,n);}
static int fail(us_context *c,const char *step){fprintf(stderr,"%s: %s\n",step,us_error(c));us_free(c);return 1;}
static const char source[]=
"struct Pair { double d; int n; };"
"struct Pair host_exchange(double,int,...);"
"float tailvalue(void){return 2.5f;}"
"struct Pair run_case(void){signed char c=-3;unsigned short u=24;_Bool b=1;int n=9;struct Pair p={4.0,11};"
"struct Pair r=host_exchange(1.5,7,c,u,b,tailvalue(),&n,p,0.0,0.0,0.0,0.0,0.0,0.0);"
"struct Pair z=host_exchange(3.25,0);r.d+=z.d;r.n+=z.n;return r;}";
static int source_shadow(const char *package,const char *target,const char *dir,int opt){
 const char *s="struct Pair {double d;int n;};struct Pair host_exchange(double,int,...);struct Pair shadow(void){return host_exchange(8.0,4,1);}struct Pair host_exchange(double a,int b,...){struct Pair p={a+1.0,b+2};return p;}";
 us_context *c=us_new(package);if(!c)return 1;
 if(bind(c,dir)||us_add_source(c,"shadow.c",s)||us_compile(c,target,opt)||us_relocate(c))return fail(c,"source shadow compile");
 struct Pair (*fn)(void)=(struct Pair(*)(void))us_sym(c,"shadow");if(!fn)return fail(c,"source shadow symbol");
 int before=mixed_calls+empty_calls;struct Pair p=fn();
 if(p.d!=9.0||p.n!=6||native_bad||before!=mixed_calls+empty_calls)return fail(c,"late source priority");us_free(c);return 0;
}
static int missing_prefix(const char *package,const char *target,const char *dir,int opt){
 us_context *c=us_new(package);if(!c)return 1;
 const char *s="struct Pair {double d;int n;};struct Pair host_exchange(double,int,...);struct Pair bad(void){return host_exchange(1.5);}";
 if(bind(c,dir)||us_add_source(c,"missing.c",s))return fail(c,"missing setup");
 int rc=us_compile(c,target,opt);if(rc!=1||!strstr(us_error(c),"not covered")){fprintf(stderr,"missing prefix accepted rc=%d\n",rc);us_free(c);return 1;}us_free(c);return 0;
}
static int prefix_mismatch(const char *package,const char *target,const char *dir,int opt){
 us_context *c=us_new(package);if(!c)return 1;
 const char *s="struct Pair {double d;int n;};struct Pair host_exchange(float,int,...);struct Pair bad(void){return host_exchange(1.5f,0);}";
 if(bind(c,dir)||us_add_source(c,"mismatch.c",s))return fail(c,"prefix mismatch setup");
 int rc=us_compile(c,target,opt);if(rc!=1||!strstr(us_error(c),"not covered"))return fail(c,"prefix mismatch accepted");us_free(c);return 0;
}
int main(int argc,char **argv){
 if(argc!=4)return 2;
 for(int opt=0;opt<3;opt++){
  us_context *c=us_new(argv[1]);if(!c)return 3;
  if(bind(c,argv[3])||us_add_source(c,"variadic.c",source)||us_compile(c,argv[2],opt)||us_relocate(c))return fail(c,"variadic compile/relocate");
  struct Pair (*fn)(void)=(struct Pair(*)(void))us_sym(c,"run_case");if(!fn)return fail(c,"run_case symbol");
  int m=mixed_calls,e=empty_calls;
  for(int i=0;i<100;i++){
   struct {uint64_t a;struct Pair p;uint64_t b;} g={UINT64_C(0x123456789abcdef),{0,0},UINT64_C(0xfedcba9876543210)};
   g.p=fn();if(g.p.d!=20.25||g.p.n!=40||g.a!=UINT64_C(0x123456789abcdef)||g.b!=UINT64_C(0xfedcba9876543210)||native_bad)return fail(c,"promotions/aggregate/canary");
  }
  if(mixed_calls-m!=100||empty_calls-e!=100)return fail(c,"site call counts");
  if(us_add_symbol_typed(c,"bad",(void*)host_exchange,"bad",3)==0)return fail(c,"bad registration accepted");
  struct Pair again=fn();if(again.d!=20.25||again.n!=40)return fail(c,"failed mutation invalidated export");
  if(source_shadow(argv[1],argv[2],argv[3],opt)||missing_prefix(argv[1],argv[2],argv[3],opt)||prefix_mismatch(argv[1],argv[2],argv[3],opt))return fail(c,"source priority/reject control");
  printf("O%d: variadic two call graphs, narrow/Bool/float promotion, nested argument, pointer/Pair, FP stack, zero tail, 100 repeats, late source priority: ok\n",opt);us_free(c);
 }
 return 0;
}

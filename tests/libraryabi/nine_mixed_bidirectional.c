/* Native C ABI -> us_sym -> actual model script -> typed native C target.
   Pair return cannot be tested by a ctypes callback signature substitute. */
#include "libunisacc.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
struct Pair { double d; int n; };
typedef struct Pair (*Exchange)(int,double,float,int*,struct Pair,int,double,int,int);
typedef double (*Double9)(double,double,double,double,double,double,double,double,double);
typedef float (*Float9)(float,float,float,float,float,float,float,float,float);
typedef long long (*Int17)(long long,long long,long long,long long,long long,long long,long long,long long,long long,long long,long long,long long,long long,long long,long long,long long,long long);
static Exchange callback9;
static struct Pair host_exchange9(int a,double b,float c,int *p,struct Pair s,int e,double f,int g,int h){if(callback9)return callback9(a,b,c,p,s,e,f,g,h);struct Pair r={b+c+s.d+f,a+*p+s.n+e+g+h};s.d=999;s.n=999;return r;}
static double host_double9(double a,double b,double c,double d,double e,double f,double g,double h,double i){return a+b*2+c*3+d*4+e*5+f*6+g*7+h*8+i*9;}
static float host_float9(float a,float b,float c,float d,float e,float f,float g,float h,float i){return a+b*2+c*3+d*4+e*5+f*6+g*7+h*8+i*9;}
static int native_integer_calls;
static long long host_integers17(long long a,long long b,long long c,long long d,long long e,long long f,long long g,long long h,long long i,long long j,long long k,long long l,long long m,long long n,long long o,long long p,long long q){native_integer_calls++;return a+b*2+c*3+d*4+e*5+f*6+g*7+h*8+i*9+j*10+k*11+l*12+m*13+n*14+o*15+p*16+q*17;}
static int bind(us_context *c,const char *dir,const char *name,void *fn){char path[2048];unsigned char bytes[8192];snprintf(path,sizeof path,"%s/%s.sig",dir,name);FILE *f=fopen(path,"rb");if(!f)return 1;size_t n=fread(bytes,1,sizeof bytes,f);int bad=ferror(f)||!feof(f);fclose(f);return bad||us_add_symbol_typed(c,name,fn,bytes,n);}
static const char source[]=
"struct Pair { double d; int n; };"
"struct Pair host_exchange9(int,double,float,int*,struct Pair,int,double,int,int);"
"double host_double9(double,double,double,double,double,double,double,double,double);"
"float host_float9(float,float,float,float,float,float,float,float,float);"
"long host_integers17(long,long,long,long,long,long,long,long,long,long,long,long,long,long,long,long,long);"
"struct Pair exchange(int a,double b,float c,int *p,struct Pair s,int e,double f,int g,int h){return host_exchange9(a,b,c,p,s,e,f,g,h);}"
"struct Pair callback9(int a,double b,float c,int *p,struct Pair s,int e,double f,int g,int h){struct Pair r;r.d=b+c+s.d+f;r.n=a+*p+s.n+e+g+h;return r;}"
"double double9(double a,double b,double c,double d,double e,double f,double g,double h,double i){return host_double9(a,b,c,d,e,f,g,h,i);}"
"float float9(float a,float b,float c,float d,float e,float f,float g,float h,float i){return host_float9(a,b,c,d,e,f,g,h,i);}"
"long integers17(long a,long b,long c,long d,long e,long f,long g,long h,long i,long j,long k,long l,long m,long n,long o,long p,long q){return host_integers17(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q);}";
static int fail(us_context *c,const char *step){fprintf(stderr,"%s: %s\n",step,us_error(c));us_free(c);return 1;}
static int bad_count(const char *package,const char *target,const char *dir,int opt,int extra){
 const char *needle="return host_float9(a,b,c,d,e,f,g,h,i);",*at=strstr(source,needle);
 const char *replace=extra?"return host_float9(a,b,c,d,e,f,g,h,i,i);":"return host_float9(a,b,c,d,e,f,g,h);";
 if(!at)return 1;char wrong[sizeof source+16];size_t prefix=(size_t)(at-source);
 memcpy(wrong,source,prefix);strcpy(wrong+prefix,replace);strcat(wrong,at+strlen(needle));
 us_context *c=us_new(package);if(!c)return 1;
 if(bind(c,dir,"host_exchange9",(void*)host_exchange9)||bind(c,dir,"host_double9",(void*)host_double9)||bind(c,dir,"host_float9",(void*)host_float9)||bind(c,dir,"host_integers17",(void*)host_integers17)||us_add_source(c,"bad-count.c",wrong))return fail(c,"bad count setup");
 int rc=us_compile(c,target,opt);if(rc!=1||!strstr(us_error(c),"library import binding or signature")){fprintf(stderr,"fixed typed %s argument call incorrectly handled rc=%d: %s\n",extra?"extra":"missing",rc,us_error(c));us_free(c);return 1;}
 us_free(c);return 0; /* Invalid source is never executed. */
}
static int source_shadow(const char *package,const char *target,const char *dir,int opt){
 us_context *c=us_new(package);if(!c)return 1;
 const char *text="long host_integers17(long a,...){return a;} long caller(void){return host_integers17(42,1,2);}";
 if(bind(c,dir,"host_integers17",(void*)host_integers17)||us_add_source(c,"source-shadow.c",text)||us_compile(c,target,opt)||us_relocate(c))return fail(c,"typed source shadow compile");
 long long (*caller)(void)=(long long(*)(void))us_sym(c,"caller");if(!caller)return fail(c,"typed source shadow export");
 int before=native_integer_calls;if(caller()!=42||native_integer_calls!=before)return fail(c,"typed source did not win");us_free(c);return 0;
}
int main(int argc,char **argv){
 if(argc!=4)return 2;
 for(int opt=0;opt<3;opt++){
  us_context *c=us_new(argv[1]);if(!c)return 3;
  if(bind(c,argv[3],"host_exchange9",(void*)host_exchange9)||bind(c,argv[3],"host_double9",(void*)host_double9)||bind(c,argv[3],"host_float9",(void*)host_float9)||bind(c,argv[3],"host_integers17",(void*)host_integers17))return fail(c,"typed bind");
  if(us_add_source(c,"nine.c",source))return fail(c,"add");
  if(us_compile(c,argv[2],opt))return fail(c,"compile");
  if(us_relocate(c))return fail(c,"relocate");
  Exchange exchange=(Exchange)us_sym(c,"exchange");if(!exchange)return fail(c,"exchange");
  Double9 d9=(Double9)us_sym(c,"double9");if(!d9)return fail(c,"double9");
  Float9 f9=(Float9)us_sym(c,"float9");if(!f9)return fail(c,"float9");
  Int17 i17=(Int17)us_sym(c,"integers17");if(!i17)return fail(c,"integers17");
  if((void*)exchange!=us_sym(c,"exchange"))return fail(c,"stable address");
  Exchange bounce=(Exchange)us_sym(c,"callback9");if(!bounce)return fail(c,"callback closure");
  for(int repeat=0;repeat<100;repeat++){
   callback9=(repeat>=50)?bounce:NULL;
   int n=4;struct Pair input={5.5,6};
   struct {uint64_t before;struct Pair value;uint64_t after;} guarded={UINT64_C(0x0123456789abcdef),{0,0},UINT64_C(0xfedcba9876543210)};
   guarded.value=exchange(1,2.5,3.25f,&n,input,7,5.75,8,14);
   if(guarded.value.d!=17.0||guarded.value.n!=40||input.d!=5.5||input.n!=6||n!=4||
     guarded.before!=UINT64_C(0x0123456789abcdef)||guarded.after!=UINT64_C(0xfedcba9876543210))return fail(c,"nine mixed/by-value/canary");
   struct Pair next=exchange(-2,-0.5,-1.25f,&n,input,-7,2.25,-8,-14);
   if(next.d!=6.0||next.n!=-21||guarded.value.d!=17.0||guarded.value.n!=40)return fail(c,"independent return copies");
   double dr=d9(1,2,3,4,5,6,7,8,9);float fr=f9(1,2,3,4,5,6,7,8,9);
   long long ir=i17(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17);
   if(dr!=285.0||fr!=285.0f||ir!=1785){fprintf(stderr,"actual scalar results: double %.17g, float %.9g, int17 %lld\n",dr,(double)fr,ir);return fail(c,"complete scalar argument list");}
   int status=-1;if(us_call_status(c,&status))return fail(c,"call status");
  }
  callback9=NULL;
  if(source_shadow(argv[1],argv[2],argv[3],opt))return fail(c,"typed source shadow");
  if(bad_count(argv[1],argv[2],argv[3],opt,0)||bad_count(argv[1],argv[2],argv[3],opt,1))return fail(c,"fixed argument count reject");
  if(!us_add_symbol_typed(c,"bad",(void*)host_exchange9,"bad",3))return fail(c,"malformed registration accepted");
  int again=4;struct Pair in={5.5,6},out=exchange(1,2.5,3.25f,&again,in,7,5.75,8,14);
  if(out.d!=17.0||out.n!=40)return fail(c,"failed registration invalidated export");
  printf("O%d: bidirectional nine mixed -> 17.00 40; by-value/copy/canary, FP9, integer17, 100 repeats including50 typed nested callbacks; failed mutation preserves export: ok\n",opt);us_free(c);
 }
 return 0;
}

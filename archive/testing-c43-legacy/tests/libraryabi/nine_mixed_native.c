/* True native C ABI -> us_sym -> actual model-compiled nine-argument function.
   Pair return cannot be tested by a ctypes callback signature substitute. */
#include "libunisacc.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
struct Pair { double d; int n; };
typedef struct Pair (*Exchange)(int,double,float,int*,struct Pair,int,double,int,int);
typedef double (*Double9)(double,double,double,double,double,double,double,double,double);
typedef float (*Float9)(float,float,float,float,float,float,float,float,float);
typedef long long (*Int17)(long long,long long,long long,long long,long long,long long,long long,long long,long long,long long,long long,long long,long long,long long,long long,long long,long long);
static const char source[]=
"struct Pair { double d; int n; };"
"struct Pair exchange(int a,double b,float c,int *p,struct Pair s,int e,double f,int g,int h){struct Pair r;r.d=b+c+s.d+f;r.n=a+*p+s.n+e+g+h;return r;}"
"double double9(double a,double b,double c,double d,double e,double f,double g,double h,double i){return a+b*2+c*3+d*4+e*5+f*6+g*7+h*8+i*9;}"
"float float9(float a,float b,float c,float d,float e,float f,float g,float h,float i){return a+b*2+c*3+d*4+e*5+f*6+g*7+h*8+i*9;}"
"long integers17(long a,long b,long c,long d,long e,long f,long g,long h,long i,long j,long k,long l,long m,long n,long o,long p,long q){return a+b*2+c*3+d*4+e*5+f*6+g*7+h*8+i*9+j*10+k*11+l*12+m*13+n*14+o*15+p*16+q*17;}";
static int fail(us_context *c,const char *step){fprintf(stderr,"%s: %s\n",step,us_error(c));us_free(c);return 1;}
int main(int argc,char **argv){
 if(argc!=3)return 2;
 for(int opt=0;opt<3;opt++){
  us_context *c=us_new(argv[1]);if(!c)return 3;
  if(us_add_source(c,"nine.c",source))return fail(c,"add");
  if(us_compile(c,argv[2],opt))return fail(c,"compile");
  if(us_relocate(c))return fail(c,"relocate");
  Exchange exchange=(Exchange)us_sym(c,"exchange");if(!exchange)return fail(c,"exchange");
  Double9 d9=(Double9)us_sym(c,"double9");if(!d9)return fail(c,"double9");
  Float9 f9=(Float9)us_sym(c,"float9");if(!f9)return fail(c,"float9");
  Int17 i17=(Int17)us_sym(c,"integers17");if(!i17)return fail(c,"integers17");
  if((void*)exchange!=us_sym(c,"exchange"))return fail(c,"stable address");
  for(int repeat=0;repeat<100;repeat++){
   int n=4;struct Pair input={5.5,6};
   struct {uint64_t before;struct Pair value;uint64_t after;} guarded={UINT64_C(0x0123456789abcdef),{0,0},UINT64_C(0xfedcba9876543210)};
   guarded.value=exchange(1,2.5,3.25f,&n,input,7,5.75,8,14);
   if(guarded.value.d!=17.0||guarded.value.n!=40||input.d!=5.5||input.n!=6||n!=4||
     guarded.before!=UINT64_C(0x0123456789abcdef)||guarded.after!=UINT64_C(0xfedcba9876543210))return fail(c,"nine mixed/by-value/canary");
   struct Pair next=exchange(-2,-0.5,-1.25f,&n,input,-7,2.25,-8,-14);
   if(next.d!=6.0||next.n!=-21||guarded.value.d!=17.0||guarded.value.n!=40)return fail(c,"independent return copies");
   if(d9(1,2,3,4,5,6,7,8,9)!=285.0||f9(1,2,3,4,5,6,7,8,9)!=285.0f||
      i17(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17)!=1785)return fail(c,"complete scalar argument list");
   int status=-1;if(us_call_status(c,&status))return fail(c,"call status");
  }
  printf("O%d: nine mixed -> 17.00 40; by-value/copy/canary, FP9, integer17, 100 repeats: ok\n",opt);us_free(c);
 }
 return 0;
}

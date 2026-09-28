/* True native composites; no wrapper selects or substitutes an ABI class. */
#include "libunisacc.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifndef COMPOSITE_CASE
#define COMPOSITE_CASE 0
#endif
#ifndef COMPOSITE_PRESSURE
#define COMPOSITE_PRESSURE 0
#endif
#ifndef COMPOSITE_CALLBACK
#define COMPOSITE_CALLBACK 0
#endif
#if COMPOSITE_CASE == 0
union U {double v;unsigned long long bits;};struct S {union U u;long k;};
#define DECL "union U {double v;unsigned long long bits;};struct S {union U u;long k;};"
#define EXTENT 16
#define ALIGNMENT 8
#elif COMPOSITE_CASE == 1
union U {double v;double w;};struct S {union U u;double k;};
#define DECL "union U {double v;double w;};struct S {union U u;double k;};"
#define EXTENT 16
#define ALIGNMENT 8
#elif COMPOSITE_CASE >= 2 && COMPOSITE_CASE <= 4
union U {float v;float w;};
#if COMPOSITE_CASE == 2
#define N 2
#define COUNT_TEXT "2"
#elif COMPOSITE_CASE == 3
#define N 4
#define COUNT_TEXT "4"
#else
#define N 5
#define COUNT_TEXT "5"
#endif
struct S {union U u[N];};
#define DECL "union U {float v;float w;};struct S {union U u[" COUNT_TEXT "];};"
#define EXTENT (4*N)
#define ALIGNMENT 4
#else
#error unknown_composite_case
#endif
_Static_assert(sizeof(struct S)==EXTENT&&_Alignof(struct S)==ALIGNMENT,"independent composite layout mismatch");
#if COMPOSITE_PRESSURE == 0
#define PREFIX
#define PARAMS ""
#define VALUES ""
#define PREFIX_OK 1
#elif COMPOSITE_PRESSURE == 1
#define PREFIX unsigned long long a,unsigned long long b,unsigned long long c,unsigned long long d,unsigned long long e,unsigned long long f,unsigned long long g,unsigned long long h,unsigned long long i,
#define PARAMS "unsigned long long a,unsigned long long b,unsigned long long c,unsigned long long d,unsigned long long e,unsigned long long f,unsigned long long g,unsigned long long h,unsigned long long i,"
#define VALUES "1ULL,2ULL,3ULL,4ULL,5ULL,6ULL,7ULL,8ULL,9ULL,"
#define PREFIX_OK (a==1&&b==2&&c==3&&d==4&&e==5&&f==6&&g==7&&h==8&&i==9)
#elif COMPOSITE_PRESSURE == 2
#define PREFIX double a,double b,double c,double d,double e,double f,double g,double h,double i,
#define PARAMS "double a,double b,double c,double d,double e,double f,double g,double h,double i,"
#define VALUES "1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,"
#define PREFIX_OK (a==1&&b==2&&c==3&&d==4&&e==5&&f==6&&g==7&&h==8&&i==9)
#else
#error unknown_composite_pressure
#endif
#if COMPOSITE_CASE < 2
#define FIRST(x) ((x).u.v)
#define LAST(x) ((x).k)
#define ROUND_CHANGE "x.u.v+=3;x.k+=7;"
#define LEAF_CHANGE "x.u.v+=1;x.k+=2;"
#else
#define FIRST(x) ((x).u[0].v)
#define LAST(x) ((x).u[N-1].v)
#if N == 2
#define LAST_TEXT "1"
#elif N == 4
#define LAST_TEXT "3"
#else
#define LAST_TEXT "4"
#endif
#define ROUND_CHANGE "x.u[0].v+=3;x.u[" LAST_TEXT "].v+=7;"
#define LEAF_CHANGE "x.u[0].v+=1;x.u[" LAST_TEXT "].v+=2;"
#endif
typedef struct S (*Leaf)(struct S);static unsigned native_calls,callback_calls;static int prefix_failed,callback_failed;
static int equal(struct S a,struct S b){
#if COMPOSITE_CASE < 2
 return FIRST(a)==FIRST(b)&&LAST(a)==LAST(b);
#else
 for(int i=0;i<N;i++)if(a.u[i].v!=b.u[i].v)return 0;return 1;
#endif
}
#if COMPOSITE_CALLBACK
static struct S host_step(PREFIX Leaf leaf,struct S x){
 native_calls++;if(!PREFIX_OK)prefix_failed=1;struct S saved=x,expected=x;FIRST(expected)+=1;LAST(expected)+=2;struct S result=leaf(x);callback_calls++;if(!equal(result,expected)||memcmp(&x,&saved,sizeof x))callback_failed=1;x=result;FIRST(x)+=5;LAST(x)+=11;return x;
}
static const char source[]=DECL "typedef struct S (*Leaf)(struct S);struct S host_step(" PARAMS "Leaf f,struct S x);struct S leaf(struct S x){" LEAF_CHANGE "return x;}struct S round(struct S x){" ROUND_CHANGE "return host_step(" VALUES "leaf,x);}";
#else
static struct S host_step(PREFIX struct S x){native_calls++;if(!PREFIX_OK)prefix_failed=1;FIRST(x)+=5;LAST(x)+=11;return x;}
static const char source[]=DECL "struct S host_step(" PARAMS "struct S x);struct S round(struct S x){" ROUND_CHANGE "return host_step(" VALUES "x);}";
#endif
typedef struct Guarded{uint64_t before;struct S x;unsigned char guard[8];uint64_t after;} Guarded;
_Static_assert(offsetof(Guarded,guard)==offsetof(Guarded,x)+sizeof(struct S),"guard must immediately follow composite bytes");
static void quoted(const char *s){putchar('"');for(;*s;s++){unsigned char c=(unsigned char)*s;if(c=='"'||c=='\\'){putchar('\\');putchar(c);}else if(c<32)printf("\\u%04x",c);else putchar(c);}putchar('"');}
static int event(us_context *c,const char *stage,int opt,int rc){printf("{\"stage\":");quoted(stage);printf(",\"case\":%d,\"pressure\":%d,\"callback\":%d,\"opt\":%d,\"rc\":%d,\"native_calls\":%u,\"callback_calls\":%u,\"extent\":%zu,\"alignment\":%zu,\"error\":",COMPOSITE_CASE,COMPOSITE_PRESSURE,COMPOSITE_CALLBACK,opt,rc,native_calls,callback_calls,sizeof(struct S),_Alignof(struct S));quoted(c?us_error(c):"context allocation failed");puts("}");fflush(stdout);return rc;}
int main(int argc,char **argv){
 if(argc!=4)return 2;unsigned char sig[65536];FILE *f=fopen(argv[3],"rb");if(!f)return 2;size_t n=fread(sig,1,sizeof sig,f);int bad=ferror(f)||!feof(f);fclose(f);if(bad)return 2;
 for(int opt=0;opt<3;opt++){
  us_context *c=us_new(argv[1]);if(!c){event(NULL,"context",opt,1);return 1;}
  if(event(c,"registration",opt,us_add_symbol_typed(c,"host_step",(void*)host_step,sig,n))||event(c,"source_add",opt,us_add_source(c,"union-composite.c",source))||event(c,"compile",opt,us_compile(c,argv[2],opt))||event(c,"relocate",opt,us_relocate(c))){us_free(c);return 1;}
  Leaf round=(Leaf)us_sym(c,"round");if(event(c,"us_sym",opt,round?0:1)){us_free(c);return 1;}
  if((void*)round!=us_sym(c,"round")){event(c,"stable_export",opt,1);us_free(c);return 1;}
  for(int i=0;i<100;i++){
   Guarded input={0},output={0};input.before=UINT64_C(0x0123456789abcdef);input.after=UINT64_C(0xfedcba9876543210);output.before=UINT64_C(0x1122334455667788);output.after=UINT64_C(0x8877665544332211);memset(input.guard,0xa5,8);memset(output.guard,0x5a,8);
#if COMPOSITE_CASE < 2
   FIRST(input.x)=32+(i%32);LAST(input.x)=41+(i%32);
#else
   for(int j=0;j<N;j++)input.x.u[j].v=32+(i%32)+j;
#endif
   struct S original=input.x,expected=input.x;FIRST(expected)+=8+COMPOSITE_CALLBACK;LAST(expected)+=18+2*COMPOSITE_CALLBACK;
   unsigned before=native_calls,callbacks=callback_calls;output.x=round(input.x);int status=-1;int guards=1;for(int j=0;j<8;j++)if(input.guard[j]!=0xa5||output.guard[j]!=0x5a)guards=0;
   if(!equal(output.x,expected)||memcmp(&input.x,&original,sizeof original)||!guards||input.before!=UINT64_C(0x0123456789abcdef)||input.after!=UINT64_C(0xfedcba9876543210)||output.before!=UINT64_C(0x1122334455667788)||output.after!=UINT64_C(0x8877665544332211)||native_calls!=before+1||callback_calls!=callbacks+COMPOSITE_CALLBACK||prefix_failed||callback_failed||us_call_status(c,&status)){
    event(c,"value_copy_canary_prefix_callback_status",opt,1);us_free(c);return 1;
   }
  }
  event(c,"calls_100",opt,0);us_free(c);
 }
 return 0;
}

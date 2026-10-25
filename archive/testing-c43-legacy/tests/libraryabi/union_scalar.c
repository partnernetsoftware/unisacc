/* True native union types. Pure FP cases contain no integer member. */
#include "libunisacc.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef UNION_CASE
#define UNION_CASE 0
#endif
#ifndef UNION_PRESSURE
#define UNION_PRESSURE 0
#endif
#if UNION_CASE == 0
union U {unsigned long long v;long long w;};
#define UDECL "union U {unsigned long long v;long long w;};"
#define USIZE 8
#define UALIGN 8
#elif UNION_CASE == 1
union U {float v;float w;};
#define UDECL "union U {float v;float w;};"
#define USIZE 4
#define UALIGN 4
#elif UNION_CASE == 2
union U {double v;double w;};
#define UDECL "union U {double v;double w;};"
#define USIZE 8
#define UALIGN 8
#elif UNION_CASE == 3
union U {double v;unsigned long long bits;};
#define UDECL "union U {double v;unsigned long long bits;};"
#define USIZE 8
#define UALIGN 8
#elif UNION_CASE == 4
union U {double v;unsigned long long bits;long long signed_bits;};
#define UDECL "union U {double v;unsigned long long bits;long long signed_bits;};"
#define USIZE 8
#define UALIGN 8
#elif UNION_CASE == 5
union U {unsigned long long bits;long long signed_bits;double v;};
#define UDECL "union U {unsigned long long bits;long long signed_bits;double v;};"
#define USIZE 8
#define UALIGN 8
#elif UNION_CASE == 6
union U {unsigned char v;signed char w;};
#define UDECL "union U {unsigned char v;signed char w;};"
#define USIZE 1
#define UALIGN 1
#elif UNION_CASE == 7
union U {unsigned short v;short w;};
#define UDECL "union U {unsigned short v;short w;};"
#define USIZE 2
#define UALIGN 2
#elif UNION_CASE == 8
union U {unsigned int v;int w;};
#define UDECL "union U {unsigned int v;int w;};"
#define USIZE 4
#define UALIGN 4
#else
#error unknown_union_case
#endif
_Static_assert(sizeof(union U)==USIZE&&_Alignof(union U)==UALIGN,"independent native layout mismatch");
#if UNION_PRESSURE == 0
#define PREFIX
#define PROTOTYPE ""
#define VALUES ""
#define PREFIX_OK 1
#elif UNION_PRESSURE == 1
#define PREFIX unsigned long long a,unsigned long long b,unsigned long long c,unsigned long long d,unsigned long long e,unsigned long long f,unsigned long long g,unsigned long long h,unsigned long long i,
#define PROTOTYPE "unsigned long long a,unsigned long long b,unsigned long long c,unsigned long long d,unsigned long long e,unsigned long long f,unsigned long long g,unsigned long long h,unsigned long long i,"
#define VALUES "1ULL,2ULL,3ULL,4ULL,5ULL,6ULL,7ULL,8ULL,9ULL,"
#define PREFIX_OK (a==1&&b==2&&c==3&&d==4&&e==5&&f==6&&g==7&&h==8&&i==9)
#elif UNION_PRESSURE == 2
#define PREFIX double a,double b,double c,double d,double e,double f,double g,double h,double i,
#define PROTOTYPE "double a,double b,double c,double d,double e,double f,double g,double h,double i,"
#define VALUES "1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,"
#define PREFIX_OK (a==1.0&&b==2.0&&c==3.0&&d==4.0&&e==5.0&&f==6.0&&g==7.0&&h==8.0&&i==9.0)
#else
#error unknown_union_pressure
#endif
static unsigned native_calls;static int prefix_failed;
static union U host_step(PREFIX union U x){native_calls++;if(!PREFIX_OK)prefix_failed=1;x.v+=5;return x;}
static const char source[]=UDECL "union U host_step(" PROTOTYPE "union U x);union U round(union U x){x.v+=3;return host_step(" VALUES "x);}";
typedef union U (*Round)(union U);
typedef struct Guarded {uint64_t before;union U x;unsigned char guard[8];uint64_t after;} Guarded;
_Static_assert(offsetof(Guarded,guard)==offsetof(Guarded,x)+sizeof(union U),"guard must immediately follow union bytes");
static void quoted(const char *s){putchar('"');for(;*s;s++){unsigned char c=(unsigned char)*s;if(c=='"'||c=='\\'){putchar('\\');putchar(c);}else if(c<32)printf("\\u%04x",c);else putchar(c);}putchar('"');}
static int event(us_context *c,const char *stage,int opt,int rc){printf("{\"stage\":");quoted(stage);printf(",\"case\":%d,\"pressure\":%d,\"opt\":%d,\"rc\":%d,\"native_calls\":%u,\"extent\":%zu,\"alignment\":%zu,\"error\":",UNION_CASE,UNION_PRESSURE,opt,rc,native_calls,sizeof(union U),_Alignof(union U));quoted(c?us_error(c):"context allocation failed");puts("}");fflush(stdout);return rc;}
int main(int argc,char **argv){
 if(argc!=4)return 2;unsigned char signature[32768];FILE *f=fopen(argv[3],"rb");if(!f)return 2;size_t n=fread(signature,1,sizeof signature,f);int bad=ferror(f)||!feof(f);fclose(f);if(bad)return 2;
 for(int opt=0;opt<3;opt++){
  us_context *c=us_new(argv[1]);if(!c){event(NULL,"context",opt,1);return 1;}
  if(event(c,"registration",opt,us_add_symbol_typed(c,"host_step",(void*)host_step,signature,n))||event(c,"source_add",opt,us_add_source(c,"union-scalar.c",source))||event(c,"compile",opt,us_compile(c,argv[2],opt))||event(c,"relocate",opt,us_relocate(c))){us_free(c);return 1;}
  Round round=(Round)us_sym(c,"round");if(event(c,"us_sym",opt,round?0:1)){us_free(c);return 1;}
  if((void*)round!=us_sym(c,"round")){event(c,"stable_export",opt,1);us_free(c);return 1;}
  for(int i=0;i<100;i++){
   static const unsigned char input_guard[8]={0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8},output_guard[8]={0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8};
   Guarded input={UINT64_C(0x0123456789abcdef),{.v=32+(i%32)},{0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8},UINT64_C(0xfedcba9876543210)},output={UINT64_C(0x1122334455667788),{.v=0},{0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8},UINT64_C(0x8877665544332211)};
   union U original=input.x;unsigned before=native_calls;output.x=round(input.x);int status=-1;
   if(output.x.v!=original.v+8||memcmp(&original,&input.x,sizeof original)||memcmp(input.guard,input_guard,8)||memcmp(output.guard,output_guard,8)||input.before!=UINT64_C(0x0123456789abcdef)||input.after!=UINT64_C(0xfedcba9876543210)||output.before!=UINT64_C(0x1122334455667788)||output.after!=UINT64_C(0x8877665544332211)||native_calls!=before+1||prefix_failed||us_call_status(c,&status)){
    event(c,"value_copy_canary_prefix_status",opt,1);us_free(c);return 1;
   }
  }
  event(c,"calls_100",opt,0);us_free(c);
 }
 return 0;
}

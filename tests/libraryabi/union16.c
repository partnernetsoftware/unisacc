/* Public natural union16 ABI probes. No carrier or class selection here. */
#include "libunisacc.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifndef UNION16_CASE
#define UNION16_CASE 0
#endif
#ifndef UNION16_PRESSURE
#define UNION16_PRESSURE 0
#endif
#ifndef UNION16_CALLBACK
#define UNION16_CALLBACK 0
#endif
#if UNION16_CASE == 0
struct P {unsigned long long a;double b;};struct Q {unsigned long long a;float b,c;};union U {struct P p;struct Q q;};
#define DECL "struct P {unsigned long long a;double b;};struct Q {unsigned long long a;float b,c;};union U {struct P p;struct Q q;};"
#define ALIGNMENT 8
#define FIRST(x) ((x).p.a)
#define LAST(x) ((x).p.b)
#define INIT(x,n) do {(x).p.a=(n)+0;(x).p.b=(n)+1;} while(0)
#define ROUND_CHANGE "x.p.a+=3;x.p.b+=7;"
#define LEAF_CHANGE "x.p.a+=1;x.p.b+=2;"
#elif UNION16_CASE == 1
struct P {double a;unsigned long long b;};struct Q {float a,b;unsigned long long c;};union U {struct P p;struct Q q;};
#define DECL "struct P {double a;unsigned long long b;};struct Q {float a,b;unsigned long long c;};union U {struct P p;struct Q q;};"
#define ALIGNMENT 8
#define FIRST(x) ((x).p.a)
#define LAST(x) ((x).p.b)
#define INIT(x,n) do {(x).p.a=(n)+0;(x).p.b=(n)+1;} while(0)
#define ROUND_CHANGE "x.p.a+=3;x.p.b+=7;"
#define LEAF_CHANGE "x.p.a+=1;x.p.b+=2;"
#elif UNION16_CASE == 2
struct P {double a,b;};union U {struct P p;double q[2];};
#define DECL "struct P {double a,b;};union U {struct P p;double q[2];};"
#define ALIGNMENT 8
#define FIRST(x) ((x).p.a)
#define LAST(x) ((x).p.b)
#define INIT(x,n) do {(x).p.a=(n)+0;(x).p.b=(n)+1;} while(0)
#define ROUND_CHANGE "x.p.a+=3;x.p.b+=7;"
#define LEAF_CHANGE "x.p.a+=1;x.p.b+=2;"
#elif UNION16_CASE == 3
struct P {unsigned long long a;double b;};struct Q {double a;unsigned long long b;};union U {struct P p;struct Q q;};
#define DECL "struct P {unsigned long long a;double b;};struct Q {double a;unsigned long long b;};union U {struct P p;struct Q q;};"
#define ALIGNMENT 8
#define FIRST(x) ((x).p.a)
#define LAST(x) ((x).p.b)
#define INIT(x,n) do {(x).p.a=(n)+0;(x).p.b=(n)+1;} while(0)
#define ROUND_CHANGE "x.p.a+=3;x.p.b+=7;"
#define LEAF_CHANGE "x.p.a+=1;x.p.b+=2;"
#elif UNION16_CASE == 4
struct P {unsigned int a,b;float c,d;};struct Q {unsigned int a,b;float c,d;};union U {struct P p;struct Q q;};
#define DECL "struct P {unsigned int a,b;float c,d;};struct Q {unsigned int a,b;float c,d;};union U {struct P p;struct Q q;};"
#define ALIGNMENT 4
#define FIRST(x) ((x).p.a)
#define LAST(x) ((x).p.d)
#define INIT(x,n) do {(x).p.a=(n)+0;(x).p.b=(n)+1;(x).p.c=(n)+2;(x).p.d=(n)+3;} while(0)
#define ROUND_CHANGE "x.p.a+=3;x.p.d+=7;"
#define LEAF_CHANGE "x.p.a+=1;x.p.d+=2;"
#elif UNION16_CASE == 5
struct P {float a,b;unsigned int c,d;};struct Q {float a,b;unsigned int c,d;};union U {struct P p;struct Q q;};
#define DECL "struct P {float a,b;unsigned int c,d;};struct Q {float a,b;unsigned int c,d;};union U {struct P p;struct Q q;};"
#define ALIGNMENT 4
#define FIRST(x) ((x).p.a)
#define LAST(x) ((x).p.d)
#define INIT(x,n) do {(x).p.a=(n)+0;(x).p.b=(n)+1;(x).p.c=(n)+2;(x).p.d=(n)+3;} while(0)
#define ROUND_CHANGE "x.p.a+=3;x.p.d+=7;"
#define LEAF_CHANGE "x.p.a+=1;x.p.d+=2;"
#elif UNION16_CASE == 6
struct P {float a,b,c,d;};union U {struct P p;float q[4];};
#define DECL "struct P {float a,b,c,d;};union U {struct P p;float q[4];};"
#define ALIGNMENT 4
#define FIRST(x) ((x).p.a)
#define LAST(x) ((x).p.d)
#define INIT(x,n) do {(x).p.a=(n)+0;(x).p.b=(n)+1;(x).p.c=(n)+2;(x).p.d=(n)+3;} while(0)
#define ROUND_CHANGE "x.p.a+=3;x.p.d+=7;"
#define LEAF_CHANGE "x.p.a+=1;x.p.d+=2;"
#elif UNION16_CASE == 7
struct P {float a,b,c,d;};struct Q {unsigned int a,b,c,d;};union U {struct P p;struct Q q;};
#define DECL "struct P {float a,b,c,d;};struct Q {unsigned int a,b,c,d;};union U {struct P p;struct Q q;};"
#define ALIGNMENT 4
#define FIRST(x) ((x).p.a)
#define LAST(x) ((x).p.d)
#define INIT(x,n) do {(x).p.a=(n)+0;(x).p.b=(n)+1;(x).p.c=(n)+2;(x).p.d=(n)+3;} while(0)
#define ROUND_CHANGE "x.p.a+=3;x.p.d+=7;"
#define LEAF_CHANGE "x.p.a+=1;x.p.d+=2;"
#elif UNION16_CASE == 8
union U {double p[2];float q[4];};
#define DECL "union U {double p[2];float q[4];};"
#define ALIGNMENT 8
#define FIRST(x) ((x).p[0])
#define LAST(x) ((x).p[1])
#define INIT(x,n) do {(x).p[0]=(n)+0;(x).p[1]=(n)+1;} while(0)
#define ROUND_CHANGE "x.p[0]+=3;x.p[1]+=7;"
#define LEAF_CHANGE "x.p[0]+=1;x.p[1]+=2;"
#else
#error unknown_union16_case
#endif
struct Alignment {char pad;union U x;};
#define NATURAL_ALIGN offsetof(struct Alignment,x)
typedef char extent_check[(sizeof(union U)==16&&NATURAL_ALIGN==ALIGNMENT)?1:-1];
#if UNION16_PRESSURE == 0
#define PREFIX 
#define PARAMS ""
#define VALUES ""
#define PREFIX_OK (1)
#elif UNION16_PRESSURE == 1
#define PREFIX unsigned long long g0,unsigned long long g1,unsigned long long g2,unsigned long long g3,unsigned long long g4,double f0,double f1,double f2,double f3,double f4,double f5,double f6,
#define PARAMS "unsigned long long g0,unsigned long long g1,unsigned long long g2,unsigned long long g3,unsigned long long g4,double f0,double f1,double f2,double f3,double f4,double f5,double f6,"
#define VALUES "11ULL,12ULL,13ULL,14ULL,15ULL,21.0,22.0,23.0,24.0,25.0,26.0,27.0,"
#define PREFIX_OK (g0==11&&g1==12&&g2==13&&g3==14&&g4==15&&f0==21&&f1==22&&f2==23&&f3==24&&f4==25&&f5==26&&f6==27)
#elif UNION16_PRESSURE == 2
#define PREFIX unsigned long long g0,unsigned long long g1,unsigned long long g2,unsigned long long g3,unsigned long long g4,unsigned long long g5,double f0,double f1,double f2,double f3,double f4,double f5,double f6,double f7,
#define PARAMS "unsigned long long g0,unsigned long long g1,unsigned long long g2,unsigned long long g3,unsigned long long g4,unsigned long long g5,double f0,double f1,double f2,double f3,double f4,double f5,double f6,double f7,"
#define VALUES "11ULL,12ULL,13ULL,14ULL,15ULL,16ULL,21.0,22.0,23.0,24.0,25.0,26.0,27.0,28.0,"
#define PREFIX_OK (g0==11&&g1==12&&g2==13&&g3==14&&g4==15&&g5==16&&f0==21&&f1==22&&f2==23&&f3==24&&f4==25&&f5==26&&f6==27&&f7==28)
#elif UNION16_PRESSURE == 3
#define PREFIX unsigned long long g0,unsigned long long g1,unsigned long long g2,unsigned long long g3,unsigned long long g4,unsigned long long g5,unsigned long long g6,double f0,double f1,double f2,double f3,double f4,double f5,double f6,
#define PARAMS "unsigned long long g0,unsigned long long g1,unsigned long long g2,unsigned long long g3,unsigned long long g4,unsigned long long g5,unsigned long long g6,double f0,double f1,double f2,double f3,double f4,double f5,double f6,"
#define VALUES "11ULL,12ULL,13ULL,14ULL,15ULL,16ULL,17ULL,21.0,22.0,23.0,24.0,25.0,26.0,27.0,"
#define PREFIX_OK (g0==11&&g1==12&&g2==13&&g3==14&&g4==15&&g5==16&&g6==17&&f0==21&&f1==22&&f2==23&&f3==24&&f4==25&&f5==26&&f6==27)
#else
#error unknown_union16_pressure
#endif
typedef union U (*Leaf)(union U);
static unsigned native_calls,callback_calls;static int prefix_failed,callback_failed;
static int equal(union U a,union U b){return memcmp(&a,&b,sizeof a)==0;}
#if UNION16_CALLBACK
static union U host_step(PREFIX Leaf leaf,union U x,unsigned long long tail,double ftail){
 native_calls++;if(!PREFIX_OK||tail!=99||ftail!=109.0)prefix_failed=1;
 union U saved=x,expected=x;FIRST(expected)+=1;LAST(expected)+=2;
 union U result=leaf(x);callback_calls++;if(!equal(result,expected)||memcmp(&x,&saved,sizeof x))callback_failed=1;
 x=result;FIRST(x)+=5;LAST(x)+=11;return x;
}
static const char source[]=DECL "typedef union U (*Leaf)(union U);union U host_step(" PARAMS "Leaf f,union U x,unsigned long long tail,double ftail);union U leaf(union U x){" LEAF_CHANGE "return x;}union U round(union U x){" ROUND_CHANGE "return host_step(" VALUES "leaf,x,99ULL,109.0);}";
#else
static union U host_step(PREFIX union U x,unsigned long long tail,double ftail){native_calls++;if(!PREFIX_OK||tail!=99||ftail!=109.0)prefix_failed=1;FIRST(x)+=5;LAST(x)+=11;return x;}
static const char source[]=DECL "union U host_step(" PARAMS "union U x,unsigned long long tail,double ftail);union U round(union U x){" ROUND_CHANGE "return host_step(" VALUES "x,99ULL,109.0);}";
#endif
typedef struct Guarded {uint64_t before;union U x;unsigned char guard[8];uint64_t after;} Guarded;
typedef char guard_check[(offsetof(Guarded,guard)==offsetof(Guarded,x)+sizeof(union U))?1:-1];
static void quoted(const char *s){putchar('"');for(;*s;s++){unsigned char c=(unsigned char)*s;if(c=='"'||c=='\\'){putchar('\\');putchar(c);}else if(c<32)printf("\\u%04x",c);else putchar(c);}putchar('"');}
static int event(us_context *c,const char *stage,int opt,int rc){printf("{\"stage\":");quoted(stage);printf(",\"case\":%d,\"pressure\":%d,\"callback\":%d,\"opt\":%d,\"rc\":%d,\"native_calls\":%u,\"callback_calls\":%u,\"extent\":%zu,\"alignment\":%zu,\"error\":",UNION16_CASE,UNION16_PRESSURE,UNION16_CALLBACK,opt,rc,native_calls,callback_calls,sizeof(union U),(size_t)NATURAL_ALIGN);quoted(c?us_error(c):"context allocation failed");puts("}");fflush(stdout);return rc;}
int main(int argc,char **argv){
 if(argc!=4)return 2;unsigned char sig[65536];FILE *f=fopen(argv[3],"rb");if(!f){perror("signature open");return 2;}size_t n=fread(sig,1,sizeof sig,f);int bad=ferror(f)||!feof(f);if(fclose(f)||bad){perror("signature read");return 2;}
 for(int opt=0;opt<3;opt++){
  us_context *c=us_new(argv[1]);if(!c){event(NULL,"context",opt,1);return 1;}
  if(event(c,"registration",opt,us_add_symbol_typed(c,"host_step",(void*)host_step,sig,n))||event(c,"source_add",opt,us_add_source(c,"union16.c",source))||event(c,"compile",opt,us_compile(c,argv[2],opt))||event(c,"relocate",opt,us_relocate(c))){us_free(c);return 1;}
  Leaf round=(Leaf)us_sym(c,"round");if(event(c,"us_sym",opt,round?0:1)){us_free(c);return 1;}
  if((void*)round!=us_sym(c,"round")){event(c,"stable_export",opt,1);us_free(c);return 1;}
  for(int i=0;i<100;i++){
   Guarded input={0},output={0};input.before=UINT64_C(0x0123456789abcdef);input.after=UINT64_C(0xfedcba9876543210);output.before=UINT64_C(0x1122334455667788);output.after=UINT64_C(0x8877665544332211);memset(input.guard,0xa5,8);memset(output.guard,0x5a,8);INIT(input.x,32+(i%32));
   union U original=input.x,expected=input.x;FIRST(expected)+=8+UNION16_CALLBACK;LAST(expected)+=18+2*UNION16_CALLBACK;
   unsigned before=native_calls,callbacks=callback_calls;output.x=round(input.x);int status=-1;int guards=1;for(int j=0;j<8;j++)if(input.guard[j]!=0xa5||output.guard[j]!=0x5a)guards=0;
   if(!equal(output.x,expected)||memcmp(&input.x,&original,sizeof original)||!guards||input.before!=UINT64_C(0x0123456789abcdef)||input.after!=UINT64_C(0xfedcba9876543210)||output.before!=UINT64_C(0x1122334455667788)||output.after!=UINT64_C(0x8877665544332211)||native_calls!=before+1||callback_calls!=callbacks+UNION16_CALLBACK||prefix_failed||callback_failed||us_call_status(c,&status)){
    event(c,"value_copy_canary_prefix_tail_callback_status",opt,1);us_free(c);return 1;
   }
  }
  event(c,"calls_100",opt,0);us_free(c);
 }
 return 0;
}

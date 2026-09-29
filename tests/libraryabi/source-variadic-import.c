/* True public source -> model facts -> V3 cross-origin VARIADIC import -> native C varargs.
 * The external prototype is a hand-written origin-2 V3 graph; the source declares the
 * same prototype (origin 1); each concrete call site is certified by the model, and the
 * native callee validates every tail value itself. Neither the caller nor the host adapter
 * selects ABI carriers or compares origins. */
#include "libunisacc.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef SOURCE_TARGET
#define SOURCE_TARGET "osx/arm64"
#endif
typedef struct {unsigned int a:5;signed int b:7;unsigned int c:20;} B4;
static unsigned native_calls,native_bad;static int different_width;
static B4 hv(B4 x,int mode,...){
 va_list ap;va_start(ap,mode);native_calls++;
 if(mode==2){double d=va_arg(ap,double);int i=va_arg(ap,int);if(d!=1.5||i!=7)native_bad++;x.a=(x.a+i)&31;x.b-=1;x.c+=(unsigned)(d*2);}
 else if(mode==0){x.a=(x.a+5)&31;x.b-=2;x.c+=11;}
 else if(mode==3){unsigned u=va_arg(ap,unsigned);double d=va_arg(ap,double);long l=va_arg(ap,long);if(u!=x.c||d!=2.5||l!=-9L)native_bad++;x.a=(x.a+1)&31;x.b+=1;x.c+=5+(unsigned)(-l)+(unsigned)d;}
 else native_bad++;
 va_end(ap);return x;
}
_Static_assert(sizeof(B4)==4&&_Alignof(B4)==4,"layout fixture");
static unsigned char signature[4096];static size_t at;
static void byte(unsigned v){assert(at<sizeof signature);signature[at++]=(unsigned char)v;}
static void word(uint64_t v){for(unsigned i=0;i<8;i++)byte((unsigned)(v>>(8*i)));}
static void head(unsigned kind,unsigned width,unsigned uns,unsigned align,unsigned rank,unsigned format,unsigned tag,size_t payload){
 word(0);word(0);word(0);word(kind);word(width);word(uns);word(align);
 byte(rank);byte(format);word(align);byte(0);byte(3);byte(2);byte(tag);word(payload);
}
static void leaf(unsigned width,unsigned uns){head(1,width,uns,width,0,0,0,0);}
static void bits(void){
 /* V3 scalar descriptor=78 bytes; member prefix=49 bytes; same natural B4 as source-bitfield-import.c. */
 head(5,4,0,4,0,0,1,8+3*(49+78));word(3);
 unsigned pos[3]={0,5,12};unsigned len[3]={5,7,20};
 for(unsigned i=0;i<3;i++){word(i);byte(1);word(4);word(0);word(pos[i]);word(len[i]-(different_width && i==2));word(4);leaf(4,i!=1);}
}
static void bind(us_context*c){
 /* One record: linkage0 defined1 variadic1 mode1, fixed count 2 (B4,int), result B4, support0. */
 at=0;for(unsigned i=0;i<8;i++)byte((unsigned char)"USLSIG3\n"[i]);word(1);word(2);byte('h');byte('v');
 byte(0);byte(1);byte(1);byte(1);word(2);bits();word(2);bits();leaf(4,0);byte(0);
 int rc=us_add_symbol_typed(c,"hv",(void*)hv,signature,at);if(rc){fprintf(stderr,"bind: %s\n",us_error(c));exit(1);}
}
static const char *source=
 "typedef struct {unsigned int a:5;signed int b:7;unsigned int c:20;} B4;"
 "B4 hv(B4,int,...);"
 "B4 f4(B4 x){return hv(x,2,1.5,7);}"
 "B4 f8(B4 x){return hv(x,0);}"
 "B4 fd(B4 x){return hv(x,3,x.c,2.5,-9L);}";
static void check(int rc,us_context*c){if(rc){fprintf(stderr,"API: %s\n",us_error(c));exit(1);}}
int main(int argc,char **argv){
 assert(argc==2);unsigned calls=0;
 for(int opt=0;opt<3;opt++){
  us_context*c=us_new(argv[1]);assert(c);check(us_set_signature_version(c,3),c);bind(c);
  check(us_add_source(c,"variadic.c",source),c);check(us_compile(c,SOURCE_TARGET,opt),c);check(us_relocate(c),c);
  B4(*f4)(B4)=(B4(*)(B4))us_sym(c,"f4");B4(*f8)(B4)=(B4(*)(B4))us_sym(c,"f8");B4(*fd)(B4)=(B4(*)(B4))us_sym(c,"fd");
  if(!f4||!f8||!fd){fprintf(stderr,"symbol: %s\n",us_error(c));return 1;}
  for(unsigned i=0;i<100;i++){
   B4 x={i&31,-3,100+i},y=f4(x);assert(y.a==((x.a+7)&31)&&y.b==-4&&y.c==x.c+3);calls++;
   B4 z=f8(x);assert(z.a==((x.a+5)&31)&&z.b==-5&&z.c==x.c+11);calls++;
   B4 r=fd(x);assert(r.a==((x.a+1)&31)&&r.b==-2&&r.c==x.c+16);calls++;
   assert(x.a==(i&31)&&x.b==-3&&x.c==100+i);
  }
  us_free(c);
 }
 /* A different prefix field width and unknown source provenance cannot cross. */
 for(unsigned bad=0;bad<2;bad++){
  us_context*c=us_new(argv[1]);assert(c);check(us_set_signature_version(c,3),c);
  different_width=bad==0;bind(c);different_width=0;
  char*text=malloc(strlen(source)+40);assert(text);snprintf(text,strlen(source)+40,"%s%s",bad==1?"#pragma pack(1)\n":"",source);
  check(us_add_source(c,"rejected.c",text),c);free(text);
  assert(us_compile(c,SOURCE_TARGET,0)!=0);assert(native_calls==900);us_free(c);
 }
 assert(native_calls==900&&native_bad==0);
 printf("source variadic imports: %u script-to-native calls; provenance preserved\n",calls);return 0;
}

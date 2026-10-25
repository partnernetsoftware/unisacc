/* True public source -> V3 cross-origin import whose prototype carries a data pointer.
 * The external origin-2 graph spells the pointee (V3 tag 5); the source declares
 * the same prototype. Identity: a pointer to a different pointee, or an opaque
 * external pointer, must be refused by the model; the host never compares types. */
#include "libunisacc.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef SOURCE_TARGET
#define SOURCE_TARGET "osx/arm64"
#endif
typedef struct {unsigned int a:5;signed int b:7;unsigned int c:20;} B4;
static unsigned native_calls;static int pointee_variant; /* 0 int, 1 double, 2 opaque */
static B4 hp(B4 x,int *p){native_calls++;*p+=(int)x.a;x.a=(x.a+5)&31;x.b-=2;x.c+=(unsigned)*p;return x;}
_Static_assert(sizeof(B4)==4&&_Alignof(B4)==4,"layout fixture");
static unsigned char signature[4096];static size_t at;
static void byte(unsigned v){assert(at<sizeof signature);signature[at++]=(unsigned char)v;}
static void word(uint64_t v){for(unsigned i=0;i<8;i++)byte((unsigned)(v>>(8*i)));}
static void head(unsigned depth,unsigned kind,unsigned width,unsigned uns,unsigned align,unsigned rank,unsigned format,unsigned tag,size_t payload){
 word(depth);word(0);word(0);word(kind);word(width);word(uns);word(align);
 byte(rank);byte(format);word(align);byte(0);byte(3);byte(2);byte(tag);word(payload);
}
static void leaf(unsigned kind,unsigned width,unsigned uns,unsigned rank,unsigned format){head(0,kind,width,uns,width,rank,format,0,0);}
static void bits(void){
 head(0,5,4,0,4,0,0,1,8+3*(49+78));word(3);
 unsigned pos[3]={0,5,12};unsigned len[3]={5,7,20};
 for(unsigned i=0;i<3;i++){word(i);byte(1);word(4);word(0);word(pos[i]);word(len[i]);word(4);leaf(1,4,i!=1,0,0);}
}
static void pointer(void){
 /* V3 scalar descriptor is 78 bytes. */
 if(pointee_variant==2){head(1,2,8,0,8,0,0,0,0);return;}
 head(1,2,8,0,8,0,0,5,78);
 if(pointee_variant==0)leaf(1,4,0,0,0);else leaf(3,8,0,2,2);
}
static void bind(us_context*c){
 at=0;for(unsigned i=0;i<8;i++)byte((unsigned char)"USLSIG3\n"[i]);word(1);word(2);byte('h');byte('p');
 byte(0);byte(1);byte(0);byte(0);word(2);bits();word(2);bits();pointer();byte(0);
 int rc=us_add_symbol_typed(c,"hp",(void*)hp,signature,at);if(rc){fprintf(stderr,"bind: %s\n",us_error(c));exit(1);}
}
static const char *source=
 "typedef struct {unsigned int a:5;signed int b:7;unsigned int c:20;} B4;"
 "B4 hp(B4,int*);"
 "B4 f4(B4 x){int n=3;B4 y=hp(x,&n);y.c+=(unsigned)n;return y;}";
static void check(int rc,us_context*c){if(rc){fprintf(stderr,"API: %s\n",us_error(c));exit(1);}}
int main(int argc,char **argv){
 assert(argc==2);unsigned calls=0;
 for(int opt=0;opt<3;opt++){
  us_context*c=us_new(argv[1]);assert(c);check(us_set_signature_version(c,3),c);bind(c);
  check(us_add_source(c,"pointee.c",source),c);check(us_compile(c,SOURCE_TARGET,opt),c);check(us_relocate(c),c);
  B4(*f4)(B4)=(B4(*)(B4))us_sym(c,"f4");if(!f4){fprintf(stderr,"symbol: %s\n",us_error(c));return 1;}
  for(unsigned i=0;i<300;i++){
   B4 x={i&31,-3,100+i},y=f4(x);unsigned n=3+(i&31);
   assert(y.a==((x.a+5)&31)&&y.b==-5&&y.c==x.c+2*n);calls++;
   assert(x.a==(i&31)&&x.b==-3&&x.c==100+i);
  }
  us_free(c);
 }
 /* A different pointee (double) and an opaque external pointer cannot bind to int*. */
 for(unsigned bad=1;bad<3;bad++){
  us_context*c=us_new(argv[1]);assert(c);check(us_set_signature_version(c,3),c);
  pointee_variant=(int)bad;bind(c);pointee_variant=0;
  check(us_add_source(c,"rejected.c",source),c);
  assert(us_compile(c,SOURCE_TARGET,0)!=0);assert(native_calls==900);us_free(c);
 }
 assert(native_calls==900);
 printf("source pointee imports: %u script-to-native calls; pointee identity preserved\n",calls);return 0;
}

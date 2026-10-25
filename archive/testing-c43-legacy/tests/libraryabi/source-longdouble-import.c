/* True public source -> V3 cross-origin import with a long double parameter.
 * On targets whose long double IS IEEE64 (Apple arm64, Windows) the model
 * certifies the F64-stored rank-3 value and the native call must be bit-exact.
 * On x87 / IEEE128 targets the model must refuse: a rank-3 value folded to F64
 * is never certified there. Neither the caller nor the host chooses. */
#include "libunisacc.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef SOURCE_TARGET
#define SOURCE_TARGET "osx/arm64"
#endif
static unsigned native_calls;
static double hl(long double a,int n){native_calls++;return (double)(a*(long double)n)+0.5;}
static unsigned char signature[4096];static size_t at;
static void byte(unsigned v){assert(at<sizeof signature);signature[at++]=(unsigned char)v;}
static void word(uint64_t v){for(unsigned i=0;i<8;i++)byte((unsigned)(v>>(8*i)));}
static void head(unsigned depth,unsigned kind,unsigned width,unsigned uns,unsigned align,unsigned rank,unsigned format,unsigned tag,size_t payload){
 word(depth);word(0);word(0);word(kind);word(width);word(uns);word(align);
 byte(rank);byte(format);word(align);byte(0);byte(3);byte(2);byte(tag);word(payload);
}
static void bind(us_context*c){
 /* double hl(long double, int): result F64 rank2; params rank3/format2 (F64 storage), int. */
 at=0;for(unsigned i=0;i<8;i++)byte((unsigned char)"USLSIG3\n"[i]);word(1);word(2);byte('h');byte('l');
 byte(0);byte(1);byte(0);byte(0);word(2);head(0,3,8,0,8,2,2,0,0);word(2);head(0,3,8,0,8,3,2,0,0);head(0,1,4,0,4,0,0,0,0);byte(0);
 int rc=us_add_symbol_typed(c,"hl",(void*)hl,signature,at);if(rc){fprintf(stderr,"bind: %s\n",us_error(c));exit(1);}
}
static const char *source=
 "double hl(long double,int);"
 "double f4(double d,int n){return hl((long double)d,n);}";
static void check(int rc,us_context*c){if(rc){fprintf(stderr,"API: %s\n",us_error(c));exit(1);}}
int main(int argc,char **argv){
 assert(argc==2);unsigned calls=0;
 int ieee64=sizeof(long double)==8; /* the host ISA's own long double: Apple arm64 / Windows */
 for(int opt=0;opt<3;opt++){
  us_context*c=us_new(argv[1]);assert(c);check(us_set_signature_version(c,3),c);bind(c);
  check(us_add_source(c,"longdouble.c",source),c);int rc=us_compile(c,SOURCE_TARGET,opt);
  if(!ieee64){assert(rc!=0);us_free(c);continue;}
  check(rc,c);check(us_relocate(c),c);
  double(*f4)(double,int)=(double(*)(double,int))us_sym(c,"f4");if(!f4){fprintf(stderr,"symbol: %s\n",us_error(c));return 1;}
  for(unsigned i=0;i<300;i++){
   double d=1.25+i,want=(double)((long double)d*(long double)(int)(i%7+1))+0.5,got=f4(d,(int)(i%7+1));
   assert(memcmp(&want,&got,sizeof got)==0);calls++;
  }
  us_free(c);
 }
 if(!ieee64){assert(native_calls==0);printf("source long double imports: refused on a non-IEEE64 long double target\n");return 0;}
 assert(native_calls==900);
 printf("source long double imports: 900 script-to-native calls; IEEE64 long double bit-exact\n");return 0;
}

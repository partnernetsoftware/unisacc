/* True public source -> model facts -> model carrier -> native C call.
 * Neither this caller nor the host adapter selects ABI carriers. */
#include "libunisacc.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef SOURCE_TARGET
#define SOURCE_TARGET "osx/arm64"
#endif
typedef struct {unsigned int a:5;signed int b:7;unsigned int c:20;} B4;
typedef struct {unsigned long long a:9;signed long long b:17;unsigned long long c:38;} B8;
typedef struct {double d;B8 bits;} DB16;
static const char *source=
 "typedef struct {unsigned int a:5;signed int b:7;unsigned int c:20;} B4;"
 "typedef struct {unsigned long long a:9;signed long long b:17;unsigned long long c:38;} B8;"
 "typedef struct {double d;B8 bits;} DB16;"
 "B4 f4(B4 x){x.a=(x.a+3)&31;x.b=x.b-1;x.c=x.c+7;return x;}"
 "B8 f8(B8 x){x.a=(x.a+3)&511;x.b=x.b-1;x.c=x.c+7;return x;}"
 "DB16 fd(DB16 x){x.d=x.d+0.5;x.bits.a=(x.bits.a+3)&511;x.bits.b=x.bits.b-1;x.bits.c=x.bits.c+7;return x;}";
static void check(int rc,us_context*c){if(rc){fprintf(stderr,"API: %s\n",us_error(c));exit(1);}}
int main(int argc,char **argv){
 assert(argc==2);unsigned calls=0;
 for(int opt=0;opt<3;opt++){
  us_context*c=us_new(argv[1]);assert(c);check(us_set_signature_version(c,3),c);
  check(us_add_source(c,"bitfields.c",source),c);check(us_compile(c,SOURCE_TARGET,opt),c);check(us_relocate(c),c);
  B4(*f4)(B4)=(B4(*)(B4))us_sym(c,"f4");
  B8(*f8)(B8)=(B8(*)(B8))us_sym(c,"f8");
  DB16(*fd)(DB16)=(DB16(*)(DB16))us_sym(c,"fd");
  if(!f4||!f8||!fd){fprintf(stderr,"symbol: %s\n",us_error(c));return 1;}
  for(unsigned i=0;i<100;i++){
   B4 x={i&31,-3,100+i},y=f4(x);assert(y.a==((x.a+3)&31)&&y.b==-4&&y.c==x.c+7);calls++;
   B8 u={i,-300,10000+i},v=f8(u);assert(v.a==i+3&&v.b==-301&&v.c==u.c+7);calls++;
   DB16 m={i+0.25,{i,-300,10000+i}},n=fd(m);assert(n.d==m.d+0.5&&n.bits.a==i+3&&n.bits.b==-301&&n.bits.c==m.bits.c+7);calls++;
  }
  us_free(c);
 }
 /* Unknown layout syntax must not become a usable native pointer. */
 us_context*c=us_new(argv[1]);assert(c);check(us_set_signature_version(c,3),c);
 char*unknown=malloc(strlen(source)+40);assert(unknown);snprintf(unknown,strlen(source)+40,"#pragma pack(1)\n%s",source);
 check(us_add_source(c,"unknown.c",unknown),c);free(unknown);check(us_compile(c,SOURCE_TARGET,0),c);check(us_relocate(c),c);
 assert(!us_sym(c,"f4"));us_free(c);
 printf("source bitfields: %u actual native calls; unknown modifier refused\n",calls);return 0;
}

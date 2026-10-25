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
static unsigned native_calls;static int different_width;
#ifdef SOURCE_CALLBACK_IMPORT
typedef B4 (*Leaf)(B4);
static unsigned apply_calls,choose_calls,echo_calls;
static Leaf observed_script;
static B4 apply(Leaf f,B4 x){
 native_calls++;apply_calls++;
 if(observed_script)assert(f==observed_script);else observed_script=f;
 B4 y=f(x);y.a=(y.a+2)&31;y.c+=4;return y;
}
static Leaf choose(void);
static Leaf echo(Leaf f){
 native_calls++;echo_calls++;assert(f==observed_script);return f;
}
static int nested_bad;
#endif
static B4 h4(B4 x){native_calls++;x.a=(x.a+5)&31;x.b-=2;x.c+=11;return x;}
#ifdef SOURCE_CALLBACK_IMPORT
static Leaf choose(void){native_calls++;choose_calls++;return h4;}
#endif
static B8 h8(B8 x){native_calls++;x.a=(x.a+5)&511;x.b-=2;x.c+=11;return x;}
static DB16 hd(DB16 x){native_calls++;x.d+=1.25;x.bits=h8(x.bits);return x;}
/* Independent declarations for layouts measured by the C caller. */
_Static_assert(sizeof(B4)==4&&_Alignof(B4)==4&&sizeof(B8)==8&&_Alignof(B8)==8&&sizeof(DB16)==16&&_Alignof(DB16)==8,"layout fixture");
static unsigned char signature[4096];static size_t at;
static void byte(unsigned v){assert(at<sizeof signature);signature[at++]=(unsigned char)v;}
static void word(uint64_t v){for(unsigned i=0;i<8;i++)byte((unsigned)(v>>(8*i)));}
static void head(unsigned kind,unsigned width,unsigned uns,unsigned align,unsigned rank,unsigned format,unsigned tag,size_t payload){
 word(0);word(0);word(0);word(kind);word(width);word(uns);word(align);
 byte(rank);byte(format);word(align);byte(0);byte(3);byte(2);byte(tag);word(payload);
}
static void leaf(unsigned width,unsigned uns){head(1,width,uns,width,0,0,0,0);}
static void bits(unsigned width){
 /* V3 scalar descriptor=78 bytes; member prefix=49 bytes. */
 head(5,width,0,width,0,0,1,8+3*(49+78));word(3);
 unsigned pos[3]={0,width==4?5:9,width==4?12:26};unsigned len[3]={width==4?5:9,width==4?7:17,width==4?20:38};
 for(unsigned i=0;i<3;i++){word(i);byte(1);word(width);word(0);word(pos[i]);word(len[i]-(different_width && width==4 && i==2));word(width);leaf(width,i!=1);}
}
static void descriptor(unsigned n){
 if(n<2){bits(n?8:4);return;}
 head(5,16,0,8,0,0,1,8+(49+78)+(49+78+8+3*(49+78)));word(2);
 word(0);byte(0);word(8);word(0);word(0);word(0);word(8);head(3,8,0,8,2,2,0,0);
 word(1);byte(0);word(8);word(8);word(0);word(0);word(8);bits(8);
}
#ifdef SOURCE_CALLBACK_IMPORT
/* An independently written recursive graph: no source signature dump or origin rewrite. */
static void callback(void){
 /* Callback payload: schema/ref/id/var/mode/count/result/stored args/support. */
 word(1);word(0);word(0);word(4);word(8);word(0);word(8);
 byte(0);byte(0);word(8);byte(0);byte(3);byte(2);byte(4);
 word(2+8+2+8+467+8+467+1);
 /* One parameter, not variadic: mode 0 for nested and root alike (modelcallbacksourcecheck relay). */
 byte(1);byte(0);word(1);byte(0);byte(0);word(1);
 int saved=different_width;different_width=nested_bad;
 bits(4);word(1);bits(4);different_width=saved;byte(0);
}
#endif
static void bind(us_context*c,unsigned n){
#ifdef SOURCE_CALLBACK_IMPORT
 const char*names[]={"apply","choose","echo"};void*functions[]={(void*)apply,(void*)choose,(void*)echo};
 at=0;for(unsigned i=0;i<8;i++)byte((unsigned char)"USLSIG3\n"[i]);word(1);word(strlen(names[n]));
 for(size_t i=0;i<strlen(names[n]);i++)byte((unsigned char)names[n][i]);
 byte(0);byte(1);byte(0);byte(0);word(n==0?2:n==1?0:1);
 if(n==0)bits(4);else callback();
 word(n==0?2:n==1?0:1);
 if(n==0)callback();
 if(n==2){
  word(1);word(0);word(0);word(4);word(8);word(0);word(8);
  byte(0);byte(0);word(8);byte(0);byte(3);byte(2);byte(4);word(10);
  byte(1);byte(1);word(1);
 }
 if(n==0)bits(4);
 byte(0);
#else
 const char*names[]={"h4","h8","hd"};void*functions[]={(void*)h4,(void*)h8,(void*)hd};
 at=0;for(unsigned i=0;i<8;i++)byte((unsigned char)"USLSIG3\n"[i]);word(1);word(2);byte(names[n][0]);byte(names[n][1]);byte(0);byte(1);byte(0);byte(0);word(1);descriptor(n);word(1);descriptor(n);byte(0);
#endif
 int rc=us_add_symbol_typed(c,names[n],functions[n],signature,at);if(rc){fprintf(stderr,"bind: %s\n",us_error(c));exit(1);}
}
static const char *source=
 "typedef struct {unsigned int a:5;signed int b:7;unsigned int c:20;} B4;"
 "typedef struct {unsigned long long a:9;signed long long b:17;unsigned long long c:38;} B8;"
 "typedef struct {double d;B8 bits;} DB16;"
#ifdef SOURCE_CALLBACK_IMPORT
 "typedef B4 (*Leaf)(B4);"
 "B4 apply(Leaf,B4);Leaf choose(void);Leaf echo(Leaf);"
 "B4 script(B4 x){x.a=(x.a+3)&31;x.b-=1;x.c+=7;return x;}"
 "B4 f4(B4 x){return apply(script,x);}"
 "B4 f8(B4 x){Leaf p=choose();return p(x);}"
 "B4 fd(B4 x){Leaf p=echo(script);return p(x);}"
#else
 "B4 h4(B4);B8 h8(B8);DB16 hd(DB16);"
#ifdef SOURCE_CALLABLE_IMPORT
 "B4 f4(B4 x){B4 (*p)(B4)=h4;return p(x);}"
 "B8 f8(B8 x){B8 (*p)(B8)=h8;return p(x);}"
 "DB16 fd(DB16 x){DB16 (*p)(DB16)=hd;return p(x);}"
#else
 "B4 f4(B4 x){return h4(x);}"
 "B8 f8(B8 x){return h8(x);}"
 "DB16 fd(DB16 x){return hd(x);}"
#endif
#endif
 ;
static void check(int rc,us_context*c){if(rc){fprintf(stderr,"API: %s\n",us_error(c));exit(1);}}
int main(int argc,char **argv){
 assert(argc==2);unsigned calls=0;
 for(int opt=0;opt<3;opt++){
  us_context*c=us_new(argv[1]);assert(c);check(us_set_signature_version(c,3),c);for(unsigned n=0;n<3;n++)bind(c,n);
  check(us_add_source(c,"bitfields.c",source),c);check(us_compile(c,SOURCE_TARGET,opt),c);check(us_relocate(c),c);
  B4(*f4)(B4)=(B4(*)(B4))us_sym(c,"f4");
#ifdef SOURCE_CALLBACK_IMPORT
  B4(*f8)(B4)=(B4(*)(B4))us_sym(c,"f8");
  B4(*fd)(B4)=(B4(*)(B4))us_sym(c,"fd");
#else
  B8(*f8)(B8)=(B8(*)(B8))us_sym(c,"f8");
  DB16(*fd)(DB16)=(DB16(*)(DB16))us_sym(c,"fd");
#endif
  if(!f4||!f8||!fd){fprintf(stderr,"symbol: %s\n",us_error(c));return 1;}
  for(unsigned i=0;i<100;i++){
#ifdef SOURCE_CALLBACK_IMPORT
   B4 x={i&31,-3,100+i},y=f4(x);assert(y.a==((x.a+5)&31)&&y.b==-4&&y.c==x.c+11);calls++;
   B4 z=f8(x);assert(z.a==((x.a+5)&31)&&z.b==-5&&z.c==x.c+11);calls++;
   B4 r=fd(x);assert(r.a==((x.a+3)&31)&&r.b==-4&&r.c==x.c+7);calls++;
   assert(x.a==(i&31)&&x.b==-3&&x.c==100+i);
#else
   B4 x={i&31,-3,100+i},y=f4(x);assert(y.a==((x.a+5)&31)&&y.b==-5&&y.c==x.c+11);calls++;
   B8 u={i,-300,10000+i},v=f8(u);assert(v.a==i+5&&v.b==-302&&v.c==u.c+11);calls++;
   DB16 m={i+0.25,{i,-300,10000+i}},n=fd(m);assert(n.d==m.d+1.25&&n.bits.a==i+5&&n.bits.b==-302&&n.bits.c==m.bits.c+11);calls++;
#endif
  }
#ifdef SOURCE_CALLBACK_IMPORT
  observed_script=NULL;
#endif
  us_free(c);
 }
 /* A different field width and unknown source provenance cannot cross. */
 for(unsigned bad=0;bad<2;bad++){
  us_context*c=us_new(argv[1]);assert(c);check(us_set_signature_version(c,3),c);
#ifdef SOURCE_CALLBACK_IMPORT
  nested_bad=bad==0;for(unsigned n=0;n<3;n++)bind(c,n);nested_bad=0;
#else
  different_width=bad==0;for(unsigned n=0;n<3;n++)bind(c,n);different_width=0;
#endif
  char*text=malloc(strlen(source)+40);assert(text);snprintf(text,strlen(source)+40,"%s%s",bad==1?"#pragma pack(1)\n":"",source);
  check(us_add_source(c,"rejected.c",text),c);free(text);
  assert(us_compile(c,SOURCE_TARGET,0)!=0);assert(native_calls==1200);us_free(c);
 }
 assert(native_calls==1200);
#ifdef SOURCE_CALLBACK_IMPORT
 assert(apply_calls==300&&choose_calls==300&&echo_calls==300);
 printf("source callback imports: %u public entries; SCRIPT callback, native callback and closure roundtrip preserved\n",calls);
#else
 printf("source imports: %u script-to-native calls; provenance preserved\n",calls);
#endif
 return 0;
}

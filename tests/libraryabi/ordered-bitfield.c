/* True C layout observations and model-certificate calls. No ABI classifier. */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {unsigned int a:5;signed int b:7;unsigned int c:20;} B4;
typedef struct {unsigned long long a:9;signed long long b:17;unsigned long long c:38;} B8;
typedef struct {B8 bits;double d;} BD16;
typedef struct {double d;B8 bits;} DB16;
static unsigned first_bit(const void *p,size_t n){const unsigned char*b=p;for(size_t i=0;i<n;i++)for(unsigned k=0;k<8;k++)if(b[i]&(1u<<k))return (unsigned)(i*8+k);abort();}
static void facts(void){B4 a={0};B8 b={0};unsigned p[6];a.a=1;p[0]=first_bit(&a,sizeof a);memset(&a,0,sizeof a);a.b=1;p[1]=first_bit(&a,sizeof a);memset(&a,0,sizeof a);a.c=1;p[2]=first_bit(&a,sizeof a);b.a=1;p[3]=first_bit(&b,sizeof b);memset(&b,0,sizeof b);b.b=1;p[4]=first_bit(&b,sizeof b);memset(&b,0,sizeof b);b.c=1;p[5]=first_bit(&b,sizeof b);
 printf("{\"b4\":[%zu,%zu],\"b8\":[%zu,%zu],\"bd16\":[%zu,%zu,%zu],\"db16\":[%zu,%zu,%zu],\"positions\":[%u,%u,%u,%u,%u,%u],\"units\":[%zu,%zu]}\n",sizeof a,_Alignof(B4),sizeof b,_Alignof(B8),sizeof(BD16),_Alignof(BD16),offsetof(BD16,d),sizeof(DB16),_Alignof(DB16),offsetof(DB16,bits),p[0],p[1],p[2],p[3],p[4],p[5],sizeof(unsigned int),sizeof(unsigned long long));}
#ifdef FACTS_ONLY
int main(void){facts();return 0;}
#else
#define US_CALLABLES_IMPLEMENTATION
#include "exec/c/librarycarrierplan.h"
static unsigned native_calls,script_calls;static int failure;
static void transform(void *p,size_t n){unsigned char*b=p;for(size_t i=0;i<n;i++)b[i]^=(unsigned char)(0x31+3*i);}
#define P0
#define V0
#define C0 1
#define P1 uint64_t g0,uint64_t g1,uint64_t g2,uint64_t g3,uint64_t g4,double f0,double f1,double f2,double f3,double f4,double f5,double f6,
#define V1 11,12,13,14,15,21,22,23,24,25,26,27,
#define C1 (g0==11&&g1==12&&g2==13&&g3==14&&g4==15&&f0==21&&f1==22&&f2==23&&f3==24&&f4==25&&f5==26&&f6==27)
#define P2 uint64_t g0,uint64_t g1,uint64_t g2,uint64_t g3,uint64_t g4,uint64_t g5,uint64_t g6,double f0,double f1,double f2,double f3,double f4,double f5,double f6,double f7,
#define V2 11,12,13,14,15,16,17,21,22,23,24,25,26,27,28,
#define C2 (g0==11&&g1==12&&g2==13&&g3==14&&g4==15&&g5==16&&g6==17&&f0==21&&f1==22&&f2==23&&f3==24&&f4==25&&f5==26&&f6==27&&f7==28)
#define ONE(T,N,K) static __attribute__((noinline)) T N##K(P##K T x,uint64_t tail,double ftail){native_calls++;if(!C##K||tail!=99||ftail!=109)failure=1;transform(&x,sizeof x);return x;} static void N##_call##K(void*code,void*out,const void*in){T x;memcpy(&x,in,sizeof x);T y=((T(*)(P##K T,uint64_t,double))code)(V##K x,99,109);memcpy(out,&y,sizeof y);}
#define CASE(T,N) ONE(T,N,0) ONE(T,N,1) ONE(T,N,2)
CASE(B4,b4) CASE(B8,b8) CASE(BD16,bd16) CASE(DB16,db16)
typedef struct {const char*name;size_t size;void(*native[3])(void);void(*call[3])(void*,void*,const void*);} Spec;
#define SPEC(T,N) {#N,sizeof(T),{(void(*)(void))N##0,(void(*)(void))N##1,(void(*)(void))N##2},{N##_call0,N##_call1,N##_call2}}
static Spec specs[]={SPEC(B4,b4),SPEC(B8,b8),SPEC(BD16,bd16),SPEC(DB16,db16)};
typedef struct {Spec*s;unsigned gp,fp;} Context;
static double value_double(uint64_t u){double d;memcpy(&d,&u,8);return d;}
static uint64_t bits_double(double d){uint64_t u;memcpy(&u,&d,8);return u;}
static int native_hook(void*owner,ffi_cif*cif,uintptr_t raw,void*out,void**args){(void)owner;ffi_call(cif,FFI_FN((void*)raw),out,args);return 0;}
static int script_hook(void*owner,const void*raw,const us_export_frame*f){(void)raw;Context*c=owner;unsigned k=0;script_calls++;if(f->count!=c->gp+c->fp+3||f->result_kind!=5||f->result_bytes!=c->s->size)return 1;for(unsigned i=0;i<c->gp;i++,k++)if(f->slots[k]!=11+i)return 1;for(unsigned i=0;i<c->fp;i++,k++)if(value_double(f->slots[k])!=21+i)return 1;if(f->slots[k+1]!=99||value_double(f->slots[k+2])!=109)return 1;memcpy(f->result,(void*)(uintptr_t)f->slots[k],c->s->size);transform(f->result,c->s->size);return 0;}
static int exercise(Spec*s,unsigned pressure,const char*path,const char*target){
 FILE*f=fopen(path,"rb");if(!f)return 1;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<=0||n>33554432)return 1;unsigned char*b=malloc(n);if(!b||fread(b,1,n,f)!=(size_t)n)return 1;fclose(f);us_carrier_certificate p={0};char error[256]={0};if(us_carrier_certificate_load(&p,target,b,n,error,sizeof error)){fprintf(stderr,"certificate: %s\n",error);return 1;}free(b);
 unsigned gps[]={0,5,7},fps[]={0,7,8};Context c={s,gps[pressure],fps[pressure]};unsigned index=c.gp+c.fp;us_export_signature original;us_callables registry;uint64_t nh=0,sh=0;void*code=0;int rc=1;us_callables_init(&registry,&c,1,script_hook,native_hook,0);
 if(us_callable_export_signature(p.original.items,&original)||us_carrier_certificate_make(&p,&registry,US_CALLABLE_NATIVE,&original,(uintptr_t)s->native[pressure],&nh,error,sizeof error)||us_carrier_certificate_make(&p,&registry,US_CALLABLE_SCRIPT,&original,1,&sh,error,sizeof error)||us_callable_pointer(&registry,sh,&original,&code,error,sizeof error)){fprintf(stderr,"plans: %s\n",error);goto done;}
 for(unsigned iteration=0;iteration<100;iteration++){union{uint64_t align;unsigned char bytes[64];}input,output;unsigned char saved[16],expected[16];uint64_t slots[20];memset(input.bytes,0xa5,64);memset(output.bytes,0x5a,64);for(size_t i=0;i<s->size;i++)input.bytes[8+i]=(unsigned char)(7*i+iteration);memcpy(saved,input.bytes+8,s->size);memcpy(expected,saved,s->size);transform(expected,s->size);for(unsigned i=0;i<c.gp;i++)slots[i]=11+i;for(unsigned i=0;i<c.fp;i++)slots[c.gp+i]=bits_double(21+i);slots[index]=(uintptr_t)(input.bytes+8);slots[index+1]=99;slots[index+2]=bits_double(109);
  if(us_callable_call(&registry,nh,&original,slots,output.bytes+8,index+3,error,sizeof error)||failure||memcmp(output.bytes+8,expected,s->size)||memcmp(input.bytes+8,saved,s->size)||output.bytes[7]!=0x5a||output.bytes[8+s->size]!=0x5a)goto done;
  memset(output.bytes,0x5a,64);s->call[pressure](code,output.bytes+8,input.bytes+8);if(failure||memcmp(output.bytes+8,expected,s->size)||memcmp(input.bytes+8,saved,s->size)||input.bytes[7]!=0xa5||input.bytes[8+s->size]!=0xa5||output.bytes[7]!=0x5a||output.bytes[8+s->size]!=0x5a)goto done;
 }rc=0;
done:us_callables_clear(&registry);us_carrier_certificate_clear(&p);if(rc)fprintf(stderr,"failed %s/%u: %s\n",s->name,pressure,error);return rc;
}
int main(int argc,char**argv){if(argc==2&&!strcmp(argv[1],"--facts")){facts();return 0;}if(argc!=5)return 2;unsigned pressure=(unsigned)strtoul(argv[2],0,10);if(pressure>2)return 2;for(size_t i=0;i<4;i++)if(!strcmp(argv[1],specs[i].name)){if(exercise(specs+i,pressure,argv[3],argv[4]))return 1;printf("{\"case\":\"%s\",\"pressure\":%u,\"native_calls\":%u,\"script_calls\":%u,\"rc\":0}\n",argv[1],pressure,native_calls,script_calls);return 0;}return 2;}
#endif

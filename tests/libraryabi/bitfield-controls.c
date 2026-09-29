/* Independent natural bitfield ABI controls, no repository/model classifier. */
#include <ffi.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
typedef struct {unsigned int a:5;signed int b:7;unsigned int c:20;} B4;
typedef union {B4 p;unsigned int a:32;} U4;
typedef struct {unsigned long long a:9;signed long long b:17;unsigned long long c:38;} B8;
typedef struct {unsigned int a:20;signed int b:20;} Cross8;
typedef union {B8 p;double d;} U8;
typedef struct {unsigned long long a:9;signed long long b:55;unsigned long long c:17;signed long long d:47;} B16;
typedef struct {B8 bits;double d;} BD16;
typedef struct {double d;B8 bits;} DB16;
typedef union {B16 bits;double fp[2];} U16;
typedef struct {B4 bits[4];} A16;
typedef struct {B4 bits[2];double d;} AD16;
typedef struct {B4 bits;float f;double d;} BFD16;
typedef union {BFD16 bits;double fp[2];} UF16;
typedef struct {B4 bits;float f;} BF8;
typedef struct {unsigned long long a:40;signed long long b:40;} Cross16;
static int failure;static unsigned native_calls,closure_calls;static const char *active;
static void transform(void *x,size_t n){unsigned char *b=x;for(size_t i=0;i<n;i++)b[i]^=(unsigned char)(0x31+3*i);}
#define P0
#define V0
#define C0 1
#define P1 uint64_t g0,uint64_t g1,uint64_t g2,uint64_t g3,uint64_t g4,double f0,double f1,double f2,double f3,double f4,double f5,double f6,
#define V1 11,12,13,14,15,21,22,23,24,25,26,27,
#define C1 (g0==11&&g1==12&&g2==13&&g3==14&&g4==15&&f0==21&&f1==22&&f2==23&&f3==24&&f4==25&&f5==26&&f6==27)
#define P2 uint64_t g0,uint64_t g1,uint64_t g2,uint64_t g3,uint64_t g4,uint64_t g5,uint64_t g6,double f0,double f1,double f2,double f3,double f4,double f5,double f6,double f7,
#define V2 11,12,13,14,15,16,17,21,22,23,24,25,26,27,28,
#define C2 (g0==11&&g1==12&&g2==13&&g3==14&&g4==15&&g5==16&&g6==17&&f0==21&&f1==22&&f2==23&&f3==24&&f4==25&&f5==26&&f6==27&&f7==28)
#define ONE(T,N,K) static __attribute__((noinline)) T N##K(P##K T x,uint64_t tail,double ftail){native_calls++;if(!C##K||tail!=99||ftail!=109){failure=1;fprintf(stderr,"bad prefix/tails %s tail=%llu ftail=%.17g\n",active,(unsigned long long)tail,ftail);}transform(&x,sizeof x);return x;} static void N##_call##K(void *code,void *out,const void *in){T x;memcpy(&x,in,sizeof x);T y=((T(*)(P##K T,uint64_t,double))code)(V##K x,99,109);memcpy(out,&y,sizeof y);}
#define CASE(T,N) ONE(T,N,0) ONE(T,N,1) ONE(T,N,2) struct N##_alignment {char c;T x;};
CASE(B4,b4) CASE(U4,u4) CASE(B8,b8) CASE(Cross8,cross8) CASE(U8,u8) CASE(B16,b16) CASE(BD16,bd16) CASE(DB16,db16) CASE(U16,u16) CASE(A16,a16) CASE(AD16,ad16) CASE(UF16,uf16) CASE(BF8,bf8) CASE(Cross16,cross16)
typedef struct {uint32_t a;} Ci;typedef struct {uint64_t a;} CI;typedef struct {uint32_t a,b;} Cii;typedef struct {uint64_t a,b;} CII;typedef struct {uint64_t a;double b;} CIS;typedef struct {double a;uint64_t b;} CSI;typedef struct {uint32_t a,b,c,d;} Ciiii;
#define CARRIER(T) struct T##_alignment {char c;T x;};
CARRIER(Ci) CARRIER(CI) CARRIER(Cii) CARRIER(CII) CARRIER(CIS) CARRIER(CSI) CARRIER(Ciiii)
typedef struct {const char *name,*recipe;size_t size,alignment,carrier_size,carrier_alignment;void (*native[3])(void);void (*call[3])(void*,void*,const void*);int padding;} Spec;
#ifdef __aarch64__
#define BD "II"
#define BDT CII
#define DB "II"
#define DBT CII
#else
#define BD "IS"
#define BDT CIS
#define DB "SI"
#define DBT CSI
#endif
#define SPEC_IMPL(T,N,R,P,CT) {#N,R,sizeof(T),offsetof(struct N##_alignment,x),sizeof(CT),offsetof(struct CT##_alignment,x),{(void(*)(void))N##0,(void(*)(void))N##1,(void(*)(void))N##2},{N##_call0,N##_call1,N##_call2},P}
#define SPEC(T,N,R,P,CT) SPEC_IMPL(T,N,R,P,CT)
static Spec specs[]={SPEC(B4,b4,"i",0,Ci),SPEC(U4,u4,"i",0,Ci),SPEC(B8,b8,"I",0,CI),SPEC(Cross8,cross8,"ii",1,Cii),SPEC(U8,u8,"I",0,CI),SPEC(B16,b16,"II",0,CII),SPEC(BD16,bd16,BD,0,BDT),SPEC(DB16,db16,DB,0,DBT),SPEC(U16,u16,"II",0,CII),SPEC(A16,a16,"iiii",0,Ciiii),SPEC(AD16,ad16,BD,0,BDT),SPEC(UF16,uf16,BD,0,BDT),SPEC(BF8,bf8,"ii",0,Cii),SPEC(Cross16,cross16,"II",1,CII)};
typedef struct {Spec *spec;unsigned gp,fp;const void *input;} Data;
static void closure(ffi_cif *c,void *result,void **args,void *opaque){(void)c;Data *d=opaque;closure_calls++;unsigned k=0;for(unsigned i=0;i<d->gp;i++,k++)if(*(uint64_t*)args[k]!=11+i)failure=1;for(unsigned i=0;i<d->fp;i++,k++)if(*(double*)args[k]!=21+i)failure=1;if(memcmp(args[k],d->input,d->spec->size)||*(uint64_t*)args[k+1]!=99||*(double*)args[k+2]!=109)failure=1;memcpy(result,args[k],d->spec->size);transform(result,d->spec->size);}
static int run(Spec *s,unsigned mode){
 const unsigned gps[]={0,5,7},fps[]={0,7,8};unsigned gp=gps[mode],fp=fps[mode],index=gp+fp,count=index+3;ffi_type *elements[5]={0};unsigned ne=0;for(const char *p=s->recipe;*p;p++)elements[ne++]=*p=='I'?&ffi_type_uint64:*p=='i'?&ffi_type_uint32:*p=='S'?&ffi_type_double:&ffi_type_float;
 ffi_type object={0,0,FFI_TYPE_STRUCT,elements};ffi_type *types[20];void *args[20];uint64_t gv[8]={11,12,13,14,15,16,17,99};double fv[9]={21,22,23,24,25,26,27,28,109};union {uint64_t alignment;unsigned char bytes[64];} input,output;unsigned char original[16],expected[16];active=s->name;
 for(unsigned i=0;i<gp;i++){types[i]=&ffi_type_uint64;args[i]=gv+i;}for(unsigned i=0;i<fp;i++){types[gp+i]=&ffi_type_double;args[gp+i]=fv+i;}types[index]=&object;args[index]=input.bytes+8;types[index+1]=&ffi_type_uint64;args[index+1]=gv+7;types[index+2]=&ffi_type_double;args[index+2]=fv+8;
 ffi_cif cif;if(ffi_prep_cif(&cif,FFI_DEFAULT_ABI,count,&object,types)!=FFI_OK||object.size!=s->size||object.alignment!=s->alignment||s->carrier_size!=s->size||s->carrier_alignment!=s->alignment){printf("{\"case\":\"%s\",\"pressure\":%u,\"rc\":1,\"reason\":\"layout\",\"actual_size\":%zu,\"actual_align\":%zu,\"carrier_size\":%zu,\"carrier_align\":%u}\n",s->name,mode,s->size,s->alignment,object.size,object.alignment);return 1;}
 Data data={s,gp,fp,input.bytes+8};void *code=NULL;ffi_closure *cl=ffi_closure_alloc(sizeof *cl,&code);if(!cl||ffi_prep_closure_loc(cl,&cif,closure,&data,code)!=FFI_OK)return 1;
 for(unsigned i=0;i<100;i++){
  memset(input.bytes,0xa5,sizeof input.bytes);memset(output.bytes,0x5a,sizeof output.bytes);for(size_t j=0;j<s->size;j++)input.bytes[8+j]=(unsigned char)(7*j+i);memcpy(original,input.bytes+8,s->size);memcpy(expected,original,s->size);transform(expected,s->size);
  ffi_call(&cif,s->native[mode],output.bytes+8,args);if(failure||memcmp(output.bytes+8,expected,s->size)||memcmp(input.bytes+8,original,s->size)||input.bytes[7]!=0xa5||input.bytes[8+s->size]!=0xa5||output.bytes[7]!=0x5a||output.bytes[8+s->size]!=0x5a){printf("{\"case\":\"%s\",\"pressure\":%u,\"rc\":1,\"reason\":\"direct_bytes_prefix_tail_canary\",\"iteration\":%u}\n",s->name,mode,i);ffi_closure_free(cl);return 1;}
  memset(output.bytes,0x5a,sizeof output.bytes);s->call[mode](code,output.bytes+8,input.bytes+8);if(failure||memcmp(output.bytes+8,expected,s->size)||memcmp(input.bytes+8,original,s->size)||output.bytes[7]!=0x5a||output.bytes[8+s->size]!=0x5a){printf("{\"case\":\"%s\",\"pressure\":%u,\"rc\":1,\"reason\":\"closure_bytes_prefix_tail_canary\",\"iteration\":%u}\n",s->name,mode,i);ffi_closure_free(cl);return 1;}
 }
 ffi_closure_free(cl);printf("{\"case\":\"%s\",\"pressure\":%u,\"rc\":0,\"size\":%zu,\"alignment\":%zu,\"recipe\":\"%s\",\"padding_observed_not_portably_required\":%d,\"native_calls\":%u,\"closure_calls\":%u}\n",s->name,mode,s->size,s->alignment,s->recipe,s->padding,native_calls,closure_calls);return 0;
}
int main(void){printf("{\"ordinary_offsets\":{\"BD16.d\":%zu,\"DB16.d\":%zu,\"DB16.bits\":%zu,\"AD16.d\":%zu,\"UF16.bits.d\":%zu,\"BF8.f\":%zu},\"bitfield_offsetof_used\":false}\n",offsetof(BD16,d),offsetof(DB16,d),offsetof(DB16,bits),offsetof(AD16,d),offsetof(BFD16,d),offsetof(BF8,f));for(size_t i=0;i<sizeof specs/sizeof specs[0];i++)for(unsigned mode=0;mode<3;mode++)if(run(specs+i,mode))return 1;return 0;}

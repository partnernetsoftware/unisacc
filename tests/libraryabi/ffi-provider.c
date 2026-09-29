/* Independent ABI dependency qualification: GP5/FP7 + mixed aggregate + stack tails. */
#ifdef __APPLE__
#include <ffi/ffi.h>
#else
#include <ffi.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <string.h>
struct P {uint64_t a;double b;};struct Q {uint64_t a;float b,c;};union U {struct P p;struct Q q;};
#define PREFIX uint64_t g0,uint64_t g1,uint64_t g2,uint64_t g3,uint64_t g4,double f0,double f1,double f2,double f3,double f4,double f5,double f6
#define ARGS 11,12,13,14,15,21,22,23,24,25,26,27
static int failed;static const char *stage;
static void observe(PREFIX,uint64_t a,double b,uint64_t tail,double ftail){
 failed=g0!=11||g1!=12||g2!=13||g3!=14||g4!=15||f0!=21||f1!=22||f2!=23||f3!=24||f4!=25||f5!=26||f6!=27||tail!=99||ftail!=109||a!=35||b!=40;
 printf("{\"stage\":\"%s\",\"rc\":%d,\"gp\":[%llu,%llu,%llu,%llu,%llu],\"fp\":[%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g],\"tail\":[%llu,%.17g],\"union_in\":[%llu,%.17g]}\n",stage,failed,(unsigned long long)g0,(unsigned long long)g1,(unsigned long long)g2,(unsigned long long)g3,(unsigned long long)g4,f0,f1,f2,f3,f4,f5,f6,(unsigned long long)tail,ftail,(unsigned long long)a,b);
}
__attribute__((noinline)) union U union_target(PREFIX,union U x,uint64_t tail,double ftail){observe(g0,g1,g2,g3,g4,f0,f1,f2,f3,f4,f5,f6,x.p.a,x.p.b,tail,ftail);x.p.a+=5;x.p.b+=11;return x;}
__attribute__((noinline)) struct P struct_target(PREFIX,struct P x,uint64_t tail,double ftail){observe(g0,g1,g2,g3,g4,f0,f1,f2,f3,f4,f5,f6,x.a,x.b,tail,ftail);x.a+=5;x.b+=11;return x;}
int main(void){
 union U in={0},out={0};in.p.a=35;in.p.b=40;stage="true_C_union";out=union_target(ARGS,in,99,109);int cfailed=failed||out.p.a!=40||out.p.b!=51;printf("{\"stage\":\"true_C_union_result\",\"rc\":%d,\"out\":[%llu,%.17g]}\n",cfailed,(unsigned long long)out.p.a,out.p.b);
 ffi_type *members[]={&ffi_type_uint64,&ffi_type_double,NULL};ffi_type carrier={0,0,FFI_TYPE_STRUCT,members};ffi_type *types[15];void *args[15];uint64_t gp[]={11,12,13,14,15,99};double fp[]={21,22,23,24,25,26,27,109};
 for(int i=0;i<5;i++){types[i]=&ffi_type_uint64;args[i]=gp+i;}for(int i=0;i<7;i++){types[5+i]=&ffi_type_double;args[5+i]=fp+i;}types[12]=&carrier;args[12]=&in;types[13]=&ffi_type_uint64;args[13]=gp+5;types[14]=&ffi_type_double;args[14]=fp+7;
 ffi_cif cif;if(ffi_prep_cif(&cif,FFI_DEFAULT_ABI,15,&carrier,types)!=FFI_OK||carrier.size!=16||carrier.alignment!=8)return 2;
 stage="ffi_union";failed=0;ffi_call(&cif,FFI_FN(union_target),&out,args);int ufailed=failed||out.p.a!=40||out.p.b!=51;printf("{\"stage\":\"ffi_union_result\",\"rc\":%d,\"out\":[%llu,%.17g]}\n",ufailed,(unsigned long long)out.p.a,out.p.b);
 stage="ffi_struct";failed=0;struct P sout={0};ffi_call(&cif,FFI_FN(struct_target),&sout,args);int sfailed=failed||sout.a!=40||sout.b!=51;printf("{\"stage\":\"ffi_struct_result\",\"rc\":%d,\"out\":[%llu,%.17g]}\n",sfailed,(unsigned long long)sout.a,sout.b);return cfailed?3:ufailed||sfailed;
}

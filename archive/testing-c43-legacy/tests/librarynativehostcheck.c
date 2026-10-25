#include "exec/c/librarynative.h"
#include "exec/c/libraryresolver.h"
struct Pair {double d;int n;};
static unsigned calls;
static struct Pair exchange(int a,double b,float c,int *p,struct Pair s,int e,double f,int g,int h){calls++;struct Pair r={b+c+s.d+f,a+*p+s.n+e+g+h};s.n=999;return r;}
static double fp(double a,double b,double c,double d,double e,double f,double g,double h,double i){calls++;return a+b+c+d+e+f+g+h+i;}
static int ints(int a,int b,int c,int d,int e,int f,int g,int h,int i,int j,int k,int l,int m,int n,int o,int p,int q){calls++;return a+b+c+d+e+f+g+h+i+j+k+l+m+n+o+p+q;}
int main(int argc,char **argv){
 if(argc!=3)return 2;FILE*f=fopen(argv[1],"rb");if(!f)return 2;fseek(f,0,SEEK_END);long len=ftell(f);rewind(f);unsigned char*sig=malloc((size_t)len);if(fread(sig,1,(size_t)len,f)!=(size_t)len)return 2;fclose(f);
 char error[200];us_bindings b={0};us_resolver r={0};us_native_plans plans={0};unsigned char*wire=NULL;size_t wirelen=0;
 uintptr_t target=!strcmp(argv[2],"pair")?(uintptr_t)exchange:!strcmp(argv[2],"fp")?(uintptr_t)fp:(uintptr_t)ints;
 if(us_bindings_add_function_typed(&b,"exchange",target,sig,(size_t)len,error,sizeof error)){fprintf(stderr,"%s\n",error);return 1;}
 unsigned char *opaque=malloc((size_t)len);memcpy(opaque,sig,(size_t)len);opaque[52]^=13;opaque[60]^=7;
 if(us_resolver_accept_injection_typed(&r,"exchange",opaque,(size_t)len,error,sizeof error))return 1;
 if(us_resolver_declare_typed(&r,&b,"exchange",opaque,(size_t)len,error,sizeof error))return 1;free(opaque);
 us_binding_type legacytype={0,4,0,1,4,0};if(us_bindings_add_function(&b,"legacy",target,&legacytype,NULL,0,0,error,sizeof error))return 1;
 if(us_resolver_freeze_with_plans(&r,&b,123,&plans,&wire,&wirelen,error,sizeof error)){fprintf(stderr,"%s\n",error);return 1;}
 if(wirelen<16||memcmp(wire,"USBIND3\n",8)||!plans.head||!us_native_plan_find(&plans,(uintptr_t)plans.head)||us_native_plan_find(&plans,1))return 1;
 const char *wirepath=getenv("US_NATIVE_WIRE");if(wirepath){FILE*wf=fopen(wirepath,"wb");if(!wf||fwrite(wire,1,wirelen,wf)!=wirelen||fclose(wf))return 1;}
 uint64_t slots[17]={0},scalar=0;struct Pair result={0},source={5,6};int pointee=4;size_t count;
 if(!strcmp(argv[2],"pair")){count=9;double d=2,e=7;float c=3;slots[0]=1;memcpy(slots+1,&d,8);memcpy(slots+2,&c,4);slots[3]=(uintptr_t)&pointee;slots[4]=(uintptr_t)&source;slots[5]=7;memcpy(slots+6,&e,8);slots[7]=8;slots[8]=14;}
 else if(!strcmp(argv[2],"fp")){count=9;for(size_t i=0;i<count;i++){double d=i+1;memcpy(slots+i,&d,8);}}
 else{count=17;for(size_t i=0;i<count;i++)slots[i]=i+1;}
 for(unsigned repeat=0;repeat<100;repeat++){us_native_arena*a=NULL;if(us_native_prepare(plans.head,slots,count,!strcmp(argv[2],"pair")?(void*)&result:(void*)&scalar,&a,error,sizeof error))return 1;
 if(us_native_invoke(a))return 1;us_native_arena_free(a);}
 if(calls!=100||source.n!=6||pointee!=4)return 1;
 if(!strcmp(argv[2],"pair")){if(result.d!=17||result.n!=40)return 1;}
 else if(!strcmp(argv[2],"fp")){double value;memcpy(&value,&scalar,8);if(value!=45)return 1;}
 else if(scalar!=153)return 1;
 us_native_arena *bad=NULL;if(!us_native_prepare(plans.head,slots,count-1,&scalar,&bad,error,sizeof error)||bad)return 1;
 if(!strcmp(argv[2],"pair")){us_export_type *t=&plans.head->graph.items[0].argtypes[4];uint64_t width=t->width;t->width=16777216;
 if(!us_native_prepare(plans.head,slots,count,&result,&bad,error,sizeof error)||bad)return 1;t->width=width;}
 size_t before=b.count;if(!us_bindings_add_function_typed(&b,"exchange",target,sig,(size_t)len,error,sizeof error)||b.count!=before)return 1;
 free(wire);us_native_plans_clear(&plans);us_resolver_clear(&r);us_bindings_clear(&b);free(sig);puts("typed native ffi: actual target100 calls, copies, ownership passed");return 0;
}

/* Frozen native candidates carry templates, never a prototype call cif. */
#include "exec/c/librarynative.h"
#include "exec/c/libraryresolver.h"
#include <stdarg.h>
static double exchange(double first,int n,...) {return first+n;}
static int checkwire(const unsigned char *v,size_t n,unsigned format,uintptr_t dispatcher,uint64_t handle){
 size_t at=16;uint64_t count,record,name,value;
 if(n<16||memcmp(v,"USBIND3\n",8))return 1;
 size_t h=8;if(us_export_u64(v,n,&h,&count)||count!=1)return 1;
 if(us_export_u64(v,n,&at,&record)||record!=n-at||us_export_u64(v,n,&at,&name)||name!=8||name>n-at)return 1;
 if(memcmp(v+at,"exchange",8))return 1;at+=8;
 if(n-at<2||v[at++]||v[at++])return 1;
 if(us_export_u64(v,n,&at,&value)||value)return 1;
 if(n-at<2||v[at++]||v[at++]!=1)return 1;
 if(us_export_u64(v,n,&at,&value)||value!=(uintptr_t)exchange)return 1;
 if(us_export_u64(v,n,&at,&value)||value!=2||at>=n||v[at++]!=format)return 1;
 if(us_export_u64(v,n,&at,&value)||value!=dispatcher)return 1;
 if(us_export_u64(v,n,&at,&value)||value!=handle)return 1;
 if(us_export_u64(v,n,&at,&value)||value!=n-at-1)return 1;
 return v[n-1]!=(handle!=0);
}
int main(int argc,char**argv){
 if(argc!=2)return 2;FILE*f=fopen(argv[1],"rb");if(!f)return 2;fseek(f,0,SEEK_END);long length=ftell(f);rewind(f);
 if(length<16)return 2;unsigned char*sig=malloc((size_t)length);if(!sig||fread(sig,1,(size_t)length,f)!=(size_t)length||fclose(f))return 2;
 us_bindings bindings={0};us_resolver resolver={0};us_native_plans plans={0};us_native_templates templates={0};unsigned char*wire=NULL;size_t n=0;char error[200]={0};
 if(us_bindings_add_function_typed(&bindings,"exchange",(uintptr_t)exchange,sig,(size_t)length,error,sizeof error))return 1;free(sig);
 /* Missing dedicated dispatcher must roll back every owned preparation. */
 if(!us_resolver_freeze_with_templates(&resolver,&bindings,77,0,&plans,&templates,&wire,&n,error,sizeof error)||wire||n||plans.head||templates.head)return 1;
 if(us_resolver_freeze_with_templates(&resolver,&bindings,77,88,&plans,&templates,&wire,&n,error,sizeof error)||plans.head||!templates.head)return 1;
 uint64_t handle=(uintptr_t)templates.head;if(!us_native_template_find(&templates,handle)||checkwire(wire,n,2,88,handle))return 1;
 free(wire);us_native_templates_clear(&templates);if(templates.head)return 1;
 /* Existing fixed-only freeze still carries the unsupported declaration. */
 if(us_resolver_freeze_with_plans(&resolver,&bindings,77,&plans,&wire,&n,error,sizeof error)||plans.head||checkwire(wire,n,1,77,0))return 1;
 free(wire);us_native_plans_clear(&plans);us_bindings_clear(&bindings);us_resolver_clear(&resolver);
 puts("variadic resolver: template freeze, rollback and fixed-only compatibility passed");return 0;
}

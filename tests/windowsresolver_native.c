/* Actual public Windows DLL resolver: model selection, source and host calls. */
#define LIBUNISACC_STATIC 1
#include "../exec/c/libunisacc.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#define API_LOAD(name) __typeof__(&name) p_##name=(__typeof__(&name))GetProcAddress(dll,#name); if(!p_##name){fprintf(stderr,"missing API %s\n",#name);return 3;}
#define REQUIRE(x) do { if(!(x)){fprintf(stderr,"failed line %d: %s\n",__LINE__,c?p_us_error(c):"no context");return 1;} } while(0)
/* A real process export. Windows process lookup must not prefer owned DLLs. */
__declspec(dllexport) int64_t layered(int64_t n){return n+3;}
static int64_t injected(int64_t n){return n+40;}
int main(int argc,char **argv){
 if(argc!=6)return 2;
 HMODULE dll=LoadLibraryA(argv[1]);if(!dll)return 3;
 API_LOAD(us_new);API_LOAD(us_free);API_LOAD(us_add_source);API_LOAD(us_compile);
 API_LOAD(us_relocate);API_LOAD(us_sym);API_LOAD(us_error);API_LOAD(us_add_symbol);
 API_LOAD(us_declare_import);API_LOAD(us_load_library);
 us_type_descriptor integer={0,0,0,1,8,0},data={0,0,0,1,4,0};
 us_signature fn={0,integer,&integer,1,0,0,0},object={1,data,0,0,0,4,1};
 us_context *c=0;int calls=0;
 for(int opt=0;opt<=2;opt++){
  for(int mode=0;mode<4;mode++){
   const char *name=mode==0?"owned_only":"layered";
   c=p_us_new(argv[2]);REQUIRE(c);
   REQUIRE(!p_us_declare_import(c,name,&fn));
   REQUIRE(!p_us_load_library(c,argv[4]));REQUIRE(!p_us_load_library(c,argv[5]));
   if(mode==2)REQUIRE(!p_us_add_symbol(c,name,(void *)(uintptr_t)injected,&fn));
   char source[256];snprintf(source,sizeof source,"long %s(long n);long check(long n){return %s(n);}%s",name,name,mode==3?"long layered(long n){return n+7;}":"");
   REQUIRE(!p_us_add_source(c,"resolver.c",source));REQUIRE(!p_us_compile(c,argv[3],opt));
   REQUIRE(!p_us_relocate(c));int64_t(*check)(int64_t)=(int64_t(*)(int64_t))p_us_sym(c,"check");
   int64_t expected=mode==0?15:mode==1?8:mode==2?45:12;
   REQUIRE(check && check(5)==expected);
   /* Failed registry changes retain the compiled generation. */
   REQUIRE(p_us_load_library(c,"C:\\no-such-r10-resolver.dll")!=0);REQUIRE(check(5)==expected);
   REQUIRE(p_us_load_library(c,argv[4])!=0);REQUIRE(check(5)==expected);
   REQUIRE(p_us_declare_import(c,name,&fn)!=0);REQUIRE(check(5)==expected);
   printf("O%d mode%d called %lld\n",opt,mode,(long long)expected);fflush(stdout);calls++;
   p_us_free(c);c=0;
  }
  c=p_us_new(argv[2]);REQUIRE(c);REQUIRE(!p_us_declare_import(c,"owned_value",&object));
  REQUIRE(!p_us_load_library(c,argv[4]));REQUIRE(!p_us_load_library(c,argv[5]));
  REQUIRE(!p_us_add_source(c,"object.c","extern int owned_value;long check(long n){owned_value=owned_value+n;return owned_value;}"));
  REQUIRE(!p_us_compile(c,argv[3],opt));REQUIRE(!p_us_relocate(c));
  int64_t(*check)(int64_t)=(int64_t(*)(int64_t))p_us_sym(c,"check");
  REQUIRE(check && check(1)==102 && check(2)==104);
  /* Probe load is a borrowed inspection; contexts own their separate refs. */
  HMODULE h=GetModuleHandleA(argv[4]);REQUIRE(h);int *value=(int *)GetProcAddress(h,"owned_value");
  REQUIRE(value && *value==104);*value=101;
  printf("O%d actual host data mutation 104\n",opt);fflush(stdout);calls++;
  p_us_free(c);c=0;
 }
 REQUIRE(calls==15);
 c=p_us_new(argv[2]);REQUIRE(c);REQUIRE(!p_us_declare_import(c,"missing_r10_symbol",&fn));
 REQUIRE(!p_us_add_source(c,"missing.c","long missing_r10_symbol(long n);long check(long n){return missing_r10_symbol(n);} "));
 REQUIRE(p_us_compile(c,argv[3],0)!=0);p_us_free(c);c=0;
 FreeLibrary(dll);puts("Windows public resolver: 15 calls/data groups and missing binding rejected");return 0;
}

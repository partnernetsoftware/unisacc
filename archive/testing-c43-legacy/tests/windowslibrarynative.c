/* Native Windows public-DLL acceptance; no model/reference substitution. */
#define LIBUNISACC_STATIC 1
#include "../exec/c/libunisacc.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#define API_LOAD(name) __typeof__(&name) p_##name=(__typeof__(&name))GetProcAddress(dll,#name); if(!p_##name){fprintf(stderr,"missing API %s\n",#name);return 3;}
int main(int argc,char **argv){
 if(argc!=4)return 2;
 HMODULE dll=LoadLibraryA(argv[1]);if(!dll){fprintf(stderr,"LoadLibrary error %lu\n",GetLastError());return 3;}
 API_LOAD(us_new); API_LOAD(us_free); API_LOAD(us_add_source); API_LOAD(us_compile); API_LOAD(us_relocate); API_LOAD(us_sym); API_LOAD(us_tape); API_LOAD(us_error);
 const char *names[]={"us_add_file","us_add_tape","us_define","us_include_path","us_add_symbol","us_declare_import","us_load_library","us_run_main","us_call_status"};
 for(size_t i=0;i<sizeof(names)/sizeof(names[0]);i++)if(!GetProcAddress(dll,names[i]))return 3;
 for(int opt=0;opt<=2;opt++){
  us_context *c=p_us_new(argv[2]);if(!c)return 4;
  if(p_us_add_source(c,"native.c","long check(long n){return n+7;}")){fprintf(stderr,"source: %s\n",p_us_error(c));p_us_free(c);return 5;}
  if(p_us_compile(c,argv[3],opt)){fprintf(stderr,"compile O%d: %s\n",opt,p_us_error(c));p_us_free(c);return 6;}
  size_t n=0;if(!p_us_tape(c,&n)||!n){p_us_free(c);return 7;}
  printf("compiled O%d %zu bytes\n",opt,n);fflush(stdout);
  if(p_us_relocate(c)){fprintf(stderr,"relocate O%d: %s\n",opt,p_us_error(c));p_us_free(c);return 8;}
  int64_t (*check)(int64_t)=(int64_t(*)(int64_t))p_us_sym(c,"check");
  if(!check||check(5)!=12){fprintf(stderr,"export failed\n");p_us_free(c);return 9;}
  p_us_free(c);printf("called O%d: 12\n",opt);fflush(stdout);
 }
 FreeLibrary(dll);return 0;
}

/* Tool-only resource profile adapter: reuse the typed observer and common core.
   Explicit ABSENT/1 library/callables, no second reader/model or product lifecycle. */
#define UNISA_CORE_EXTERNAL
#define core_host_fetch callback_original_fetch
#define main callback_shared_main
#include "librarytypedreader-observer.c"
#undef main
#undef core_host_fetch
int core_host_fetch(const unsigned char*p,int n,unsigned char**b,int*l){
 static unsigned char one[8]={1};
 if(n==18&&!memcmp(p,"\0library/callables",18)){
  const char*flag=getenv("CB_CALLABLES");
  if(flag&&!strcmp(flag,"1")){*b=one;*l=8;return 1;}
  *b=0;*l=0;return 0;
 }
 return callback_original_fetch(p,n,b,l);
}
int main(int n,char**v){return callback_shared_main(n,v);}

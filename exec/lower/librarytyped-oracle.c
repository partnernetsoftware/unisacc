/* Independent existing host decoder is an observation reference, not product policy. */
#define UNISA_RUNTIME_LIBRARY
#include "../c/run.c"
#include "../c/libraryexports.h"
int main(int argc,char **argv){
 if(argc!=4)return 2;int n;unsigned char*b=readfile(argv[3],&n,0);us_exports e={0};char error[1024]={0};
 int bad=us_exports_load(&e,b,(size_t)n,error,sizeof error);
 if(!bad){us_export*x=e.items;bad=e.count!=1||x->version!=3||x->variadic||strcmp(x->name,argv[1])||x->count!=(uint64_t)strtoull(argv[2],NULL,10)||us_export_has_callbacks(x);}
 us_exports_clear(&e);free(b);if(bad){fputs("not covered: libraryfn fixed typed ABI\n",stderr);return 1;}return 0;
}

/* Observation only: existing V3 decoder and type equality, not product policy. */
#define UNISA_RUNTIME_LIBRARY
#include "../c/run.c"
#include "../c/librarycallplans.h"
#include "../c/librarybindings.h"
int main(int argc,char **argv){
 if(argc!=6)return 2;int pn,cn;unsigned char*p=readfile(argv[3],&pn,0),*c=readfile(argv[4],&cn,0);
 us_exports a={0},b={0};char error[1024]={0};int bad=us_exports_load(&a,p,pn,error,sizeof error)||us_exports_load(&b,c,cn,error,sizeof error);
 unsigned long long fixed=strtoull(argv[2],NULL,10);int prototype_only=atoi(argv[5]);
 if(!bad){us_export*x=a.items,*y=b.items;bad=a.count!=1||b.count!=1||x->version!=3||y->version!=3||!x->variadic||x->mode!=1||x->count!=fixed||strcmp(x->name,argv[1])||us_export_has_callbacks(x)||us_export_has_callbacks(y);
  if(!bad&&!prototype_only){bad=y->variadic||y->mode!=1||y->count<fixed||strcmp(y->name,argv[1])||!us_binding_graph_equal(&x->result,&y->result);for(size_t i=0;!bad&&i<fixed;i++)bad=!us_binding_graph_equal(us_export_arg(x,i),us_export_arg(y,i));}
 }
 us_exports_clear(&a);us_exports_clear(&b);free(p);free(c);if(bad){fputs("not covered: librarycallsite signature\n",stderr);return 1;}return 0;
}

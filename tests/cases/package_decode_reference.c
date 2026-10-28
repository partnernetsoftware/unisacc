/* Host-only snapshot of the production C package directory. */
#define UNISA_RUNTIME_LIBRARY
#include "../../exec/c/run.c"
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    Package p={0};package_parse(&p,package_open(argv[1]),argv[1]);
    printf("package %d %d %d %d %d\n",p.ver,p.n,p.m,p.s,p.nr);
    for(int i=0;i<p.s;i++)printf("stage %s %s %s %s %d\n",p.stages[i].route,p.stages[i].name,p.stages[i].in,p.stages[i].out,p.stages[i].model);
    for(int i=0;i<p.m;i++)printf("model %d %d %d %u\n",p.off[i],p.len[i],p.raw[i],p.crc[i]);
    for(int i=0;i<p.nr;i++)printf("resource %d %d %d %d\n",p.res[i].name,p.res[i].n,p.res[i].data,p.res[i].len);
    package_release(&p);return 0;
}

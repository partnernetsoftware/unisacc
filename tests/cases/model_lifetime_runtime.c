/* Actual runtime adapter with an observable package allocation domain. */
#include <stdlib.h>
static void *known[4]; static int logv[32], count;
static void observed_free(void *p) {
    int id=0; if(p) for(int i=0;i<4;i++) if(known[i]==p) id=i+1;
    logv[count++]=id; free(p);
}
#define UNISA_PACKAGE_FREE observed_free
#define UNISA_PACKAGE_REALLOC realloc
#define UNISA_RUNTIME_LIBRARY
#include "../../archive/reference-c/exec/c/run.c"   /* reference C; its headers stay in exec/c (-I) */
int main(void) {
    Model m={0}, empty={0}; m.ns=2; m.nstr=2; m.loaded=1;
    m.str=calloc(2,sizeof *m.str); known[0]=m.str;
    m.str[0]=malloc(1); known[1]=m.str[0];
    m.rowk=calloc(2,sizeof *m.rowk); known[2]=m.rowk;
    m.rowk[1]=malloc(sizeof(int)); known[3]=m.rowk[1];
    model_release(&m);
    printf("zero=%d calls=%d",!memcmp(&m,&empty,sizeof m),count);
    for(int i=0;i<count;i++) printf(" %d",logv[i]);
    putchar('\n'); return 0;
}

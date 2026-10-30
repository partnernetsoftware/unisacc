/* Deterministic failure injection into the real model allocation domain. */
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <stdio.h>
static jmp_buf trap;
static void *owned[128]; static int nowned, calls, fail_at, invalid, growth_attempted, growth_failed;
static void *injected_realloc(void *p,size_t n) {
    calls++;if(p)growth_attempted++;
    if(calls==fail_at){if(p)growth_failed++;return 0;}
    if(p){for(int i=0;i<nowned;i++)if(owned[i]==p){void *q=realloc(p,n);if(q)owned[i]=q;return q;}invalid++;return 0;}
    void *q=malloc(n);if(!q)return 0;memset(q,0xa5,n);owned[nowned++]=q;return q;
}
static void injected_free(void *p) {
    if(!p)return;
    for(int i=0;i<nowned;i++)if(owned[i]==p){owned[i]=0;free(p);return;}
    invalid++;
}
#define UNISA_PACKAGE_REALLOC injected_realloc
#define UNISA_PACKAGE_FREE injected_free
#define UNISA_RUNTIME_PANIC(message) longjmp(trap,1)
#define UNISA_RUNTIME_LIBRARY
#include "../../archive/reference-c/exec/c/run.c"   /* reference C; its headers stay in exec/c (-I) */
int main(void) {
    unsigned char data[]={'U','N','I','N','E','T','B','1','N',2,2,0,2,0,0,'S',0,'Q',0,'H',0,0,128,4,0,0,0};
    const char keyed[]="N 1 1 0 1 0 0\nS 41\nQ 1 1 7\nH 3 -1 0 1 0 0 0 1 0 0 0 0\n";
    char *large=malloc(140000);if(!large)return 2;
    int used=snprintf(large,140000,"T 1 1 0 0 0\nQ 32769\n");
    for(int i=0;i<32769;i++){memcpy(large+used,"1 7 ",4);used+=4;}
    memcpy(large+used,"\nR 0 0 -1 0\n",12);used+=12;
    int failures=0,tested[3]={0,0,0},complete=0,saw_growth_failure=0,saw_growth_success=0;
    for(int fixture=0;fixture<3;fixture++) {
      int success=0;
      for(int step=1;step<=40;step++) {
        memset(owned,0,sizeof owned); nowned=calls=invalid=growth_attempted=growth_failed=0;fail_at=step;model_clear();
        int jumped=setjmp(trap);
        if(jumped)tested[fixture]++;
        if(!jumped){unsigned char *input=fixture==0?data:fixture==1?(unsigned char *)large:(unsigned char *)keyed;int n=fixture==0?(int)sizeof data:fixture==1?used:(int)sizeof keyed-1;loadbytes(input,n);success=1;if(fixture==1&&(NQA!=65538||QLEN[0]!=32769||QA[65536]!=1||QA[65537]!=7))failures++;if(fixture==2&&(!RETOK[0]||!RETOK[0][0]||RETS[0]!=0||ROWC[0]!=1||ROWK[0][0]!=0||STRL[0]!=1||STR[0][0]!='A'))failures++;}
        if(fixture==1&&growth_failed)saw_growth_failure++;
        if(fixture==1&&success&&growth_attempted)saw_growth_success++;
        Model m;model_capture(&m);model_release(&m);model_clear();
        int leaks=0;for(int i=0;i<nowned;i++)if(owned[i]){free(owned[i]);leaks++;}
        if(invalid||leaks){printf("FAIL fixture=%d step=%d invalid=%d leaks=%d\n",fixture,step,invalid,leaks);failures++;}
        if(success)break;
      }
      complete+=success;
    }
    free(large);
    if(saw_growth_failure!=1||saw_growth_success!=1)failures++;
    printf("MODEL_ALLOCATION_FAILURE failures=%d complete=%d injected=%d,%d,%d growth=%d,%d\n",failures,complete,tested[0],tested[1],tested[2],saw_growth_failure,saw_growth_success);
    return failures||complete!=3;
}

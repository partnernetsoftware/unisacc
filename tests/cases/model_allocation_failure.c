/* Deterministic failure injection into the real model allocation domain. */
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <stdio.h>
static jmp_buf trap;
static void *owned[128]; static int nowned, calls, fail_at, invalid;
static void *injected_realloc(void *p,size_t n) {
    if(++calls==fail_at)return 0;
    if(p)return 0; /* the minimal fixture does not grow an existing block */
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
#include "../../exec/c/run.c"
int main(void) {
    unsigned char data[]={'U','N','I','N','E','T','B','1','N',2,2,0,2,0,0,'S',0,'Q',0,'H',0,0,128,4,0,0,0};
    int failures=0, success=0, tested=0;
    for(int step=1;step<=32;step++) {
        memset(owned,0,sizeof owned); nowned=calls=invalid=0;fail_at=step;model_clear();
        int jumped=setjmp(trap);
        if(jumped)tested++;
        if(!jumped){loadbytes(data,sizeof data);success=1;}
        Model m;model_capture(&m);model_release(&m);model_clear();
        int leaks=0;for(int i=0;i<nowned;i++)if(owned[i]){free(owned[i]);leaks++;}
        if(invalid||leaks){printf("FAIL step=%d invalid=%d leaks=%d\n",step,invalid,leaks);failures++;}
        if(success)break;
    }
    printf("MODEL_ALLOCATION_FAILURE failures=%d complete=%d injected=%d\n",failures,success,tested);
    return failures||!success;
}

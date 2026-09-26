/* Real C stack bodies against ASM; moving allocator validates all retained
   elements. Allocation failures below are explicitly SIMULATED. */
#include "../core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
void stack_push_c(CoreStack *,int); void stack_pop_c(CoreStack *);
void frame_push_c(CoreFrames *,const CoreFrame *); void frame_pop_c(CoreFrames *);
void core_stack_push(CoreStack *,int); void core_stack_pop(CoreStack *);
void core_frame_push(CoreFrames *,const CoreFrame *); void core_frame_pop(CoreFrames *);
static void *ptr[4];static size_t sizes[4];static int calls,fail,expected;
static CoreStack s;static CoreFrames f;
void *stack_realloc(void *p,size_t n) {
    calls++;if(fail)return NULL;int k=0;
    if(p)while(k<4 && ptr[k]!=p)k++;else while(k<4 && ptr[k])k++;
    assert(k<4);void *q=malloc(n);assert(q);memset(q,0xa5,n);
    if(p) { memcpy(q,p,sizes[k]<n?sizes[k]:n);free(p); }
    ptr[k]=q;sizes[k]=n;return q;
}
void core_host_panic(const char *reason) {
    assert(expected);
    const char *want=expected<=4?"out of memory":expected==5?"stack capacity overflow":expected==6?"frame capacity overflow":"pop of an empty stack";
    assert(!strcmp(reason,want));assert(calls==(expected<=4?1:0));
    if(expected==1)assert(s.n==0 && s.cap==1024 && !s.entries);
    if(expected==2)assert(f.n==0 && f.cap==16 && !f.entries);
    if(expected==3)assert(s.n==1024 && s.cap==2048 && s.entries[1023]==1023);
    if(expected==4)assert(f.n==16 && f.cap==32 && f.entries[15].i==15);
    printf("SIMULATED stack panic: %s\n",reason);exit(2);
}
int core_host_fetch(const unsigned char *p,int n,unsigned char **b,int *len) { (void)p;(void)n;(void)b;(void)len;return 0; }
static unsigned char bytes[256];static I attrs[256];
int main(int argc,char **argv) {
    CoreFrame v={bytes,attrs,0,256};
    if(argc==3) {
        int k=atoi(argv[2]);assert(k>=1 && k<=7);int c=argv[1][0]=='c';
        if(k==3)for(int i=0;i<1024;i++) { if(c)stack_push_c(&s,i);else core_stack_push(&s,i); }
        if(k==4)for(int i=0;i<16;i++) { v.i=i;if(c)frame_push_c(&f,&v);else core_frame_push(&f,&v); }
        expected=k;calls=0;fail=k<=4;
        if(k==5)s.n=s.cap=1<<30;
        if(k==6)f.n=f.cap=1<<30;
        if(k==7) { if(c)stack_pop_c(&s);else core_stack_pop(&s); }
        else if(k%2) { if(c)stack_push_c(&s,7);else core_stack_push(&s,7); }
        else { if(c)frame_push_c(&f,&v);else core_frame_push(&f,&v); }
        return 1;
    }
    assert(argc==1);CoreStack t={0};CoreFrames g={0};
    frame_pop_c(&f);core_frame_pop(&g);assert(f.n==0 && g.n==0);
    for(int i=0;i<20000;i++) {
        int value=(int)((unsigned)i*2654435761u);
        stack_push_c(&s,value);core_stack_push(&t,value);
        v.b=bytes+(i%256);v.at=(i%2)?attrs:NULL;v.i=(I)i*123456789;v.end=INT64_MAX-i;
        frame_push_c(&f,&v);core_frame_push(&g,&v);
    }
    assert(s.n==20000 && t.n==20000 && s.cap==32768 && t.cap==32768);
    assert(f.n==20000 && g.n==20000 && f.cap==32768 && g.cap==32768);
    assert(!memcmp(s.entries,t.entries,s.n*sizeof(int)) && !memcmp(f.entries,g.entries,f.n*sizeof(CoreFrame)));
    for(int i=19999;i>=0;i--) {
        assert(t.entries[i]==(int)((unsigned)i*2654435761u));
        CoreFrame *a=&g.entries[i];assert(a->b==bytes+i%256 && a->at==((i%2)?attrs:NULL));
        assert(a->i==(I)i*123456789 && a->end==INT64_MAX-i);
        stack_pop_c(&s);core_stack_pop(&t);frame_pop_c(&f);core_frame_pop(&g);
    }
    assert(s.n==0 && t.n==0 && f.n==1 && g.n==1);
    int oldcalls=calls;stack_push_c(&s,-1);core_stack_push(&t,-1);
    frame_push_c(&f,&v);core_frame_push(&g,&v);assert(calls==oldcalls);
    for(int i=0;i<4;i++)free(ptr[i]);
    puts("stack C/ASM: 20000 values and frames, moving growth, pop floor and reuse equal");return 0;
}

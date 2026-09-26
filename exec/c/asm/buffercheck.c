/* SIMULATED moving allocator: verifies the actual C body and assembly,
   including allocation order, moved pointers, and fail-stop state. */
#include "../core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
void core_put_c(Buf *,int,I);
void core_buffer_put(Buf *,int,I);
static void *ptr[8];
static size_t len[8];
static int calls,fail_at,expected;
static Buf fault;
void *buffer_realloc(void *p,size_t n) {
    int k=0;
    calls++;
    if (calls==fail_at) return NULL;
    if(p) { while(k<8 && ptr[k]!=p) k++; }
    else { while(k<8 && ptr[k]) k++; }
    assert(k<8);
    void *q=malloc(n); assert(q); memset(q,0x5a,n);
    if(p) { memcpy(q,p,len[k]<n?len[k]:n); free(p); }
    ptr[k]=q;len[k]=n;return q;
}
int core_host_fetch(const unsigned char *p,int n,unsigned char **out,int *size) {
    (void)p;(void)n;(void)out;(void)size;return 0;
}
void core_host_panic(const char *s) {
    if(!expected) { fprintf(stderr,"unexpected panic: %s\n",s);exit(1); }
    if(expected==3) {
        assert(!strcmp(s,"buffer capacity overflow"));
        assert(calls==0 && fault.n==(1<<30) && fault.cap==(1<<30));
    } else {
        assert(!strcmp(s,"out of memory"));assert(calls==expected);
        assert(fault.n==0 && fault.cap==256 && !fault.at);
        assert((fault.b!=NULL)==(expected==2));
    }
    printf("SIMULATED panic checked: %s, calls %d\n",s,calls);exit(2);
}
static I attr(int i) { return (I)((uint64_t)i*UINT64_C(0xfedcba9876543211)); }
static void compare(Buf *a,Buf *b) {
    assert(a->n==b->n && a->cap==b->cap);
    assert(!memcmp(a->b,b->b,a->cap));
    assert(!memcmp(a->at,b->at,(size_t)a->cap*sizeof(I)));
}
int main(int argc,char **argv) {
    if(argc==3) {
        expected=atoi(argv[2]); assert(expected>=1 && expected<=3);
        if(expected==3)fault.n=fault.cap=1<<30;
        else fail_at=expected;
        if(argv[1][0]=='c')core_put_c(&fault,123,456);
        else core_buffer_put(&fault,123,456);
        return 1;
    }
    assert(argc==1);Buf a={0},b={0};
    for(int i=0;i<70000;i++) {
        core_put_c(&a,i*19-10000,attr(i));
        core_buffer_put(&b,i*19-10000,attr(i));
        assert(b.b[i]==(unsigned char)(i*19-10000) && b.at[i]==attr(i));
        if(i==0 || (i&(i-1))==0)compare(&a,&b);
    }
    compare(&a,&b);int before=calls;
    a.n=b.n=7;
    for(int i=7;i<1024;i++) { core_put_c(&a,-i,attr(-i));core_buffer_put(&b,-i,attr(-i)); }
    compare(&a,&b);assert(calls==before);
    a.n=b.n=0;core_put_c(&a,511,INT64_MIN);core_buffer_put(&b,511,INT64_MIN);
    compare(&a,&b);assert(b.b[0]==255 && b.at[0]==INT64_MIN && calls==before);
    for(int k=0;k<8;k++)free(ptr[k]);
    puts("buffer C/ASM: 70000 appends, moving growth, truncation and reset equal");return 0;
}

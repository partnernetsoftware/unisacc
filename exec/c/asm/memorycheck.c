/* Actual retained C map versus assembly, plus independent key/value and
   collision expectations. calloc failure injection is SIMULATED. */
#include "../core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
I core_mget_c(const CoreMemory *,I);
void core_mset_c(CoreMemory *,I,I);
I core_memory_get(const CoreMemory *,I);
void core_memory_set(CoreMemory *,I,I);
static int calls,fail_at,expected;
static size_t allocation_cap;
static CoreMemory fault;
void *memory_calloc(size_t n,size_t size) {
    int which=calls%3;calls++;
    if(!which)allocation_cap=n;
    assert(n==allocation_cap && size==(which==2?1:8));
    if(calls==fail_at)return NULL;
    assert(n<=SIZE_MAX/size);void *p=malloc(n*size);assert(p);
    memset(p,0,n*size);return p;
}
int core_host_fetch(const unsigned char *p,int n,unsigned char **out,int *size) {
    (void)p;(void)n;(void)out;(void)size;return 0;
}
void core_host_panic(const char *s) {
    if(!expected) { fprintf(stderr,"unexpected panic: %s\n",s);exit(1); }
    if(expected==4) {
        assert(!strcmp(s,"memory capacity overflow") && calls==0);
        assert(fault.cap==((size_t)1<<60) && fault.n==fault.cap/2);
    } else {
        assert(!strcmp(s,"out of memory") && calls==3 && fault.cap==65536 && fault.n==0);
        assert((fault.keys==NULL)==(expected==1));
        assert((fault.values==NULL)==(expected==2));
        assert((fault.used==NULL)==(expected==3));
    }
    printf("SIMULATED map panic checked: %s, calls %d\n",s,calls);exit(2);
}
static void clear(CoreMemory *m) {
    free(m->keys);free(m->values);free(m->used);memset(m,0,sizeof *m);
}
static void compare(const CoreMemory *a,const CoreMemory *b) {
    assert(a->cap==b->cap && a->n==b->n);
    if(a->cap) {
        assert(!memcmp(a->keys,b->keys,a->cap*sizeof(I)));
        assert(!memcmp(a->values,b->values,a->cap*sizeof(I)));
        assert(!memcmp(a->used,b->used,a->cap));
    }
}
static I key(int i) { return (I)((uint64_t)i*UINT64_C(0x9e3779b97f4a7c15)); }
static I value(int i) { return (I)((uint64_t)i*UINT64_C(0xfedcba9876543211)); }
int main(int argc,char **argv) {
    if(argc==3) {
        expected=atoi(argv[2]);assert(expected>=1 && expected<=4);
        if(expected==4) { fault.cap=(size_t)1<<60;fault.n=fault.cap/2; }
        else fail_at=expected;
        if(argv[1][0]=='c')core_mset_c(&fault,123,456);
        else core_memory_set(&fault,123,456);
        return 1;
    }
    assert(argc==1);CoreMemory a={0},b={0};
    I edges[]={0,-1,1,INT64_MIN,INT64_MAX,((I)1<<40),-((I)1<<40)};
    for(int i=0;i<7;i++) {
        assert(core_mget_c(&a,edges[i])==0 && core_memory_get(&b,edges[i])==0);
        core_mset_c(&a,edges[i],edges[6-i]);core_memory_set(&b,edges[i],edges[6-i]);
    }
    for(int i=0;i<7;i++)assert(core_mget_c(&a,edges[i])==edges[6-i] && core_memory_get(&b,edges[i])==edges[6-i]);
    compare(&a,&b);clear(&a);clear(&b);
    uint64_t factor=UINT64_C(0x9e3779b97f4a7c15),inv=1;
    for(int i=0;i<6;i++)inv*=2-factor*inv;
    assert(factor*inv==1);
    for(int i=0;i<64;i++) {
        I k=(I)((((uint64_t)i<<36)|((uint64_t)65535<<20))*inv);
        core_mset_c(&a,k,i+100);core_memory_set(&b,k,i+100);
        size_t slot=(65535+i)&65535;
        assert(b.used[slot] && b.keys[slot]==k && b.values[slot]==i+100);
        assert(core_memory_get(&b,k)==i+100);
    }
    compare(&a,&b);assert(core_memory_get(&b,0)==0);clear(&a);clear(&b);
    size_t oldcap=0;
    for(int i=0;i<140000;i++) {
        core_mset_c(&a,key(i),value(i));core_memory_set(&b,key(i),value(i));
        assert(core_memory_get(&b,key(i))==value(i));
        if(b.cap!=oldcap) { compare(&a,&b);oldcap=b.cap; }
    }
    assert(b.n==140000 && b.cap==524288);
    for(int i=0;i<140000;i+=7) { core_mset_c(&a,key(i),0);core_memory_set(&b,key(i),0); }
    assert(b.n==140000);
    for(int i=0;i<140000;i++) {
        I want=i%7?value(i):0;
        assert(core_mget_c(&a,key(i))==want && core_memory_get(&b,key(i))==want);
    }
    assert(core_memory_get(&b,key(140001))==0);compare(&a,&b);
    clear(&a);clear(&b);assert(core_memory_get(&b,0)==0);
    core_mset_c(&a,-123,456);core_memory_set(&b,-123,456);compare(&a,&b);
    assert(b.n==1 && core_memory_get(&b,-123)==456);clear(&a);clear(&b);
    puts("sparse memory C/ASM: 140000 keys, overwrite, collisions, wrap, growth and reset equal");return 0;
}

/* Actual retained C intern implementation versus assembly. IDs, copies,
   binary lengths and collisions have independent expectations. Allocation
   failures and the huge-capacity state are explicitly SIMULATED. */
#include "../core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
uint64_t core_hbytes_c(const unsigned char *,int);
uint64_t core_hash_bytes(const unsigned char *,int);
I core_intern_c(CoreIntern *,const unsigned char *,int);
I core_string_intern(CoreIntern *,const unsigned char *,int);
static int nc,nr,failc,failr,expected;
static size_t last_size;
static CoreIntern fault;
void *intern_calloc(size_t n,size_t size) {
    nc++;assert(size==sizeof(CoreInternEntry));
    if(nc==failc)return NULL;
    assert(n<=SIZE_MAX/size);void *p=malloc(n*size);assert(p);memset(p,0,n*size);return p;
}
void *intern_realloc(void *p,size_t n) {
    nr++;last_size=n;assert(p==NULL && n>0);
    if(nr==failr)return NULL;
    p=malloc(n);assert(p);memset(p,0xa5,n);return p;
}
int core_host_fetch(const unsigned char *p,int n,unsigned char **out,int *size) {
    (void)p;(void)n;(void)out;(void)size;return 0;
}
void core_host_panic(const char *s) {
    if(!expected) { fprintf(stderr,"unexpected panic: %s\n",s);exit(1); }
    if(expected==3) {
        assert(!strcmp(s,"intern capacity overflow") && nc==0 && nr==0);
        assert(fault.cap==((size_t)1<<59) && fault.n==fault.cap/2);
    } else {
        assert(!strcmp(s,"out of memory") && nc==1);
        if(expected==1)assert(nr==0 && fault.cap==1024 && fault.n==0 && !fault.entries);
        if(expected==2)assert(nr==1 && last_size==4 && fault.cap==1024 && fault.n==0 && fault.entries);
        if(expected==4)assert(nr==0 && fault.cap==2048 && fault.n==512 && !fault.entries);
    }
    printf("SIMULATED intern panic checked: %s, calloc %d, realloc %d\n",s,nc,nr);exit(2);
}
static void clear(CoreIntern *t) {
    for(size_t i=0;i<t->cap;i++)free(t->entries[i].b);
    free(t->entries);memset(t,0,sizeof *t);
}
static void compare(const CoreIntern *a,const CoreIntern *b) {
    assert(a->cap==b->cap && a->n==b->n);
    for(size_t i=0;i<a->cap;i++) {
        const CoreInternEntry *x=&a->entries[i],*y=&b->entries[i];
        assert((x->b==NULL)==(y->b==NULL));
        assert(x->n==y->n && x->v==y->v);
        if(x->b)assert(!memcmp(x->b,y->b,x->n));
    }
}
static void bytes(unsigned char *b,uint64_t v) {
    for(int i=0;i<8;i++)b[i]=(unsigned char)(v>>(8*i));
}
int main(int argc,char **argv) {
    if(argc==3) {
        int c=atoi(argv[2]);assert(c>=1 && c<=4);
        I (*put)(CoreIntern *,const unsigned char *,int)=argv[1][0]=='c'?core_intern_c:core_string_intern;
        if(c==4) { unsigned char b[8];for(int i=0;i<512;i++) { bytes(b,i);assert(put(&fault,b,8)==i+1); } }
        expected=c;nc=nr=0;
        if(c==3) { fault.cap=(size_t)1<<59;fault.n=fault.cap/2; }
        else if(c==2)failr=1;
        else failc=1;
        put(&fault,(const unsigned char *)"abc",3);return 1;
    }
    assert(argc==1);
    unsigned char all[256],bin[]={ 'a',0,'b' };
    for(int i=0;i<256;i++)all[i]=(unsigned char)i;
    const unsigned char *vectors[]={(const unsigned char *)"",(const unsigned char *)"\0",(const unsigned char *)"abc",bin,all};
    int lens[]={0,1,3,3,256};
    uint64_t hashes[]={UINT64_C(0x14650fb0739d0383),UINT64_C(0x44bd2bd473ccf799),UINT64_C(0xe16801510db89efd),UINT64_C(0xe02908510caa06f0),UINT64_C(0x16d173bdfcdae583)};
    CoreIntern a={0},b={0};
    for(int i=0;i<5;i++) {
        assert(core_hbytes_c(vectors[i],lens[i])==hashes[i] && core_hash_bytes(vectors[i],lens[i])==hashes[i]);
        assert(core_intern_c(&a,vectors[i],lens[i])==i+1 && core_string_intern(&b,vectors[i],lens[i])==i+1);
    }
    assert(core_intern_c(&a,bin,1)==6 && core_string_intern(&b,bin,1)==6);
    for(int i=0;i<5;i++)assert(core_intern_c(&a,vectors[i],lens[i])==i+1 && core_string_intern(&b,vectors[i],lens[i])==i+1);
    compare(&a,&b);clear(&a);clear(&b);
    unsigned char buf[8];int found=0;
    for(uint64_t i=0;found<16;i++) {
        assert(i<1000000);bytes(buf,i);
        if((core_hbytes_c(buf,8)&1023)!=1023)continue;
        assert(core_intern_c(&a,buf,8)==found+1 && core_string_intern(&b,buf,8)==found+1);
        size_t slot=(1023+found)&1023;
        assert(b.entries[slot].v==found+1 && !memcmp(b.entries[slot].b,buf,8));found++;
    }
    compare(&a,&b);clear(&a);clear(&b);
    size_t cap=0;
    for(int i=0;i<20000;i++) {
        bytes(buf,i);
        assert(core_hbytes_c(buf,8)==core_hash_bytes(buf,8));
        assert(core_intern_c(&a,buf,8)==i+1 && core_string_intern(&b,buf,8)==i+1);
        memset(buf,0xff,8); /* tables must own a copy */
        if(b.cap!=cap) { compare(&a,&b);cap=b.cap; }
        if(i==511) {
            bytes(buf,0);int oldnr=nr,oldnc=nc;
            assert(core_intern_c(&a,buf,8)==1 && core_string_intern(&b,buf,8)==1);
            assert(a.cap==2048 && b.cap==2048 && a.n==512 && b.n==512);
            assert(nr==oldnr && nc==oldnc+2); /* grow even on duplicate */
        }
    }
    assert(b.cap==65536 && b.n==20000);
    for(int i=19999;i>=0;i--) {
        bytes(buf,i);assert(core_intern_c(&a,buf,8)==i+1 && core_string_intern(&b,buf,8)==i+1);
    }
    unsigned char large[4097];for(int i=0;i<4097;i++)large[i]=(unsigned char)(i*13);
    assert(core_hbytes_c(large,4097)==core_hash_bytes(large,4097));
    assert(core_intern_c(&a,large,4097)==20001 && core_string_intern(&b,large,4097)==20001);
    large[4096]^=1;
    assert(core_intern_c(&a,large,4097)==20002 && core_string_intern(&b,large,4097)==20002);
    compare(&a,&b);clear(&a);clear(&b);
    assert(core_intern_c(&a,bin,3)==1 && core_string_intern(&b,bin,3)==1);compare(&a,&b);clear(&a);clear(&b);
    puts("intern C/ASM: 20000 strings, binary/empty/prefix, collisions, growth, stable IDs, ownership and reset equal");return 0;
}

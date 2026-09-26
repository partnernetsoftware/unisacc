/* Actual C helpers versus assembly. SIMULATED allocator failures; a moving
   allocator and host-fetch double check owned/borrowed/absent lifecycle. */
#include "../core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#undef free
void free(void *);
int core_badd_c(CoreBlobs *,const unsigned char *,int);
int core_blob_add(CoreBlobs *,const unsigned char *,int);
int core_rfind_c(CoreResources *,CoreBlobs *,const unsigned char *,int);
int core_resource_find(CoreResources *,CoreBlobs *,const unsigned char *,int);
static void *ptr[4096];static size_t sizes[4096];
static int calls,fail_at,expected,fetches,owned_frees;
static size_t request;
static unsigned char borrowed[16];static unsigned char *owned;
static CoreBlobs fault;static CoreResources cache_fault;
void *bytes_realloc(void *p,size_t n) {
    calls++;request=n;if(calls==fail_at)return NULL;
    assert(n>0);int k=0;
    if(p) { while(k<4096 && ptr[k]!=p)k++; }
    else { while(k<4096 && ptr[k])k++; }
    assert(k<4096);void *q=malloc(n);assert(q);memset(q,0xa5,n);
    if(p) { memcpy(q,p,sizes[k]<n?sizes[k]:n);free(p); }
    ptr[k]=q;sizes[k]=n;return q;
}
void bytes_free(void *p) {
    if(!p)return;
    assert(p!=borrowed);
    if(p==owned) { owned_frees++;owned=0;memset(p,0xdd,1);free(p);return; }
    int k=0;while(k<4096 && ptr[k]!=p)k++;assert(k<4096);
    ptr[k]=0;sizes[k]=0;free(p);
}
int core_host_fetch(const unsigned char *p,int n,unsigned char **out,int *len) {
    fetches++;assert(*out==NULL && *len==0);
    if(!n || p[0]<1 || p[0]>3)return 0;
    int seed=n>1?p[1]:0;
    if(p[0]==1) {
        for(int i=0;i<16;i++)borrowed[i]=(unsigned char)(seed+i);
        *out=borrowed;*len=16;return 1;
    }
    assert(!owned);owned=malloc(16);assert(owned);
    for(int i=0;i<16;i++)owned[i]=(unsigned char)(seed^i);
    *out=owned;*len=p[0]==3?0:16;return 2;
}
void core_host_panic(const char *s) {
    if(!expected) { fprintf(stderr,"unexpected panic: %s\n",s);exit(1); }
    if(expected==5) {
        assert(!strcmp(s,"blob capacity overflow") && calls==0 && fetches==0);
        assert(fault.n==(1<<30) && fault.cap==(1<<30));
    } else {
        assert(!strcmp(s,"out of memory"));
        if(expected<=2) {
            assert(calls==expected && fault.n==0 && fault.cap==64 && fetches==0);
            assert((fault.entries!=NULL)==(expected==2));
            assert(request==(expected==1?1024:4));
        } else {
            assert(calls==expected-2 && cache_fault.n==0 && fault.n==1 && fetches==1);
            assert((cache_fault.entries!=NULL)==(expected==4));
            assert(request==(expected==3?16:2));
        }
    }
    printf("SIMULATED byte-store panic checked: %s, allocations %d, fetches %d\n",s,calls,fetches);exit(2);
}
static void clear(CoreBlobs *b,CoreResources *r) {
    for(int i=0;i<b->n;i++)bytes_free(b->entries[i].b);
    bytes_free(b->entries);memset(b,0,sizeof *b);
    for(int i=0;i<r->n;i++)bytes_free(r->entries[i].p);
    bytes_free(r->entries);memset(r,0,sizeof *r);
}
static void compare(const CoreBlobs *a,const CoreBlobs *b,const CoreResources *x,const CoreResources *y) {
    assert(a->n==b->n && a->cap==b->cap && x->n==y->n);
    for(int i=0;i<a->n;i++) {
        assert(a->entries[i].n==b->entries[i].n);
        assert(!memcmp(a->entries[i].b,b->entries[i].b,a->entries[i].n));
    }
    for(int i=0;i<x->n;i++) {
        assert(x->entries[i].n==y->entries[i].n && x->entries[i].id==y->entries[i].id);
        assert(!memcmp(x->entries[i].p,y->entries[i].p,x->entries[i].n));
    }
}
int main(int argc,char **argv) {
    if(argc==3) {
        int c=atoi(argv[2]);assert(c>=1 && c<=5);
        int usec=argv[1][0]=='c';
        if(c==3 || c==4) {
            if(usec)core_badd_c(&fault,(const unsigned char *)"",0);
            else core_blob_add(&fault,(const unsigned char *)"",0);
        }
        expected=c;calls=0;
        if(c==5)fault.n=fault.cap=1<<30;
        else fail_at=c<=2?c:c-2;
        if(c==3 || c==4) {
            if(usec)core_rfind_c(&cache_fault,&fault,(const unsigned char *)"X",1);
            else core_resource_find(&cache_fault,&fault,(const unsigned char *)"X",1);
        } else {
            if(usec)core_badd_c(&fault,(const unsigned char *)"abc",3);
            else core_blob_add(&fault,(const unsigned char *)"abc",3);
        }
        return 1;
    }
    assert(argc==1);CoreBlobs a={0},b={0};CoreResources x={0},y={0};unsigned char data[256];
    for(int i=0;i<300;i++) {
        int n=i%257;for(int j=0;j<n;j++)data[j]=(unsigned char)(i+j);
        assert(core_badd_c(&a,data,n)==i && core_blob_add(&b,data,n)==i);
        memset(data,0xff,sizeof data); /* blocks must own copies */
    }
    compare(&a,&b,&x,&y);assert(a.cap==512);
    for(int i=0;i<300;i++)for(int j=0;j<i%257;j++)assert(b.entries[i].b[j]==(unsigned char)(i+j));
    clear(&a,&x);clear(&b,&y);
    assert(core_badd_c(&a,data,0)==0 && core_blob_add(&b,data,0)==0); /* reserved missing ID */
    int ids[256],next=1;unsigned char key[3];
    for(int i=0;i<256;i++) {
        key[0]=(unsigned char)(i%4+1);key[1]=(unsigned char)(i/4);key[2]=0;
        int want=key[0]==4?0:next++;
        assert(core_rfind_c(&x,&a,key,3)==want && core_resource_find(&y,&b,key,3)==want);
        ids[i]=want;memset(key,0xff,sizeof key);
    }
    assert(fetches==512 && owned_frees==256 && owned==NULL);
    int saved=calls;
    for(int i=255;i>=0;i--) {
        key[0]=(unsigned char)(i%4+1);key[1]=(unsigned char)(i/4);key[2]=0;
        assert(core_rfind_c(&x,&a,key,3)==ids[i] && core_resource_find(&y,&b,key,3)==ids[i]);
        if(ids[i]) {
            CoreBlob *v=&b.entries[ids[i]];assert(v->n==(key[0]==3?0:16));
            for(int j=0;j<v->n;j++)assert(v->b[j]==(unsigned char)(key[0]==1?key[1]+j:key[1]^j));
        }
    }
    assert(fetches==512 && calls==saved && owned_frees==256);
    key[0]=1;key[1]=7;key[2]=0;
    assert(core_rfind_c(&x,&a,key,2)==next && core_resource_find(&y,&b,key,2)==next);
    assert(core_rfind_c(&x,&a,key,0)==0 && core_resource_find(&y,&b,key,0)==0);
    assert(fetches==516);compare(&a,&b,&x,&y);clear(&a,&x);clear(&b,&y);
    core_badd_c(&a,data,0);core_blob_add(&b,data,0);
    assert(core_rfind_c(&x,&a,key,2)==1 && core_resource_find(&y,&b,key,2)==1);
    assert(fetches==518);compare(&a,&b,&x,&y);clear(&a,&x);clear(&b,&y);
    for(int i=0;i<4096;i++)assert(!ptr[i]);
    puts("byte store C/ASM: 300 moving blocks, 256 resource keys, cached misses, ownership and reset equal");return 0;
}

#include "exec/c/librarycallplans.h"
#include <stdarg.h>
struct Pair { double d; int n; };
static unsigned calls;
static double mixed(double first,int count,...){va_list ap;va_start(ap,count);double r=first;for(int i=0;i<count;i++){r+=va_arg(ap,int);r+=va_arg(ap,double);}va_end(ap);calls++;return r;}
static struct Pair pair(int unused,...){(void)unused;va_list ap;va_start(ap,unused);struct Pair p=va_arg(ap,struct Pair);p.d+=va_arg(ap,double);p.n+=va_arg(ap,int);va_end(ap);calls++;return p;}
static unsigned char *readfile(const char *name,size_t *len){FILE *f=fopen(name,"rb");if(!f)return NULL;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<1){fclose(f);return NULL;}unsigned char *b=malloc((size_t)n);if(!b||fread(b,1,(size_t)n,f)!=(size_t)n){free(b);fclose(f);return NULL;}fclose(f);*len=(size_t)n;return b;}
static void put(unsigned char *b,size_t at,uint64_t n){for(unsigned i=0;i<8;i++)b[at+i]=(unsigned char)(n>>(8*i));}
static size_t record(unsigned char *b,size_t at,uint64_t site,uint64_t th,uint64_t fixed,const unsigned char *sig,size_t n){put(b,at,32+n);put(b,at+8,site);put(b,at+16,th);put(b,at+24,fixed);put(b,at+32,n);memcpy(b+at+40,sig,n);return at+40+n;}
#define CHECK(x) do { if(!(x)){fprintf(stderr,"check failed at %d: %s\n",__LINE__,#x);return 1;} }while(0)
int main(int argc,char **argv){
    if(argc!=6)return 2;size_t pn,zn,an,ppn,pan;unsigned char *ps=readfile(argv[1],&pn),*zs=readfile(argv[2],&zn),*as=readfile(argv[3],&an),*pps=readfile(argv[4],&ppn),*pas=readfile(argv[5],&pan);CHECK(ps&&zs&&as&&pps&&pas);
    us_native_templates templates={0};us_native_callsites sites={0};char err[200];uint64_t h=0,ph=0;
    CHECK(!us_native_template_add(&templates,(uintptr_t)mixed,ps,pn,&h,err,sizeof err)&&h);
    CHECK(!us_native_template_add(&templates,(uintptr_t)pair,pps,ppn,&ph,err,sizeof err)&&ph);
    unsigned char *broken=malloc(pn);CHECK(broken);uint64_t unused=99;
    memcpy(broken,ps,pn);broken[16+8+5+2]=0;CHECK(us_native_template_add(&templates,(uintptr_t)mixed,broken,pn,&unused,err,sizeof err)&&!unused);
    memcpy(broken,ps,pn);put(broken,16+8+5+4,0);CHECK(us_native_template_add(&templates,(uintptr_t)mixed,broken,pn,&unused,err,sizeof err)&&!unused);
    memcpy(broken,ps,pn);put(broken,114+24,0);put(broken,114+32,0);put(broken,114+48,0);
    CHECK(!us_native_template_add(&templates,(uintptr_t)mixed,broken,pn,&unused,err,sizeof err)&&!unused);free(broken);
    size_t cap=136+zn+an+pan;unsigned char *b=calloc(1,cap),*bad=malloc(cap);CHECK(b&&bad);memcpy(b,"USCPLAN1",8);put(b,8,3);
    size_t at=16,second;at=record(b,at,1,h,2,zs,zn);second=at;at=record(b,at,2,h,2,as,an);at=record(b,at,3,ph,1,pas,pan);
    CHECK(!us_native_callsites_load(&sites,&templates,b,at,err,sizeof err));us_native_plan *one=us_native_callsite_find(&sites,h,1),*two=us_native_callsite_find(&sites,h,2),*three=us_native_callsite_find(&sites,ph,3);CHECK(one&&two&&three&&!us_native_callsite_find(&sites,h,3));
    uint64_t zero[2]={0},many[20]={0},pairslots[4]={0},value=0;double d=.5;memcpy(zero,&d,8);memcpy(many,&d,8);many[1]=9;
    for(size_t i=0;i<9;i++){many[2+2*i]=i+1;d=(i+1)*.25;memcpy(many+3+2*i,&d,8);}struct Pair source={7.25,11},result={0};pairslots[0]=1;pairslots[1]=(uintptr_t)&source;d=2.5;memcpy(pairslots+2,&d,8);pairslots[3]=3;
    for(unsigned i=0;i<100;i++){
        us_native_arena *arena=NULL;CHECK(!us_native_prepare(one,zero,2,&value,&arena,err,sizeof err));CHECK(!us_native_invoke(arena));us_native_arena_free(arena);memcpy(&d,&value,8);CHECK(d==.5);
        CHECK(!us_native_prepare(two,many,20,&value,&arena,err,sizeof err));CHECK(!us_native_invoke(arena));us_native_arena_free(arena);memcpy(&d,&value,8);CHECK(d==56.75);
        CHECK(!us_native_prepare(three,pairslots,4,&result,&arena,err,sizeof err));CHECK(!us_native_invoke(arena));us_native_arena_free(arena);CHECK(result.d==9.75&&result.n==14);
    }CHECK(calls==300&&source.d==7.25&&source.n==11);
    /* Every truncation must preserve already-published pointers. */
    for(size_t n=0;n<at;n++){CHECK(us_native_callsites_load(&sites,&templates,b,n,err,sizeof err));CHECK(us_native_callsite_find(&sites,h,1)==one);}
    for(unsigned k=0;k<11;k++){
        memcpy(bad,b,at);
        if(k==0)put(bad,second+8,1); /* duplicate site */
        if(k==1)put(bad,32,0); /* zero template */
        if(k==2)put(bad,32,1); /* unknown live template */
        if(k==3)put(bad,40,1); /* prefix count mismatch */
        if(k==4)put(bad,48,zn-1); /* exact signature length */
        if(k==5)put(bad,16,UINT64_MAX); /* overflow payload */
        if(k==6)bad[56+16+8+5+4+8+24]=1; /* changed result kind */
        if(k==7)put(bad,8,8193);
        if(k==8)bad[56+16+8+5+4+8+65+8+24]=1; /* fixed prefix kind differs */
        if(k==9){put(bad,second+40+114+3*65+32,4);put(bad,second+40+114+3*65+48,4);} /* unpromoted float tail */
        if(k==10)bad[56+24]='z'; /* name differs */
        int failed=us_native_callsites_load(&sites,&templates,bad,at,err,sizeof err);if(!failed)fprintf(stderr,"mutation %u accepted\n",k);CHECK(failed);CHECK(us_native_callsite_find(&sites,h,1)==one);
    }
    /* Same valid frame again replaces the complete set. Empty frame clears it. */
    CHECK(!us_native_callsites_load(&sites,&templates,b,at,err,sizeof err));put(b,8,0);CHECK(!us_native_callsites_load(&sites,&templates,b,16,err,sizeof err)&&!sites.head&&!sites.plans.head);
    us_native_callsites_clear(&sites);us_native_templates_clear(&templates);CHECK(!templates.head);free(b);free(bad);free(ps);free(zs);free(as);free(pps);free(pas);
    puts("callsite plans: 300 real calls, two sites, Pair, zero-tail, framing and rollback passed");return 0;
}

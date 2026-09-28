/* Lifecycle differential tests with an allocation ledger. Both implementations
   use the same moving allocator; every nonfatal return must free all temporary
   owners, retaining only the result's accepted/error byte arrays. */
#include "../core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#undef free
void free(void *);
int core_run_c(const CoreModel *,unsigned char *,int,const char *,I,CoreResult *);
static void *ptr[128];static size_t sizes[128];static int live,calloc_calls,fail_at,fetches,cases;
void *run_realloc(void *p,size_t n) {
    int k=0;if(p)while(k<128 && ptr[k]!=p)k++;else while(k<128 && ptr[k])k++;
    assert(k<128 && n);void *q=malloc(n);assert(q);memset(q,0xa5,n);
    if(p) { memcpy(q,p,sizes[k]<n?sizes[k]:n);free(p); } else live++;
    ptr[k]=q;sizes[k]=n;return q;
}
void *run_calloc(size_t n,size_t width) {
    calloc_calls++;if(calloc_calls==fail_at)return NULL;
    assert(width && n<=SIZE_MAX/width);void *p=run_realloc(NULL,n*width);memset(p,0,n*width);return p;
}
void run_free(void *p) {
    if(!p)return;int k=0;while(k<128 && ptr[k]!=p)k++;assert(k<128);
    free(p);ptr[k]=NULL;sizes[k]=0;live--;
}
void core_host_panic(const char *reason) {
    assert(fail_at && calloc_calls==fail_at && !strcmp(reason,"out of memory"));
    assert(live==(fail_at==1?0:4));
    printf("SIMULATED initial calloc %d rejected\n",fail_at);exit(2);
}
int core_host_fetch(const unsigned char *key,int n,unsigned char **p,int *len) {
    static unsigned char data[]={71,0,72};assert(n==1 && key[0]=='x');
    fetches++;*p=data;*len=3;return 1;
}
static int mode[]={0,1,2,0},count[]={0,1,0,0};
static int sparse[]={77};static int *keys[]={NULL,sparse,NULL,NULL};
static int nx[4][257],seq[4][257];static int *next[]={nx[0],nx[1],nx[2],nx[3]},*seqs[]={seq[0],seq[1],seq[2],seq[3]};
static int qoff[]={0,4,12,15},qlen[]={2,4,2,1};
static I qa[64]={CORE_OUT,65,PUSH,77, POP,CORE_OUT,66,LDI,0,2,RLD,0, CORE_OUT,67,ADV, ACCEPT};
static char *str[]={"denied"};static int strl[]={6};
static int lo[]={97,77,2,256},hi[]={97,77,2,256},base_next[]={1,2,3,3},base_seq[]={0,1,2,3},zero[4];
static CoreModel m;
static void setup(int net) {
    for(int i=0;i<4;i++)for(int j=0;j<257;j++) { nx[i][j]=-1;seq[i][j]=0; }
    nx[0][97]=1;seq[0][97]=0;nx[1][0]=2;seq[1][0]=1;nx[2][2]=3;seq[2][2]=2;nx[3][256]=3;seq[3][256]=3;
    memset(&m,0,sizeof m);m.ns=4;m.nq=4;m.nrg=1;m.start=0;m.isnet=net;m.str=str;m.strl=strl;
    m.mode=mode;m.count=net?zero:count;m.keys=keys;m.next=next;m.seq=seqs;m.qoff=qoff;m.qlen=qlen;m.qa=qa;
    m.lo=lo;m.hi=hi;m.base_next=base_next;m.base_seq=base_seq;
    qlen[3]=1;qa[15]=ACCEPT;qa[16]=0;base_next[0]=1;
}
static int call(int c,unsigned char *input,int n,I steps,CoreResult *r) {
    assert(live==0);memset(r,0xa5,sizeof *r);
    int rc=c?core_run_c(&m,input,n,"source",steps,r):core_run(&m,input,n,"source",steps,r);
    assert(r->out.at==NULL && r->err.at==NULL && r->out.cap==0 && r->err.cap==0);
    assert(live==(r->out.b!=NULL)+(r->err.b!=NULL));
    return rc;
}
static void release(CoreResult *r) { run_free(r->out.b);run_free(r->err.b);assert(live==0); }
static void check(const char *input,I steps,int want,const char *out,const char *err,const char *reason) {
    CoreResult r;unsigned char bytes[32];size_t n=strlen(input);assert(n<sizeof bytes);memcpy(bytes,input,n+1);
    for(int c=0;c<2;c++) {
        assert(call(c,bytes,(int)n,steps,&r)==want);
        assert(r.out.n==(int)strlen(out) && r.err.n==(int)strlen(err));
        if(r.out.n)assert(!memcmp(r.out.b,out,r.out.n));if(r.err.n)assert(!memcmp(r.err.b,err,r.err.n));
        assert(!memcmp(bytes,input,n+1));
        assert((r.reason==NULL)==(reason==NULL));if(reason)assert(!strcmp(r.reason,reason));
        assert(r.reason_n==((want==1 && reason)?(int)strlen(reason):0));
        release(&r);cases++;
    }
}
int main(int argc,char **argv) {
    setup(0);
    if(argc==3) {
        CoreResult r;fail_at=atoi(argv[2]);assert(fail_at==1 || fail_at==2);
        unsigned char in[]="a";call(argv[1][0]=='c',in,1,10,&r);return 1;
    }
    assert(argc==1);
    for(int net=0;net<2;net++) {
        setup(net);
        check("a",4,0,"ABC","",NULL);check("a",INT64_MAX,0,"ABC","",NULL);
        m.start=3;check("",1,0,"","",NULL);m.start=0;
        for(int limit=-1;limit<=3;limit++)check("a",limit,3,"","","timeout");
        check("b",10,2,"","",net?"observation outside network domain":"no transition");
        check("",10,2,"","",net?"observation outside network domain":"no transition");
        /* Stopping actions must prevent later actions in their sequence. */
        qlen[3]=2;qa[16]=CORE_OUT;qa[17]=90;check("a",4,0,"ABC","",NULL);
        qa[15]=REJECT;qa[16]=0;qa[17]=CORE_OUT;qa[18]=90;check("a",4,1,"","","denied");
        qa[15]=OFILL;qa[16]=0;qa[17]=0;qa[18]=0;qa[19]=CORE_OUT;qa[20]=90;check("a",4,1,"","","field overflow");
        qlen[3]=0;check("a",7,3,"","","timeout");
    }
    setup(1);base_next[0]=4;check("a",5,2,"","","invalid network output");
    /* Exercise every owner at cleanup: memory, intern, blobs, resource cache,
       nested input, scratch/output attributes, and an owned SWAP input. */
    setup(0);int owner_mode[]={0};int owner_next[257],owner_seq[257];
    for(int i=0;i<257;i++) { owner_next[i]=0;owner_seq[i]=0; }
    int *on[]={owner_next},*os[]={owner_seq};int off[]={0},len[]={20};
    I owned[]={LDI,0,9,STX,0,100,0,SBOUT,120,SBFIND,0,SBFIND,0,SBINTERN,0,SBSAVE,0,INPUSH,0,INPOP,
        OSEL,1,CORE_OUT,69,OSEL,0,CORE_OUT,88,SWAP,CORE_OUT,89,SWAP,CORE_OUT,90,ACCEPT};
    m.ns=1;m.nq=1;m.mode=owner_mode;m.next=on;m.seq=os;m.qoff=off;m.qlen=len;m.qa=owned;
    /* Count decoded actions independently from the expected output. */
    int words=sizeof owned/sizeof *owned,nact=0;for(int p=0;p<words;p+=1+ARITY[owned[p]])nact++;len[0]=nact;
    int oldfetch=fetches;check("a",1,0,"Z","E",NULL);assert(fetches==oldfetch+2);
    owned[words-1]=REJECT;I rejected[64];memcpy(rejected,owned,sizeof owned);rejected[words]=0;m.qa=rejected;
    check("a",1,1,"","E","denied");
    for(int i=0;i<128;i++)assert(ptr[i]==NULL);
    printf("run C/ASM: %d exits, all observation modes, limits/stops, owners released and results transferred\n",cases);return 0;
}

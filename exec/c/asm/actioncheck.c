/* Every decoded action is executed by the actual C body and assembly.
   Helpers also have independent value/ownership suites; this checks wiring,
   branch/stop semantics and all mutable state after each action. */
#include "../core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
int action_run_c(CoreMachine *,const I *);
int core_action(CoreMachine *,const I *);
static int fatal_expected,fetches,seen[NOP_],checks;
static unsigned char input[]="abcdefghijklmnop";
static unsigned char fetched[]={71,0,72};
static char *reasons[]={"first","second"};static int lens[]={5,6};
static CoreModel model;
void core_host_panic(const char *reason) { assert(fatal_expected && !strcmp(reason,"bad action"));puts("bad action rejected");exit(2); }
int core_host_fetch(const unsigned char *p,int n,unsigned char **b,int *len) {
    fetches++;assert(n==3 && !memcmp(p,"key",3));*b=fetched;*len=3;return 1;
}
static void buf(Buf *b,int n) {
    b->b=calloc(64,1);b->at=calloc(64,sizeof(I));assert(b->b && b->at);b->cap=64;b->n=n;
    for(int i=0;i<n;i++) { b->b[i]=(unsigned char)('0'+i);b->at[i]=500+i; }
}
static void init(CoreMachine *s) {
    memset(s,0,sizeof *s);s->model=&model;s->result=calloc(1,sizeof(CoreResult));
    s->regs=calloc(64,sizeof(I));for(int i=0;i<64;i++)s->regs[i]=i+2;
    s->regs[1]=2;s->regs[2]=4;s->regs[3]=6;s->regs[4]=7;
    s->input=s->x=input;s->xn=16;s->xattr=calloc(16,sizeof(I));for(int i=0;i<16;i++)s->xattr[i]=1000+i;
    s->ot=123;s->r=-9;
    buf(&s->out,8);buf(&s->err,0);buf(&s->scratch,3);memcpy(s->scratch.b,"key",3);
    s->frames.entries=calloc(16,sizeof(CoreFrame));s->frames.n=1;s->frames.cap=16;
    s->frames.entries[0].b=input;s->frames.entries[0].at=s->xattr;s->frames.entries[0].i=3;s->frames.entries[0].end=16;
    s->stack.entries=calloc(1024,sizeof(int));s->stack.n=2;s->stack.cap=1024;s->stack.entries[0]=9;s->stack.entries[1]=-5;
    s->blobs.entries=calloc(64,sizeof(CoreBlob));s->blobs.cap=64;s->blobs.n=3;
    for(int i=0;i<3;i++) { s->blobs.entries[i].b=malloc(4);memcpy(s->blobs.entries[i].b,"Q\0R",3);s->blobs.entries[i].n=i?3:0; }
}
static void freebuf(Buf *b) { free(b->b);free(b->at); }
static void drop(CoreMachine *s) {
    free(s->result);free(s->regs);if(s->x!=input)free(s->x);free(s->xattr);
    freebuf(&s->out);freebuf(&s->err);freebuf(&s->scratch);free(s->frames.entries);free(s->stack.entries);
    free(s->memory.keys);free(s->memory.values);free(s->memory.used);
    for(int i=0;i<s->blobs.n;i++)free(s->blobs.entries[i].b);free(s->blobs.entries);
    for(size_t i=0;i<s->strings.cap;i++)free(s->strings.entries[i].b);free(s->strings.entries);
    for(int i=0;i<s->resources.n;i++)free(s->resources.entries[i].p);free(s->resources.entries);
}
static void samebuf(Buf *a,Buf *b) {
    assert(a->n==b->n && a->cap==b->cap);
    assert(!memcmp(a->b,b->b,a->n) && !memcmp(a->at,b->at,a->n*sizeof(I)));
}
static void same(CoreMachine *a,CoreMachine *b) {
    assert(a->xn==b->xn && a->r==b->r && a->ot==b->ot && a->osel==b->osel && a->status==b->status);
    assert(!memcmp(a->regs,b->regs,64*sizeof(I)) && !memcmp(a->x,b->x,a->xn) && !memcmp(a->xattr,b->xattr,a->xn*sizeof(I)));
    samebuf(&a->out,&b->out);samebuf(&a->err,&b->err);samebuf(&a->scratch,&b->scratch);
    assert(a->stack.n==b->stack.n && a->stack.cap==b->stack.cap && !memcmp(a->stack.entries,b->stack.entries,a->stack.n*sizeof(int)));
    assert(a->frames.n==b->frames.n && a->frames.cap==b->frames.cap);
    for(int i=0;i<a->frames.n;i++) {
        CoreFrame *x=&a->frames.entries[i],*y=&b->frames.entries[i];
        assert(x->i==y->i && x->end==y->end && !memcmp(x->b,y->b,x->end));
        assert((x->at==NULL)==(y->at==NULL));if(x->at)assert(!memcmp(x->at,y->at,x->end*sizeof(I)));
    }
    assert(a->memory.n==b->memory.n && a->memory.cap==b->memory.cap);
    for(size_t i=0;i<a->memory.cap;i++) {
        assert(a->memory.used[i]==b->memory.used[i]);
        if(a->memory.used[i])assert(a->memory.keys[i]==b->memory.keys[i] && a->memory.values[i]==b->memory.values[i]);
    }
    assert(a->blobs.n==b->blobs.n && a->blobs.cap==b->blobs.cap);
    for(int i=0;i<a->blobs.n;i++)assert(a->blobs.entries[i].n==b->blobs.entries[i].n && !memcmp(a->blobs.entries[i].b,b->blobs.entries[i].b,a->blobs.entries[i].n));
    assert(a->strings.n==b->strings.n && a->strings.cap==b->strings.cap);
    for(size_t i=0;i<a->strings.cap;i++) {
        CoreInternEntry *x=&a->strings.entries[i],*y=&b->strings.entries[i];assert((x->b==NULL)==(y->b==NULL));
        if(x->b)assert(x->n==y->n && x->v==y->v && !memcmp(x->b,y->b,x->n));
    }
    assert(a->resources.n==b->resources.n);
    for(int i=0;i<a->resources.n;i++) {
        CoreResourceEntry *x=&a->resources.entries[i],*y=&b->resources.entries[i];
        assert(x->n==y->n && x->id==y->id && !memcmp(x->p,y->p,x->n));
    }
    assert(a->result->reason_n==b->result->reason_n && (a->result->reason==NULL)==(b->result->reason==NULL));
    if(a->result->reason) { if(strcmp(a->result->reason,b->result->reason)) fprintf(stderr,"reason mismatch: C [%s], ASM [%s]\n",a->result->reason,b->result->reason);assert(!strcmp(a->result->reason,b->result->reason)); }
}
static int step(CoreMachine *a,CoreMachine *b,const I *op) {
    int x=action_run_c(a,op),y=core_action(b,op);assert(x==y);same(a,b);seen[op[0]]=1;checks++;return x;
}
int main(int argc,char **argv) {
    model.str=reasons;model.strl=lens;
    CoreMachine a,b;
    if(argc==2) { init(&a);I bad[]={-1};fatal_expected=1;if(argv[1][0]=='c')action_run_c(&a,bad);else core_action(&a,bad);return 1; }
    assert(argc==1);
    for(int op=0;op<NOP_;op++) {
        init(&a);init(&b);I ins[]={op,1,2,3,4};
        if(op==BLEN)a.regs[2]=b.regs[2]=1;
        int stop=step(&a,&b,ins);assert(stop==(op==ACCEPT || op==REJECT));
        if(op==REJECT)assert(b.status==1 && b.result->reason_n==6);
        if(op==ACCEPT)assert(b.status==0);
        if(op==COPY)assert(b.out.b[8]=='d' && b.out.at[8]==1003);
        if(op==COPYT)assert(b.out.b[8]=='d' && b.out.at[8]==123);
        if(op==INPOP)assert(b.frames.n==1);
        if(op==SBFIND) { int n=fetches;step(&a,&b,ins);assert(fetches==n && b.regs[1]==3); }
        if(op==INTERN || op==SBINTERN) { step(&a,&b,ins);assert(b.regs[1]==1); }
        if(op==SWAP) { assert(b.out.n==0 && b.xn==8 && b.frames.n==1);step(&a,&b,ins);assert(b.xn==0); }
        drop(&a);drop(&b);
    }
    /* Output selector, absent attributes, empty/clipped/reversed spans. */
    int copies[]={COPY,COPYT,SPAN,SPANT,SPAN2,SBSPAN};
    for(int sel=0;sel<2;sel++)for(int attr=0;attr<2;attr++)for(int edge=0;edge<4;edge++)for(int j=0;j<6;j++) {
        init(&a);init(&b);a.osel=b.osel=sel;
        if(!attr)a.frames.entries[0].at=b.frames.entries[0].at=NULL;
        I starts[]={-7,0,12,30},ends[]={20,0,7,40};
        a.regs[1]=b.regs[1]=starts[edge];a.regs[2]=b.regs[2]=ends[edge];
        if(edge==3)a.frames.entries[0].i=b.frames.entries[0].i=16;
        I op[]={copies[j],1,2};step(&a,&b,op);drop(&a);drop(&b);
    }
    I values[]={INT64_MIN,INT64_MAX,-1,0,1,255,256,257,4294967295ll};
    int math[]={ALU,ALUI,A64,A64I,CMP,CMPI,C64,C64U,INC,RLD,DIVMOD10};
    for(int i=0;i<9;i++)for(int j=0;j<9;j++)for(int k=0;k<11;k++) {
        init(&a);init(&b);a.regs[1]=b.regs[1]=values[i];a.regs[2]=b.regs[2]=values[j];
        a.regs[3]=b.regs[3]=values[i];a.regs[4]=b.regs[4]=values[j];
        I ins[]={math[k],1,2,3,4};step(&a,&b,ins);drop(&a);drop(&b);
    }
    init(&a);init(&b);I st[]={STX,1,2,3},ld[]={LDX,5,2,0};step(&a,&b,st);step(&a,&b,ld);assert(b.regs[5]==6);drop(&a);drop(&b);
    init(&a);init(&b);I fail[]={OFILL,1,2,0};assert(step(&a,&b,fail)==1);assert(b.status==1 && !strcmp(b.result->reason,"field overflow"));drop(&a);drop(&b);
    for(int i=0;i<NOP_;i++)assert(seen[i]);
    printf("action C/ASM: %d state comparisons, all %d opcodes, stop/error and ownership state equal\n",checks,NOP_);return 0;
}

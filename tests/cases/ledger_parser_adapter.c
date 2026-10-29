/* Compile twice: legacy C parser and CX parser, with the same in-memory IO. */
#ifdef USE_CX
#define UNISA_CX_SOURCE_FRAME
#endif
#include "../../exec/c/resourceledger.h"
int ledger_probe(const unsigned char *bytes,size_t n,char *out,size_t cap) {
    static const unsigned char empty[]={0};
    static const unsigned char abc[]={'a','b','c'};
    RLFileView views[]={{"resources.tsv",bytes,n},{"empty",empty,0},{"abc",abc,3}};
    RLLedger l={0};char err[512]={0};int rc=rl_load_views("memory",views,3,&l,err,sizeof err);
    int used=snprintf(out,cap,"%d|%s|%d|%d|%d|%s|%s",rc,err,l.n,l.has_id,l.trace,l.stage,l.input_hex);
    if(used<0||(size_t)used>=cap){rl_clear(&l);return 1;}
    for(int i=0;i<l.n;i++) {
        RLRow *r=&l.rows[i];int z=snprintf(out+used,cap-(size_t)used,"|%d,%d,%d,%s,",r->kn,r->present,r->vn,r->blob);
        if(z<0||(size_t)z>=cap-(size_t)used){rl_clear(&l);return 1;}used+=z;
        for(int j=0;j<r->kn;j++){z=snprintf(out+used,cap-(size_t)used,"%02x",r->key[j]);if(z<0||(size_t)z>=cap-(size_t)used){rl_clear(&l);return 1;}used+=z;}
        for(int j=0;j<r->vn;j++){z=snprintf(out+used,cap-(size_t)used,"%02x",r->value[j]);if(z<0||(size_t)z>=cap-(size_t)used){rl_clear(&l);return 1;}used+=z;}
    }
    rl_clear(&l);return 0;
}

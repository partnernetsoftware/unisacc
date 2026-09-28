#ifndef UNISACC_LIBRARYCARRIERPLAN_H
#define UNISACC_LIBRARYCARRIERPLAN_H
/* Internal model output only. A user-supplied carrier is not an ABI proof.
   The target model chooses the carrier; this decoder owns and binds its graphs.
   Clear after all dependent callable entries have been quiesced. */
#include "librarycallables.h"
typedef struct us_carrier_certificate {
    us_exports original,carrier;
    char target[32];
} us_carrier_certificate;
static void us_carrier_certificate_clear(us_carrier_certificate *p){
    if(!p)return;
    us_exports_clear(&p->original);us_exports_clear(&p->carrier);
    memset(p,0,sizeof *p);
}
static int us_carrier_certificate_slice(const unsigned char *b,size_t n,size_t *at,
                                        const unsigned char **out,size_t *size){
    if(*at>n||n-*at<8)return 1;
    uint64_t z=0;for(unsigned i=0;i<8;i++)z|=(uint64_t)b[*at+i]<<(8*i);
    *at+=8;if(z>n-*at)return 1;
    *out=b+*at;*size=(size_t)z;*at+=(size_t)z;return 0;
}
static int us_carrier_certificate_load(us_carrier_certificate *dest,
                                      const char *target,const void *blob,size_t length,
                                      char *error,size_t cap){
    static const unsigned char magic[]="USLNCAR1\n";
    if(!dest||!target||!blob||length<sizeof magic-1||length>33554432)
        return us_export_error(error,cap,"invalid model carrier certificate");
    const unsigned char *b=blob,*name,*original,*carrier;size_t at=sizeof magic-1,nn,on,cn;
    if(memcmp(b,magic,sizeof magic-1)||
       us_carrier_certificate_slice(b,length,&at,&name,&nn)||
       nn==0||nn>=sizeof dest->target||nn!=strlen(target)||memcmp(name,target,nn)||
       us_carrier_certificate_slice(b,length,&at,&original,&on)||
       us_carrier_certificate_slice(b,length,&at,&carrier,&cn)||at!=length)
        return us_export_error(error,cap,"model carrier framing or target mismatch");
    us_carrier_certificate tmp={0};
    if(us_exports_load_bridge(&tmp.original,original,on,error,cap)||
       us_exports_load_bridge(&tmp.carrier,carrier,cn,error,cap))goto bad;
    if(tmp.original.count!=1||tmp.carrier.count!=1)goto shape;
    us_export *x=tmp.original.items,*y=tmp.carrier.items;
    if(x->version!=2||y->version!=2||x->linkage||y->linkage||
       x->defined!=1||y->defined!=1||x->variadic||y->variadic||
       x->count!=x->stored||y->count!=y->stored||
       x->count!=y->count||x->mode!=y->mode||strcmp(x->name,y->name)||
       y->supported!=1||x->result.width!=y->result.width||
       x->result.alignment!=y->result.alignment)goto shape;
    for(size_t i=0;i<x->count;i++){
        const us_export_type *a=us_export_arg(x,i),*c=us_export_arg(y,i);
        if(a->width!=c->width||a->alignment!=c->alignment)goto shape;
    }
    memcpy(tmp.target,name,nn);tmp.target[nn]=0;
    us_carrier_certificate_clear(dest);*dest=tmp;return 0;
shape:us_export_error(error,cap,"model carrier signature storage mismatch");
bad:us_carrier_certificate_clear(&tmp);return 1;
}
static int us_carrier_certificate_make(const us_carrier_certificate *p,
                                      us_callables *registry,unsigned origin,
                                      const us_export_signature *expected,
                                      uintptr_t raw,uint64_t *handle,
                                      char *error,size_t cap){
    us_export_signature original,carrier;
    if(!p||p->original.count!=1||p->carrier.count!=1||!expected||
       us_callable_export_signature(p->original.items,&original)||
       us_callable_export_signature(p->carrier.items,&carrier)||
       !us_callable_signature_equal(expected,&original))
        return us_export_error(error,cap,"carrier certificate original declaration mismatch");
    return us_callable_make_carrier(registry,origin,&original,&carrier,raw,handle,error,cap);
}
#endif

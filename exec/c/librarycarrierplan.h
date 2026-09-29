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
    if((x->version!=2&&x->version!=3)||y->version!=2||x->linkage||y->linkage||
       x->defined!=1||y->defined!=1||x->variadic||y->variadic||
       x->count!=x->stored||y->count!=y->stored||
       x->count!=y->count||x->mode!=y->mode||strcmp(x->name,y->name)||
       y->supported!=1||x->result.width!=y->result.width||
       x->result.alignment!=y->result.alignment)goto shape;
    for(size_t i=0;i<x->count;i++){
        const us_export_type *a=us_export_arg(x,i),*c=us_export_arg(y,i);
        if(a->width!=c->width||a->alignment!=c->alignment)goto shape;
    }
    /* Internal model authority chooses the recipe; this is storage/edge pairing only. */
    us_export_signature original_view,carrier_view;
    if(us_callable_export_signature(x,&original_view)||us_callable_export_signature(y,&carrier_view)||
       !us_callable_carrier_valid(&original_view,&carrier_view))goto shape;
    memcpy(tmp.target,name,nn);tmp.target[nn]=0;
    us_carrier_certificate_clear(dest);*dest=tmp;return 0;
shape:us_export_error(error,cap,"model carrier signature storage mismatch");
bad:us_carrier_certificate_clear(&tmp);return 1;
}
/* Consume the decoded model certificate only after every mechanical check and
   CIF preparation succeeds. All frame memory continues to use the original. */
static int us_carrier_certificate_native_add(us_native_plans *plans,uintptr_t raw,
                                             us_carrier_certificate *certificate,uint64_t *handle,
                                             char *error,size_t cap){
    if(!plans||!raw||!certificate||!handle)return us_export_error(error,cap,"invalid carrier native plan output");
    *handle=0;us_export_signature original,carrier;
    if(certificate->original.count!=1||certificate->carrier.count!=1||!certificate->target[0]||
       us_callable_export_signature(certificate->original.items,&original)||
       us_callable_export_signature(certificate->carrier.items,&carrier)||
       !us_callable_carrier_valid(&original,&carrier)||
       strcmp(certificate->original.items->name,certificate->carrier.items->name)||
       !(us_export_supported(certificate->carrier.items)||us_export_bridge_supported(certificate->carrier.items)))
        return us_export_error(error,cap,"invalid carrier native plan certificate");
    us_native_plan *plan=calloc(1,sizeof *plan);if(!plan)return us_export_error(error,cap,"carrier native plan allocation failed");
    if(us_export_has_callbacks(certificate->original.items)){plan->bridge_required=1;goto publish;}
    plan->args=calloc(carrier.count?(size_t)carrier.count:1,sizeof *plan->args);if(!plan->args)goto bad;
    for(size_t i=0;i<carrier.count;i++)if(!(plan->args[i]=us_export_ffitype(carrier.argtypes+i,0)))goto bad;
    ffi_type *result=us_export_ffitype(&carrier.result,1);if(!result||
       ffi_prep_cif(&plan->cif,FFI_DEFAULT_ABI,(unsigned)carrier.count,result,plan->args)!=FFI_OK)goto bad;
publish:
    plan->graph=certificate->original;plan->carrier=certificate->carrier;plan->signature=original;plan->target=raw;
    memset(certificate,0,sizeof *certificate);
    plan->next=plans->head;plans->head=plan;*handle=(uintptr_t)plan;return 0;
bad:us_native_plan_free(plan);return us_export_error(error,cap,"carrier native ffi signature rejected");
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

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
/* One preparation entry with an explicit mechanism parameter. Existing
   numeric 0/1 callers retain their fixed/variadic meaning. */
typedef enum us_native_prepare_kind {
    US_NATIVE_PREPARE_FIXED=0,
    US_NATIVE_PREPARE_VARIADIC=1,
    US_NATIVE_PREPARE_CALLBACK_GRAPH=2
} us_native_prepare_kind;
/* Distinct class attachment identity, common address-free callable cleanup.
   This is owned by the existing pending native plan, not a callable registry. */
static void us_callback_graph_release(void *value){if(!value)return;us_callable_free(value);US_LATE_NOTE_RELEASE();}
/* Mechanism parameters select validation/CIF work; no class test enters the
   common owner/rollback/publish path. Logical kinds remain owned by W rows. */
typedef int (*us_native_prepare_validate_fn)(const us_carrier_certificate *,const us_export_signature *,const us_export_signature *,uint64_t,char *,size_t);
typedef int (*us_native_prepare_cif_fn)(us_native_plan *,const us_carrier_certificate *,const us_export_signature *,const us_export_signature *,uint64_t,int);
typedef struct us_native_prepare_mode_ops {us_native_prepare_validate_fn validate;us_native_prepare_cif_fn cif;int variadic;} us_native_prepare_mode_ops;
static int us_native_prepare_fixed_validate(const us_carrier_certificate *c,const us_export_signature *o,const us_export_signature *v,uint64_t fixed,char *error,size_t cap){return 0;}
static int us_native_prepare_variadic_validate(const us_carrier_certificate *c,const us_export_signature *o,const us_export_signature *v,uint64_t fixed,char *error,size_t cap){
    if(!fixed||fixed>o->count||o->variadic||v->variadic||us_export_has_callbacks(c->original.items))
        return us_export_error(error,cap,"invalid variadic carrier plan certificate");
    for(size_t i=(size_t)fixed;i<v->count;i++){
        const us_export_type *t=v->argtypes+i;
        if((t->kind==3&&t->width==4)||(t->kind==1&&t->width<4))return us_export_error(error,cap,"variadic tail requires promoted types");
    }
    return 0;
}
static int us_native_prepare_graph_validate(const us_carrier_certificate *c,const us_export_signature *o,const us_export_signature *v,uint64_t fixed,char *error,size_t cap){
    if(o->variadic||v->variadic||!us_export_has_callbacks(c->original.items))return us_export_error(error,cap,"carrier native ffi signature rejected");return 0;
}
static int us_native_prepare_scalar_cif(us_native_plan *plan,const us_carrier_certificate *certificate,const us_export_signature *o,const us_export_signature *v,uint64_t fixed,int variadic){
    if(us_export_has_callbacks(certificate->original.items)){plan->bridge_required=1;return 0;}
    plan->args=calloc(v->count?(size_t)v->count:1,sizeof *plan->args);if(!plan->args)return 1;
    for(size_t i=0;i<v->count;i++)if(!(plan->args[i]=us_export_ffitype(v->argtypes+i,0)))return 1;
    ffi_type *result=us_export_ffitype(&v->result,1);if(!result)return 1;
    ffi_status status=variadic ? ffi_prep_cif_var(&plan->cif,FFI_DEFAULT_ABI,(unsigned)fixed,(unsigned)v->count,result,plan->args) :
        ffi_prep_cif(&plan->cif,FFI_DEFAULT_ABI,(unsigned)v->count,result,plan->args);
    return status!=FFI_OK;
}
static int us_native_prepare_graph_cif(us_native_plan *plan,const us_carrier_certificate *certificate,const us_export_signature *o,const us_export_signature *v,uint64_t fixed,int variadic){
    us_callable *prepared=calloc(1,sizeof *prepared);if(!prepared)return 1;
    if(us_callable_prepare_graph(prepared,o,v)){us_callable_free(prepared);return 1;}
    plan->attachment=prepared;plan->release_attachment=us_callback_graph_release;plan->bridge_required=1;return 0;
}
static const us_native_prepare_mode_ops US_NATIVE_PREPARE_MODES[]={
    {us_native_prepare_fixed_validate,us_native_prepare_scalar_cif,0},
    {us_native_prepare_variadic_validate,us_native_prepare_scalar_cif,1},
    {us_native_prepare_graph_validate,us_native_prepare_graph_cif,0}
};
/* One preparation entry and one pending-owner publish point, modes 0/1/2. */
static int us_carrier_certificate_native_prepare_mode(us_native_plans *plans,
                                             us_carrier_certificate *certificate,us_native_prepare_kind kind,uint64_t fixed_count,uint64_t *handle,
                                             char *error,size_t cap){
    if(!plans||!certificate||!handle)return us_export_error(error,cap,"invalid carrier native plan output");
    *handle=0;us_export_signature original,carrier;
    if((unsigned)kind>=sizeof US_NATIVE_PREPARE_MODES/sizeof US_NATIVE_PREPARE_MODES[0])
        return us_export_error(error,cap,"invalid native preparation kind");
    const us_native_prepare_mode_ops *ops=US_NATIVE_PREPARE_MODES+(unsigned)kind;
    if(certificate->original.count!=1||certificate->carrier.count!=1||!certificate->target[0]||
       us_callable_export_signature(certificate->original.items,&original)||
       us_callable_export_signature(certificate->carrier.items,&carrier)||
       !us_callable_carrier_valid(&original,&carrier)||
       strcmp(certificate->original.items->name,certificate->carrier.items->name)||
       !(us_export_supported(certificate->carrier.items)||us_export_bridge_supported(certificate->carrier.items)))
        return us_export_error(error,cap,"invalid carrier native plan certificate");
    if(ops->validate(certificate,&original,&carrier,fixed_count,error,cap))return 1;
    us_native_plan *plan=calloc(1,sizeof *plan);if(!plan)return us_export_error(error,cap,"carrier native plan allocation failed");
    if(ops->cif(plan,certificate,&original,&carrier,fixed_count,ops->variadic))goto bad;
    plan->graph=certificate->original;plan->carrier=certificate->carrier;plan->signature=original;plan->target=0;
    memset(certificate,0,sizeof *certificate);
    plan->next=plans->head;plans->head=plan;*handle=(uintptr_t)plan;return 0;
bad:us_native_plan_free(plan);return us_export_error(error,cap,"carrier native ffi signature rejected");
}
static int us_carrier_certificate_native_prepare(us_native_plans *plans,us_carrier_certificate *certificate,uint64_t *handle,char *error,size_t cap){
    return us_carrier_certificate_native_prepare_mode(plans,certificate,0,0,handle,error,cap);
}
/* Existing address-bearing entry retains its nonzero-target contract. */
static int us_carrier_certificate_native_add(us_native_plans *plans,uintptr_t raw,
                                             us_carrier_certificate *certificate,uint64_t *handle,
                                             char *error,size_t cap){
    if(!plans||!raw||!certificate||!handle)return us_export_error(error,cap,"invalid carrier native plan output");
    int rc=us_carrier_certificate_native_prepare(plans,certificate,handle,error,cap);
    if(!rc)us_native_plan_find(plans,*handle)->target=raw;return rc;
}
/* Variadic concrete site: the certified fixed concrete graph is the original,
   the CIF is prepared from the carrier with ffi_prep_cif_var over the model's
   fixed prefix count. Callback edges stay rejected here (no bridge for varargs). */
static int us_carrier_certificate_native_add_variadic(us_native_plans *plans,uintptr_t raw,
                                                      us_carrier_certificate *certificate,uint64_t fixed_count,
                                                      uint64_t *handle,char *error,size_t cap){
    if(!plans||!raw||!certificate||!handle)return us_export_error(error,cap,"invalid variadic carrier plan output");
    int rc=us_carrier_certificate_native_prepare_mode(plans,certificate,1,fixed_count,handle,error,cap);
    if(!rc)us_native_plan_find(plans,*handle)->target=raw;return rc;
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

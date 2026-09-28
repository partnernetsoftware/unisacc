#ifndef UNISACC_LIBRARYNATIVE_H
#define UNISACC_LIBRARYNATIVE_H
/* Explicit declared native ABI only. Root owns frame cleanup registration. */
#include "libraryexports.h"
typedef struct us_native_plan {
    struct us_native_plan *next; us_exports graph,carrier; ffi_cif cif; ffi_type **args;
    uintptr_t target;unsigned bridge_required;us_export_signature signature;
} us_native_plan;
typedef struct us_native_plans us_native_plans;
/* Internal provider classifies model declarations; users cannot supply carriers. */
typedef int (*us_native_carrier_provider)(void *,us_native_plans *,uintptr_t,const void *,size_t,uint64_t *,char *,size_t);
struct us_native_plans { us_native_plan *head;us_native_carrier_provider carrier_provider;void *carrier_owner; };
typedef struct us_call_outcome {
    int failed,exited,exit_status;char message[1024];
} us_call_outcome;
static void us_call_outcome_merge(us_call_outcome *to,const us_call_outcome *from){
    if(to && from && !to->failed && !to->exited && (from->failed || from->exited))*to=*from;
}
typedef struct us_native_boundary {
    void *owner;us_call_outcome outcome;struct us_native_boundary *previous;
} us_native_boundary;
typedef struct us_native_arena {
    us_native_plan *plan; void **values; void **owned; void *native_result;
    void *result_target; size_t count;us_native_boundary boundary;
} us_native_arena;
static void us_native_plan_free(us_native_plan *p){if(!p)return;free(p->args);us_exports_clear(&p->graph);us_exports_clear(&p->carrier);free(p);}
static void us_native_plans_clear(us_native_plans *p){if(!p)return;while(p->head){us_native_plan *n=p->head->next;us_native_plan_free(p->head);p->head=n;}p->carrier_provider=NULL;p->carrier_owner=NULL;}
static us_native_plan *us_native_plan_find(const us_native_plans *p,uint64_t handle){
    if(p)for(us_native_plan *n=p->head;n;n=n->next)if((uint64_t)(uintptr_t)n==handle)return n;return NULL;
}
static int us_native_plan_add_call_capability(us_native_plans *plans,uintptr_t target,const void *sig,size_t length,int variadic,uint64_t fixed_count,int bridge,uint64_t *handle,char *error,size_t cap){
    if(!plans||!handle||!target||!sig)return us_export_error(error,cap,"invalid native plan output or target");
    *handle=0;us_native_plan *p=calloc(1,sizeof *p);if(!p)return us_export_error(error,cap,"native plan allocation failed");
    if(us_exports_load_capability(&p->graph,sig,length,bridge,error,cap)||p->graph.count!=1){us_native_plan_free(p);return us_export_error(error,cap,"invalid typed native graph");}
    us_export *x=p->graph.items;
    if(x->version!=2||x->linkage||x->defined!=1){us_native_plan_free(p);return us_export_error(error,cap,"invalid typed native graph");}
    if(variadic && (!fixed_count || fixed_count>x->count || x->variadic)){
        us_native_plan_free(p);return us_export_error(error,cap,"invalid variadic fixed prefix");
    }
    if(variadic)for(size_t i=(size_t)fixed_count;i<(size_t)x->count;i++){
        const us_export_type *t=us_export_arg(x,i);
        if((t->kind==3 && t->width==4) || (t->kind==1 && t->width<4)){
            us_native_plan_free(p);return us_export_error(error,cap,"variadic tail requires promoted types");
        }
    }
    if(bridge && !variadic && us_export_has_callbacks(x)){
        if(!us_export_bridge_supported(x)){us_native_plan_free(p);return 0;}
        p->bridge_required=1;p->signature.count=x->count;p->signature.stored=x->stored;
        p->signature.mode=x->mode;p->signature.supported=x->supported;
        p->signature.result=x->result;p->signature.argtypes=x->argtypes;
        p->target=target;p->next=plans->head;plans->head=p;*handle=(uintptr_t)p;return 0;
    }
    if(!us_export_supported(x)){
        int eligible=!variadic&&!x->variadic&&!us_export_has_callbacks(x);
        us_native_plan_free(p);
        return eligible&&plans->carrier_provider ? plans->carrier_provider(plans->carrier_owner,plans,target,sig,length,handle,error,cap):0;
    }
    p->args=calloc(x->count ? (size_t)x->count:1,sizeof *p->args);if(!p->args){us_native_plan_free(p);return us_export_error(error,cap,"native plan allocation failed");}
    for(size_t i=0;i<(size_t)x->count;i++)p->args[i]=us_export_ffitype(us_export_arg(x,i),0);
    ffi_status status=variadic ?
        ffi_prep_cif_var(&p->cif,FFI_DEFAULT_ABI,(unsigned)fixed_count,(unsigned)x->count,us_export_ffitype(&x->result,1),p->args) :
        ffi_prep_cif(&p->cif,FFI_DEFAULT_ABI,(unsigned)x->count,us_export_ffitype(&x->result,1),p->args);
    if(status!=FFI_OK){us_native_plan_free(p);return us_export_error(error,cap,"native ffi signature rejected");}
    p->target=target;p->next=plans->head;plans->head=p;*handle=(uintptr_t)p;return 0;
}
static int us_native_plan_add_call(us_native_plans *plans,uintptr_t target,const void *sig,size_t length,int variadic,uint64_t fixed_count,uint64_t *handle,char *error,size_t cap){
    return us_native_plan_add_call_capability(plans,target,sig,length,variadic,fixed_count,0,handle,error,cap);
}
/* Concrete call declaration: total arguments, after model-owned promotions.
   The function prototype is validated separately by the model, not inferred here. */
static int us_native_plan_add_variadic(us_native_plans *plans,uintptr_t target,const void *sig,size_t length,uint64_t fixed_count,uint64_t *handle,char *error,size_t cap){
    return us_native_plan_add_call_capability(plans,target,sig,length,1,fixed_count,0,handle,error,cap);
}
static int us_native_plan_add(us_native_plans *plans,uintptr_t target,const void *sig,size_t length,uint64_t *handle,char *error,size_t cap){
    return us_native_plan_add_call_capability(plans,target,sig,length,0,0,0,handle,error,cap);
}
static int us_native_plan_add_bridge(us_native_plans *plans,uintptr_t target,const void *sig,size_t length,uint64_t *handle,char *error,size_t cap){
    return us_native_plan_add_call_capability(plans,target,sig,length,0,0,1,handle,error,cap);
}
static void us_native_arena_free(us_native_arena *a){if(!a)return;if(a->owned)for(size_t i=0;i<a->count;i++)free(a->owned[i]);free(a->owned);free(a->values);free(a->native_result);free(a);}
static int us_native_prepare(us_native_plan *p,const uint64_t *slots,uint64_t count,void *result_target,us_native_arena **out,char *error,size_t cap){
    if(!out)return us_export_error(error,cap,"invalid native arena output");*out=NULL;
    if(!p || p->bridge_required || !p->target || p->graph.count!=1 || count!=p->graph.items[0].count || count>1024 || (count&&!slots))return us_export_error(error,cap,"invalid native call frame");
    us_export *x=p->graph.items;
    if(x->result.kind && !result_target)return us_export_error(error,cap,"missing native result storage");
    size_t total=sizeof(us_native_arena)+(size_t)count*2*sizeof(void*),limit=16777216;
    for(size_t i=0;i<(size_t)count;i++){const us_export_type *t=us_export_arg(x,i);size_t n=t->kind==5 ? (size_t)t->width:8;if(n>limit-total)return us_export_error(error,cap,"native call arena exceeds 16 MiB");total+=n;}
    size_t rn=x->result.kind ? (size_t)x->result.width:0;if(rn && rn<sizeof(ffi_arg))rn=sizeof(ffi_arg);
    if(rn>limit-total)return us_export_error(error,cap,"native call arena exceeds 16 MiB");
    us_native_arena *a=calloc(1,sizeof *a);if(!a)return us_export_error(error,cap,"native arena allocation failed");
    a->plan=p;a->count=(size_t)count;a->result_target=result_target;
    a->values=calloc(count ? (size_t)count:1,sizeof *a->values);a->owned=calloc(count ? (size_t)count:1,sizeof *a->owned);
    if(!a->values||!a->owned)goto bad;
    for(size_t i=0;i<(size_t)count;i++){
        const us_export_type *t=us_export_arg(x,i);size_t n=t->kind==5 ? (size_t)t->width:8;
        a->owned[i]=calloc(1,n);if(!a->owned[i])goto bad;a->values[i]=a->owned[i];
        if(t->kind==5){if(!slots[i])goto bad;memcpy(a->owned[i],(void*)(uintptr_t)slots[i],n);}
        else memcpy(a->owned[i],&slots[i],(size_t)t->width);
    }
    if(x->result.kind){size_t n=(size_t)x->result.width;if(n<sizeof(ffi_arg))n=sizeof(ffi_arg);a->native_result=calloc(1,n);if(!a->native_result)goto bad;}
    *out=a;return 0;
bad:us_native_arena_free(a);return us_export_error(error,cap,"native arena allocation or argument failed");
}
/* Register arena with ScriptFrame BEFORE calling: explicit script exit can unwind. */
static int us_native_call(us_native_arena *a){
    if(!a||!a->plan||a->plan->bridge_required)return 1;
    ffi_call(&a->plan->cif,FFI_FN((void*)a->plan->target),a->native_result,a->values);
    return 0;
}
/* Commit only after the owner checked the callback boundary outcome. */
static int us_native_commit(us_native_arena *a){
    if(!a||!a->plan||a->plan->bridge_required||a->boundary.outcome.failed||a->boundary.outcome.exited)return 1;
    us_export *x=a->plan->graph.items;
    if(x->result.kind==5)memcpy(a->result_target,a->native_result,(size_t)x->result.width);
    else if(x->result.kind){uint64_t v=us_export_slot(&x->result,a->native_result);memcpy(a->result_target,&v,8);}
    return 0;
}
static int us_native_invoke(us_native_arena *a){return us_native_call(a)||us_native_commit(a);}
#endif

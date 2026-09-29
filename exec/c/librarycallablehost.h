#ifndef UNISACC_LIBRARYCALLABLEHOST_H
#define UNISACC_LIBRARYCALLABLEHOST_H
/* Host ABI dispatch only: model supplies origin, declaration key and callsite.
   Included after ScriptFrame helpers; registry cleans its copies before unwind. */
static int library_callable_script_hook(void *owner,const void *raw,const us_export_signature *signature,const us_export_frame *values){
    us_context *c=owner;
    if(!signature||!values||signature->count!=values->count||signature->mode!=values->mode)return library_invocation_error(c,"invalid declared script callable frame");
    LibraryCallableScope scope={c,signature,values,library_callable_scopes};library_callable_scopes=&scope;
    int rc=library_invoke_frame(owner,raw,values);library_callable_scopes=scope.previous;return rc;
}
static void library_callable_failure_hook(void *owner,const char *message){
    library_invocation_error(owner,message);
}
static int library_callable_native_hook(void *owner,ffi_cif *cif,uintptr_t target,void *result,void **args){
    us_context *c=owner;ScriptFrame *f=script_frames;
    if(!c||!f||f->owner!=c)return library_invocation_error(c,"native callable outside owner frame");
    us_native_boundary boundary={0};boundary.owner=c;boundary.previous=native_boundaries;native_boundaries=&boundary;
    ffi_call(cif,FFI_FN((void*)target),result,args);
    native_boundaries=boundary.previous;
    us_call_outcome_merge(f->outcome,&boundary.outcome);
    return boundary.outcome.failed||boundary.outcome.exited;
}
static uint64_t library_callable_propagate(us_context *c,int rc){
    ScriptFrame *f=script_frames;
    if(f&&f->owner==c&&(f->outcome->failed||f->outcome->exited)){
        f->failed=f->outcome->failed;f->exited=f->outcome->exited;f->status=f->outcome->exit_status;
        if(f->outcome->message[0])snprintf(c->error,sizeof c->error,"%s",f->outcome->message);
        longjmp(f->returned,1); /* registry temporary objects already cleaned. */
    }
    return rc?library_dispatch_error(c,NULL):0;
}
static int library_callable_slots(us_context *c,ScriptFrame *f,const us_export_signature *s,uint64_t slots,uint64_t count,uint64_t result){
    size_t bytes=s->result.kind==5?(size_t)s->result.width:s->result.kind?8:0;
    if(count!=s->count||count>1024||!library_frame_region(c,f,slots,(size_t)count*8)||!library_frame_region(c,f,result,bytes))return 1;
    const uint64_t *values=(const uint64_t*)(uintptr_t)slots;
    for(size_t i=0;i<count;i++)if(s->argtypes[i].kind==5&&!library_frame_region(c,f,values[i],(size_t)s->argtypes[i].width))return 1;
    return 0;
}
static uint64_t library_callable_make_dispatch(uint64_t origin,uint64_t key,uint64_t raw,uint64_t output,uint64_t r0,uint64_t r1){
    us_context *c=active;ScriptFrame *f=script_frames;
    if(!c||!f||f->owner!=c)return 1;
    const us_export_signature *s=library_callable_signature(c,key);
    if(!s||r1||!library_frame_region(c,f,output,8))return library_dispatch_error(c,"invalid callable introduction frame");
    const us_export_signature *carrier=NULL;us_export_signature carrier_view;
    if(r0){
        /* Model-issued import alias: the frozen candidate stays the identity anchor, the
           independently certified source plan supplies the carrier. No fallback. */
        LibraryImportAlias *alias=origin==US_CALLABLE_NATIVE?library_import_alias(c,r0):NULL;
        us_native_plan *frozen=alias?us_native_plan_find(&c->native_plans,alias->frozen):NULL;
        us_native_plan *plan=alias?us_native_plan_find(&c->import_alias_plans,alias->plan):NULL;
        if(!alias||alias->key!=key||alias->raw!=raw||!frozen||frozen->target!=raw||!plan||plan->target!=raw||
           !us_callable_signature_equal(&plan->signature,s)||!plan->carrier.count)return library_dispatch_error(c,"invalid native import alias introduction");
        if(us_callable_export_signature(plan->carrier.items,&carrier_view))return library_dispatch_error(c,"invalid import alias carrier graph");
        carrier=&carrier_view;
    }else if(origin==US_CALLABLE_SCRIPT){
        if(!library_region((uintptr_t)c->image,(size_t)c->image_text_size,(uintptr_t)raw,1))return library_dispatch_error(c,"script callable outside owner code");
        for(LibraryCarrierExport *entry=c->carrier_exports;entry;entry=entry->next){
            us_export_signature original;const void *address=NULL;int kind=-1;
            if(!library_lookup(c,entry->source->name,&address,&kind)&&!kind&&(uintptr_t)address==raw&&
               !us_callable_export_signature(entry->source,&original)&&us_callable_signature_equal(&original,s)){
                if(us_callable_export_signature(entry->certificate.carrier.items,&carrier_view))return library_dispatch_error(c,"invalid cached script carrier graph");
                carrier=&carrier_view;break;
            }
        }
    }else if(origin==US_CALLABLE_NATIVE){
        int declared=0;for(us_native_plan *p=c->native_plans.head;p;p=p->next){
            us_export_signature view;if(p->target==raw&&!us_callable_export_signature(p->graph.items,&view)&&us_callable_signature_equal(&view,s)){declared=1;if(p->carrier.count){if(us_callable_export_signature(p->carrier.items,&carrier_view))return library_dispatch_error(c,"invalid frozen carrier graph");carrier=&carrier_view;}break;}}
        if(!declared)for(us_native_template *p=c->native_templates.head;p;p=p->next){
            us_export_signature view;if(p->target==raw&&!us_callable_export_signature(p->graph.items,&view)&&us_callable_signature_equal(&view,s)){declared=1;break;}}
        if(!declared)return library_dispatch_error(c,"native callable lacks frozen candidate declaration");
    }else return library_dispatch_error(c,"invalid callable origin");
    uint64_t h=0;int rc=carrier ? us_callable_make_carrier(&c->callables,(unsigned)origin,s,carrier,(uintptr_t)raw,&h,c->error,sizeof c->error) : us_callable_make(&c->callables,(unsigned)origin,s,(uintptr_t)raw,&h,c->error,sizeof c->error);
    if(!rc)memcpy((void*)(uintptr_t)output,&h,8);
    return library_callable_propagate(c,rc);
}
static uint64_t library_callable_call_dispatch(uint64_t handle,uint64_t key,uint64_t slots,uint64_t result,uint64_t count,uint64_t site){
    us_context *c=active;ScriptFrame *f=script_frames;
    if(!c||!f||f->owner!=c)return 1;
    const us_export_signature *s=library_callable_signature(c,key);
    const LibraryCallableSite *concrete=site ? library_callable_site(c,key,site):NULL;
    if(!s||(site ? !concrete:s->variadic)||
       library_callable_slots(c,f,concrete?&concrete->signature:s,slots,count,result))return library_dispatch_error(c,"invalid declared callable call frame");
    int rc=concrete ?
        us_callable_call_concrete(&c->callables,handle,s,&concrete->signature,(const uint64_t*)(uintptr_t)slots,(void*)(uintptr_t)result,count,c->error,sizeof c->error):
        us_callable_call(&c->callables,handle,s,(const uint64_t*)(uintptr_t)slots,(void*)(uintptr_t)result,count,c->error,sizeof c->error);
    return library_callable_propagate(c,rc);
}
static uint64_t library_callable_plan_invoke(us_context *c,ScriptFrame *f,us_native_plan *plan,uint64_t slots,uint64_t result,uint64_t count){
    const us_export_signature *s=&plan->signature;
    if(library_callable_slots(c,f,s,slots,count,result))return library_dispatch_error(c,"invalid callback native plan frame");
    us_export_signature carrier;int has_carrier=plan->carrier.count!=0;
    if(has_carrier&&us_callable_export_signature(plan->carrier.items,&carrier))return library_dispatch_error(c,"invalid callback carrier plan");
    uint64_t handle=0;int rc=has_carrier?us_callable_make_carrier(&c->callables,US_CALLABLE_NATIVE,s,&carrier,plan->target,&handle,c->error,sizeof c->error):us_callable_make(&c->callables,US_CALLABLE_NATIVE,s,plan->target,&handle,c->error,sizeof c->error);
    if(!rc)rc=us_callable_call(&c->callables,handle,s,(const uint64_t*)(uintptr_t)slots,(void*)(uintptr_t)result,count,c->error,sizeof c->error);
    return library_callable_propagate(c,rc);
}
#endif

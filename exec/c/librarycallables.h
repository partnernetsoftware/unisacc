#ifndef UNISACC_LIBRARYCALLABLES_H
#define UNISACC_LIBRARYCALLABLES_H
/* Declared callable ABI mechanism. Quiesce before clear; borrowed native targets
   must outlive the generation. No source parser, raw-address origin classifier,
   or untracked ffi_call is present. Define IMPLEMENTATION in exactly one TU. */
#include "librarycallplans.h"
#include <stdatomic.h>
#ifdef US_CALLABLES_IMPLEMENTATION
_Atomic uint64_t us_callable_global_token=1;
#else
extern _Atomic uint64_t us_callable_global_token;
#endif
#define US_CALLABLE_SCRIPT 1
#define US_CALLABLE_NATIVE 2
typedef int (*us_callable_native_call)(void *,ffi_cif *,uintptr_t,void *,void **);
typedef void (*us_callable_failure)(void *,const char *);
typedef int (*us_callable_script_call)(void *,const void *,const us_export_signature *,const us_export_frame *);
typedef struct us_callable us_callable;
typedef struct us_callable_conversion {
    unsigned action;const us_export_type *original,*carrier;
    struct us_callable_conversion **members,*element;
} us_callable_conversion;
typedef struct us_callables {
    void *owner;uint64_t generation;us_callable *head;
    us_export_invoke_frame invoke_frame;us_callable_script_call script_call;us_callable_native_call native_call;
    us_callable_failure failure;
} us_callables;
struct us_callable {
    us_callable *next,*global_next;us_callables *registry;uint64_t token,generation;
    unsigned origin;uintptr_t target;us_export_graph graph,carrier_graph;ffi_cif cif;ffi_type **args;
    ffi_closure *closure;void *code;unsigned char opaque_result,*opaque_args;
    us_callable_conversion *result_plan,**arg_plans;
};
#ifdef US_CALLABLES_IMPLEMENTATION
us_callable *us_callable_global_closures;
atomic_flag us_callable_global_lock=ATOMIC_FLAG_INIT;
#else
extern us_callable *us_callable_global_closures;
extern atomic_flag us_callable_global_lock;
#endif
static void us_callable_lock(void){while(atomic_flag_test_and_set_explicit(&us_callable_global_lock,memory_order_acquire)){} }
static void us_callable_unlock(void){atomic_flag_clear_explicit(&us_callable_global_lock,memory_order_release);}
static int us_callable_error(char *e,size_t n,const char *s){return us_export_error(e,n,s);}
static void us_callables_init(us_callables *r,void *owner,uint64_t generation,
                             us_export_invoke_frame invoke,us_callable_native_call native,
                             us_callable_failure failure){
    memset(r,0,sizeof *r);r->owner=owner;r->generation=generation;r->invoke_frame=invoke;r->native_call=native;r->failure=failure;
}
static void us_callable_conversion_free(us_callable_conversion *p){
    if(!p)return;if(p->members)for(size_t i=0;i<p->original->nmembers;i++)us_callable_conversion_free(p->members[i]);
    us_callable_conversion_free(p->element);free(p->members);free(p);
}
static void us_callable_free(us_callable *p){
    if(!p)return;if(p->closure)ffi_closure_free(p->closure);
    us_callable_conversion_free(p->result_plan);
    if(p->arg_plans)for(size_t i=0;i<p->graph.signatures[0]->count;i++)us_callable_conversion_free(p->arg_plans[i]);
    free(p->arg_plans);free(p->opaque_args);free(p->args);us_export_graph_clear(&p->graph);us_export_graph_clear(&p->carrier_graph);free(p);
}
static void us_callables_clear(us_callables *r){
    if(!r)return;us_callable_lock();us_callable **q=&us_callable_global_closures;
    while(*q){if((*q)->registry==r)*q=(*q)->global_next;else q=&(*q)->global_next;}
    us_callable_unlock();while(r->head){us_callable *p=r->head;r->head=p->next;us_callable_free(p);}r->generation=0;
}
static int us_callable_export_signature(const us_export *x,us_export_signature *s){
    if(!x||!s||(x->version!=2&&x->version!=3)||x->count!=x->stored||x->count>1024)return 1;
    memset(s,0,sizeof *s);s->count=x->count;s->stored=x->stored;s->variadic=x->variadic;s->mode=x->mode;s->supported=x->supported;s->result=x->result;s->argtypes=x->argtypes;return 0;
}
static int us_callable_budget(const us_export_signature *s){
    size_t total=sizeof(uint64_t)*(size_t)s->count+2*sizeof(void*)*(size_t)s->count;
    if(s->result.width>16777216-total)return 1;total+=(size_t)s->result.width;
    for(size_t i=0;i<s->count;i++){size_t n=s->argtypes[i].kind==5?(size_t)s->argtypes[i].width:8;if(n>16777216-total)return 1;total+=n;}
    return 0;
}
static void us_callable_commit_slot(const us_export_type *t,void *dest,const void *value){
    if(t->kind==5)memcpy(dest,value,(size_t)t->width);
    else if(t->kind){uint64_t v=0;if(t->kind==4)memcpy(&v,value,8);else v=us_export_slot(t,value);memcpy(dest,&v,8);}
}
static int us_callable_type_identity(const us_export_type *a,const us_export_type *b){
    us_native_type_pair *work=calloc(16384,sizeof *work);
    us_native_signature_pair *seen=calloc(16384,sizeof *seen);
    size_t pending=0,visited=0,steps=0;int equal=0;
    if(!work||!seen)goto done;
    work[pending++]=(us_native_type_pair){a,b};
    while(pending){
        us_native_type_pair pair=work[--pending];a=pair.a;b=pair.b;
        if(!a||!b||++steps>16384)goto done;
        if(a->depth!=b->depth||a->kind!=b->kind||a->width!=b->width||a->uns!=b->uns||a->alignment!=b->alignment||a->tag!=b->tag||a->nmembers!=b->nmembers||a->count!=b->count||a->stride!=b->stride)goto done;
        if((a->wire_version==3)!=(b->wire_version==3))goto done;
        if(a->wire_version==3 && (a->fp_rank!=b->fp_rank||a->fp_format!=b->fp_format||
           a->natural_alignment!=b->natural_alignment||a->layout_flags!=b->layout_flags||
           a->layout_known_mask!=b->layout_known_mask||a->layout_origin!=b->layout_origin))goto done;
        if((a->element==NULL)!=(b->element==NULL)||(a->signature==NULL)!=(b->signature==NULL)||(a->pointee==NULL)!=(b->pointee==NULL))goto done;
        if(a->pointee){if(pending>=16384)goto done;work[pending++]=(us_native_type_pair){a->pointee,b->pointee};}
        if(a->nmembers>128 || (a->nmembers && (!a->members||!b->members)))goto done;
        if(a->element){if(pending>=16384)goto done;work[pending++]=(us_native_type_pair){a->element,b->element};}
        for(size_t i=0;i<(size_t)a->nmembers;i++){
            const us_export_member *x=a->members+i,*y=b->members+i;
            if(x->offset!=y->offset||x->bit_offset!=y->bit_offset||x->bit_width!=y->bit_width||x->storage!=y->storage||pending>=16384)goto done;
            if(a->wire_version==3 && (x->ordinal!=y->ordinal||x->entry_kind!=y->entry_kind||x->effective_alignment!=y->effective_alignment))goto done;
            work[pending++]=(us_native_type_pair){x->type,y->type};
        }
        if(a->signature){
            const us_export_signature *x=a->signature,*y=b->signature;size_t i;
            for(i=0;i<visited;i++)if(seen[i].a==x && seen[i].b==y)break;
            if(i<visited)continue;
            if(visited>=16384||x->variadic!=y->variadic||x->mode!=y->mode||x->count!=y->count||x->stored!=y->stored||x->stored!=x->count||x->count>1024||
               (x->count && (!x->argtypes||!y->argtypes))||pending+1+x->count>16384)goto done;
            seen[visited++]=(us_native_signature_pair){x,y};
            work[pending++]=(us_native_type_pair){&x->result,&y->result};
            for(i=0;i<(size_t)x->count;i++)work[pending++]=(us_native_type_pair){&x->argtypes[i],&y->argtypes[i]};
        }
    }
    equal=1;
done:free(work);free(seen);return equal;
}
static int us_callable_signature_equal(const us_export_signature *a,const us_export_signature *b){
    us_export_type x={0},y={0};x.kind=y.kind=4;x.depth=y.depth=1;x.width=y.width=8;x.alignment=y.alignment=8;x.tag=y.tag=4;
    x.signature=(us_export_signature*)a;y.signature=(us_export_signature*)b;return a&&b&&us_callable_type_identity(&x,&y);
}
typedef struct us_callable_clone {
    us_export_graph *graph;const us_export_signature *source[1024];size_t nodes;
} us_callable_clone;
static us_export_signature *us_callable_clone_signature(us_callable_clone *c,const us_export_signature *s){
    if(!s)return NULL;for(size_t i=0;i<c->graph->count;i++)if(c->source[i]==s)return c->graph->signatures[i];
    if(c->graph->count>=1024)return NULL;us_export_signature *d=calloc(1,sizeof *d);if(!d)return NULL;
    size_t i=c->graph->count++;c->source[i]=s;c->graph->signatures[i]=d;return d;
}
static int us_callable_clone_type(us_callable_clone *c,us_export_type *d,const us_export_type *s,unsigned depth){
    if(!s||depth>32||++c->nodes>16384||s->nmembers>128)return 1;
    *d=*s;d->members=NULL;d->element=NULL;d->elements=NULL;d->ffi=NULL;memset(&d->native,0,sizeof d->native);d->signature=NULL;
    if(s->signature){d->signature=us_callable_clone_signature(c,s->signature);if(!d->signature)return 1;}
    if(s->nmembers){if(!s->members)return 1;d->members=calloc((size_t)s->nmembers,sizeof *d->members);if(!d->members)return 1;
        for(size_t i=0;i<s->nmembers;i++){d->members[i]=s->members[i];d->members[i].type=calloc(1,sizeof *d->members[i].type);
            if(!d->members[i].type||us_callable_clone_type(c,d->members[i].type,s->members[i].type,depth+1))return 1;}}
    if(s->element){d->element=calloc(1,sizeof *d->element);if(!d->element||us_callable_clone_type(c,d->element,s->element,depth+1))return 1;}
    return 0;
}
static int us_callable_clone_graph(us_export_graph *g,const us_export_signature *root){
    us_callable_clone c={0};c.graph=g;g->signatures=calloc(1024,sizeof *g->signatures);if(!g->signatures||!us_callable_clone_signature(&c,root))return 1;
    for(size_t i=0;i<g->count;i++){
        const us_export_signature *s=c.source[i];us_export_signature *d=g->signatures[i];
        if(s->variadic>1||s->mode>1||s->count>1024||s->stored!=s->count||(s->count&&!s->argtypes)||(s->variadic&&(s->mode!=1||!s->count)))return 1;
        d->id=i+1;d->count=d->stored=s->count;d->variadic=s->variadic;d->mode=s->mode;d->supported=s->supported;
        if(us_callable_clone_type(&c,&d->result,&s->result,0))return 1;
        d->argtypes=calloc(s->count?(size_t)s->count:1,sizeof *d->argtypes);if(!d->argtypes)return 1;
        for(size_t j=0;j<s->count;j++)if(us_callable_clone_type(&c,d->argtypes+j,s->argtypes+j,0))return 1;
    }return 0;
}
/* Layout belongs to cloned descriptors, never the borrowed declaration. */
static ffi_type *us_callable_layout(us_export_type *t,int result,unsigned depth){
    if(depth>32)return NULL;if(t->ffi)return t->ffi;
    if(t->kind==4)return t->depth==1&&t->width==8&&t->alignment==8&&t->tag==4&&t->signature ? &ffi_type_pointer:NULL;
    if(t->kind!=5){
        ffi_type *f=us_export_ffitype(t,result);
        if(!f||t->tag||t->nmembers||t->element||t->signature||t->uns>1)return NULL;
        if(t->kind==0)return result&&t->width==0&&t->alignment==0?f:NULL;
        return f->size==t->width&&f->alignment==t->alignment?f:NULL;
    }
    if(t->depth||!t->width||t->width>16777216||!t->alignment)return NULL;
    size_t n=t->tag==1?(size_t)t->nmembers:t->tag==3?(size_t)t->count:0;
    if(!n||n>16384||(t->tag==1&&!t->members)||(t->tag==3&&!t->element))return NULL;
    t->elements=calloc(n+1,sizeof *t->elements);size_t *offsets=calloc(n,sizeof *offsets);
    if(!t->elements||!offsets){free(offsets);return NULL;}
    for(size_t i=0;i<n;i++){
        if(t->tag==1 && (t->members[i].bit_width||t->members[i].bit_offset)){free(offsets);return NULL;}
        us_export_type *child=t->tag==1?t->members[i].type:t->element;
        if(t->tag==3 && i)t->elements[i]=t->elements[0];else t->elements[i]=us_callable_layout(child,0,depth+1);
        if(!t->elements[i]){free(offsets);return NULL;}
    }
    t->native.type=FFI_TYPE_STRUCT;t->native.elements=t->elements;
    if(ffi_get_struct_offsets(FFI_DEFAULT_ABI,&t->native,offsets)!=FFI_OK){free(offsets);return NULL;}
    int bad=t->native.size!=t->width||t->native.alignment!=t->alignment;
    for(size_t i=0;i<n;i++)if(offsets[i]!=(t->tag==1?t->members[i].offset:i*t->stride))bad=1;
    if(t->tag==3 && (!t->stride || t->element->width!=t->stride || t->count>t->width/t->stride || t->count*t->stride!=t->width))bad=1;
    free(offsets);if(bad)return NULL;t->ffi=&t->native;return t->ffi;
}
static us_callable *us_callable_find(us_callables *r,uint64_t token){
    if(!r||!r->generation)return NULL;for(us_callable *p=r->head;p;p=p->next)if(p->token==token&&p->generation==r->generation)return p;return NULL;
}
static int us_callable_make(us_callables *,unsigned,const us_export_signature *,uintptr_t,uint64_t *,char *,size_t);
static int us_callable_pointer(us_callables *,uint64_t,const us_export_signature *,void **,char *,size_t);
static int us_callable_from_native(us_callables *,const us_export_signature *,void *,uint64_t *,char *,size_t);
static int us_callable_make_carrier(us_callables *,unsigned,const us_export_signature *,const us_export_signature *,uintptr_t,uint64_t *,char *,size_t);
static int us_callable_pointer_carrier(us_callables *,uint64_t,const us_export_signature *,const us_export_signature *,void **,char *,size_t);
static int us_callable_from_native_carrier(us_callables *,const us_export_signature *,const us_export_signature *,void *,uint64_t *,char *,size_t);
/* Only by-value members/array elements are traversed; signature edges describe
   pointer slots and are never traversed as object memory. Pointees stay opaque. */
static int us_callable_convert(us_callables *r,const us_export_type *t,void *dest,const void *source,int to_native,unsigned depth,char *error,size_t cap){
    if(!t||!dest||!source||depth>32)return us_callable_error(error,cap,"invalid callable value conversion");
    memcpy(dest,source,(size_t)t->width);
    if(t->kind==4){uint64_t h=0;void *p=NULL;
        if(to_native){memcpy(&h,source,8);if(us_callable_pointer(r,h,t->signature,&p,error,cap))return 1;memcpy(dest,&p,8);}
        else {memcpy(&p,source,8);if(us_callable_from_native(r,t->signature,p,&h,error,cap))return 1;memcpy(dest,&h,8);}
    }else if(t->kind==5){
        if(t->tag==1)for(size_t i=0;i<t->nmembers;i++){
            const us_export_member *m=t->members+i;if(!m->type||m->offset>t->width||m->type->width>t->width-m->offset)return 1;
            if(us_callable_convert(r,m->type,(char*)dest+m->offset,(const char*)source+m->offset,to_native,depth+1,error,cap))return 1;}
        else if(t->tag==3){for(size_t i=0;i<t->count;i++)if(us_callable_convert(r,t->element,(char*)dest+i*t->stride,(const char*)source+i*t->stride,to_native,depth+1,error,cap))return 1;}
        else return us_callable_error(error,cap,"unsupported callable aggregate");
    }return 0;
}
/* Carrier certificates are supplied by the model. This layer checks storage,
   owns both graphs, and never chooses an ABI class from union members. */
static int us_callable_opaque_safe(const us_export_type *t,unsigned depth,size_t *nodes){
    if(!t||depth>32||++*nodes>16384||t->kind==4||t->nmembers>128||t->width>16777216)return 0;
    if(t->nmembers&&!t->members)return 0;
    for(size_t i=0;i<t->nmembers;i++){
        const us_export_member *m=t->members+i;
        if(!m->type||m->offset>t->width||(m->entry_kind!=3&&m->type->width>t->width-m->offset)||!us_callable_opaque_safe(m->type,depth+1,nodes))return 0;
    }
    return !t->element||us_callable_opaque_safe(t->element,depth+1,nodes);
}
typedef struct us_callable_pair_check {us_native_signature_pair *pairs;size_t count,nodes;} us_callable_pair_check;
static int us_callable_pair_add(us_callable_pair_check *c,const us_export_signature *a,const us_export_signature *b){
    if(!a||!b)return 0;for(size_t i=0;i<c->count;i++)if(c->pairs[i].a==a&&c->pairs[i].b==b)return 1;
    if(c->count==16384)return 0;c->pairs[c->count++]=(us_native_signature_pair){a,b};return 1;
}
static int us_callable_pair_type(us_callable_pair_check *c,const us_export_type *a,const us_export_type *b,unsigned depth){
    if(!a||!b||depth>32||++c->nodes>16384||a->width!=b->width||a->alignment!=b->alignment||a->width>16777216)return 0;
    if(a->kind==4||b->kind==4)return a->kind==4&&b->kind==4&&a->depth==1&&b->depth==1&&a->width==8&&a->alignment==8&&a->tag==4&&b->tag==4&&
        !a->nmembers&&!b->nmembers&&!a->element&&!b->element&&us_callable_pair_add(c,a->signature,b->signature);
    if(a->kind!=5)return us_native_type_equal(a,b);
    if(a->depth||!a->width||!a->alignment||(a->alignment&(a->alignment-1)))return 0;
    if(a->tag==2 || (a->wire_version==3&&!us_export_type_has_callback(a))){size_t nodes=0;return us_callable_opaque_safe(a,0,&nodes)&&us_callable_opaque_safe(b,0,&nodes);}
    if(b->kind!=5||b->depth||a->tag!=b->tag)return 0;
    if(a->tag==1){
        if(!a->nmembers||a->nmembers>64||a->nmembers!=b->nmembers||!a->members||!b->members)return 0;
        for(size_t i=0;i<a->nmembers;i++){
            const us_export_member *x=a->members+i,*y=b->members+i;
            if(x->offset!=y->offset||x->bit_offset||y->bit_offset||x->bit_width||y->bit_width||x->storage!=y->storage||
               !x->type||x->offset>a->width||x->type->width>a->width-x->offset||!us_callable_pair_type(c,x->type,y->type,depth+1))return 0;
        }return 1;
    }
    return a->tag==3&&a->count&&a->count==b->count&&a->stride&&a->stride==b->stride&&a->count<=a->width/a->stride&&
        a->count*a->stride==a->width&&a->element&&a->element->width==a->stride&&us_callable_pair_type(c,a->element,b->element,depth+1);
}
static int us_callable_carrier_valid(const us_export_signature *original,const us_export_signature *carrier){
    us_callable_pair_check c={0};int valid=0;c.pairs=calloc(16384,sizeof *c.pairs);if(!c.pairs)return 0;
    if(!us_callable_pair_add(&c,original,carrier))goto done;
    for(size_t i=0;i<c.count;i++){
        const us_export_signature *a=c.pairs[i].a,*b=c.pairs[i].b;
        if(a->variadic||b->variadic||a->mode>1||a->mode!=b->mode||a->count>1024||(!a->mode&&a->count>6)||a->count!=b->count||
           a->stored!=a->count||b->stored!=b->count||(a->count&&(!a->argtypes||!b->argtypes))||us_callable_budget(a)||us_callable_budget(b)||
           !us_callable_pair_type(&c,&a->result,&b->result,0))goto done;
        for(size_t j=0;j<a->count;j++)if(!us_callable_pair_type(&c,a->argtypes+j,b->argtypes+j,0))goto done;
    }valid=1;
done:free(c.pairs);return valid;
}
static int us_callable_carrier_type_valid(const us_export_type *a,const us_export_type *b){
    if(!a||!b)return 0;us_export_signature x={0},y={0};x.mode=y.mode=1;x.result=*a;y.result=*b;return us_callable_carrier_valid(&x,&y);
}
/* Conversion plans contain only finite object-layout edges. Callback signature
   graph cycles live in the owned graph pair, never in this ownership tree. */
static us_callable_conversion *us_callable_conversion_make(const us_export_type *a,const us_export_type *b,unsigned depth){
    if(depth>32)return NULL;us_callable_conversion *p=calloc(1,sizeof *p);if(!p)return NULL;p->original=a;p->carrier=b;
    if(a->kind==4)p->action=2;
    else if(a->kind==5&&(a->tag==2||(a->wire_version==3&&!us_export_type_has_callback(a))))p->action=1;
    else if(a->kind==5){p->action=3;
        if(a->tag==1){p->members=calloc(a->nmembers,sizeof *p->members);if(!p->members)goto bad;
            for(size_t i=0;i<a->nmembers;i++)if(!(p->members[i]=us_callable_conversion_make(a->members[i].type,b->members[i].type,depth+1)))goto bad;
        }else if(!(p->element=us_callable_conversion_make(a->element,b->element,depth+1)))goto bad;
    }return p;
bad:us_callable_conversion_free(p);return NULL;
}
static int us_callable_paired_convert(us_callable *callable,const us_callable_conversion *p,void *dest,const void *source,int to_native,unsigned depth,char *error,size_t cap){
    if(!p||!dest||!source||depth>32||p->original->width>16777216)return us_callable_error(error,cap,"invalid paired callable conversion");
    const us_export_type *a=p->original,*b=p->carrier;memcpy(dest,source,(size_t)a->width);
    if(p->action==2){uint64_t h=0;void *raw=NULL;
        if(to_native){memcpy(&h,source,8);if(us_callable_pointer_carrier(callable->registry,h,a->signature,b->signature,&raw,error,cap))return 1;memcpy(dest,&raw,8);}
        else{memcpy(&raw,source,8);if(us_callable_from_native_carrier(callable->registry,a->signature,b->signature,raw,&h,error,cap))return 1;memcpy(dest,&h,8);}
    }else if(p->action==3){
        if(p->members){for(size_t i=0;i<a->nmembers;i++){size_t offset=(size_t)a->members[i].offset;
            if(us_callable_paired_convert(callable,p->members[i],(char*)dest+offset,(const char*)source+offset,to_native,depth+1,error,cap))return 1;}}
        else for(size_t i=0;i<a->count;i++)if(us_callable_paired_convert(callable,p->element,(char*)dest+i*a->stride,(const char*)source+i*a->stride,to_native,depth+1,error,cap))return 1;
    }return 0;
}
static int us_callable_value_convert(us_callable *p,const us_export_type *t,const us_callable_conversion *plan,
                                    void *dest,const void *source,int to_native,char *error,size_t cap){
    return plan?us_callable_paired_convert(p,plan,dest,source,to_native,0,error,cap):us_callable_convert(p->registry,t,dest,source,to_native,0,error,cap);
}
static void us_callable_callback(ffi_cif *cif,void *result,void **args,void *data){
    us_callable *p=data;us_callables *r=p->registry;const us_export_signature *s=p->graph.signatures[0];
    uint64_t *slots=NULL;void **owned=NULL;void *converted=NULL;char error[256]="callable callback failed";int rc=1,reported=0;
    size_t rn=s->result.kind?(size_t)s->result.width:0;
    if(result&&rn)memset(result,0,rn);
    if(p->generation!=r->generation||(!r->script_call&&!r->invoke_frame)||us_callable_budget(s))goto done;
    slots=calloc(s->count?(size_t)s->count:1,sizeof *slots);owned=calloc(s->count?(size_t)s->count:1,sizeof *owned);
    size_t scriptbytes=s->result.kind==5?rn:s->result.kind?8:0;
    converted=calloc(scriptbytes?scriptbytes:1,1);if(!slots||!owned||!converted)goto done;
    for(size_t i=0;i<s->count;i++){
        const us_export_type *t=s->argtypes+i;size_t n=t->kind==5?(size_t)t->width:8;
        owned[i]=calloc(n,1);if(!owned[i]||us_callable_value_convert(p,t,p->arg_plans?p->arg_plans[i]:NULL,owned[i],args[i],0,error,sizeof error))goto done;
        slots[i]=t->kind==5?(uintptr_t)owned[i]:us_export_slot(t,owned[i]);
        if(t->kind==4)memcpy(slots+i,owned[i],8);
    }
    us_export_frame frame={slots,s->count,s->mode,(unsigned)s->result.kind,scriptbytes,converted};
    int status=r->script_call?r->script_call(r->owner,(const void*)p->target,s,&frame):r->invoke_frame(r->owner,(const void*)p->target,&frame);
    if(status){reported=1;goto done;}
    if(rn){void *out=calloc(rn,1);if(!out)goto done;
        rc=us_callable_value_convert(p,&s->result,p->result_plan,out,converted,1,error,sizeof error);
        if(!rc)memcpy(result,out,rn);free(out);
    }else rc=0;
done:
    if(rc&&!reported&&r->failure)r->failure(r->owner,error);
    if(owned)for(size_t i=0;i<s->count;i++)free(owned[i]);free(owned);free(slots);free(converted);(void)cif;
}
static int us_callable_make_impl(us_callables *r,unsigned origin,const us_export_signature *s,const us_export_signature *carrier,uintptr_t raw,uint64_t *handle,char *error,size_t cap){
    if(!r||!handle||!r->owner||!r->generation||!s||(origin!=US_CALLABLE_SCRIPT&&origin!=US_CALLABLE_NATIVE))return us_callable_error(error,cap,"invalid declared callable introduction");
    *handle=0;
    if(!carrier){
        if(s->count>1024 || (s->count&&!s->argtypes))return us_callable_error(error,cap,"invalid callable argument graph");
        if(s->result.wire_version==3)return us_callable_error(error,cap,"V3 callable requires model carrier certificate");
        for(size_t i=0;i<s->count;i++)if(s->argtypes[i].wire_version==3)return us_callable_error(error,cap,"V3 callable requires model carrier certificate");
    }
    if(carrier&&!us_callable_carrier_valid(s,carrier))return us_callable_error(error,cap,"invalid callable carrier certificate");if(!raw)return 0;
    if((origin==US_CALLABLE_SCRIPT&&!r->script_call&&!r->invoke_frame)||(origin==US_CALLABLE_NATIVE&&!r->native_call))return us_callable_error(error,cap,"missing declared callable owner hook");
    for(us_callable *p=r->head;p;p=p->next)if(p->generation==r->generation&&p->origin==origin&&p->target==raw&&us_callable_signature_equal(p->graph.signatures[0],s)&&
       ((!carrier&&!p->carrier_graph.count)||(carrier&&p->carrier_graph.count&&us_callable_signature_equal(p->carrier_graph.signatures[0],carrier)))){*handle=p->token;return 0;}
    us_callable *p=calloc(1,sizeof *p);if(!p)return us_callable_error(error,cap,"callable allocation failed");
    p->registry=r;p->origin=origin;p->generation=r->generation;p->target=raw;
    if(us_callable_clone_graph(&p->graph,s)||(carrier&&us_callable_clone_graph(&p->carrier_graph,carrier)))goto bad;
    us_export_signature *x=carrier?p->carrier_graph.signatures[0]:p->graph.signatures[0];
    if(carrier){const us_export_signature *original=p->graph.signatures[0];
        p->opaque_args=calloc(x->count?(size_t)x->count:1,1);p->arg_plans=calloc(x->count?(size_t)x->count:1,sizeof *p->arg_plans);
        p->result_plan=us_callable_conversion_make(&original->result,&x->result,0);if(!p->opaque_args||!p->arg_plans||!p->result_plan)goto bad;
        p->opaque_result=p->result_plan->action==1;
        for(size_t i=0;i<x->count;i++){
            p->arg_plans[i]=us_callable_conversion_make(original->argtypes+i,x->argtypes+i,0);if(!p->arg_plans[i])goto bad;
            p->opaque_args[i]=p->arg_plans[i]->action==1;
        }
    }
    if(carrier)for(size_t k=0;k<p->carrier_graph.count;k++){
        us_export_signature *node=p->carrier_graph.signatures[k];ffi_type **types=calloc(node->count?(size_t)node->count:1,sizeof *types);ffi_cif check;
        if(!types)goto bad;ffi_type *result=us_callable_layout(&node->result,1,0);int invalid=!result;
        for(size_t i=0;i<node->count&&!invalid;i++)if(!(types[i]=us_callable_layout(node->argtypes+i,0,0)))invalid=1;
        if(!invalid&&ffi_prep_cif(&check,FFI_DEFAULT_ABI,(unsigned)node->count,result,types)!=FFI_OK)invalid=1;
        free(types);if(invalid)goto bad;
    }
    p->args=calloc(x->count?(size_t)x->count:1,sizeof *p->args);if(!p->args)goto bad;
    ffi_type *ret=us_callable_layout(&x->result,1,0);if(!ret)goto bad;
    for(size_t i=0;i<x->count;i++)if(!(p->args[i]=us_callable_layout(x->argtypes+i,0,0)))goto bad;
    if(!x->variadic&&ffi_prep_cif(&p->cif,FFI_DEFAULT_ABI,(unsigned)x->count,ret,p->args)!=FFI_OK)goto bad;
    uint64_t token=atomic_load_explicit(&us_callable_global_token,memory_order_relaxed);
    do {if(!token||token==UINT64_MAX)goto bad;}while(!atomic_compare_exchange_weak_explicit(&us_callable_global_token,&token,token+1,memory_order_relaxed,memory_order_relaxed));
    p->token=token;p->next=r->head;r->head=p;*handle=token;return 0;
bad:us_callable_free(p);return us_callable_error(error,cap,"unsupported or invalid callable signature");
}
/* Legacy callers never opt into carrier layout. */
static int us_callable_make(us_callables *r,unsigned origin,const us_export_signature *s,uintptr_t raw,uint64_t *h,char *e,size_t n){
    return us_callable_make_impl(r,origin,s,NULL,raw,h,e,n);
}
static int us_callable_make_carrier(us_callables *r,unsigned origin,const us_export_signature *original,const us_export_signature *carrier,
                                    uintptr_t raw,uint64_t *h,char *e,size_t n){
    if(!carrier)return us_callable_error(e,n,"missing callable carrier certificate");
    return us_callable_make_impl(r,origin,original,carrier,raw,h,e,n);
}
static int us_callable_pointer(us_callables *r,uint64_t h,const us_export_signature *s,void **out,char *error,size_t cap){
    if(!out||!s)return 1;*out=NULL;if(!h)return 0;us_callable *p=us_callable_find(r,h);
    if(!p||!us_callable_signature_equal(p->graph.signatures[0],s))return us_callable_error(error,cap,"stale foreign or incompatible callable handle");
    if(p->origin==US_CALLABLE_NATIVE){*out=(void*)p->target;return 0;}
    if(p->graph.signatures[0]->variadic)return us_callable_error(error,cap,"variadic script closure requires concrete specialization");
    if(!p->closure){p->closure=ffi_closure_alloc(sizeof *p->closure,&p->code);
        if(!p->closure||ffi_prep_closure_loc(p->closure,&p->cif,us_callable_callback,p,p->code)!=FFI_OK){if(p->closure)ffi_closure_free(p->closure);p->closure=NULL;p->code=NULL;return 1;}
        us_callable_lock();p->global_next=us_callable_global_closures;us_callable_global_closures=p;us_callable_unlock();}
    *out=p->code;return 0;
}
static int us_callable_from_native(us_callables *r,const us_export_signature *s,void *raw,uint64_t *out,char *error,size_t cap){
    if(!r||!s||!out)return 1;*out=0;if(!raw)return 0;
    us_callable_lock();for(us_callable *p=us_callable_global_closures;p;p=p->global_next)if(p->code==raw){
        int bad=p->registry!=r||p->generation!=r->generation||!us_callable_signature_equal(p->graph.signatures[0],s);
        if(!bad)*out=p->token;us_callable_unlock();return bad?us_callable_error(error,cap,"foreign or incompatible closure"):0;
    }us_callable_unlock();return us_callable_make(r,US_CALLABLE_NATIVE,s,(uintptr_t)raw,out,error,cap);
}
static int us_callable_carrier_identity(const us_callable *p,const us_export_signature *original,const us_export_signature *carrier){
    return us_callable_signature_equal(p->graph.signatures[0],original)&&
        us_callable_signature_equal(p->carrier_graph.count?p->carrier_graph.signatures[0]:p->graph.signatures[0],carrier);
}
static int us_callable_pointer_carrier(us_callables *r,uint64_t h,const us_export_signature *original,const us_export_signature *carrier,void **out,char *error,size_t cap){
    if(!out||!original||!carrier)return 1;*out=NULL;if(!h)return 0;us_callable *p=us_callable_find(r,h);
    if(!p||!us_callable_carrier_identity(p,original,carrier))return us_callable_error(error,cap,"incompatible callable carrier handle");
    return us_callable_pointer(r,h,original,out,error,cap);
}
static int us_callable_from_native_carrier(us_callables *r,const us_export_signature *original,const us_export_signature *carrier,void *raw,uint64_t *out,char *error,size_t cap){
    if(!r||!original||!carrier||!out)return 1;*out=0;if(!raw)return 0;
    us_callable_lock();for(us_callable *p=us_callable_global_closures;p;p=p->global_next)if(p->code==raw){
        int bad=p->registry!=r||p->generation!=r->generation||!us_callable_carrier_identity(p,original,carrier);
        if(!bad)*out=p->token;us_callable_unlock();return bad?us_callable_error(error,cap,"foreign or incompatible carrier closure"):0;
    }us_callable_unlock();return us_callable_make_carrier(r,US_CALLABLE_NATIVE,original,carrier,(uintptr_t)raw,out,error,cap);
}
static int us_callable_invoke(us_callables *r,us_callable *p,
                            const uint64_t *slots,void *result,uint64_t count,char *error,size_t cap){
    const us_export_signature *s=p->graph.signatures[0];size_t rn=s->result.kind?(size_t)s->result.width:0;
    if(count!=s->count||(count&&!slots)||(rn&&!result)||us_callable_budget(s))return us_callable_error(error,cap,"invalid callable frame");
    void *temp=calloc(rn>sizeof(ffi_arg)?rn:sizeof(ffi_arg),1);void **owned=NULL,**values=NULL;int rc=1;
    if(!temp)goto done;
    if(p->origin==US_CALLABLE_SCRIPT){if(!r->script_call&&!r->invoke_frame)goto done;us_export_frame frame={slots,count,s->mode,(unsigned)s->result.kind,s->result.kind==5?rn:s->result.kind?8:0,temp};rc=r->script_call?r->script_call(r->owner,(const void*)p->target,s,&frame):r->invoke_frame(r->owner,(const void*)p->target,&frame);}
    else {
        if(!r->native_call)goto done;owned=calloc(count?(size_t)count:1,sizeof *owned);values=calloc(count?(size_t)count:1,sizeof *values);if(!owned||!values)goto done;
        for(size_t i=0;i<count;i++){const us_export_type *t=s->argtypes+i;size_t n=t->kind==5?(size_t)t->width:8;owned[i]=calloc(n,1);values[i]=owned[i];
            const void *from=t->kind==5?(const void*)(uintptr_t)slots[i]:slots+i;
            if(!owned[i]||!from||us_callable_value_convert(p,t,p->arg_plans?p->arg_plans[i]:NULL,owned[i],from,1,error,cap))goto done;}
        if(r->native_call(r->owner,&p->cif,p->target,temp,values))goto done;
        void *script=calloc(rn?rn:1,1);if(!script)goto done;
        rc=rn?us_callable_value_convert(p,&s->result,p->result_plan,script,temp,0,error,cap):0;
        if(!rc&&rn)us_callable_commit_slot(&s->result,result,script);free(script);goto done;
    }
    if(!rc&&rn)us_callable_commit_slot(&s->result,result,temp);
done:if(owned)for(size_t i=0;i<count;i++)free(owned[i]);free(owned);free(values);free(temp);
    if(rc&&error&&cap&&!error[0])us_callable_error(error,cap,"declared callable invocation failed");return rc;
}
/* A prototype declares only the fixed prefix. A concrete signature declares
   the promoted tail and is never used as a closure's universal signature. */
static int us_callable_concrete_valid(const us_export_signature *proto,const us_export_signature *concrete){
    if(!proto||!concrete||proto->variadic!=1||proto->mode!=1||!proto->count||proto->count>1024||proto->stored!=proto->count||!proto->argtypes||
       concrete->variadic||concrete->mode!=1||concrete->count<proto->count||concrete->count>1024||concrete->stored!=concrete->count||!concrete->argtypes)return 0;
    if(!us_native_type_equal(&proto->result,&concrete->result))return 0;
    for(size_t i=0;i<proto->count;i++)if(!us_native_type_equal(proto->argtypes+i,concrete->argtypes+i))return 0;
    for(size_t i=proto->count;i<concrete->count;i++){
        const us_export_type *t=concrete->argtypes+i;
        if((t->kind==3&&t->width==4)||(t->kind==1&&t->width<4))return 0;
    }return 1;
}
static int us_callable_call(us_callables *r,uint64_t handle,const us_export_signature *expected,
                            const uint64_t *slots,void *result,uint64_t count,char *error,size_t cap){
    us_callable *p=us_callable_find(r,handle);
    if(!p||!expected||!us_callable_signature_equal(p->graph.signatures[0],expected))return us_callable_error(error,cap,"stale foreign or incompatible callable call");
    if(p->graph.signatures[0]->variadic)return us_callable_error(error,cap,"variadic callable requires concrete signature");
    return us_callable_invoke(r,p,slots,result,count,error,cap);
}
static int us_callable_call_concrete(us_callables *r,uint64_t handle,const us_export_signature *expected_proto,
        const us_export_signature *concrete,const uint64_t *slots,void *result,uint64_t count,char *error,size_t cap){
    us_callable *source=us_callable_find(r,handle);
    if(!source||!expected_proto||!us_callable_signature_equal(source->graph.signatures[0],expected_proto))
        return us_callable_error(error,cap,"stale foreign or incompatible variadic callable");
    const us_export_signature *proto=source->graph.signatures[0];
    if(!us_callable_concrete_valid(proto,concrete)||count!=concrete->count)
        return us_callable_error(error,cap,"invalid concrete variadic callable signature");
    us_callable *p=calloc(1,sizeof *p);if(!p)return us_callable_error(error,cap,"callable allocation failed");
    p->registry=r;p->origin=source->origin;p->generation=r->generation;p->target=source->target;int rc=1;
    if(us_callable_clone_graph(&p->graph,concrete))goto done;
    us_export_signature *s=p->graph.signatures[0];p->args=calloc((size_t)s->count,sizeof *p->args);if(!p->args)goto done;
    ffi_type *ret=us_callable_layout(&s->result,1,0);if(!ret)goto done;
    for(size_t i=0;i<s->count;i++)if(!(p->args[i]=us_callable_layout(s->argtypes+i,0,0)))goto done;
    if(p->origin==US_CALLABLE_NATIVE&&ffi_prep_cif_var(&p->cif,FFI_DEFAULT_ABI,(unsigned)proto->count,(unsigned)s->count,ret,p->args)!=FFI_OK)goto done;
    rc=us_callable_invoke(r,p,slots,result,count,error,cap);
done:us_callable_free(p);if(rc&&error&&cap&&!error[0])us_callable_error(error,cap,"concrete variadic invocation failed");return rc;
}
#endif

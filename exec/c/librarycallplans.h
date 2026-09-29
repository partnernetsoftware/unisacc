#ifndef UNISACC_LIBRARYCALLPLANS_H
#define UNISACC_LIBRARYCALLPLANS_H
/* Owned model declarations; no source parsing, promotion or candidate selection. */
#include "librarynative.h"
typedef struct us_native_template {
    struct us_native_template *next; uintptr_t target; us_exports graph;
} us_native_template;
typedef struct us_native_templates { us_native_template *head; } us_native_templates;
typedef struct us_native_callsite {
    struct us_native_callsite *next; uint64_t site_id,template_handle,plan_handle;
    /* V3 sites keep their exact concrete wire until the model runtime is idle;
       nativeabi then certifies the fixed concrete graph and the plan is built. */
    unsigned char *pending; size_t pending_length; uint64_t fixed;
} us_native_callsite;
typedef struct us_native_callsites { us_native_callsite *head; us_native_plans plans; } us_native_callsites;
static void us_native_templates_clear(us_native_templates *s){
    if(!s)return;while(s->head){us_native_template *n=s->head->next;us_exports_clear(&s->head->graph);free(s->head);s->head=n;}
}
static us_native_template *us_native_template_find(const us_native_templates *s,uint64_t h){
    if(s)for(us_native_template *p=s->head;p;p=p->next)if((uint64_t)(uintptr_t)p==h)return p;return NULL;
}
static int us_native_template_add(us_native_templates *s,uintptr_t target,const void *sig,size_t len,uint64_t *handle,char *error,size_t cap){
    if(!s||!handle||!target||!sig)return us_export_error(error,cap,"invalid variadic template output or target");
    *handle=0;us_native_template *p=calloc(1,sizeof *p);if(!p)return us_export_error(error,cap,"variadic template allocation failed");
    if(us_exports_load(&p->graph,sig,len,error,cap)||p->graph.count!=1)goto bad;
    us_export *x=p->graph.items;
    if((x->version!=2&&x->version!=3)||x->linkage||x->defined!=1||x->variadic!=1||x->mode!=1||!x->count||x->count>1024||x->stored!=x->count)goto bad;
    /* V3 prototypes carry no ffi support bit: every concrete site is certified by
       nativeabi after E3 (see us_native_callsites_prepare in libunisacc.c). */
    if(x->version==2){
        if(!x->result.ffi)goto unsupported;
        for(size_t i=0;i<(size_t)x->count;i++)if(!us_export_arg(x,i)->ffi)goto unsupported;
    }
    p->target=target;p->next=s->head;s->head=p;*handle=(uintptr_t)p;return 0;
unsupported:us_exports_clear(&p->graph);free(p);return 0;
bad:us_exports_clear(&p->graph);free(p);return us_export_error(error,cap,"invalid variadic template graph");
}
/* Iterative comparison is bounded by the wire type/signature limits. Signature
   pairs are visited coinductively; sharing and parser IDs are not ABI identity. */
typedef struct us_native_type_pair { const us_export_type *a,*b; } us_native_type_pair;
typedef struct us_native_signature_pair { const us_export_signature *a,*b; } us_native_signature_pair;
static int us_native_type_equal(const us_export_type *a,const us_export_type *b){
    us_native_type_pair *work=calloc(16384,sizeof *work);
    us_native_signature_pair *seen=calloc(16384,sizeof *seen);
    size_t pending=0,visited=0,steps=0;int equal=0;
    if(!work||!seen)goto done;
    work[pending++]=(us_native_type_pair){a,b};
    while(pending){
        us_native_type_pair pair=work[--pending];a=pair.a;b=pair.b;
        if(!a||!b||++steps>16384)goto done;
        if(a->depth!=b->depth||a->kind!=b->kind||a->width!=b->width||a->uns!=b->uns||a->alignment!=b->alignment||a->tag!=b->tag||a->nmembers!=b->nmembers||a->count!=b->count||a->stride!=b->stride)goto done;
        if((a->element==NULL)!=(b->element==NULL)||(a->signature==NULL)!=(b->signature==NULL)||(a->pointee==NULL)!=(b->pointee==NULL))goto done;
        if(a->pointee){if(pending>=16384)goto done;work[pending++]=(us_native_type_pair){a->pointee,b->pointee};}
        if(a->nmembers>64 || (a->nmembers && (!a->members||!b->members)))goto done;
        if(a->element){if(pending>=16384)goto done;work[pending++]=(us_native_type_pair){a->element,b->element};}
        for(size_t i=0;i<(size_t)a->nmembers;i++){
            const us_export_member *x=a->members+i,*y=b->members+i;
            if(x->offset!=y->offset||x->bit_offset!=y->bit_offset||x->bit_width!=y->bit_width||x->storage!=y->storage||pending>=16384)goto done;
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
static void us_native_callsites_clear(us_native_callsites *s){
    if(!s)return;while(s->head){us_native_callsite *n=s->head->next;free(s->head->pending);free(s->head);s->head=n;}us_native_plans_clear(&s->plans);
}
static us_native_plan *us_native_callsite_find(const us_native_callsites *s,uint64_t template_handle,uint64_t site_id){
    if(s)for(us_native_callsite *p=s->head;p;p=p->next)if(p->template_handle==template_handle&&p->site_id==site_id)return us_native_plan_find(&s->plans,p->plan_handle);return NULL;
}
static int us_native_callsites_load(us_native_callsites *out,const us_native_templates *templates,const void *input,size_t len,char *error,size_t cap){
    const unsigned char *b=input;size_t at=8;uint64_t count;us_native_callsites tmp={0};us_exports graph={0};
    if(!out||!b||len<16||memcmp(b,"USCPLAN1",8)||us_export_u64(b,len,&at,&count)||count>8192)goto bad;
    for(uint64_t i=0;i<count;i++){
        uint64_t payload,site,th,fixed,sl;size_t end;
        if(us_export_u64(b,len,&at,&payload)||payload>len-at||payload<32)goto bad;end=at+(size_t)payload;
        if(us_export_u64(b,end,&at,&site)||us_export_u64(b,end,&at,&th)||us_export_u64(b,end,&at,&fixed)||us_export_u64(b,end,&at,&sl)||!site||!th||sl!=end-at)goto bad;
        for(us_native_callsite *p=tmp.head;p;p=p->next)if(p->site_id==site)goto bad;
        us_native_template *t=us_native_template_find(templates,th);if(!t||fixed!=t->graph.items[0].count)goto bad;
        if(us_exports_load(&graph,b+at,(size_t)sl,error,cap)||graph.count!=1)goto bad;
        us_export *x=graph.items,*proto=t->graph.items;
        if(x->version!=proto->version||x->mode!=1||x->variadic||(x->version==2&&!us_export_supported(x))||x->count<fixed||strcmp(x->name,proto->name)||!us_native_type_equal(&x->result,&proto->result))goto bad;
        for(size_t j=0;j<(size_t)fixed;j++)if(!us_native_type_equal(us_export_arg(x,j),us_export_arg(proto,j)))goto bad;
        unsigned site_version=x->version;
        us_exports_clear(&graph);
        us_native_callsite *p=calloc(1,sizeof *p);if(!p)goto bad;
        p->site_id=site;p->template_handle=th;p->fixed=fixed;p->next=tmp.head;tmp.head=p;
        if(site_version==2){
            uint64_t ph=0;if(us_native_plan_add_variadic(&tmp.plans,t->target,b+at,(size_t)sl,fixed,&ph,error,cap)||!ph)goto bad;
            p->plan_handle=ph;
        }else{
            p->pending=malloc((size_t)sl?(size_t)sl:1);if(!p->pending)goto bad;
            memcpy(p->pending,b+at,(size_t)sl);p->pending_length=(size_t)sl;
        }
        at=end;
    }
    if(at!=len)goto bad;us_native_callsites_clear(out);*out=tmp;return 0;
bad:us_exports_clear(&graph);us_native_callsites_clear(&tmp);return us_export_error(error,cap,"invalid variadic callsite declarations");
}
#endif

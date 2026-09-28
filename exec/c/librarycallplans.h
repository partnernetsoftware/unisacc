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
    if(x->version!=2||x->linkage||x->defined!=1||x->variadic!=1||x->mode!=1||!x->count||x->count>1024||x->stored!=x->count)goto bad;
    if(!x->result.ffi)goto unsupported;
    for(size_t i=0;i<(size_t)x->count;i++)if(!us_export_arg(x,i)->ffi)goto unsupported;
    p->target=target;p->next=s->head;s->head=p;*handle=(uintptr_t)p;return 0;
unsupported:us_exports_clear(&p->graph);free(p);return 0;
bad:us_exports_clear(&p->graph);free(p);return us_export_error(error,cap,"invalid variadic template graph");
}
static int us_native_type_equal(const us_export_type *a,const us_export_type *b){
    if(a->depth!=b->depth||a->kind!=b->kind||a->width!=b->width||a->uns!=b->uns||a->alignment!=b->alignment||a->tag!=b->tag||a->nmembers!=b->nmembers||a->count!=b->count||a->stride!=b->stride)return 0;
    if((a->element==NULL)!=(b->element==NULL))return 0;
    if(a->element&&!us_native_type_equal(a->element,b->element))return 0;
    for(size_t i=0;i<(size_t)a->nmembers;i++){
        const us_export_member *x=a->members+i,*y=b->members+i;
        if(x->offset!=y->offset||x->bit_offset!=y->bit_offset||x->bit_width!=y->bit_width||x->storage!=y->storage||!us_native_type_equal(x->type,y->type))return 0;
    }return 1;
}
static void us_native_callsites_clear(us_native_callsites *s){
    if(!s)return;while(s->head){us_native_callsite *n=s->head->next;free(s->head);s->head=n;}us_native_plans_clear(&s->plans);
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
        if(x->version!=2||x->mode!=1||x->variadic||!us_export_supported(x)||x->count<fixed||strcmp(x->name,proto->name)||!us_native_type_equal(&x->result,&proto->result))goto bad;
        for(size_t j=0;j<(size_t)fixed;j++)if(!us_native_type_equal(us_export_arg(x,j),us_export_arg(proto,j)))goto bad;
        us_exports_clear(&graph);
        uint64_t ph=0;if(us_native_plan_add_variadic(&tmp.plans,t->target,b+at,(size_t)sl,fixed,&ph,error,cap)||!ph)goto bad;
        us_native_callsite *p=calloc(1,sizeof *p);if(!p)goto bad;
        p->site_id=site;p->template_handle=th;p->plan_handle=ph;p->next=tmp.head;tmp.head=p;at=end;
    }
    if(at!=len)goto bad;us_native_callsites_clear(out);*out=tmp;return 0;
bad:us_exports_clear(&graph);us_native_callsites_clear(&tmp);return us_export_error(error,cap,"invalid variadic callsite declarations");
}
#endif

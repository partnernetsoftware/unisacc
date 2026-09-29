#ifndef UNISACC_LIBRARYEXPORTS_H
#define UNISACC_LIBRARYEXPORTS_H
/* Host ABI adaptation of model-produced USLSIG1/USLSIG2 declarations;
   USLSIG3 is owned diagnostic facts only, never runtime certification. No C parser.
   Owner must quiesce all callbacks before clear/recompile/free; returned code
   pointers then become invalid. Context invocation/error/TLS/stack/init rules
   belong to the injected host invoke callback, never to this byte decoder. */
#ifdef __APPLE__
#include <ffi/ffi.h>
#else
#include <ffi.h>
#endif
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef struct us_export_type us_export_type;
typedef struct us_export_signature us_export_signature;
typedef struct us_export_graph {
    us_export_signature **signatures; size_t count;
} us_export_graph;
typedef struct us_export_member { uint64_t offset,bit_offset,bit_width,storage; us_export_type *type;
    uint64_t ordinal,effective_alignment; unsigned entry_kind;
} us_export_member;
struct us_export_type {
    uint64_t depth,base,shape,kind,width,uns,alignment,tag,nmembers,count,stride;
    us_export_member *members; us_export_type *element;
    us_export_signature *signature; /* borrowed edge; graph owns each node once */
    us_export_type *pointee; /* V3 tag 5: one shallow pointee descriptor; NULL when opaque */
    ffi_type native; ffi_type **elements; ffi_type *ffi;
    unsigned wire_version; uint64_t natural_alignment; unsigned fp_rank,fp_format,layout_flags,layout_known_mask,layout_origin;
};
struct us_export_signature {
    uint64_t id,count,stored; unsigned variadic,mode,supported;
    us_export_type result,*argtypes;
};
typedef struct us_export_frame {
    const uint64_t *slots; uint64_t count; unsigned mode,result_kind;
    size_t result_bytes; void *result;
} us_export_frame;
typedef int (*us_export_invoke_frame)(void *owner,const void *raw,const us_export_frame *frame);
typedef int (*us_export_invoke)(void *owner,const void *raw,const uint64_t slots[6],uint64_t *result);
typedef int (*us_export_lookup)(void *owner,const char *name,const void **raw,int *kind);
typedef struct us_export {
    char *name; unsigned linkage,defined,variadic,supported;
    unsigned version,mode; uint64_t count,stored; us_export_type result,args[8];
    us_export_type *argtypes; ffi_type **dynamic_ffiargs; us_export_invoke_frame invoke_frame;
    ffi_cif cif; ffi_type *ffiargs[6]; ffi_closure *closure; void *code;
    const void *raw; void *owner; us_export_invoke invoke;
    int last_status; us_export_graph graph;
    unsigned char *wire; size_t wire_length; /* owned single-record declaration */
} us_export;
typedef struct us_exports { us_export *items; size_t count; } us_exports;
static int us_export_error(char *error,size_t cap,const char *text) {
    if (error && cap) snprintf(error,cap,"%s",text);return 1;
}
static void us_export_type_clear(us_export_type *t) {
    if(!t)return;
    for(size_t i=0;t->members && i<(size_t)t->nmembers;i++){us_export_type_clear(t->members[i].type);free(t->members[i].type);}
    free(t->members);if(t->element){us_export_type_clear(t->element);free(t->element);}
    if(t->pointee){us_export_type_clear(t->pointee);free(t->pointee);}
    free(t->elements);memset(t,0,sizeof *t);
}
static void us_export_graph_clear(us_export_graph *g) {
    if(!g)return;
    for(size_t i=0;i<g->count;i++){
        us_export_signature *s=g->signatures[i];
        us_export_type_clear(&s->result);
        for(size_t j=0;s->argtypes && j<(size_t)s->stored;j++)us_export_type_clear(&s->argtypes[j]);
        free(s->argtypes);free(s);
    }
    free(g->signatures);memset(g,0,sizeof *g);
}
static const us_export_type *us_export_arg(const us_export *x,size_t i) { return x->version>=2 ? &x->argtypes[i] : &x->args[i]; }
static void us_exports_clear(us_exports *set) {
    if (!set) return;
    for(size_t i=0;i<set->count;i++) {
        if(set->items[i].closure) ffi_closure_free(set->items[i].closure);
        free(set->items[i].name);
        free(set->items[i].wire);
        us_export_type_clear(&set->items[i].result);
        if(set->items[i].argtypes){for(size_t j=0;j<(size_t)set->items[i].stored;j++)us_export_type_clear(&set->items[i].argtypes[j]);}
        free(set->items[i].argtypes);free(set->items[i].dynamic_ffiargs);
        us_export_graph_clear(&set->items[i].graph);
    }
    free(set->items);memset(set,0,sizeof *set);
}
static int us_export_u64(const unsigned char *bytes,size_t length,size_t *at,uint64_t *v) {
    if(*at>length || length-*at<8)return 1;
    *v=0;for(unsigned i=0;i<8;i++)*v|=(uint64_t)bytes[*at+i]<<(8*i);*at+=8;return 0;
}
static int us_export_descriptor(const unsigned char *bytes,size_t length,size_t *at,us_export_type *t) {
    return us_export_u64(bytes,length,at,&t->depth)||us_export_u64(bytes,length,at,&t->base)||
        us_export_u64(bytes,length,at,&t->shape)||us_export_u64(bytes,length,at,&t->kind)||
        us_export_u64(bytes,length,at,&t->width)||us_export_u64(bytes,length,at,&t->uns)||
        t->kind>6 || t->uns>1 || (t->width!=0 && t->width!=1 && t->width!=2 && t->width!=4 && t->width!=8);
}
static ffi_type *us_export_ffitype(const us_export_type *t,int result) {
    if(t->kind==0)return result && t->width==0 && t->depth==0 ? &ffi_type_void : NULL;
    if(t->kind==2)return t->width==sizeof(void*) && t->depth>0 ? &ffi_type_pointer : NULL;
    if(t->ffi)return t->ffi;
    if(t->kind==3 && !t->depth)return t->width==4 ? &ffi_type_float : t->width==8 ? &ffi_type_double : NULL;
    if(t->kind!=1 || t->depth)return NULL;
    switch(t->width) {
    case 1:return t->uns ? &ffi_type_uint8 : &ffi_type_sint8;
    case 2:return t->uns ? &ffi_type_uint16 : &ffi_type_sint16;
    case 4:return t->uns ? &ffi_type_uint32 : &ffi_type_sint32;
    case 8:return t->uns ? &ffi_type_uint64 : &ffi_type_sint64;
    default:return NULL;
    }
}
static int us_export_supported(const us_export *x) {
    if(x->version==3)return 0; /* No V3 certification seam yet. */
    if(x->linkage || x->defined!=1 || !x->supported || x->variadic || x->count>(x->version==2 ? 1024 : 6) || x->stored!=x->count || !us_export_ffitype(&x->result,1))return 0;
    if(x->version==2 && !x->result.ffi)return 0;
    if(x->version!=2){
        if(x->result.kind!=0 && x->result.kind!=1 && x->result.kind!=2)return 0;
        for(size_t i=0;i<(size_t)x->count;i++)if(x->args[i].kind!=1 && x->args[i].kind!=2)return 0;
    }
    for(size_t i=0;i<(size_t)x->count;i++)if(x->version==2 ? !x->argtypes[i].ffi : !us_export_ffitype(us_export_arg(x,i),0))return 0;
    return 1;
}
/* Recursive payloads are bounded independently; no child can consume a sibling. */
static int us_export_descriptor_version(const unsigned char *b,size_t len,size_t *at,
        us_export_type *t,unsigned depth,size_t *nodes,us_export_graph *graph,unsigned version) {
    uint64_t payload; size_t end;
    t->wire_version=version;
    if(depth>32 || ++*nodes>16384 || us_export_u64(b,len,at,&t->depth) ||
       us_export_u64(b,len,at,&t->base)||us_export_u64(b,len,at,&t->shape)||
       us_export_u64(b,len,at,&t->kind)||us_export_u64(b,len,at,&t->width)||
       us_export_u64(b,len,at,&t->uns)||us_export_u64(b,len,at,&t->alignment)||*at>=len)return 1;
    if(version==3){
    if(len-*at<13)return 1;
    t->fp_rank=b[(*at)++];t->fp_format=b[(*at)++];
    if(us_export_u64(b,len,at,&t->natural_alignment))return 1;
    t->layout_flags=b[(*at)++];t->layout_known_mask=b[(*at)++];t->layout_origin=b[(*at)++];
    if(t->fp_rank>3 || t->fp_format>4 || t->layout_flags>3 || t->layout_known_mask>3 ||
       (t->layout_flags & ~t->layout_known_mask) || t->layout_origin>2 ||
       (t->natural_alignment && (t->natural_alignment>65536 || (t->natural_alignment&(t->natural_alignment-1)))) ||
       (t->layout_origin==1 && (t->layout_known_mask!=3 || (t->width && !t->natural_alignment))))return 1;
    if(t->kind!=3){if(t->fp_rank || t->fp_format)return 1;}
    else if((t->fp_format==0 && (t->fp_rank || t->layout_origin!=0)) ||
            (t->fp_format==1 && t->width!=4) || (t->fp_format==2 && t->width!=8) ||
            (t->fp_format>=3 && t->width!=16) ||
            (t->fp_rank==1 && t->fp_format!=1) || (t->fp_rank==2 && t->fp_format!=2) ||
            (t->fp_rank==3 && t->fp_format<2))return 1;
    }
    if(*at>=len)return 1;
    t->tag=b[(*at)++];
    if(t->kind>6 || t->uns>1 || t->width>16777216 || t->tag>4 ||
       us_export_u64(b,len,at,&payload)||payload>16777216||payload>len-*at)return 1;
    end=*at+(size_t)payload;
    if(t->kind==0 || t->kind==6){if(t->width || t->alignment)return 1;}
    else if(!t->alignment || t->alignment>65536 || (t->alignment&(t->alignment-1)))return 1;
    if(t->tag==0){
        if(payload || (t->kind==1 && (t->depth || (t->width!=1&&t->width!=2&&t->width!=4&&t->width!=8))) ||
           (t->kind==2 && (t->width!=8||!t->depth)) ||
           (t->kind==3 && (t->depth||(t->width!=4&&t->width!=8&&(version!=3||t->width!=16)))) || t->kind==5)return 1;
    }else if(t->tag==1 || t->tag==2){
        if(t->kind!=5 || t->depth || !t->width || us_export_u64(b,end,at,&t->nmembers)||t->nmembers>(version==3 ? 128:64))return 1;
        t->members=calloc(t->nmembers ? (size_t)t->nmembers:1,sizeof *t->members);if(!t->members)return 1;
        for(size_t i=0;i<(size_t)t->nmembers;i++){
            us_export_member *m=&t->members[i];m->type=calloc(1,sizeof *m->type);if(!m->type)return 1;
            if(version==3){
            if(us_export_u64(b,end,at,&m->ordinal)||m->ordinal!=i||*at>=end)return 1;
            m->entry_kind=b[(*at)++];
            if(m->entry_kind>4 || us_export_u64(b,end,at,&m->effective_alignment) ||
               !m->effective_alignment || m->effective_alignment>65536 ||
               (m->effective_alignment&(m->effective_alignment-1)) ||
               us_export_u64(b,end,at,&m->offset)||us_export_u64(b,end,at,&m->bit_offset)||
               us_export_u64(b,end,at,&m->bit_width)||us_export_u64(b,end,at,&m->storage)||
               us_export_descriptor_version(b,end,at,m->type,depth+1,nodes,graph,version)||m->offset>t->width ||
               m->storage!=m->type->width)return 1;
            if(m->entry_kind==0 || m->entry_kind==4){
                if(m->bit_width || m->bit_offset || m->storage>t->width-m->offset ||
                   (m->entry_kind==4 && m->type->tag!=1 && m->type->tag!=2))return 1;
            }else{
                if(m->type->kind!=1 || m->type->depth || m->type->tag)return 1;
                if(m->entry_kind==3){if(m->bit_width || m->bit_offset)return 1;}
                else if(!m->bit_width || m->storage>t->width-m->offset ||
                        m->bit_width>m->storage*8 || m->bit_offset>m->storage*8-m->bit_width)return 1;
            }
            }else{
            if(us_export_u64(b,end,at,&m->offset)||us_export_u64(b,end,at,&m->bit_offset)||
               us_export_u64(b,end,at,&m->bit_width)||us_export_u64(b,end,at,&m->storage)||
               us_export_descriptor_version(b,end,at,m->type,depth+1,nodes,graph,version)||m->offset>t->width ||
               m->type->width>t->width-m->offset || m->storage>t->width-m->offset ||
               m->bit_width>m->storage*8 || m->bit_offset>m->storage*8-m->bit_width)return 1;
            }
        }
    }else if(t->tag==3){
        if(t->kind!=5 || t->depth || us_export_u64(b,end,at,&t->count)||us_export_u64(b,end,at,&t->stride)||
           !t->count || t->count>16384 || !t->stride || t->stride>16777216 || t->count>16777216/t->stride ||
           t->width!=t->count*t->stride)return 1;
        t->element=calloc(1,sizeof *t->element);if(!t->element || us_export_descriptor_version(b,end,at,t->element,depth+1,nodes,graph,version)||t->element->width!=t->stride)return 1;
    }else if(t->tag==5){
        /* V3 data pointer with one shallow pointee: scalar/void/unknown fully, an
           aggregate as tag+extent with no members, a further pointer level opaque. */
        if(version!=3 || t->kind!=2 || t->width!=8 || !t->depth)return 1;
        if(payload){
            t->pointee=calloc(1,sizeof *t->pointee);
            if(!t->pointee || us_export_descriptor_version(b,end,at,t->pointee,depth+1,nodes,graph,version))return 1;
            const us_export_type *q=t->pointee;
            if(t->depth==1 ? q->depth!=0 : (q->kind!=2||q->depth!=t->depth-1||q->pointee))return 1;
            if(q->nmembers || q->element || q->signature)return 1;
        }
    }else{
        if(t->kind!=4 || t->width!=8 || !t->depth)return 1;
        if(payload){
            uint64_t id;unsigned form;
            if(t->depth!=1 || t->alignment!=8 || end-*at<2 || b[(*at)++]!=1)return 1;
            form=b[(*at)++];
            if(form>1 || us_export_u64(b,end,at,&id)||!id||id>1024)return 1;
            if(form==1){
                if(id>graph->count)return 1;
                t->signature=graph->signatures[id-1];
            }else{
                if(id!=graph->count+1 || end-*at<2)return 1;
                if(!graph->signatures){graph->signatures=calloc(1024,sizeof *graph->signatures);if(!graph->signatures)return 1;}
                us_export_signature *sig=calloc(1,sizeof *sig);if(!sig)return 1;
                sig->id=id;graph->signatures[graph->count++]=sig;t->signature=sig;
                /* Register before children: only already introduced/self edges are legal. */
                sig->variadic=b[(*at)++];sig->mode=b[(*at)++];
                if(sig->variadic>1 || sig->mode>1 || us_export_u64(b,end,at,&sig->count)||sig->count>1024 ||
                   (!sig->mode && (sig->count>6 || sig->variadic)) ||
                   us_export_descriptor_version(b,end,at,&sig->result,depth+1,nodes,graph,version) ||
                   us_export_u64(b,end,at,&sig->stored)||sig->stored!=sig->count)return 1;
                sig->argtypes=calloc(sig->stored ? (size_t)sig->stored:1,sizeof *sig->argtypes);if(!sig->argtypes)return 1;
                for(size_t i=0;i<(size_t)sig->stored;i++)
                    if(us_export_descriptor_version(b,end,at,&sig->argtypes[i],depth+1,nodes,graph,version))return 1;
                if(*at>=end || (sig->supported=b[(*at)++])>1 || (version==3 && sig->supported))return 1;
            }
        }
    }
    return *at!=end;
}
/* Keep the V2 helper API and byte acceptance unchanged. V3 never certifies ABI. */
static int us_export_descriptor2(const unsigned char *b,size_t n,size_t *at,us_export_type *t,unsigned depth,size_t *nodes,us_export_graph *g){
    return us_export_descriptor_version(b,n,at,t,depth,nodes,g,2);
}
static int us_export_descriptor3(const unsigned char *b,size_t n,size_t *at,us_export_type *t,unsigned depth,size_t *nodes,us_export_graph *g){
    return us_export_descriptor_version(b,n,at,t,depth,nodes,g,3);
}
/* Signature edges are not followed: a callable slot itself requires conversion.
   By-value child ownership is a depth-bounded tree, even in cyclic signatures. */
static int us_export_type_has_callback(const us_export_type *t) {
    if(t->kind==4)return 1;
    if(t->element && us_export_type_has_callback(t->element))return 1;
    for(size_t i=0;i<(size_t)t->nmembers;i++)if(us_export_type_has_callback(t->members[i].type))return 1;
    return 0;
}
static int us_export_graph_support_valid(const us_export_graph *g) {
    for(size_t i=0;i<g->count;i++){
        const us_export_signature *s=g->signatures[i];if(!s->supported)continue;
        if(us_export_type_has_callback(&s->result))return 0;
        for(size_t j=0;j<(size_t)s->count;j++)if(us_export_type_has_callback(&s->argtypes[j]))return 0;
    }
    return 1;
}
/* Build only representable libffi layouts. Unsupported declarations stay visible. */
static ffi_type *us_export_native(us_export_type *t,int result) {
    if(t->tag==0){
        ffi_type *f=us_export_ffitype(t,result);
        if(f && t->kind!=0 && (f->size!=t->width || f->alignment!=t->alignment))return NULL;
        return f;
    }
    if(t->tag!=1 && t->tag!=3)return NULL;
    size_t n=t->tag==1 ? (size_t)t->nmembers : (size_t)t->count;
    if(!n)return NULL;
    t->elements=calloc(n+1,sizeof *t->elements);if(!t->elements)return NULL;
    for(size_t i=0;i<n;i++){
        us_export_type *child=t->tag==1 ? t->members[i].type : t->element;
        if(t->tag==1 && (t->members[i].bit_width || t->members[i].bit_offset))return NULL;
        ffi_type *f=child->ffi ? child->ffi : us_export_native(child,0);if(!f || f==&ffi_type_void)return NULL;
        child->ffi=f;t->elements[i]=f;
    }
    t->native.type=FFI_TYPE_STRUCT;t->native.elements=t->elements;
    size_t *offsets=calloc(n,sizeof *offsets);if(!offsets)return NULL;
    int bad=ffi_get_struct_offsets(FFI_DEFAULT_ABI,&t->native,offsets)!=FFI_OK;
    for(size_t i=0;i<n && !bad;i++)if(offsets[i]!=(t->tag==1 ? t->members[i].offset : i*t->stride))bad=1;
    free(offsets);
    if(bad || t->native.size!=t->width || t->native.alignment!=t->alignment)return NULL;
    return &t->native;
}
/* Capability-only structural check. The registry builds and verifies its own
   ffi layouts; no callback ffi pointer is installed in these borrowed graphs. */
typedef struct us_export_bridge_check {const us_export_signature *seen[1024];size_t signatures,nodes;} us_export_bridge_check;
static int us_export_bridge_type(const us_export_type *,int,unsigned,us_export_bridge_check *);
static int us_export_bridge_signature(const us_export_signature *s,us_export_bridge_check *c){
    if(!s||s->variadic>1||s->mode>1||s->count>1024||s->stored!=s->count||(!s->mode&&(s->count>6||s->variadic))||(s->variadic&&!s->count)||(s->count&&!s->argtypes))return 0;
    for(size_t i=0;i<c->signatures;i++)if(c->seen[i]==s)return 1;
    if(c->signatures>=1024)return 0;c->seen[c->signatures++]=s;
    if(!us_export_bridge_type(&s->result,1,1,c))return 0;
    for(size_t i=0;i<(size_t)s->count;i++)if(!us_export_bridge_type(&s->argtypes[i],0,1,c))return 0;
    return 1;
}
static int us_export_bridge_type(const us_export_type *t,int result,unsigned depth,us_export_bridge_check *c){
    if(!t||depth>32||++c->nodes>16384)return 0;
    if(t->kind==4)return t->depth==1&&t->width==8&&t->alignment==8&&!t->uns&&t->tag==4&&t->signature&&us_export_bridge_signature(t->signature,c);
    if(t->tag==0){ffi_type *f=us_export_ffitype(t,result);return f && (t->kind==0 || (f->size==t->width&&f->alignment==t->alignment));}
    if(t->kind!=5||t->depth||(t->tag!=1&&t->tag!=3))return 0;
    if(t->tag==3)return t->count&&t->count<=16384&&t->element&&t->stride==t->element->width&&t->width==t->count*t->stride&&t->alignment==t->element->alignment&&us_export_bridge_type(t->element,0,depth+1,c);
    if(!t->nmembers||t->nmembers>64||!t->members)return 0;
    uint64_t offset=0,alignment=1;
    for(size_t i=0;i<(size_t)t->nmembers;i++){
        const us_export_member *m=t->members+i;const us_export_type *v=m->type;
        if(m->bit_width||m->bit_offset||!us_export_bridge_type(v,0,depth+1,c)||!v->alignment)return 0;
        offset=(offset+v->alignment-1)&~(v->alignment-1);
        if(m->offset!=offset||offset>t->width||v->width>t->width-offset)return 0;
        offset+=v->width;if(v->alignment>alignment)alignment=v->alignment;
    }
    return t->alignment==alignment&&t->width==((offset+alignment-1)&~(alignment-1));
}
static int us_export_bridge_supported(const us_export *x){
    if(!x||x->version!=2||x->linkage||x->defined!=1||x->variadic||x->count>1024||x->stored!=x->count)return 0;
    us_export_bridge_check c={0};
    if(!us_export_bridge_type(&x->result,1,1,&c))return 0;
    for(size_t i=0;i<(size_t)x->count;i++)if(!us_export_bridge_type(us_export_arg(x,i),0,1,&c))return 0;
    return 1;
}
static int us_export_has_callbacks(const us_export *x){
    if(us_export_type_has_callback(&x->result))return 1;
    for(size_t i=0;i<(size_t)x->count;i++)if(us_export_type_has_callback(us_export_arg(x,i)))return 1;
    return 0;
}
/* Decode atomically: the old set is unchanged on every failure. */
static int us_exports_load_capability(us_exports *set,const void *data,size_t length,int bridge,char *error,size_t cap) {
    const unsigned char *bytes=data;size_t at=8;uint64_t count;us_exports tmp={0};unsigned version;
    if(!set || !bytes || length<16)goto bad;
    version=!memcmp(bytes,"USLSIG3\n",8) ? 3 : !memcmp(bytes,"USLSIG2\n",8) ? 2 : !memcmp(bytes,"USLSIG1\n",8) ? 1 : 0;
    if(!version || us_export_u64(bytes,length,&at,&count)||count>8192||count>SIZE_MAX/sizeof(us_export))goto bad;
    tmp.items=calloc(count ? (size_t)count:1,sizeof(us_export));if(!tmp.items)return us_export_error(error,cap,"export allocation failed");tmp.count=(size_t)count;
    for(size_t i=0;i<tmp.count;i++){
        us_export *x=&tmp.items[i];uint64_t n;size_t nodes=0,record_start=at;x->version=version;
        if(us_export_u64(bytes,length,&at,&n)||!n||n>length-at||n>=SIZE_MAX||memchr(bytes+at,0,(size_t)n))goto bad;
        x->name=malloc((size_t)n+1);if(!x->name)goto bad;memcpy(x->name,bytes+at,(size_t)n);x->name[n]=0;at+=(size_t)n;
        for(size_t j=0;j<(size_t)n;j++){unsigned ch=(unsigned char)x->name[j];int letter=(ch>=65&&ch<=90)||(ch>=97&&ch<=122)||ch==95;if(!letter && !(j&&((ch>=48&&ch<=57)||ch==46||ch==36)))goto bad;}
        for(size_t j=0;j<i;j++)if(!strcmp(x->name,tmp.items[j].name))goto bad;
        if(length-at<(version>=2 ? 4:3))goto bad;
        x->linkage=bytes[at++];x->defined=bytes[at++];x->variadic=bytes[at++];if(version>=2)x->mode=bytes[at++];
        if(x->linkage>1||x->defined!=1||x->variadic>1||x->mode>1||us_export_u64(bytes,length,&at,&x->count))goto bad;
        if(version==3){
            if(x->count>1024 || (!x->mode && (x->count>6||x->variadic)) ||
               us_export_descriptor3(bytes,length,&at,&x->result,1,&nodes,&x->graph)||
               us_export_u64(bytes,length,&at,&x->stored)||x->stored!=x->count)goto bad;
            x->argtypes=calloc(x->stored ? (size_t)x->stored:1,sizeof *x->argtypes);if(!x->argtypes)goto bad;
            for(size_t j=0;j<(size_t)x->stored;j++)if(us_export_descriptor3(bytes,length,&at,&x->argtypes[j],1,&nodes,&x->graph))goto bad;
        }else if(version==2){
            if(x->count>1024 || (!x->mode && (x->count>6||x->variadic)) ||
               us_export_descriptor2(bytes,length,&at,&x->result,1,&nodes,&x->graph)||us_export_u64(bytes,length,&at,&x->stored)||x->stored!=x->count)goto bad;
            x->argtypes=calloc(x->stored ? (size_t)x->stored:1,sizeof *x->argtypes);if(!x->argtypes)goto bad;
            for(size_t j=0;j<(size_t)x->stored;j++)if(us_export_descriptor2(bytes,length,&at,&x->argtypes[j],1,&nodes,&x->graph))goto bad;
            if(!bridge && !us_export_graph_support_valid(&x->graph))goto bad;
            if(bridge)for(size_t j=0;j<x->graph.count;j++)if(x->graph.signatures[j]->supported){
                us_export_bridge_check check={0};if(!us_export_bridge_signature(x->graph.signatures[j],&check))goto bad;
            }
            x->result.ffi=us_export_native(&x->result,1);
            for(size_t j=0;j<(size_t)x->stored;j++)x->argtypes[j].ffi=us_export_native(&x->argtypes[j],0);
        }else{
            if(us_export_descriptor(bytes,length,&at,&x->result)||us_export_u64(bytes,length,&at,&x->stored)||x->stored!=(x->count<8 ? x->count:8))goto bad;
            for(size_t j=0;j<(size_t)x->stored;j++)if(us_export_descriptor(bytes,length,&at,&x->args[j]))goto bad;
        }
        if(at>=length || (x->supported=bytes[at++])>1)goto bad;
        if((version>=2 && at-record_start>16777216) || (version==3 && x->supported) || (x->supported && !(bridge && version==2 ? us_export_bridge_supported(x):us_export_supported(x))))goto bad;
        /* Preserve the exact encoded graph rather than reconstructing ABI rules. */
        size_t record_length=at-record_start;
        if(record_length>SIZE_MAX-16)goto bad;
        x->wire_length=16+record_length;x->wire=malloc(x->wire_length);
        if(!x->wire)goto bad;
        memcpy(x->wire,bytes,8);memset(x->wire+8,0,8);x->wire[8]=1;
        memcpy(x->wire+16,bytes+record_start,record_length);
    }
    if(at!=length)goto bad;us_exports_clear(set);*set=tmp;return 0;
bad:us_exports_clear(&tmp);return us_export_error(error,cap,"malformed library signature declaration");
}
static int us_exports_load(us_exports *s,const void *b,size_t n,char *e,size_t cap){return us_exports_load_capability(s,b,n,0,e,cap);}
static int us_exports_load_bridge(us_exports *s,const void *b,size_t n,char *e,size_t cap){return us_exports_load_capability(s,b,n,1,e,cap);}
static uint64_t us_export_slot(const us_export_type *t,const void *p) {
    if(t->kind==3){uint64_t v=0;memcpy(&v,p,(size_t)t->width);return v;}
    if(t->kind==2){void *v;memcpy(&v,p,sizeof v);return (uintptr_t)v;}
#define US_SLOT(W,U,S) if(t->width==W){if(t->uns){U v;memcpy(&v,p,W);return v;}else{S v;memcpy(&v,p,W);return (uint64_t)(int64_t)v;}}
    US_SLOT(1,uint8_t,int8_t) US_SLOT(2,uint16_t,int16_t) US_SLOT(4,uint32_t,int32_t) US_SLOT(8,uint64_t,int64_t)
#undef US_SLOT
    return 0;
}
static void us_export_result(const us_export_type *t,void *result,uint64_t value) {
    if(t->kind==0)return;
    if(t->kind==3){memcpy(result,&value,(size_t)t->width);return;}
    if(t->kind==2){void *p=(void*)(uintptr_t)value;memcpy(result,&p,sizeof p);return;}
    if(t->uns){ffi_arg v=(ffi_arg)value;if(t->width<8)v&=(((uint64_t)1<<(8*t->width))-1);memcpy(result,&v,sizeof v);}
    else{ffi_sarg v=t->width==1 ? (int8_t)value : t->width==2 ? (int16_t)value : t->width==4 ? (int32_t)value : (int64_t)value;memcpy(result,&v,sizeof v);}
}
static void us_export_callback(ffi_cif *cif,void *result,void **args,void *userdata) {
    (void)cif;us_export *x=userdata;uint64_t slots[6]={0},value=0;
    for(size_t i=0;i<(size_t)x->count;i++)slots[i]=us_export_slot(&x->args[i],args[i]);
    x->last_status=x->invoke(x->owner,x->raw,slots,&value);
    if(x->last_status)value=0;
    us_export_result(&x->result,result,value);
}
/* Every invocation owns aggregate copies and result storage independently. */
static void us_export_callback_frame(ffi_cif *cif,void *result,void **args,void *userdata) {
    (void)cif;us_export *x=userdata;size_t n=(size_t)x->count;
    uint64_t *slots=calloc(n ? n:1,sizeof *slots);
    void **copies=calloc(n ? n:1,sizeof *copies);
    uint64_t scalar=0;void *aggregate=NULL;int status=1;
    if(!slots || !copies)goto done;
    for(size_t i=0;i<n;i++){
        const us_export_type *t=us_export_arg(x,i);
        if(t->kind==5){copies[i]=malloc((size_t)t->width);if(!copies[i])goto done;
            memcpy(copies[i],args[i],(size_t)t->width);slots[i]=(uintptr_t)copies[i];}
        else slots[i]=us_export_slot(t,args[i]);
    }
    if(x->result.kind==5){aggregate=calloc(1,(size_t)x->result.width);if(!aggregate)goto done;}
    us_export_frame frame={slots,x->count,x->mode,(unsigned)x->result.kind,
        x->result.kind==5 ? (size_t)x->result.width : x->result.kind==0 ? 0:sizeof scalar,
        x->result.kind==5 ? aggregate : x->result.kind==0 ? NULL:&scalar};
    status=x->invoke_frame(x->owner,x->raw,&frame);
done:
    x->last_status=status;
    if(x->result.kind==5){if(!status && aggregate)memcpy(result,aggregate,(size_t)x->result.width);else memset(result,0,(size_t)x->result.width);}
    else us_export_result(&x->result,result,status ? 0:scalar);
    if(copies)for(size_t i=0;i<n;i++)free(copies[i]);free(copies);free(slots);free(aggregate);
}
static void *us_exports_symbol_frame(us_exports *set,const char *name,void *owner,
        us_export_lookup lookup,us_export_invoke_frame invoke,char *error,size_t cap) {
    if(!set || !name || !lookup || !invoke){us_export_error(error,cap,"invalid export lookup");return NULL;}
    us_export *x=NULL;for(size_t i=0;i<set->count;i++)if(!strcmp(name,set->items[i].name)){x=&set->items[i];break;}
    if(!x || !us_export_supported(x)){us_export_error(error,cap,"missing or unsupported function export");return NULL;}
    if(x->code){if(x->owner==owner && x->invoke_frame==invoke)return x->code;us_export_error(error,cap,"export ownership changed");return NULL;}
    int kind=-1;const void *raw=NULL;
    if(lookup(owner,name,&raw,&kind)||!raw||kind!=0){us_export_error(error,cap,"export has no code address");return NULL;}
    x->dynamic_ffiargs=calloc(x->count ? (size_t)x->count:1,sizeof *x->dynamic_ffiargs);
    if(!x->dynamic_ffiargs){us_export_error(error,cap,"ffi allocation failed");return NULL;}
    for(size_t i=0;i<(size_t)x->count;i++)x->dynamic_ffiargs[i]=us_export_ffitype(us_export_arg(x,i),0);
    if(ffi_prep_cif(&x->cif,FFI_DEFAULT_ABI,(unsigned)x->count,us_export_ffitype(&x->result,1),x->dynamic_ffiargs)!=FFI_OK){
        free(x->dynamic_ffiargs);x->dynamic_ffiargs=NULL;us_export_error(error,cap,"ffi signature rejected");return NULL;}
    x->owner=owner;x->invoke_frame=invoke;x->raw=raw;
    x->closure=ffi_closure_alloc(sizeof *x->closure,&x->code);
    if(!x->closure||!x->code||ffi_prep_closure_loc(x->closure,&x->cif,us_export_callback_frame,x,x->code)!=FFI_OK){
        if(x->closure)ffi_closure_free(x->closure);x->closure=NULL;x->code=NULL;
        free(x->dynamic_ffiargs);x->dynamic_ffiargs=NULL;us_export_error(error,cap,"ffi closure creation failed");return NULL;}
    return x->code;
}
static void *us_exports_symbol(us_exports *set,const char *name,void *owner,
        us_export_lookup lookup,us_export_invoke invoke,char *error,size_t cap) {
    if(!set || !name || !lookup || !invoke){us_export_error(error,cap,"invalid export lookup");return NULL;}
    us_export *x=NULL;for(size_t i=0;i<set->count;i++)if(!strcmp(name,set->items[i].name)){x=&set->items[i];break;}
    if(!x || x->count>6 || x->version>=2 || !us_export_supported(x)){us_export_error(error,cap,"missing or unsupported function export");return NULL;}
    if(x->code){if(x->owner==owner && x->invoke==invoke)return x->code;us_export_error(error,cap,"export ownership changed");return NULL;}
    int kind=-1;const void *raw=NULL;
    if(lookup(owner,name,&raw,&kind) || !raw || kind!=0){us_export_error(error,cap,"export has no code address");return NULL;}
    for(size_t i=0;i<(size_t)x->count;i++)x->ffiargs[i]=us_export_ffitype(us_export_arg(x,i),0);
    if(ffi_prep_cif(&x->cif,FFI_DEFAULT_ABI,(unsigned)x->count,us_export_ffitype(&x->result,1),x->ffiargs)!=FFI_OK){us_export_error(error,cap,"ffi signature rejected");return NULL;}
    x->owner=owner;x->invoke=invoke;x->raw=raw;
    x->closure=ffi_closure_alloc(sizeof *x->closure,&x->code);
    if(!x->closure || !x->code || ffi_prep_closure_loc(x->closure,&x->cif,us_export_callback,x,x->code)!=FFI_OK){
        if(x->closure)ffi_closure_free(x->closure);x->closure=NULL;x->code=NULL;us_export_error(error,cap,"ffi closure creation failed");return NULL;
    }
    return x->code;
}
#endif

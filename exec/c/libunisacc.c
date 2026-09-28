/* Host adaptation for the same product network routes. No parser or
   compilation rule lives here. One context may be used by one thread at a
   time; different contexts execute concurrently, without a global lock. */
#define LIBUNISACC_BUILD
#include "libunisacc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <setjmp.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "libraryexports.h"
#include "librarycall.h"
#include "librarynative.h"
#include "librarybindings.h"
#include "libraryresolver.h"
#define US_CALLABLES_IMPLEMENTATION
#include "librarycallables.h"
#include "librarycarrierplan.h"

typedef struct Allocation { void *p; struct Allocation *next, *prev, *hash_next; } Allocation;
typedef struct GuestMap { void *base; size_t length; struct GuestMap *next; } GuestMap;
typedef struct Symbol { char *name; uintptr_t address; int kind; } Symbol;
typedef struct Source { char *name, *bytes; struct Source *next; } Source;
typedef struct LibraryCallableDeclaration {uint64_t key;us_exports graph;us_export_signature signature;} LibraryCallableDeclaration;
typedef struct LibraryCallableSite {uint64_t site,key,fixed;us_exports graph;us_export_signature signature;} LibraryCallableSite;
typedef struct LibraryCarrierExport {us_export *source;us_carrier_certificate certificate;struct LibraryCarrierExport *next;} LibraryCarrierExport;
struct us_context {
    char *package, *definitions, *include_path, *target;
    Source *sources;
    int source_count, optimisation;
    unsigned char *tape;
    size_t tape_length, definitions_length;
    unsigned char *signatures; size_t signatures_length;
    int input_is_tape;
    unsigned char *image; int64_t image_size, image_dataoff, image_text_size, image_data_size; int image_entry;
    us_exports exports;
    LibraryCarrierExport *carrier_exports;
    us_bindings bindings;
    us_resolver resolver;
    us_native_plans native_plans;
    us_native_templates native_templates;
    us_native_callsites native_callsites;
    us_callables callables;uint64_t image_generation;
    LibraryCallableDeclaration *callable_declarations;size_t callable_count;
    LibraryCallableSite *callable_sites;size_t callable_site_count;
    unsigned char *binding_blob; size_t binding_length;
    unsigned char *call_stack; size_t call_stack_size;
    int initialised, call_exited, call_exit_status, call_failed;
    Symbol *symbols; size_t symbol_count;
    GuestMap *guest_maps;
    int argc; char **argv;
    uint64_t process_slots[2];
    char error[1024];
};
_Static_assert(sizeof(((us_context *)0)->process_slots)==16,"process protocol requires two 64-bit slots");
static _Thread_local us_context *active;
static _Thread_local Allocation *allocations;
#define ALLOCATION_BUCKETS 4096
static _Thread_local Allocation *allocation_buckets[ALLOCATION_BUCKETS];
typedef struct UsOpenFile { int fd; struct UsOpenFile *next; } UsOpenFile;
static _Thread_local UsOpenFile *open_files;
static _Thread_local jmp_buf failure;
static void __us_panic(const char *message) {
    if(message!=active->error)snprintf(active->error,sizeof active->error,"%s",message);
    longjmp(failure,1);
}
static int tracked_open(const char *path,int flags) {
    int fd=open(path,flags);if (fd<0) return fd;
    UsOpenFile *node=malloc(sizeof *node);
    if (!node) {close(fd);__us_panic("out of memory");}
    node->fd=fd;node->next=open_files;open_files=node;return fd;
}
static int tracked_close(int fd) {
    UsOpenFile **at=&open_files;
    while (*at && (*at)->fd!=fd) at=&(*at)->next;
    if (*at) {UsOpenFile *node=*at;*at=node->next;free(node);}
    return close(fd);
}
/* Track all runtime allocations, including partial model loads on errors.
   The host context itself is outside this per-compilation cleanup domain. */
static size_t allocation_bucket(void *p) {
    uintptr_t x=(uintptr_t)p >> 4;
    x^=x>>17; x^=x>>9;
    return (size_t)x & (ALLOCATION_BUCKETS-1);
}
static Allocation **allocation_slot(void *p) {
    Allocation **at=&allocation_buckets[allocation_bucket(p)];
    while (*at && (*at)->p!=p) at=&(*at)->hash_next;
    return at;
}
static void *tracked_realloc(void *p,size_t n) {
    Allocation **at=p ? allocation_slot(p) : 0;
    Allocation *a=at ? *at : 0;
    if (p && !a) __us_panic("unowned runtime allocation");
    void *q=realloc(p,n ? n : 1);
    if (!q) __us_panic("out of memory");
    if (a) *at=a->hash_next;
    else {
        a=malloc(sizeof *a);
        if (!a) { free(q); __us_panic("out of memory"); }
        a->prev=0; a->next=allocations;
        if (allocations) allocations->prev=a;
        allocations=a;
    }
    a->p=q;
    size_t bucket=allocation_bucket(q);
    a->hash_next=allocation_buckets[bucket]; allocation_buckets[bucket]=a;
    return q;
}
static void *tracked_calloc(size_t count,size_t size) {
    if (size && count>SIZE_MAX/size) __us_panic("allocation overflow");
    size_t n=count*size; void *p=tracked_realloc(0,n); memset(p,0,n); return p;
}
static void tracked_free(void *p) {
    if (!p) return;
    Allocation **at=allocation_slot(p);
    if (!*at) __us_panic("unowned runtime free");
    Allocation *node=*at; *at=node->hash_next;
    if (node->prev) node->prev->next=node->next; else allocations=node->next;
    if (node->next) node->next->prev=node->prev;
    free(p); free(node);
}
static void cleanup(void) {
    while (open_files) {UsOpenFile *node=open_files;open_files=node->next;close(node->fd);free(node);}
    while (allocations) { Allocation *a=allocations; allocations=a->next; free(a->p); free(a); }
    memset(allocation_buckets,0,sizeof allocation_buckets);
}
static void diagnostic(int status,const char *reason,int n,const void *errors);
#define UNISA_RUNTIME_LIBRARY
#define UNISA_RUNTIME_STATE static _Thread_local
#define UNISA_RUNTIME_PANIC(message) __us_panic(message)
#define UNISA_RUNTIME_DIAGNOSTIC(status,reason,n,err) diagnostic(status,reason,n,err)
#define core_host_fetch __us_core_host_fetch
#define core_host_panic __us_core_host_panic
#define core_run __us_core_run
#define realloc tracked_realloc
#define calloc tracked_calloc
#define free tracked_free
#define open tracked_open
#define close tracked_close
#include "run.c"
#undef open
#undef close
#undef realloc
#undef calloc
#undef free
#include "memory.c"
#include "../../src/host_dl.h"
#if defined(_WIN32) && !defined(__UNISA__)
#include "librarywinimports.h"
#endif

static void diagnostic(int status,const char *reason,int n,const void *errors) {
    if (!status) return;
    const Buf *e=errors;
    if (reason && (n || *reason)) snprintf(active->error,sizeof active->error,"%.*s",n ? n : (int)strlen(reason),reason);
    else if (e && e->n) snprintf(active->error,sizeof active->error,"%.*s",e->n,(const char *)e->b);
    else snprintf(active->error,sizeof active->error,"compiler status %d",status);
}

#define API US_API
static size_t library_page_size(void) {
#ifdef _WIN32
    SYSTEM_INFO info;GetSystemInfo(&info);return info.dwPageSize;
#else
    long n=sysconf(_SC_PAGESIZE);return n>0?(size_t)n:0;
#endif
}
static void library_release_map(void *p,size_t extent) {
#ifdef _WIN32
    (void)extent;VirtualFree(p,0,MEM_RELEASE);
#else
    munmap(p,extent);
#endif
}
static void library_callable_catalog_clear(us_context *c){
    for(size_t i=0;i<c->callable_count;i++)us_exports_clear(&c->callable_declarations[i].graph);
    free(c->callable_declarations);c->callable_declarations=NULL;c->callable_count=0;
    for(size_t i=0;i<c->callable_site_count;i++)us_exports_clear(&c->callable_sites[i].graph);
    free(c->callable_sites);c->callable_sites=NULL;c->callable_site_count=0;
}
static void discard_image(us_context *c) {
    us_callables_clear(&c->callables);if(c->image_generation!=UINT64_MAX)c->image_generation++;
    while(c->carrier_exports){LibraryCarrierExport *p=c->carrier_exports;c->carrier_exports=p->next;us_carrier_certificate_clear(&p->certificate);free(p);}
    us_exports_clear(&c->exports);c->initialised=0;
    if(c->call_stack){library_release_map(c->call_stack,c->call_stack_size);c->call_stack=0;c->call_stack_size=0;}
    while (c->guest_maps) {GuestMap *m=c->guest_maps;c->guest_maps=m->next;library_release_map(m->base,m->length);free(m);}
    if (c->image) {
        library_release_map(c->image,(size_t)c->image_size);
        c->image=0;
        c->image_data_size=0;
    }
    for (size_t i=0;i<c->symbol_count;i++) free(c->symbols[i].name);
    free(c->symbols); c->symbols=0; c->symbol_count=0;
}
static char *copy_string(const char *s) {
    if (!s) return 0;
    size_t n=strlen(s)+1; char *p=malloc(n); if (p) memcpy(p,s,n); return p;
}
static int error(us_context *c,const char *s) {
    if (c) snprintf(c->error,sizeof c->error,"%s",s);
    return 1;
}
API us_context *us_new(const char *path) {
    if (!path) return 0;
    us_context *c=calloc(1,sizeof *c);
    if (c && !(c->package=copy_string(path))) { free(c); return 0; }
    return c;
}
API void us_free(us_context *c) {
    if (!c) return;
    discard_image(c);
    us_native_callsites_clear(&c->native_callsites);
    us_native_templates_clear(&c->native_templates);
    us_native_plans_clear(&c->native_plans);
    us_resolver_clear(&c->resolver);
    us_bindings_clear(&c->bindings);free(c->binding_blob);
    free(c->package); free(c->definitions); free(c->target);
    for (Source *s=c->sources;s;) { Source *next=s->next; free(s->name); free(s->bytes); free(s); s=next; }
    for (int i=0;i<c->argc;i++) free(c->argv[i]); free(c->argv);
    library_callable_catalog_clear(c);free(c->include_path); free(c->tape); free(c->signatures); free(c);
}
API int us_add_source(us_context *c,const char *name,const char *source) {
    if (!c || !name || !source) return error(c,"missing source");
    if (c->input_is_tape) return error(c,"cannot mix tape and C inputs");
    if (c->source_count==INT_MAX) return error(c,"too many sources");
    Source *s=calloc(1,sizeof *s);
    if (!s) return error(c,"out of memory");
    s->name=copy_string(name); s->bytes=copy_string(source);
    if (!s->name || !s->bytes) { free(s->name); free(s->bytes); free(s); return error(c,"out of memory"); }
    Source **tail=&c->sources; while (*tail) tail=&(*tail)->next;
    *tail=s; c->source_count++; return 0;
}
API int us_add_file(us_context *c,const char *path) {
    if (!c || !path) return error(c,"missing file");
    FILE *f=fopen(path,"rb"); if (!f) return error(c,"cannot open source file");
    if (fseek(f,0,SEEK_END)) { fclose(f); return error(c,"cannot seek source"); }
    long n=ftell(f);
    if (n<0 || n>=INT_MAX || fseek(f,0,SEEK_SET)) { fclose(f); return error(c,"source too large"); }
    char *s=malloc((size_t)n+1); if (!s) { fclose(f); return error(c,"out of memory"); }
    size_t got=fread(s,1,(size_t)n,f); int bad=ferror(f); int closed=fclose(f);
    if (got!=(size_t)n || bad || closed) { free(s); return error(c,"cannot read source"); }
    s[n]=0; int rc=us_add_source(c,path,s); free(s); return rc;
}
API int us_add_tape(us_context *c,const void *bytes,size_t length) {
    if (!c || !bytes || length>=INT_MAX || c->sources || c->input_is_tape) return error(c,"invalid tape input");
    unsigned char *p=malloc(length ? length : 1); if (!p) return error(c,"out of memory");
    memcpy(p,bytes,length); c->tape=p; c->tape_length=length; c->input_is_tape=1; return 0;
}
API int us_define(us_context *c,const char *d) {
    if (!c || !d || strchr(d,'\n')) return error(c,"invalid definition");
    size_t old=c->definitions_length,n=strlen(d);
    if (n>=INT_MAX || old>(size_t)INT_MAX-n-1) return error(c,"definitions too large");
    char *p=realloc(c->definitions,old+n+1); if (!p) return error(c,"out of memory");
    memcpy(p+old,d,n); p[old+n]=0; c->definitions=p; c->definitions_length=old+n+1; return 0;
}
API int us_include_path(us_context *c,const char *path) {
    if (!c || !path) return error(c,"invalid include path");
    char *p=copy_string(path); if (!p) return error(c,"out of memory");
    free(c->include_path); c->include_path=p; return 0;
}
API const char *us_error(const us_context *c) { return c ? c->error : "null context"; }
API const void *us_tape(const us_context *c,size_t *length) {
    if (length) *length=c ? c->tape_length : 0; return c ? c->tape : 0;
}
static uint64_t library_u64(const Buf *b,size_t *at);
static int library_lookup(void *owner,const char *name,const void **raw,int *kind);
static const char *library_native_target(void);
static uint64_t library_native_dispatch(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
static uint64_t library_variadic_dispatch(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
static uint64_t library_callable_make_dispatch(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
static uint64_t library_callable_call_dispatch(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
static int library_invoke_frame(void *,const void *,const us_export_frame *);
static int library_callable_native_hook(void *,ffi_cif *,uintptr_t,void *,void **);
static int library_callable_script_hook(void *,const void *,const us_export_signature *,const us_export_frame *);
static void library_callable_failure_hook(void *,const char *);
static int library_callable_catalog_load(us_context *c,const unsigned char *b,size_t n){
    size_t at=9,built=0,sites_built=0;uint64_t count=0,site_count=0;
    LibraryCallableDeclaration *items=NULL;LibraryCallableSite *sites=NULL;
    if(!c||!b||n<17||n>INT_MAX)return error(c,"invalid callable catalogue");
    int version2=!memcmp(b,"USLCALL2\n",9);
    if(!version2&&memcmp(b,"USLCALL1\n",9))return error(c,"invalid callable catalogue");
    if(us_export_u64(b,n,&at,&count)||count>8192)goto bad;
    items=calloc(count?count:1,sizeof *items);if(!items)goto bad;
    for(size_t i=0;i<count;i++){
        uint64_t key,len;
        if(us_export_u64(b,n,&at,&key)||us_export_u64(b,n,&at,&len)||!key||len>n-at||len>16777216)goto bad;
        for(size_t j=0;j<i;j++)if(items[j].key==key)goto bad;
        items[i].key=key;built=i+1;
        if(us_exports_load_bridge(&items[i].graph,b+at,(size_t)len,c->error,sizeof c->error)||items[i].graph.count!=1||
           us_callable_export_signature(items[i].graph.items,&items[i].signature))goto bad;
        at+=(size_t)len;
    }
    if(version2){
        if(us_export_u64(b,n,&at,&site_count)||site_count>8192)goto bad;
        sites=calloc(site_count?site_count:1,sizeof *sites);if(!sites)goto bad;
        for(size_t i=0;i<site_count;i++){
            uint64_t payload,len;size_t end;const us_export_signature *proto=NULL;
            if(us_export_u64(b,n,&at,&payload)||payload<32||payload>n-at)goto bad;
            end=at+(size_t)payload;
            LibraryCallableSite *site=sites+i;sites_built=i+1;
            if(us_export_u64(b,end,&at,&site->site)||us_export_u64(b,end,&at,&site->key)||
               us_export_u64(b,end,&at,&site->fixed)||us_export_u64(b,end,&at,&len)||
               !site->site||!site->key||len>16777216||len!=end-at)goto bad;
            for(size_t j=0;j<i;j++)if(sites[j].site==site->site)goto bad;
            for(size_t j=0;j<count;j++)if(items[j].key==site->key){proto=&items[j].signature;break;}
            if(!proto||proto->variadic!=1||site->fixed!=proto->count||
               us_exports_load_bridge(&site->graph,b+at,(size_t)len,c->error,sizeof c->error)||site->graph.count!=1||
               us_callable_export_signature(site->graph.items,&site->signature)||
               !us_callable_concrete_valid(proto,&site->signature))goto bad;
            at=end;
        }
    }
    if(at!=n)goto bad;
    library_callable_catalog_clear(c);c->callable_declarations=items;c->callable_count=(size_t)count;
    c->callable_sites=sites;c->callable_site_count=(size_t)site_count;return 0;
bad:
    for(size_t i=0;i<built;i++)us_exports_clear(&items[i].graph);free(items);
    for(size_t i=0;i<sites_built;i++)us_exports_clear(&sites[i].graph);free(sites);
    return error(c,"invalid callable catalogue record");
}
static const LibraryCallableSite *library_callable_site(us_context *c,uint64_t key,uint64_t site){
    for(size_t i=0;i<c->callable_site_count;i++)if(c->callable_sites[i].key==key&&c->callable_sites[i].site==site)return c->callable_sites+i;
    return NULL;
}
static const us_export_signature *library_callable_signature(us_context *c,uint64_t key){
    for(size_t i=0;i<c->callable_count;i++)if(c->callable_declarations[i].key==key)return &c->callable_declarations[i].signature;
    return NULL;
}
static void invalidate_bindings(us_context *c) {
    discard_image(c);library_callable_catalog_clear(c);us_native_callsites_clear(&c->native_callsites);
    us_native_templates_clear(&c->native_templates);us_native_plans_clear(&c->native_plans);free(c->tape);c->tape=0;c->tape_length=0;
    free(c->signatures);c->signatures=0;c->signatures_length=0;
    free(c->binding_blob);c->binding_blob=0;c->binding_length=0;
    free(c->target);c->target=0;c->error[0]=0;
}
static us_binding_type binding_type(us_type_descriptor t) {
    us_binding_type r={t.depth,t.base,t.shape,t.kind,t.width,t.uns};return r;
}
API int us_add_symbol(us_context *c,const char *name,void *address,const us_signature *sig) {
    if(!c || !sig || sig->kind>1 || active)return error(c,"invalid symbol registration");
    if(c->input_is_tape)return error(c,"symbol injection requires C declarations");
    us_binding_type result=binding_type(sig->result),*args=0;
    if(sig->count>1024 || (sig->count&&!sig->args))return error(c,"invalid symbol parameters");
    if(sig->kind && (sig->count || sig->variadic))return error(c,"invalid data symbol declaration");
    if(sig->count){args=malloc(sig->count*sizeof *args);if(!args)return error(c,"out of memory");
        for(size_t i=0;i<sig->count;i++)args[i]=binding_type(sig->args[i]);}
    int rc=us_resolver_accept_injection(&c->resolver,name,sig->kind,&result,args,sig->count,
                 sig->variadic,sig->extent,sig->writable,c->error,sizeof c->error);
    if(rc){free(args);return rc;}
    rc=sig->kind ? us_bindings_add_data(&c->bindings,name,(uintptr_t)address,&result,
                      sig->extent,sig->writable,c->error,sizeof c->error) :
        us_bindings_add_function(&c->bindings,name,(uintptr_t)address,&result,args,sig->count,
                                sig->variadic,c->error,sizeof c->error);
    free(args);if(rc)return rc;
    c->resolver.generation++;invalidate_bindings(c);return 0;
}
API int us_declare_import(us_context *c,const char *name,const us_signature *sig) {
    if(!c || !sig || sig->kind>1 || active || c->input_is_tape)
        return error(c,"invalid import declaration");
    if(sig->count>1024 || (sig->count&&!sig->args))return error(c,"invalid import parameters");
    us_binding_type result=binding_type(sig->result),*args=0;
    if(sig->count){args=malloc(sig->count*sizeof *args);if(!args)return error(c,"out of memory");
        for(size_t i=0;i<sig->count;i++)args[i]=binding_type(sig->args[i]);}
    int rc=us_resolver_declare(&c->resolver,&c->bindings,name,sig->kind,&result,args,
          sig->count,sig->variadic,sig->extent,sig->writable,c->error,sizeof c->error);
    free(args);if(rc)return rc;invalidate_bindings(c);return 0;
}
API int us_add_symbol_typed(us_context *c,const char *name,void *address,const void *signature,size_t length) {
    if(!c || active || c->input_is_tape || !address)return error(c,"invalid typed symbol registration");
    if(us_resolver_accept_injection_typed_bridge(&c->resolver,name,signature,length,c->error,sizeof c->error))return 1;
    if(us_bindings_add_function_typed_bridge(&c->bindings,name,(uintptr_t)address,signature,length,c->error,sizeof c->error))return 1;
    c->resolver.generation++;invalidate_bindings(c);return 0;
}
API int us_declare_import_typed(us_context *c,const char *name,const void *signature,size_t length) {
    if(!c || active || c->input_is_tape)return error(c,"invalid typed import declaration");
    if(us_resolver_declare_typed_bridge(&c->resolver,&c->bindings,name,signature,length,c->error,sizeof c->error))return 1;
    invalidate_bindings(c);return 0;
}
API int us_load_library(us_context *c,const char *path) {
    if(!c || active || c->input_is_tape)return error(c,"invalid library load");
    int rc=us_resolver_load(&c->resolver,path,c->error,sizeof c->error);
    if(rc)return rc;invalidate_bindings(c);return 0;
}
/* A separate runtime allocation domain, used only while no script is active.
   The network, not this adapter, selects or rejects the ABI carrier. */
typedef struct LibraryCarrierOwner {us_context *context;const char *target;} LibraryCarrierOwner;
static int library_carrier_model(us_context *c,const char *target,const void *wire,size_t length,us_carrier_certificate *cert){
    if(!c||!target||!wire||length>INT_MAX||active)return error(c,"carrier model requires an idle context");
    active=c;volatile int rc=1;
    if(!setjmp(failure)){
        RI=0;NRI=0;NR=0;FILE_READ_RECORD=0;FILE_READ_COUNT=0;FILE_READ_PATHS=0;
        package(c->package);
        char route[64];snprintf(route,sizeof route,"%s/nativeabi",target);
        int present=0;for(int i=0;i<PS;i++)if(!strcmp(STAGES[i].route,route))present=1;
        if(!present)rc=2;
        else{
            ResourceInput resource={0};resource.name=(const unsigned char*)"\0cli/target";resource.n=11;
            resource.data=(const unsigned char*)target;resource.len=(int)strlen(target);RI=&resource;NRI=1;
            Buf input={0};input.b=tracked_realloc(NULL,length);input.n=(int)length;memcpy(input.b,wire,length);
            int rejected=runroute(route,&input,"native ABI declaration");
            rc=rejected?2:us_carrier_certificate_load(cert,target,input.b,(size_t)input.n,c->error,sizeof c->error);
        }
    }
    cleanup();active=NULL;RI=NULL;NRI=0;NR=0;
    if(rc==2)c->error[0]=0;
    return (int)rc;
}
static int library_carrier_exports_prepare(us_context *c){
    for(size_t i=0;i<c->exports.count;i++){
        us_export *x=c->exports.items+i;
        if(x->version!=2||x->linkage||x->defined!=1||x->variadic||us_export_supported(x)||
           (us_export_has_callbacks(x)&&us_export_bridge_supported(x)))continue;
        LibraryCarrierExport *entry=calloc(1,sizeof *entry);
        if(!entry)return error(c,"carrier export allocation failed");
        int rc=library_carrier_model(c,c->target,x->wire,x->wire_length,&entry->certificate);
        if(rc){us_carrier_certificate_clear(&entry->certificate);free(entry);if(rc==1)return 1;continue;}
        entry->source=x;entry->next=c->carrier_exports;c->carrier_exports=entry;
    }
    return 0;
}
static int library_carrier_provider(void *owner,us_native_plans *plans,uintptr_t raw,const void *wire,size_t length,uint64_t *handle,char *message,size_t cap){
    LibraryCarrierOwner *request=owner;us_carrier_certificate cert={0};
    int rc=library_carrier_model(request->context,request->target,wire,length,&cert);
    if(rc==2){*handle=0;return 0;}
    if(!rc)rc=us_carrier_certificate_native_add(plans,raw,&cert,handle,message,cap);
    else us_export_error(message,cap,request->context->error);
    us_carrier_certificate_clear(&cert);return rc;
}
API int us_compile(us_context *c,const char *target,int level) {
    if (!c || !target || level<0 || level>2) return error(c,"invalid compilation options");
    if (!c->sources && !c->input_is_tape) return error(c,"no input");
    if (active) return error(c,"recursive compilation not yet supported");
    c->error[0]=0;
    unsigned char *frozen=0;size_t frozen_length=0;us_native_plans plans={0};us_native_templates templates={0};
    LibraryCarrierOwner carrier_owner={c,target};plans.carrier_provider=library_carrier_provider;plans.carrier_owner=&carrier_owner;
    if(c->bindings.count || c->resolver.declarations.count || c->resolver.handle_count){
        if(strcmp(target,library_native_target()))return error(c,"native resolver target differs from host");
        if(us_resolver_freeze_with_templates_bridge(&c->resolver,&c->bindings,(uintptr_t)library_native_dispatch,(uintptr_t)library_variadic_dispatch,&plans,&templates,&frozen,&frozen_length,c->error,sizeof c->error))return 1;
    }
    plans.carrier_provider=NULL;plans.carrier_owner=NULL;
    if(frozen_length>=INT_MAX){free(frozen);us_native_plans_clear(&plans);us_native_templates_clear(&templates);return error(c,"bindings too large");}
    char *chosen=copy_string(target); if (!chosen){free(frozen);us_native_plans_clear(&plans);us_native_templates_clear(&templates);return error(c,"out of memory");}
    discard_image(c);library_callable_catalog_clear(c);us_native_callsites_clear(&c->native_callsites);
    us_native_templates_clear(&c->native_templates);us_native_plans_clear(&c->native_plans);
    c->native_plans=plans;c->native_templates=templates;
    free(c->binding_blob);c->binding_blob=frozen;c->binding_length=frozen_length;
    free(c->target); c->target=chosen; c->optimisation=level;
    if (c->input_is_tape) return 0;
    free(c->tape);c->tape=0;c->tape_length=0;free(c->signatures);c->signatures=0;c->signatures_length=0;
    active=c; volatile int rc=1;
    if (!setjmp(failure)) {
        /* Reinitialise thread-local adapter inputs on every invocation. */
        RI=0; NRI=0; NR=0; FILE_READ_RECORD=0; FILE_READ_COUNT=0; FILE_READ_PATHS=0;
        INCDIR=c->include_path;
        package(c->package);
        ResourceInput resources[9]; memset(resources,0,sizeof resources);
        unsigned char callable_values[2][8];resource_u64(callable_values[0],(uintptr_t)library_callable_make_dispatch);resource_u64(callable_values[1],(uintptr_t)library_callable_call_dispatch);
        unsigned char library_request[8]={1,0,0,0,0,0,0,0};
        RI=resources; NRI=0;
        resources[NRI].name=(const unsigned char *)"\0cli/target"; resources[NRI].n=11;
        resources[NRI].data=(const unsigned char *)target; resources[NRI].len=(int)strlen(target); NRI++;
        if (c->definitions) {
            resources[NRI].name=(const unsigned char *)"\0cli/defines"; resources[NRI].n=12;
            resources[NRI].data=(const unsigned char *)c->definitions;
            resources[NRI].len=(int)c->definitions_length; NRI++;
        }
        if (c->include_path) {
            resources[NRI].name=(const unsigned char *)"\0cli/include-dir"; resources[NRI].n=16;
            resources[NRI].data=(const unsigned char *)c->include_path;
            resources[NRI].len=(int)strlen(c->include_path); NRI++;
        }
        resources[NRI].name=(const unsigned char *)"\0library/symbols";resources[NRI].n=16;
        resources[NRI].data=library_request;resources[NRI].len=8;NRI++;
        resources[NRI].name=(const unsigned char *)"\0library/module";resources[NRI].n=15;
        resources[NRI].data=library_request;resources[NRI].len=8;NRI++;
        if(c->binding_blob){
            resources[NRI].name=(const unsigned char *)"\0library/bindings";resources[NRI].n=17;
            resources[NRI].data=c->binding_blob;resources[NRI].len=(int)c->binding_length;NRI++;
        }
        resources[NRI].name=(const unsigned char *)"\0library/callables";resources[NRI].n=18;resources[NRI].data=library_request;resources[NRI].len=8;NRI++;
        resources[NRI].name=(const unsigned char *)"\0library/callablemake";resources[NRI].n=21;resources[NRI].data=callable_values[0];resources[NRI].len=8;NRI++;
        resources[NRI].name=(const unsigned char *)"\0library/callablecall";resources[NRI].n=21;resources[NRI].data=callable_values[1];resources[NRI].len=8;NRI++;
        Buf input={0};
        char route[128];
        int z=snprintf(route,sizeof route,"%s/%stape/O%d",target,c->source_count>1 ? "multi/" : "",level);
        if (z<0 || z>=(int)sizeof route) __us_panic("target too long");
        rc=0;
        for (Source *s=c->sources;s;s=s->next) {
            size_t n=strlen(s->bytes); if (n>=INT_MAX) __us_panic("source too large");
            Buf unit={0}; unit.b=tracked_realloc(0,n); memcpy(unit.b,s->bytes,n); unit.n=(int)n;
            if (c->source_count==1) { input=unit; break; }
            char unitroute[128];
            z=snprintf(unitroute,sizeof unitroute,"%s/unit",target);
            if (z<0 || z>=(int)sizeof unitroute) __us_panic("target too long");
            rc=runroute(unitroute,&unit,s->name); if (rc) break;
            size_t namelen=strlen(s->name);
            size_t framed=(size_t)unit.n+4+namelen;
            if (framed>INT_MAX || input.n>INT_MAX-4-(int)framed) __us_panic("unit frame too large");
            for (int j=0;j<4;j++) bput(&input,(int)(framed>>(8*j))&255,0);
            for (int j=0;j<4;j++) bput(&input,(int)(namelen>>(8*j))&255,0);
            for (size_t j=0;j<namelen;j++) bput(&input,(unsigned char)s->name[j],0);
            for (int j=0;j<unit.n;j++) bput(&input,unit.b[j],0);
            tracked_free(unit.b);
        }
        if (!rc) rc=runroute_range(route,0,"e3",&input,c->sources->name);
        if (!rc) {
            size_t begin=0,length=(size_t)input.n;
            if (input.n>=9 && (!memcmp(input.b,"USLTAPE1\n",9) || !memcmp(input.b,"USLTAPE2\n",9) || !memcmp(input.b,"USLTAPE3\n",9))) {
                int version3=!memcmp(input.b,"USLTAPE3\n",9);
                int version2=version3 || !memcmp(input.b,"USLTAPE2\n",9);
                size_t at=9;uint64_t tape=library_u64(&input,&at),meta=library_u64(&input,&at);
                uint64_t calls=version2 ? library_u64(&input,&at) : 0;
                uint64_t catalogue=version3 ? library_u64(&input,&at) : 0;
                if (tape>(size_t)input.n-at || meta>(size_t)input.n-at-tape ||
                    calls>(size_t)input.n-at-tape-meta || catalogue!=(size_t)input.n-at-tape-meta-calls || meta<16 ||
                    (memcmp(input.b+at+(size_t)tape,"USLSIG1\n",8) &&
                     memcmp(input.b+at+(size_t)tape,"USLSIG2\n",8))) __us_panic("bad library tape envelope");
                c->signatures=malloc((size_t)meta);if (!c->signatures) __us_panic("out of memory");
                memcpy(c->signatures,input.b+at+(size_t)tape,(size_t)meta);c->signatures_length=(size_t)meta;
                if(version2 && us_native_callsites_load(&c->native_callsites,&c->native_templates,
                    input.b+at+(size_t)tape+(size_t)meta,(size_t)calls,c->error,sizeof c->error)) __us_panic(c->error);
                if(version3 && library_callable_catalog_load(c,input.b+at+(size_t)tape+(size_t)meta+(size_t)calls,(size_t)catalogue))__us_panic(c->error);
                begin=at;length=(size_t)tape;
            }
            if (level) {
                /* E4 consumes tape bytes, never the opaque signature envelope. */
                Buf optimised={0};optimised.b=tracked_realloc(0,length ? length : 1);
                memcpy(optimised.b,input.b+begin,length);optimised.n=(int)length;
                rc=runroute_from(route,"e4",&optimised,c->sources->name);
                if (!rc) { input=optimised;begin=0;length=(size_t)input.n; }
            }
            if (!rc) {
                unsigned char *out=malloc(length ? length : 1);
                if (!out) __us_panic("out of memory");
                memcpy(out,input.b+begin,length);c->tape=out;c->tape_length=length;
            }
        }
    } else rc=1;
    cleanup(); active=0; RI=0; NRI=0;
    if(rc){library_callable_catalog_clear(c);us_native_callsites_clear(&c->native_callsites);us_native_templates_clear(&c->native_templates);
        us_native_plans_clear(&c->native_plans);free(c->signatures);c->signatures=0;c->signatures_length=0;}
    return rc;
}

/* The decoder handles a bounded byte format only; all symbol selection and
   address computation belong to the memory writer's model. */
static uint64_t library_u64(const Buf *b,size_t *at) {
    if (*at>(size_t)b->n || (size_t)b->n-*at<8) __us_panic("truncated library symbol map");
    uint64_t n=0; for (int j=7;j>=0;j--) n=(n<<8)|b->b[*at+j]; *at+=8; return n;
}
static int symbol_order(const void *a,const void *b) {
    return strcmp(((const Symbol *)a)->name,((const Symbol *)b)->name);
}
static void library_image(us_context *c,Buf *bytes,MemoryImage *m,MemoryMap *mapping) {
    if (bytes->n<40 || memcmp(bytes->b,"UNILIB1\n",8)) __us_panic("library model output not supported");
    m->text=memory_field(bytes->b+8);m->extent=memory_field(bytes->b+16);
    m->stored=memory_field(bytes->b+24);m->entry=memory_field(bytes->b+32);
    size_t at=40+(size_t)m->text+m->stored;
    if (m->text<=0 || m->entry>=m->text || m->stored>m->extent ||
        at>(size_t)bytes->n || (size_t)bytes->n-at<14 || memcmp(bytes->b+at,"SYMS1\n",6))
        __us_panic("bad library image bounds");
    at+=6;uint64_t count=library_u64(bytes,&at);
    if (count>((size_t)bytes->n-at)/18 || count>SIZE_MAX/sizeof(Symbol)) __us_panic("bad library symbol count");
    c->symbols=calloc(count ? (size_t)count : 1,sizeof(Symbol));
    if (!c->symbols) __us_panic("out of memory");
    c->symbol_count=(size_t)count;
    int64_t dataoff=((int64_t)m->text+16383)&-16384;
    for (size_t i=0;i<c->symbol_count;i++) {
        if (at>=(size_t)bytes->n) __us_panic("truncated library symbol");
        int kind=bytes->b[at++];uint64_t n=library_u64(bytes,&at),addr=library_u64(bytes,&at);
        if (kind>1 || !n || n>(size_t)bytes->n-at || n>=INT_MAX || memchr(bytes->b+at,0,(size_t)n))
            __us_panic("bad library symbol");
        uintptr_t lo=(uintptr_t)mapping->base+(kind ? dataoff : 0);
        uintptr_t hi=lo+(kind ? m->extent : m->text);
        if (addr<lo || addr>hi) __us_panic("library symbol outside image");
        Symbol *x=&c->symbols[i];x->kind=kind;x->address=(uintptr_t)addr;
        x->name=malloc((size_t)n+1);if (!x->name) __us_panic("out of memory");
        memcpy(x->name,bytes->b+at,(size_t)n);x->name[n]=0;at+=(size_t)n;
    }
    if (at!=(size_t)bytes->n) __us_panic("trailing library symbol bytes");
    qsort(c->symbols,c->symbol_count,sizeof(Symbol),symbol_order);
    for (size_t i=1;i<c->symbol_count;i++)
        if (!strcmp(c->symbols[i-1].name,c->symbols[i].name)) __us_panic("duplicate library symbol");
}
typedef struct NativeCleanup { us_native_arena *arena; struct NativeCleanup *next; } NativeCleanup;
typedef struct LibraryCallableScope {us_context *owner;const us_export_signature *signature;const us_export_frame *values;struct LibraryCallableScope *previous;} LibraryCallableScope;
static _Thread_local LibraryCallableScope *library_callable_scopes;
typedef struct ScriptFrame {
    jmp_buf returned;
    volatile int status,exited,failed;
    NativeCleanup *volatile native_arenas;
    unsigned char *stack_base;size_t stack_size;
    struct ScriptFrame *previous;us_context *owner;us_call_outcome *outcome;
    const us_export_signature *argument_signature;const uint64_t *argument_slots;
} ScriptFrame;
static _Thread_local ScriptFrame *script_frames;
static _Thread_local us_native_boundary *native_boundaries;
static void library_boundary_note(us_context *c,const us_call_outcome *outcome){
    for(us_native_boundary *b=native_boundaries;b;b=b->previous)
        if(b->owner==c){us_call_outcome_merge(&b->outcome,outcome);break;}
}
static int library_invocation_error(us_context *c,const char *message){
    if(message)error(c,message);
    us_call_outcome outcome={0};outcome.failed=1;
    if(c)snprintf(outcome.message,sizeof outcome.message,"%s",c->error);
    library_boundary_note(c,&outcome);
    int same_owner=0;for(ScriptFrame *f=script_frames;f;f=f->previous)if(f->owner==c){same_owner=1;break;}
    if(c && !same_owner){c->call_failed=1;c->call_exited=0;c->call_exit_status=0;}
    return 1;
}
static void script_frame_cleanup(ScriptFrame *frame) {
    while(frame->native_arenas){NativeCleanup *node=frame->native_arenas;frame->native_arenas=node->next;
        us_native_arena_free(node->arena);free(node);}
}
static int library_region(uintptr_t base,size_t extent,uintptr_t at,size_t bytes) {
    return at>=base && at-base<=extent && bytes<=extent-(at-base);
}
static int library_frame_region(us_context *c,ScriptFrame *f,uint64_t address,size_t bytes) {
    if(!bytes)return 1;
    size_t page=library_page_size();
    if(page && f->stack_size>=2*page && library_region((uintptr_t)f->stack_base+page,
              f->stack_size-2*page,(uintptr_t)address,bytes))return 1;
    if(f->argument_signature&&f->argument_slots)for(size_t i=0;i<f->argument_signature->count;i++){
        const us_export_type *t=f->argument_signature->argtypes+i;
        if(t->kind==5&&library_region((uintptr_t)f->argument_slots[i],(size_t)t->width,(uintptr_t)address,bytes))return 1;
    }
    return c->image_dataoff>=0 && c->image_data_size>=0 &&
        library_region((uintptr_t)c->image+(uintptr_t)c->image_dataoff,
                       (size_t)c->image_data_size,(uintptr_t)address,bytes);
}
static uint64_t library_dispatch_error(us_context *c,const char *message) {
    if(c && message)error(c,message);
    if(script_frames){script_frames->failed=1;longjmp(script_frames->returned,1);}
    return 1;
}
#include "librarycallablehost.h"
/* A declared ABI mechanism: model selected this exact plan; no host name winner
   or source type classification. Native code and borrowed objects obey C's
   lifetime contract; this bridge is not an untrusted-code sandbox. */
static uint64_t library_native_invoke(us_context *c,ScriptFrame *frame,us_native_plan *plan,
                                      uint64_t slots,uint64_t result,uint64_t count) {
    if(!plan || count>1024 || count!=plan->graph.items[0].count ||
       !library_frame_region(c,frame,slots,(size_t)count*8))
        return library_dispatch_error(c,"invalid declared native call plan or slots");
    us_export *x=plan->graph.items;size_t bytes=x->result.kind==5 ? (size_t)x->result.width : x->result.kind ? 8:0;
    if(!library_frame_region(c,frame,result,bytes))
        return library_dispatch_error(c,"native result is outside owned script frame");
    if(plan->bridge_required)return library_callable_plan_invoke(c,frame,plan,slots,result,count);
    us_native_arena *arena=NULL;
    if(us_native_prepare(plan,(const uint64_t *)(uintptr_t)slots,count,(void *)(uintptr_t)result,
                         &arena,c->error,sizeof c->error))return library_dispatch_error(c,NULL);
    NativeCleanup *node=malloc(sizeof *node);
    if(!node){us_native_arena_free(arena);return library_dispatch_error(c,"native cleanup allocation failed");}
    node->arena=arena;node->next=frame->native_arenas;frame->native_arenas=node;
    arena->boundary.owner=c;arena->boundary.previous=native_boundaries;
    native_boundaries=&arena->boundary;
    int rc=us_native_call(arena);
    native_boundaries=arena->boundary.previous;
    us_call_outcome outcome=arena->boundary.outcome;
    if(!rc && !outcome.failed && !outcome.exited)rc=us_native_commit(arena);
    frame->native_arenas=node->next;us_native_arena_free(arena);free(node);
    if(outcome.failed || outcome.exited){
        us_call_outcome_merge(frame->outcome,&outcome);
        frame->failed=outcome.failed;frame->exited=outcome.exited;frame->status=outcome.exit_status;
        if(outcome.message[0])snprintf(c->error,sizeof c->error,"%s",outcome.message);
        longjmp(frame->returned,1); /* ffi_call/native target has returned normally. */
    }
    if(rc)return library_dispatch_error(c,"declared native call failed");return 0;
}
static uint64_t library_native_dispatch(uint64_t handle,uint64_t slots,uint64_t result,
                                       uint64_t count,uint64_t reserved0,uint64_t reserved1) {
    us_context *c=active;ScriptFrame *frame=script_frames;
    if(!c || !frame || frame->owner!=c)return 1;
    if(reserved0 || reserved1)return library_dispatch_error(c,"invalid native call reserved fields");
    return library_native_invoke(c,frame,us_native_plan_find(&c->native_plans,handle),slots,result,count);
}
static uint64_t library_variadic_dispatch(uint64_t handle,uint64_t slots,uint64_t result,
                                         uint64_t count,uint64_t site,uint64_t reserved0) {
    us_context *c=active;ScriptFrame *frame=script_frames;
    if(!c || !frame || frame->owner!=c)return 1;
    if(reserved0 || !site)return library_dispatch_error(c,"invalid variadic call site or reserved field");
    return library_native_invoke(c,frame,us_native_callsite_find(&c->native_callsites,handle,site),slots,result,count);
}
/* Host memory lifetime adaptation. The model decides which operation and
   argument sequence to issue; these callbacks never inspect source or tape. */
static int64_t library_mmap(int64_t addr,int64_t length,int64_t prot,int64_t flags,int64_t fd,int64_t offset) {
#ifdef _WIN32
    /* Windows gates use VirtualAlloc(addr,size,type,protect), not POSIX mmap.
       Commit-only is restricted to an owned reservation; no foreign memory. */
    if(!active || length<=0 || addr<0 || fd || offset ||
       (uint64_t)length>SIZE_MAX || (uintptr_t)addr>UINTPTR_MAX-(size_t)length ||
       (uint64_t)prot>UINT32_MAX || (prot & ~(MEM_RESERVE|MEM_COMMIT|MEM_TOP_DOWN)) ||
       !(prot & (MEM_RESERVE|MEM_COMMIT)) || (uint64_t)flags>UINT32_MAX) {
        SetLastError(ERROR_INVALID_PARAMETER);return 0;
    }
    size_t page=library_page_size();
    if(!page || (size_t)length>SIZE_MAX-page+1){SetLastError(ERROR_INVALID_PARAMETER);return 0;}
    size_t extent=((size_t)length+page-1)/page*page;
    GuestMap *existing=0;
    if(!(prot & MEM_RESERVE)) {
        uintptr_t begin=(uintptr_t)addr/page*page;
        size_t leading=(uintptr_t)addr-begin;
        if((size_t)length>SIZE_MAX-leading-page+1){SetLastError(ERROR_INVALID_PARAMETER);return 0;}
        extent=((size_t)length+leading+page-1)/page*page;
        for(GuestMap *n=active->guest_maps;n;n=n->next)
            if(begin>=(uintptr_t)n->base && begin-(uintptr_t)n->base<=n->length &&
               extent<=n->length-(begin-(uintptr_t)n->base)){existing=n;break;}
        if(!existing){SetLastError(ERROR_INVALID_PARAMETER);return 0;}
    }
    GuestMap *node=existing ? 0 : malloc(sizeof *node);
    if(!existing && !node){SetLastError(ERROR_NOT_ENOUGH_MEMORY);return 0;}
    void *p=VirtualAlloc((void *)(uintptr_t)addr,(SIZE_T)length,(DWORD)prot,(DWORD)flags);
    if(!p){free(node);return 0;}
    if(!existing){
        MEMORY_BASIC_INFORMATION info;
        if(!VirtualQuery(p,&info,sizeof info) || info.AllocationBase!=p){
            VirtualFree(p,0,MEM_RELEASE);free(node);SetLastError(ERROR_INVALID_ADDRESS);return 0;
        }
        node->base=p;node->length=info.RegionSize;node->next=active->guest_maps;active->guest_maps=node;
    }
    return (int64_t)(uintptr_t)p;
#else
    if (!active || length<=0 || (flags&MAP_FIXED)) return -EINVAL;
    GuestMap *node=malloc(sizeof *node);if (!node) return -ENOMEM;
    void *p=mmap((void *)addr,(size_t)length,(int)prot,(int)flags,(int)fd,(off_t)offset);
    if (p==MAP_FAILED) {int code=errno;free(node);return -code;}
    long page=sysconf(_SC_PAGESIZE);
    if (page<=0 || (size_t)length>SIZE_MAX-(size_t)page+1) {munmap(p,(size_t)length);free(node);return -EINVAL;}
    node->base=p;node->length=((size_t)length+(size_t)page-1)/(size_t)page*(size_t)page;
    node->next=active->guest_maps;active->guest_maps=node;
    return (int64_t)(uintptr_t)p;
#endif
}
static int64_t library_munmap(int64_t addr,int64_t length) {
#ifdef _WIN32
    /* Match the Windows gate: release an entire owned allocation by base.
       Its size argument is not a POSIX partial-unmap request. */
    if(!active || addr<=0 || length<0){SetLastError(ERROR_INVALID_PARAMETER);return -1;}
    GuestMap **at=&active->guest_maps;
    while(*at && (uintptr_t)(*at)->base!=(uintptr_t)addr)at=&(*at)->next;
    if(!*at){SetLastError(ERROR_INVALID_ADDRESS);return -1;}
    GuestMap *node=*at;
    if(!VirtualFree(node->base,0,MEM_RELEASE))return -1;
    *at=node->next;free(node);return 0;
#else
    if (!active || length<=0 || (uintptr_t)addr>UINTPTR_MAX-(size_t)length) return -EINVAL;
    long page=sysconf(_SC_PAGESIZE);
    if (page<=0 || (size_t)length>SIZE_MAX-(size_t)page+1) return -EINVAL;
    size_t extent=((size_t)length+(size_t)page-1)/(size_t)page*(size_t)page;
    if ((uintptr_t)addr>UINTPTR_MAX-extent) return -EINVAL;
    GuestMap **at=&active->guest_maps;
    uintptr_t begin=(uintptr_t)addr,end=begin+extent;
    while (*at) {
        uintptr_t lo=(uintptr_t)(*at)->base,hi=lo+(*at)->length;
        if (begin>=lo && end<=hi) break;
        at=&(*at)->next;
    }
    if (!*at) return -EINVAL;
    GuestMap *node=*at,*tail=0;
    uintptr_t lo=(uintptr_t)node->base,hi=lo+node->length;
    if (begin>lo && end<hi) {tail=malloc(sizeof *tail);if (!tail) return -ENOMEM;}
    if (munmap((void *)addr,(size_t)length)) {int code=errno;free(tail);return -code;}
    if (begin==lo && end==hi) {*at=node->next;free(node);}
    else if (begin==lo) {node->base=(void *)end;node->length=hi-end;}
    else {node->length=begin-lo;if (tail) {tail->base=(void *)end;tail->length=hi-end;tail->next=node->next;node->next=tail;}}
    return 0;
#endif
}
static int64_t library_exit(int64_t status) {
    if(!script_frames)return -EINVAL;
    script_frames->status=(int)status;script_frames->exited=1;longjmp(script_frames->returned,1);
}
static const char *library_native_target(void) {
#if defined(__aarch64__) || defined(_M_ARM64)
#ifdef _WIN32
    return "win/arm64";
#elif defined(__APPLE__)
    return "osx/arm64";
#else
    return "lnx/arm64";
#endif
#else
#ifdef _WIN32
    return "win/x86_64";
#elif defined(__APPLE__)
    return "osx/x86_64";
#else
    return "lnx/x86_64";
#endif
#endif
}
API int us_relocate(us_context *c) {
    if (!c || !c->target || !c->tape || !c->tape_length) return error(c,"no compiled tape");
    if (strcmp(c->target,library_native_target())) return error(c,"library execution target differs from host");
    if (active) return error(c,"recursive compilation not yet supported");
#if defined(_WIN32) && !defined(__UNISA__)
    LibraryWinImports winimports={0};
    /* Construct ownership before setjmp: the object survives model longjmp. */
    if(library_winimports_init(&winimports) || winimports.count>INT_MAX-16){
        library_winimports_free(&winimports);return error(c,"cannot enumerate Windows OS exports");
    }
#endif
    discard_image(c);c->error[0]=0;active=c;volatile int rc=1;
    /* Mapping identity must survive longjmp on a malformed model output. */
    if (!setjmp(failure)) {
        RI=0;NRI=0;NR=0;FILE_READ_RECORD=0;FILE_READ_COUNT=0;FILE_READ_PATHS=0;
        package(c->package);
        MemoryMap mapping={0};memory_reserve(&mapping);
        c->image=mapping.base;c->image_size=mapping.reserved;
        /* Four loader slots and eight scalar resources. */
        size_t capacity=19;
#if defined(_WIN32) && !defined(__UNISA__)
        capacity+=(size_t)winimports.count;
#endif
        ResourceInput *actual=tracked_calloc(capacity,sizeof *actual);unsigned char scalar[17][8];
        const char *names[]={"\0process/argc","\0process/argv","\0memory/text","\0memory/reserve",
            "\0library/symbols","\0library/exit","\0library/mmap","\0library/munmap","\0process/dl/0","\0process/dl/1","\0process/dl/2","\0process/dl/3","\0library/module","\0library/process","\0library/callables","\0library/callablemake","\0library/callablecall"};
        uint64_t vals[]={c->argc,(uintptr_t)c->argv,(uintptr_t)mapping.base,mapping.reserved,1,(uintptr_t)library_exit,(uintptr_t)library_mmap,(uintptr_t)library_munmap,
            host_dl_slot(0),host_dl_slot(1),host_dl_slot(2),host_dl_slot(3),1,(uintptr_t)c->process_slots,1,(uintptr_t)library_callable_make_dispatch,(uintptr_t)library_callable_call_dispatch};
        int lengths[]={13,13,12,15,16,13,13,15,13,13,13,13,15,16,18,21,21};
        for (int i=0;i<17;i++) {resource_u64(scalar[i],vals[i]);actual[i].name=(const unsigned char *)names[i];actual[i].n=lengths[i];actual[i].data=scalar[i];actual[i].len=8;}
        RI=actual;NRI=17;
        if (c->signatures) {
            actual[NRI].name=(const unsigned char *)"\0library/signatures";actual[NRI].n=19;
            actual[NRI].data=c->signatures;actual[NRI].len=(int)c->signatures_length;NRI++;
        }
        if(c->binding_blob){
            actual[NRI].name=(const unsigned char *)"\0library/bindings";actual[NRI].n=17;
            actual[NRI].data=c->binding_blob;actual[NRI].len=(int)c->binding_length;NRI++;
        }
#if defined(_WIN32) && !defined(__UNISA__)
        memcpy(actual+NRI,winimports.rows,(size_t)winimports.count*sizeof *actual);NRI+=winimports.count;
#endif
        Buf input={0};input.n=(int)c->tape_length;input.b=tracked_realloc(0,c->tape_length);memcpy(input.b,c->tape,c->tape_length);
        char route[128];int z=snprintf(route,sizeof route,"%s/run/O%d",c->target,c->optimisation);
        if (z<0 || z>=(int)sizeof route) __us_panic("target too long");
        rc=runroute_from(route,"prune",&input,c->sources ? c->sources->name : "library.tape");
        if (!rc) {snprintf(route,sizeof route,"%s/memory",c->target);rc=runroute(route,&input,"library.tape");}
        if (!rc) {
            MemoryImage m;library_image(c,&input,&m,&mapping);memory_commit(&m,&mapping);
            c->image_dataoff=mapping.dataoff;c->image_entry=m.entry;c->image_text_size=m.text;c->image_data_size=m.extent;
            memcpy(mapping.base,input.b+40,m.text);memcpy(mapping.base+mapping.dataoff,input.b+40+m.text,m.stored);
            if (memory_protect_code(&mapping,m.text)) __us_panic("cannot protect library code");
            if(c->signatures && us_exports_load_bridge(&c->exports,c->signatures,c->signatures_length,c->error,sizeof c->error)) rc=1;
        }
    } else rc=1;
    cleanup();active=0;RI=0;NRI=0;
#if defined(_WIN32) && !defined(__UNISA__)
    library_winimports_free(&winimports);
#endif
    if(!rc)rc=library_carrier_exports_prepare(c);
    if (rc) discard_image(c);
    else if(c->image_generation==UINT64_MAX){discard_image(c);return error(c,"image generation capacity exceeded");}
    else {us_callables_init(&c->callables,c,c->image_generation,library_invoke_frame,library_callable_native_hook,library_callable_failure_hook);c->callables.script_call=library_callable_script_hook;}
    return rc;
}
/* Addresses are model-declared symbols; native callability comes only from
   the soft-stack adapter and an independently validated ABI declaration. */
static int library_lookup(void *owner,const char *name,const void **raw,int *kind) {
    us_context *c=owner;
    for(size_t i=0;i<c->symbol_count;i++) if(!strcmp(c->symbols[i].name,name)) {
        Symbol *s=&c->symbols[i];
        if(!s->kind && (s->address<(uintptr_t)c->image || s->address>=(uintptr_t)c->image+(uintptr_t)c->image_text_size)) return 1;
        *raw=(const void *)s->address;*kind=s->kind;return 0;
    }
    return 1;
}
static int library_stack_alloc(us_context *c,unsigned char **base,size_t *size) {
    size_t page=library_page_size(),usable=16U*1024U*1024U;
    if(!page || page>(SIZE_MAX-usable)/2)return error(c,"invalid host page size");
    size_t total=usable+2*page;
#ifdef _WIN32
    unsigned char *p=VirtualAlloc(0,total,MEM_RESERVE,PAGE_NOACCESS);
    if(!p)return error(c,"cannot reserve library call stack");
    if(VirtualAlloc(p+page,usable,MEM_COMMIT,PAGE_READWRITE)!=p+page){library_release_map(p,total);return error(c,"cannot commit library call stack");}
#else
    unsigned char *p=mmap(0,total,PROT_NONE,MAP_PRIVATE|MAP_ANON,-1,0);
    if(p==MAP_FAILED)return error(c,"cannot reserve library call stack");
    if(mprotect(p+page,usable,PROT_READ|PROT_WRITE)){library_release_map(p,total);return error(c,"cannot commit library call stack");}
#endif
    *base=p;*size=total;return 0;
}
static int library_invoke_frame(void *owner,const void *raw,const us_export_frame *values) {
    us_context *c=owner;
    if(!c || !c->image || !raw || !values || (!values->slots && values->count) ||
       values->count>US_LIBRARY_STACK_ARGUMENT_LIMIT || values->mode>1 ||
       (values->mode==0 && values->count>6) || values->result_kind>6 ||
       (values->result_kind && !values->result) ||
       (values->result_kind==5 && (!values->result_bytes || values->result_bytes>16U*1024U*1024U)) ||
       (values->result_kind && values->result_kind!=5 && values->result_bytes!=8))
        return library_invocation_error(c,"invalid declared library call frame");
    /* Reentry is permitted only inside a declared native call's script frame,
       never while a compiler/loader owns thread-local runtime state. */
    if(active && !script_frames)return library_invocation_error(c,"execution during compilation is not supported");
    unsigned char *stack=0;size_t stack_size=0;int nested=script_frames!=0;
    if(nested){if(library_stack_alloc(c,&stack,&stack_size))return library_invocation_error(c,NULL);}
    else {
        if(!c->call_stack && library_stack_alloc(c,&c->call_stack,&c->call_stack_size))return library_invocation_error(c,NULL);
        stack=c->call_stack;stack_size=c->call_stack_size;
    }
    us_call_outcome *outcome=calloc(1,sizeof *outcome);
    if(!outcome){if(nested)library_release_map(stack,stack_size);return library_invocation_error(c,"call outcome allocation failed");}
    int same_owner_parent=0;
    for(ScriptFrame *p=script_frames;p;p=p->previous)if(p->owner==c){same_owner_parent=1;break;}
    if(!same_owner_parent){c->error[0]=0;c->call_failed=0;c->call_exited=0;c->call_exit_status=0;}
    if(values->result_kind)memset(values->result,0,values->result_bytes);
    us_context *previous_active=active;
    ScriptFrame frame;frame.previous=script_frames;frame.status=0;frame.exited=0;frame.failed=0;
    frame.native_arenas=NULL;frame.stack_base=stack;frame.stack_size=stack_size;frame.owner=c;frame.outcome=outcome;
    frame.argument_signature=NULL;frame.argument_slots=NULL;
    if(library_callable_scopes&&library_callable_scopes->owner==c&&library_callable_scopes->values==values){
        frame.argument_signature=library_callable_scopes->signature;frame.argument_slots=values->slots;
    }
    active=c;script_frames=&frame;
    int exited=setjmp(frame.returned);
    if(!exited) {
        size_t page=library_page_size();
        void *top=stack+stack_size-page;uint64_t value=0;
        if(values->mode==1) {
            if(!us_library_call_stack(raw,values->slots,top,stack_size-2*page,
                                      (size_t)values->count,&value)) {
                frame.failed=1;error(c,"declared library call exceeds private stack");goto finished;
            }
        } else {
            uint64_t slots[6]={0};
            for(size_t i=0;i<(size_t)values->count;i++)slots[i]=values->slots[i];
            value=us_library_bridge_raw(raw,slots,top);
        }
        /* Move the declared result while this call frame is still alive.
           For current script aggregates the model returns its image-owned
           return buffer address; no host ABI classification happens here. */
        if(values->result_kind==5) {
            uintptr_t start=(uintptr_t)c->image+(uintptr_t)c->image_dataoff,at=(uintptr_t)value;
            (void)start;
            if(!library_frame_region(c,&frame,(uint64_t)at,values->result_bytes)) {
                frame.failed=1;error(c,"aggregate return is outside owned script image");goto finished;
            }
            memcpy(values->result,(const void *)at,values->result_bytes);
        } else if(values->result_kind)memcpy(values->result,&value,8);
    }
finished:
    if(frame.failed || frame.exited){
        us_call_outcome current={0};current.failed=frame.failed;current.exited=frame.exited;current.exit_status=frame.status;
        if(frame.exited)snprintf(current.message,sizeof current.message,"script exited with status %d",frame.status);
        else snprintf(current.message,sizeof current.message,"%s",c->error);
        us_call_outcome_merge(outcome,&current);
    }
    script_frame_cleanup(&frame);script_frames=frame.previous;active=previous_active;
    if(nested)library_release_map(stack,stack_size);
    int failed=outcome->failed || outcome->exited;
    if(failed){
        if(values->result_kind)memset(values->result,0,values->result_bytes);
        library_boundary_note(c,outcome);
        /* Legacy fixed-GP hostcalls do not create a typed ffi boundary. Keep
           their nested script failure in the nearest owner invocation too;
           native code returns normally before its caller observes failure. */
        for(ScriptFrame *p=frame.previous;p;p=p->previous)if(p->owner==c){
            us_call_outcome_merge(p->outcome,outcome);break;
        }
    }
    if(!same_owner_parent){
        c->call_exited=outcome->exited;c->call_exit_status=outcome->exited ? outcome->exit_status:0;c->call_failed=failed;
        snprintf(c->error,sizeof c->error,"%s",failed ? outcome->message:"");
    }
    free(outcome);return failed;
}
static int library_invoke(void *owner,const void *raw,const uint64_t slots[6],uint64_t *result) {
    us_export_frame frame={slots,6,0,1,8,result};
    return library_invoke_frame(owner,raw,&frame);
}
static int library_initialise(us_context *c) {
    if(c->initialised==1)return 0;
    if(c->initialised)return error(c,"library initialisation failed or reentered");
    const void *raw=0;int kind=-1;uint64_t args[6]={0},ignored=0;
    if(library_lookup(c,"__init",&raw,&kind) || kind)return error(c,"missing model library initialisation entry");
    c->initialised=2;
    if(library_invoke(c,raw,args,&ignored)){c->initialised=-1;return 1;}
    c->initialised=1;return 0;
}
API void *us_sym(us_context *c,const char *name) {
    if(!c || !name || !c->image){error(c,"library is not relocated");return 0;}
    if(library_initialise(c))return 0;
    c->error[0]=0;
    for(size_t i=0;i<c->exports.count;i++)if(!strcmp(c->exports.items[i].name,name)){
        us_export *x=c->exports.items+i;
        if(x->version==2&&(us_export_supported(x)||(us_export_has_callbacks(x)&&us_export_bridge_supported(x)))&&!x->linkage&&x->defined==1){
            const void *raw=NULL;int kind=-1;us_export_signature view;uint64_t h=0;void *code=NULL;
            if(library_lookup(c,name,&raw,&kind)||kind||us_callable_export_signature(x,&view)||
               us_callable_make(&c->callables,US_CALLABLE_SCRIPT,&view,(uintptr_t)raw,&h,c->error,sizeof c->error)||
               us_callable_pointer(&c->callables,h,&view,&code,c->error,sizeof c->error))return NULL;
            return code;
        }
        if(x->version==2&&!x->linkage&&x->defined==1&&!x->variadic){
            LibraryCarrierExport *entry=c->carrier_exports;
            while(entry&&entry->source!=x)entry=entry->next;
            if(entry){
                int rc;
                const void *raw=NULL;int kind=-1;us_export_signature view;uint64_t h=0;void *code=NULL;
                rc=library_lookup(c,name,&raw,&kind)||kind||us_callable_export_signature(x,&view)||
                   us_carrier_certificate_make(&entry->certificate,&c->callables,US_CALLABLE_SCRIPT,&view,(uintptr_t)raw,&h,c->error,sizeof c->error)||
                   us_callable_pointer(&c->callables,h,&view,&code,c->error,sizeof c->error);
                return rc?NULL:code;
            }
        }
        break;
    }
    return us_exports_symbol_frame(&c->exports,name,c,library_lookup,library_invoke_frame,c->error,sizeof c->error);
}
/* A caller-supplied complete ABI is a fixed entry specialization, not a
   universal variadic closure. The model export supplies the real prototype. */
API void *us_sym_typed(us_context *c,const char *name,const void *signature,size_t length) {
    if(!c||!name||!c->image){error(c,"library is not relocated");return NULL;}
    c->error[0]=0;us_exports declared={0};void *code=NULL;
    if(us_exports_load_bridge(&declared,signature,length,c->error,sizeof c->error))return NULL;
    if(declared.count!=1||declared.items[0].version!=2||strcmp(declared.items[0].name,name)){
        error(c,"invalid concrete export declaration");goto done;
    }
    us_export *source=NULL;
    for(size_t i=0;i<c->exports.count;i++)if(!strcmp(c->exports.items[i].name,name)){source=c->exports.items+i;break;}
    us_export_signature proto,concrete;
    if(!source||source->linkage||source->defined!=1||us_callable_export_signature(source,&proto)||
       us_callable_export_signature(declared.items,&concrete)||!us_callable_concrete_valid(&proto,&concrete)){
        error(c,"incompatible concrete variadic export signature");goto done;
    }
    if(library_initialise(c))goto done;
    const void *raw=NULL;int kind=-1;uint64_t handle=0;
    if(library_lookup(c,name,&raw,&kind)||kind||
       us_callable_make(&c->callables,US_CALLABLE_SCRIPT,&concrete,(uintptr_t)raw,&handle,c->error,sizeof c->error)||
       us_callable_pointer(&c->callables,handle,&concrete,&code,c->error,sizeof c->error))code=NULL;
 done:us_exports_clear(&declared);return code;
}
API int us_call_status(const us_context *c,int *exit_status) {
    if(!c)return 1;
    if(exit_status)*exit_status=c->call_exit_status;
    return c->call_failed;
}
API int us_run_main(us_context *c,int argc,const char *const *argv,int *status) {
    if (!c || !status || argc<0 || (argc && !argv)) return error(c,"invalid main arguments");
    if (active) return error(c,"recursive script execution not yet supported");
    char **args=calloc((size_t)argc+2,sizeof(char *));if (!args) return error(c,"out of memory");
    for (int i=0;i<argc;i++) {
        if (!argv[i] || !(args[i]=copy_string(argv[i]))) {
            for (int j=0;j<i;j++) free(args[j]);free(args);return error(c,"invalid argument or out of memory");
        }
    }
    for (int i=0;i<c->argc;i++) free(c->argv[i]);free(c->argv);c->argv=args;c->argc=argc;
    c->process_slots[0]=argc;c->process_slots[1]=(uintptr_t)args;
    int rc=c->image ? 0 : us_relocate(c);if(rc)return rc;
    if(library_initialise(c))return 1;
    const void *raw=0;int kind=-1;
    if(library_lookup(c,"main",&raw,&kind) || kind)return error(c,"no main function in library");
    uint64_t slots[6]={(uint64_t)argc,(uintptr_t)c->argv,0,0,0,0},result=0;
    rc=library_invoke(c,raw,slots,&result);
    if(rc && !c->call_exited)return rc;
    *status=c->call_exited ? c->call_exit_status : (int)result;
    c->error[0]=0;return 0;
}

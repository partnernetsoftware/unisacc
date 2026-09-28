/* Host adaptation for the same product network routes. No parser or
   compilation rule lives here. One context may be used by one thread at a
   time; different contexts execute concurrently, without a global lock. */
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
#include "librarybindings.h"

typedef struct Allocation { void *p; struct Allocation *next, *prev, *hash_next; } Allocation;
typedef struct GuestMap { void *base; size_t length; struct GuestMap *next; } GuestMap;
typedef struct Symbol { char *name; uintptr_t address; int kind; } Symbol;
typedef struct Source { char *name, *bytes; struct Source *next; } Source;
struct us_context {
    char *package, *definitions, *include_path, *target;
    Source *sources;
    int source_count, optimisation;
    unsigned char *tape;
    size_t tape_length, definitions_length;
    unsigned char *signatures; size_t signatures_length;
    int input_is_tape;
    unsigned char *image; long image_size, image_dataoff, image_text_size; int image_entry;
    us_exports exports;
    us_bindings bindings;
    unsigned char *binding_blob; size_t binding_length;
    unsigned char *call_stack; size_t call_stack_size;
    int initialised, call_exited, call_exit_status, call_failed;
    Symbol *symbols; size_t symbol_count;
    GuestMap *guest_maps;
    int argc; char **argv;
    long process_slots[2];
    char error[1024];
};
static _Thread_local us_context *active;
static _Thread_local Allocation *allocations;
#define ALLOCATION_BUCKETS 4096
static _Thread_local Allocation *allocation_buckets[ALLOCATION_BUCKETS];
typedef struct OpenFile { int fd; struct OpenFile *next; } OpenFile;
static _Thread_local OpenFile *open_files;
static _Thread_local jmp_buf failure;
static void __us_panic(const char *message) {
    snprintf(active->error,sizeof active->error,"%s",message);
    longjmp(failure,1);
}
static int tracked_open(const char *path,int flags) {
    int fd=open(path,flags);if (fd<0) return fd;
    OpenFile *node=malloc(sizeof *node);
    if (!node) {close(fd);__us_panic("out of memory");}
    node->fd=fd;node->next=open_files;open_files=node;return fd;
}
static int tracked_close(int fd) {
    OpenFile **at=&open_files;
    while (*at && (*at)->fd!=fd) at=&(*at)->next;
    if (*at) {OpenFile *node=*at;*at=node->next;free(node);}
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
    while (open_files) {OpenFile *node=open_files;open_files=node->next;close(node->fd);free(node);}
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

static void diagnostic(int status,const char *reason,int n,const void *errors) {
    if (!status) return;
    const Buf *e=errors;
    if (reason && (n || *reason)) snprintf(active->error,sizeof active->error,"%.*s",n ? n : (int)strlen(reason),reason);
    else if (e && e->n) snprintf(active->error,sizeof active->error,"%.*s",e->n,(const char *)e->b);
    else snprintf(active->error,sizeof active->error,"compiler status %d",status);
}

#if defined(__GNUC__)
#define API __attribute__((visibility("default")))
#else
#define API
#endif
static void discard_image(us_context *c) {
    us_exports_clear(&c->exports);c->initialised=0;
    if(c->call_stack){munmap(c->call_stack,c->call_stack_size);c->call_stack=0;c->call_stack_size=0;}
    while (c->guest_maps) {GuestMap *m=c->guest_maps;c->guest_maps=m->next;munmap(m->base,m->length);free(m);}
    if (c->image) {
#ifdef _WIN32
        /* Windows context library adaptation is a later platform slice. */
        __us_panic("Windows library unload not implemented");
#else
        munmap(c->image,(size_t)c->image_size);
#endif
        c->image=0;
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
    us_bindings_clear(&c->bindings);free(c->binding_blob);
    free(c->package); free(c->definitions); free(c->target);
    for (Source *s=c->sources;s;) { Source *next=s->next; free(s->name); free(s->bytes); free(s); s=next; }
    for (int i=0;i<c->argc;i++) free(c->argv[i]); free(c->argv);
    free(c->include_path); free(c->tape); free(c->signatures); free(c);
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
    int rc=sig->kind ? us_bindings_add_data(&c->bindings,name,(uintptr_t)address,&result,
                      sig->extent,sig->writable,c->error,sizeof c->error) :
        us_bindings_add_function(&c->bindings,name,(uintptr_t)address,&result,args,sig->count,
                                sig->variadic,c->error,sizeof c->error);
    free(args);if(rc)return rc;
    discard_image(c);free(c->tape);c->tape=0;c->tape_length=0;
    free(c->signatures);c->signatures=0;c->signatures_length=0;
    free(c->binding_blob);c->binding_blob=0;c->binding_length=0;
    free(c->target);c->target=0;c->error[0]=0;return 0;
}
API int us_compile(us_context *c,const char *target,int level) {
    if (!c || !target || level<0 || level>2) return error(c,"invalid compilation options");
    if (!c->sources && !c->input_is_tape) return error(c,"no input");
    if (active) return error(c,"recursive compilation not yet supported");
    c->error[0]=0;
    free(c->binding_blob);c->binding_blob=0;c->binding_length=0;
    if(c->bindings.count && us_bindings_serialize(&c->bindings,&c->binding_blob,&c->binding_length,c->error,sizeof c->error))return 1;
    if(c->binding_length>=INT_MAX)return error(c,"bindings too large");
    discard_image(c);
    char *chosen=copy_string(target); if (!chosen) return error(c,"out of memory");
    free(c->target); c->target=chosen; c->optimisation=level;
    if (c->input_is_tape) return 0;
    free(c->tape);c->tape=0;c->tape_length=0;free(c->signatures);c->signatures=0;c->signatures_length=0;
    active=c; volatile int rc=1;
    if (!setjmp(failure)) {
        /* Reinitialise thread-local adapter inputs on every invocation. */
        RI=0; NRI=0; NR=0; FILE_READ_RECORD=0; FILE_READ_COUNT=0; FILE_READ_PATHS=0;
        INCDIR=c->include_path;
        package(c->package);
        ResourceInput resources[6]; memset(resources,0,sizeof resources);
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
            if (input.n>=9 && !memcmp(input.b,"USLTAPE1\n",9)) {
                size_t at=9;uint64_t tape=library_u64(&input,&at),meta=library_u64(&input,&at);
                if (tape>(size_t)input.n-at || meta!=(size_t)input.n-at-tape || meta<16 ||
                    memcmp(input.b+at+(size_t)tape,"USLSIG1\n",8)) __us_panic("bad library tape envelope");
                c->signatures=malloc((size_t)meta);if (!c->signatures) __us_panic("out of memory");
                memcpy(c->signatures,input.b+at+(size_t)tape,(size_t)meta);c->signatures_length=(size_t)meta;
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
    cleanup(); active=0; RI=0; NRI=0; return rc;
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
    long dataoff=((long)m->text+16383)&-16384;
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
typedef struct ScriptFrame {
    jmp_buf returned;
    volatile int status;
    struct ScriptFrame *previous;
} ScriptFrame;
static _Thread_local ScriptFrame *script_frames;
/* Host memory lifetime adaptation. The model decides which operation and
   argument sequence to issue; these callbacks never inspect source or tape. */
static long library_mmap(long addr,long length,long prot,long flags,long fd,long offset) {
    if (!active || length<=0 || (flags&MAP_FIXED)) return -EINVAL;
    GuestMap *node=malloc(sizeof *node);if (!node) return -ENOMEM;
    void *p=mmap((void *)addr,(size_t)length,(int)prot,(int)flags,(int)fd,(off_t)offset);
    if (p==MAP_FAILED) {int code=errno;free(node);return -code;}
    long page=sysconf(_SC_PAGESIZE);
    if (page<=0 || (size_t)length>SIZE_MAX-(size_t)page+1) {munmap(p,(size_t)length);free(node);return -EINVAL;}
    node->base=p;node->length=((size_t)length+(size_t)page-1)/(size_t)page*(size_t)page;
    node->next=active->guest_maps;active->guest_maps=node;
    return (long)p;
}
static long library_munmap(long addr,long length) {
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
}
static long library_exit(long status) {
    if(!script_frames)return -EINVAL;
    script_frames->status=(int)status;longjmp(script_frames->returned,1);
}
static const char *library_native_target(void) {
#ifdef __aarch64__
#ifdef __APPLE__
    return "osx/arm64";
#else
    return "lnx/arm64";
#endif
#else
#ifdef __APPLE__
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
    discard_image(c);c->error[0]=0;active=c;volatile int rc=1;
    /* Mapping identity must survive longjmp on a malformed model output. */
    if (!setjmp(failure)) {
        RI=0;NRI=0;NR=0;FILE_READ_RECORD=0;FILE_READ_COUNT=0;FILE_READ_PATHS=0;
        package(c->package);
        MemoryMap mapping={0};memory_reserve(&mapping);
        c->image=mapping.base;c->image_size=mapping.reserved;
        /* Four loader slots and eight scalar resources. */
        ResourceInput actual[16];unsigned char scalar[14][8];memset(actual,0,sizeof actual);
        const char *names[]={"\0process/argc","\0process/argv","\0memory/text","\0memory/reserve",
            "\0library/symbols","\0library/exit","\0library/mmap","\0library/munmap","\0process/dl/0","\0process/dl/1","\0process/dl/2","\0process/dl/3","\0library/module","\0library/process"};
        long vals[]={c->argc,(long)c->argv,(long)mapping.base,mapping.reserved,1,(long)library_exit,(long)library_mmap,(long)library_munmap,
            host_dl_slot(0),host_dl_slot(1),host_dl_slot(2),host_dl_slot(3),1,(long)c->process_slots};
        int lengths[]={13,13,12,15,16,13,13,15,13,13,13,13,15,16};
        for (int i=0;i<14;i++) {resource_u64(scalar[i],vals[i]);actual[i].name=(const unsigned char *)names[i];actual[i].n=lengths[i];actual[i].data=scalar[i];actual[i].len=8;}
        RI=actual;NRI=14;
        if (c->signatures) {
            actual[NRI].name=(const unsigned char *)"\0library/signatures";actual[NRI].n=19;
            actual[NRI].data=c->signatures;actual[NRI].len=(int)c->signatures_length;NRI++;
        }
        if(c->binding_blob){
            actual[NRI].name=(const unsigned char *)"\0library/bindings";actual[NRI].n=17;
            actual[NRI].data=c->binding_blob;actual[NRI].len=(int)c->binding_length;NRI++;
        }
        Buf input={0};input.n=(int)c->tape_length;input.b=tracked_realloc(0,c->tape_length);memcpy(input.b,c->tape,c->tape_length);
        char route[128];int z=snprintf(route,sizeof route,"%s/run/O%d",c->target,c->optimisation);
        if (z<0 || z>=(int)sizeof route) __us_panic("target too long");
        rc=runroute_from(route,"prune",&input,c->sources ? c->sources->name : "library.tape");
        if (!rc) {snprintf(route,sizeof route,"%s/memory",c->target);rc=runroute(route,&input,"library.tape");}
        if (!rc) {
            MemoryImage m;library_image(c,&input,&m,&mapping);memory_commit(&m,&mapping);
            c->image_dataoff=mapping.dataoff;c->image_entry=m.entry;c->image_text_size=m.text;
            memcpy(mapping.base,input.b+40,m.text);memcpy(mapping.base+mapping.dataoff,input.b+40+m.text,m.stored);
            __builtin___clear_cache((char *)mapping.base,(char *)mapping.base+m.text);
            if (mprotect(mapping.base,(size_t)mapping.dataoff,PROT_READ|PROT_EXEC)) __us_panic("cannot protect library code");
            if(c->signatures && us_exports_load(&c->exports,c->signatures,c->signatures_length,c->error,sizeof c->error)) rc=1;
        }
    } else rc=1;
    cleanup();active=0;RI=0;NRI=0;if (rc) discard_image(c);return rc;
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
    long page=sysconf(_SC_PAGESIZE);size_t usable=16U*1024U*1024U;
    if(page<=0 || (size_t)page>SIZE_MAX/2)return error(c,"invalid host page size");
    size_t total=usable+2*(size_t)page;
    unsigned char *p=mmap(0,total,PROT_NONE,MAP_PRIVATE|MAP_ANON,-1,0);
    if(p==MAP_FAILED)return error(c,"cannot reserve library call stack");
    if(mprotect(p+page,usable,PROT_READ|PROT_WRITE)){munmap(p,total);return error(c,"cannot commit library call stack");}
    *base=p;*size=total;return 0;
}
static int library_invoke(void *owner,const void *raw,const uint64_t slots[6],uint64_t *result) {
    us_context *c=owner;
    if(c){c->call_failed=1;c->call_exited=0;c->call_exit_status=0;}
    if(!c || !c->image || !raw || !slots || !result)return error(c,"invalid library call");
    /* Reentry is permitted only inside a declared native call's script frame,
       never while a compiler/loader owns thread-local runtime state. */
    if(active && !script_frames)return error(c,"execution during compilation is not supported");
    unsigned char *stack=0;size_t stack_size=0;int nested=script_frames!=0;
    if(nested){if(library_stack_alloc(c,&stack,&stack_size))return 1;}
    else {
        if(!c->call_stack && library_stack_alloc(c,&c->call_stack,&c->call_stack_size))return 1;
        stack=c->call_stack;stack_size=c->call_stack_size;
    }
    c->error[0]=0;*result=0;
    us_context *previous_active=active;
    ScriptFrame frame;frame.previous=script_frames;frame.status=0;
    active=c;script_frames=&frame;
    int exited=setjmp(frame.returned);
    if(!exited) {
        size_t page=(size_t)sysconf(_SC_PAGESIZE);
        *result=us_library_bridge_raw(raw,slots,stack+stack_size-page);
    }
    script_frames=frame.previous;active=previous_active;
    if(nested)munmap(stack,stack_size);
    c->call_exited=exited ? 1 : 0;c->call_exit_status=exited ? frame.status : 0;
    c->call_failed=c->call_exited;
    if(exited)snprintf(c->error,sizeof c->error,"script exited with status %d",frame.status);
    else c->error[0]=0;
    return c->call_failed;
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
    return us_exports_symbol(&c->exports,name,c,library_lookup,library_invoke,c->error,sizeof c->error);
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
    c->process_slots[0]=argc;c->process_slots[1]=(long)args;
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

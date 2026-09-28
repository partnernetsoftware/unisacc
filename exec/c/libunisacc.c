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

typedef struct Allocation { void *p; struct Allocation *next; } Allocation;
struct us_context {
    char *package, *name, *source, *definitions, *include_path;
    unsigned char *tape;
    size_t tape_length, definitions_length;
    int input_is_tape;
    char error[1024];
};
static _Thread_local us_context *active;
static _Thread_local Allocation *allocations;
static _Thread_local jmp_buf failure;
static void panic(const char *message) {
    snprintf(active->error,sizeof active->error,"%s",message);
    longjmp(failure,1);
}
/* Track all runtime allocations, including partial model loads on errors.
   The host context itself is outside this per-compilation cleanup domain. */
static void *tracked_realloc(void *p,size_t n) {
    Allocation *a=p ? allocations : 0;
    if (p) { while (a && a->p!=p) a=a->next; if (!a) panic("unowned runtime allocation"); }
    void *q=realloc(p,n ? n : 1);
    if (!q) panic("out of memory");
    if (!a) { a=malloc(sizeof *a); if (!a) { free(q); panic("out of memory"); }
        a->next=allocations; allocations=a; }
    a->p=q; return q;
}
static void *tracked_calloc(size_t count,size_t size) {
    if (size && count>SIZE_MAX/size) panic("allocation overflow");
    size_t n=count*size; void *p=tracked_realloc(0,n); memset(p,0,n); return p;
}
static void tracked_free(void *p) {
    if (!p) return;
    Allocation **a=&allocations;
    while (*a && (*a)->p!=p) a=&(*a)->next;
    if (!*a) panic("unowned runtime free");
    Allocation *node=*a; *a=node->next; free(p); free(node);
}
static void cleanup(void) {
    while (allocations) { Allocation *a=allocations; allocations=a->next; free(a->p); free(a); }
}
static void diagnostic(int status,const char *reason,int n,const void *errors);
#define UNISA_RUNTIME_LIBRARY
#define UNISA_RUNTIME_STATE static _Thread_local
#define UNISA_RUNTIME_PANIC(message) panic(message)
#define UNISA_RUNTIME_DIAGNOSTIC(status,reason,n,err) diagnostic(status,reason,n,err)
#define core_host_fetch __us_core_host_fetch
#define core_host_panic __us_core_host_panic
#define core_run __us_core_run
#define realloc tracked_realloc
#define calloc tracked_calloc
#define free tracked_free
#include "run.c"
#undef realloc
#undef calloc
#undef free

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
    free(c->package); free(c->name); free(c->source); free(c->definitions);
    free(c->include_path); free(c->tape); free(c);
}
API int us_add_source(us_context *c,const char *name,const char *source) {
    if (!c || !name || !source) return error(c,"missing source");
    if (c->source || c->input_is_tape) return error(c,"multiple inputs not yet implemented");
    char *n=copy_string(name),*s=copy_string(source);
    if (!n || !s) { free(n); free(s); return error(c,"out of memory"); }
    c->name=n; c->source=s; return 0;
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
    if (!c || !bytes || length>=INT_MAX || c->source || c->input_is_tape) return error(c,"invalid tape input");
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
API int us_compile(us_context *c,const char *target,int level) {
    if (!c || !target || level<0 || level>2) return error(c,"invalid compilation options");
    if (!c->source && !c->input_is_tape) return error(c,"no input");
    if (active) return error(c,"recursive compilation not yet supported");
    c->error[0]=0;
    if (c->input_is_tape) return 0;
    active=c; volatile int rc=1;
    if (!setjmp(failure)) {
        /* Reinitialise thread-local adapter inputs on every invocation. */
        RI=0; NRI=0; NR=0; FILE_READ_RECORD=0; FILE_READ_COUNT=0; FILE_READ_PATHS=0;
        INCDIR=c->include_path;
        package(c->package);
        ResourceInput resource;
        if (c->definitions) {
            resource.name=(const unsigned char *)"\0cli/defines"; resource.n=12;
            resource.data=(const unsigned char *)c->definitions; resource.len=(int)c->definitions_length;
            RI=&resource; NRI=1;
        }
        size_t n=strlen(c->source); if (n>=INT_MAX) panic("source too large");
        Buf input={0}; input.b=tracked_realloc(0,n); memcpy(input.b,c->source,n); input.n=(int)n;
        char route[128];
        int z=snprintf(route,sizeof route,"%s/tape/O%d",target,level);
        if (z<0 || z>=(int)sizeof route) panic("target too long");
        rc=runroute(route,&input,c->name);
        if (!rc) {
            unsigned char *out=malloc(input.n ? (size_t)input.n : 1);
            if (!out) panic("out of memory");
            memcpy(out,input.b,input.n); free(c->tape); c->tape=out; c->tape_length=(size_t)input.n;
        }
    }
    cleanup(); active=0; RI=0; NRI=0; return rc;
}

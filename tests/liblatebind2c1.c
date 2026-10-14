/* liblatebind2c1 PACKAGE TARGET -- library late binding, step 2c1: data symbols.
 * Drafted by cdx3; query order and diagnostics follow research/c43-late-binding-cut2c1-step0.md.
 * Built and run by tests/liblatebindcheck.cx once exec/c/libunisacc.h declares the 2c1 entry points.
 * (native host, 32-bit int).
 * Structure follows tests/liblatebind2b.c, but includes only the public header:
 * no private context fields, no adapter/RI/model/stamp inspection.
 *
 * Entry points (non-stable, named in cut2c1 step 0):
 *   us_declare_injected_data(c, name, requirement)
 *     accepts kind=1, declares injected-only; no address/provider lookup.
 *   us_register_data_provider(c, name, address, independent_offer)
 *     registers a structurally valid offer independently of requirement.
 *     Requirement/offer mismatch is checked at relocate, not hidden by an
 *     early registration error; host retains object ownership/lifetime.
 *   us_library_data_fragment(c, &n)
 *     model-marked E3 data-reference fragment, nonempty for used hostvalue.
 *   us_library_data_lower_unbound(c, &n)
 *     model-marked unbound data lowering + its relocation spans, not whole
 *     lower; declaration/type/permission checking, no provider lookup.
 *
 * Diagnostics and stages (cut2c1 step 0):
 *   readonly-write (compile or unbound-lower, before binding);
 *   data-extent-mismatch / data-type-mismatch / missing-provider (relocate).
 * All must name hostvalue; missing-provider should also carry stable symbol ID.
 * Current draft checks code+name, not an invented diagnostic grammar for ID.
 * A rejected context publishes neither exported function and never mutates host
 * data. Unsupported target/capability is NOT an expected negative pass.
 *
 * Proof: contract, model-selected E3 data refs, and unbound data fragment only.
 * Runtime bridges/mapping/argv and other unmigrated classes are out of scope.
 * This is deliberately not linked/run against today's missing placeholder APIs.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include "exec/c/libunisacc.h"

extern int us_declare_injected_data(us_context *, const char *, const us_signature *);
extern int us_register_data_provider(us_context *, const char *, void *, const us_signature *);
extern const void *us_library_data_fragment(us_context *, size_t *);
extern const void *us_library_data_lower_unbound(us_context *, size_t *);

#define ERR_RO      "readonly-write"       /* cut2c1 step 0 */
#define ERR_EXTENT  "data-extent-mismatch" /* cut2c1 step 0 */
#define ERR_TYPE    "data-type-mismatch"   /* cut2c1 step 0 */
#define ERR_MISSING "missing-provider"
#define NAME "hostvalue"
_Static_assert(sizeof(int) == 4 && sizeof(uintptr_t) == 8, "draft needs int32/native64");

struct cell { uint32_t left; int value; uint32_t right; };
static struct cell object_a = {0x12345678u, 11, 0x87654321u};
static struct cell object_b = {0x23456789u, 22, 0x98765432u};
static const char *RW = "extern int hostvalue;\n"
    "int read_value(void){return hostvalue;}\n"
    "int write_value(int n){hostvalue=n;return hostvalue;}\n";
static const char *READ = "extern int hostvalue;\nint read_value(void){return hostvalue;}\n";
static int bad;
static void report(int ok, const char *what) {
    printf("  %s %s\n", ok ? "ok  " : "FAIL", what); if (!ok) bad++;
}
static us_signature data_signature(uint64_t extent, unsigned writable, unsigned uns) {
    us_signature s; memset(&s, 0, sizeof s); s.kind=1;
    s.result.kind=1; s.result.width=4; s.result.uns=uns;
    s.extent=extent; s.writable=writable; return s;
}
struct snapshot { unsigned char *p; size_t n; };
static struct snapshot save(const void *p, size_t n) {
    struct snapshot s={0,0}; if(!p || !n) return s;
    s.p=malloc(n); if(s.p){memcpy(s.p,p,n);s.n=n;} return s;
}
static int same(struct snapshot a, struct snapshot b) {
    return a.p && b.p && a.n && a.n==b.n && !memcmp(a.p,b.p,a.n);
}
static int contains(struct snapshot s, const void *v, size_t n) {
    size_t i; if(!s.p || n>s.n) return 0;
    for(i=0;i+n<=s.n;i++)if(!memcmp(s.p+i,v,n))return 1;
    return 0;
}
/* Tape/lower are text: raw little-endian-only checks would miss imm DECIMAL.
 * Check both byte orders, unsigned/signed decimal and upper/lower hex digits.
 * Exact unbound-span/role/ID validation remains the producer/binder's job. */
static int address_free(struct snapshot s, uintptr_t address) {
    unsigned char le[8],be[8];char u[64],d[64],h[64],H[64];int k;
    for(k=0;k<8;k++){le[k]=(unsigned char)((uint64_t)address>>(8*k));be[7-k]=le[k];}
    snprintf(u,sizeof u,"%" PRIuPTR,address);
    snprintf(d,sizeof d,"%" PRId64,(int64_t)(uint64_t)address);
    snprintf(h,sizeof h,"%" PRIxPTR,address);snprintf(H,sizeof H,"%" PRIXPTR,address);
    return s.p && s.n && !contains(s,le,8) && !contains(s,be,8) &&
        !contains(s,u,strlen(u)) && !contains(s,d,strlen(d)) &&
        !contains(s,h,strlen(h)) && !contains(s,H,strlen(H));
}
static int clean_pair(struct snapshot a, struct snapshot b, uintptr_t x, uintptr_t y) {
    return address_free(a,x) && address_free(a,y) && address_free(b,x) && address_free(b,y);
}
static int named_error(us_context *c,const char *code) {
    const char *e=us_error(c);int ok=e && strstr(e,code) && strstr(e,NAME);
    printf("  diagnostic %s\n",e?e:"<none>");return ok;
}
static int no_exports(us_context *c) {
    /* Called only AFTER copying/checking the original diagnostic: sym may
       change us_error. Never invoke any function in a failed context. */
    void *r=us_sym(c,"read_value"), *w=us_sym(c,"write_value");return !r && !w;
}
static int declare_source(us_context *c,const us_signature *s,const char *src) {
    return !us_declare_injected_data(c,NAME,s) && !us_add_source(c,"data.c",src);
}
static struct snapshot contract(us_context *c,const char *target) {
    size_t n=0;const void *p=us_binding_contract(c,target,&n);return save(p,n);
}
static struct snapshot e3(us_context *c) {
    size_t n=0;const void *p=us_library_data_fragment(c,&n);return save(p,n);
}
static struct snapshot lower(us_context *c) {
    size_t n=0;const void *p=us_library_data_lower_unbound(c,&n);return save(p,n);
}
static int prepare_views(us_context *c,const char *target) {
    struct snapshot e,l;int ok;
    (void)l;
    if(us_compile(c,target,1))return 0;
    e=e3(c);ok=e.p && e.n;free(e.p);return ok;
}
static int positive(const char *package,const char *target) {
    us_context *a=us_new(package),*b=us_new(package);us_signature s=data_signature(4,1,0);
    struct snapshot ca={0,0},cb={0,0},ea={0,0},eb={0,0},la={0,0},lb={0,0};
    uintptr_t pa=(uintptr_t)&object_a.value,pb=(uintptr_t)&object_b.value;int ok=0;
    int (*ra)(void)=0,(*rb)(void)=0,(*wa)(int)=0,(*wb)(int)=0;
    if(!a || !b || !declare_source(a,&s,RW) || !declare_source(b,&s,RW) ||
       us_register_data_provider(a,NAME,&object_a.value,&s) ||
       us_register_data_provider(b,NAME,&object_b.value,&s)) {
        report(0,"positive setup failed (not a negative pass)");goto done;
    }
    ca=contract(a,target);cb=contract(b,target);
    report(pa!=pb && same(ca,cb),"contract: distinct providers, byte-identical nonempty declarations");
    if(us_compile(a,target,1) || us_compile(b,target,1)){
        report(0,"positive compile failed");goto done;
    }
    ea=e3(a);eb=e3(b);
    report(same(ea,eb),"E3: model-marked data references identical and nonempty");
    /* the unbound lower view is a read-only receipt produced by us_relocate */
    if(us_relocate(a) || us_relocate(b)){report(0,"positive relocate failed");goto done;}
    la=lower(a);lb=lower(b);
    report(same(la,lb),"lower: model-marked unbound data/spans identical and nonempty");
    report(clean_pair(ca,cb,pa,pb) && clean_pair(ea,eb,pa,pb) && clean_pair(la,lb,pa,pb),
           "clean: all six snapshots contain neither provider address, in bytes or text");
    if(bad)goto done;
    ra=(int(*)(void))us_sym(a,"read_value");rb=(int(*)(void))us_sym(b,"read_value");
    wa=(int(*)(int))us_sym(a,"write_value");wb=(int(*)(int))us_sym(b,"write_value");
    if(!ra || !rb || !wa || !wb){report(0,"positive exports absent");goto done;}
    ok=ra()==11 && rb()==22 && !us_call_status(a,0) && !us_call_status(b,0);
    report(ok,"bound: each context reads its own object");
    ok=wa(101)==101 && object_a.value==101 && object_b.value==22 && rb()==22 &&
       !us_call_status(a,0) && !us_call_status(b,0);
    report(ok,"bound: A write mutates only A");
    ok=wb(202)==202 && object_b.value==202 && object_a.value==101 && ra()==101 &&
       !us_call_status(a,0) && !us_call_status(b,0);
    report(ok,"bound: B write mutates only B");
    report(object_a.left==0x12345678u && object_a.right==0x87654321u &&
           object_b.left==0x23456789u && object_b.right==0x98765432u,"bound: extent guards unchanged");
    ok=1;
done:
    free(ca.p);free(cb.p);free(ea.p);free(eb.p);free(la.p);free(lb.p);us_free(a);us_free(b);return ok;
}
static void readonly_case(const char *package,const char *target) {
    us_context *c=us_new(package);int value=99,ok=0,rc;
    us_signature s=data_signature(4,0,0);struct snapshot l={0,0};
    if(!c || !declare_source(c,&s,RW) || us_register_data_provider(c,NAME,&value,&s)){
        report(0,"readonly setup failed (valid readonly declaration/provider must be accepted)");goto done;
    }
    rc=us_compile(c,target,1);
    if(!rc) rc=us_relocate(c);        /* lowering runs here and refuses before any provider is used */
    ok=rc && named_error(c,ERR_RO);
    if(ok){l=lower(c); ok=!l.p || !l.n;}  /* a model refusal leaves no successful receipt */
    ok=ok && no_exports(c) && value==99;
    report(ok,"readonly: write refused before binding by code/name; no export or host mutation");
done:free(l.p);us_free(c);
}
static void mismatch_case(const char *package,const char *target,int extent_case) {
    us_context *c=us_new(package);int values[2]={77,88};unsigned offered=77;int ok=0;
    us_signature requirement=data_signature(extent_case?8:4,1,0);
    us_signature offer=data_signature(4,1,extent_case?0:1);
    /* Both offers are structurally valid (extent >= width). Extent case offers
       4 bytes for an 8-byte requirement; type case differs only signedness,
       keeping width/extent equal so extent cannot mask the type diagnostic. */
    void *address=extent_case?(void*)values:(void*)&offered;
    if(!c || !declare_source(c,&requirement,READ) ||
       us_register_data_provider(c,NAME,address,&offer)){
        report(0,"mismatch setup failed: independent valid declarations were rejected early");goto done;
    }
    if(!prepare_views(c,target)){
        report(0,"mismatch compile/unbound fragment failed before binding");goto done;
    }
    ok=us_relocate(c)!=0;
    ok=ok && named_error(c,extent_case?ERR_EXTENT:ERR_TYPE);
    ok=ok && no_exports(c) && values[0]==77 && values[1]==88 && offered==77;
    report(ok,extent_case?"extent: undersized provider refused at bind, no export/mutation":
                         "type: independent signedness conflict refused at bind, no export/mutation");
done:us_free(c);
}
static void missing_case(const char *package,const char *target) {
    us_context *c=us_new(package);us_signature s=data_signature(4,1,0);int ok=0;
    struct snapshot d={0,0},e={0,0},l={0,0};
    if(!c || !declare_source(c,&s,READ)) {report(0,"missing declaration/source failed");goto done;}
    d=contract(c,target);
    if(!d.p || !d.n || us_compile(c,target,1)){
        report(0,"missing: contract/compile must succeed without provider");goto done;
    }
    e=e3(c);
    if(!e.p || !e.n){report(0,"missing: the E3 view must be observable after compile");goto done;}
    ok=us_relocate(c)!=0;
    ok=ok && named_error(c,ERR_MISSING);ok=ok && no_exports(c);
    if(ok){l=lower(c); ok=l.p && l.n;}  /* a missing provider keeps the unbound receipt */
    report(ok,"missing: injected-only data stays compiled/unbound, bind refuses by code/name");
done:free(d.p);free(e.p);free(l.p);us_free(c);
}
int main(int argc,char **argv) {
    if(argc!=3){fprintf(stderr,"usage: liblatebind2c1 PACKAGE TARGET\n");return 2;}
    /* Fail fast: a missing placeholder/model is a failure, never KNOWN/SKIP. */
    if(!positive(argv[1],argv[2]) || bad)return 1;
    readonly_case(argv[1],argv[2]);if(bad)return 1;
    mismatch_case(argv[1],argv[2],1);if(bad)return 1;
    mismatch_case(argv[1],argv[2],0);if(bad)return 1;
    missing_case(argv[1],argv[2]);
    printf("liblatebind 2c1 draft: %d failures\n",bad);return bad?1:0;
}

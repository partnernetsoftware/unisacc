/* The Windows driver uses the bundled realpath and temporary-file helpers.
   Their kernel32 calls use the product's Windows host channel. */
/* Product-driver migration: IO and route selection only. All source handling
   is performed by the package's networks. Unsupported CLI remains an explicit
   error until migrated; no call to a reference compiler. Not the default yet. */
#include "../../src/version.h"
#include <limits.h>
/* Match the existing selfcheck fuel budget; explicit environment wins. */
#ifndef UNISA_DEFAULT_MAXSTEPS
#define UNISA_DEFAULT_MAXSTEPS 400000000000LL
#endif
#define UNISA_RUNTIME_LIBRARY
#define UNISA_CX_CODEC
#define UNISA_CX_PACKAGEFOOTER
#define UNISA_CX_ATTRIBUTE_FRAME
#define UNISA_CX_SOURCE_FRAME
#define UNISA_CX_MODEL_NUMBER
#define UNISA_CX_MODEL_LIFETIME
#define UNISA_CX_MODEL_DECODE
#define UNISA_CX_MODELLOAD
#define UNISA_CX_PACKAGE_LIFETIME
#define UNISA_CX_PACKAGE_DECODE
#define UNISA_CX_ROUTESPAN
#define UNISA_CX_ROUTESTEP
#define UNISA_CX_ROUTETIME
#define UNISA_CX_RUNEXEC
#define UNISA_CORE_ASM_TRANSITION
#define UNISA_CORE_EXTERNAL
const char *bx_file_mode(int);
int bx_open_miss(int, int, int);
int bx_env_temp(const char *);
const char *bx_env_dot(const char *);
int bx_env_set(const char *);
const char *bx_env_blank(const char *);
#define UNISA_CX_FILEMODE
#include "run.c"
#undef UNISA_CX_MODEL_LIFETIME
#undef UNISA_CX_MODEL_DECODE
#undef UNISA_CX_MODELLOAD
#undef UNISA_CX_PACKAGE_LIFETIME
#undef UNISA_CX_PACKAGE_DECODE
#undef UNISA_CX_ROUTESPAN
#undef UNISA_CX_ROUTESTEP
#undef UNISA_CX_ROUTETIME
#undef UNISA_CX_RUNEXEC
#undef UNISA_CORE_ASM_TRANSITION
#undef UNISA_CORE_EXTERNAL
#undef UNISA_CX_MODEL_NUMBER
#undef UNISA_CX_ATTRIBUTE_FRAME
#undef UNISA_CX_SOURCE_FRAME
#undef UNISA_CX_PACKAGEFOOTER
#undef UNISA_CX_CODEC
#undef UNISA_RUNTIME_LIBRARY
#include "tapebin.h"
#include "tapebin_emit.h"
/* OS map stays here. Image layout is the linked unit cx/runtime/memlayout.cx. */
#include "memory.c"
#include "../../src/host_dl.h"
#include "bind.h"
int bx_dep_mark(const char *, int, int *, int *);
int bx_dep_len(int, int, int);
const char *bx_dep_why(void);
int bx_dep_only(const char *, int, const char *, const char **);
int bx_dep_gate(const char *, int, int, int);
int bx_dep_target(const char *, int, const char **);
int bx_dep_ret(int, int);
int bx_image_name(const char *, int, int, int, const char **);
int bx_run_enter(int);
int bx_exec_forward(int);
int bx_run_args(int, int, int);
int bx_run_count(int, int);
int bx_sym_out(const char *, int, int, int, const char **);
int bx_diag_opt(const char *, const char *, int, int, int, int);
int bx_diag_run(const char *, int);
const char *bx_same_why(int, int);
int bx_sym_need(int, int, const char *);
int bx_fwd_want(int, int, int, int, int, const char *);
int bx_fwd_maxargs(const char *, int *);
int bx_obj_foreign(int, const char *);
int bx_obj_route(int, int);
int bx_sym_model(int);
int bx_sym_span(int);
int bx_sym_bytes(int);
int bx_sym_head(int, unsigned char *);
int bx_host_skip(int, int);
int bx_host_from(const char *);
int bx_host_side(int, const char *);
int bx_host_kind(int);
int bx_host_fill(int);
int bx_host_bit(int, int, int *);
int bx_host_keyn(void);
int bx_host_elf(int, int);
int bx_tmp_name(const char *, char *, int);
const char *bx_tmp_why(void);
int bx_key_len(int);
int bx_key_fit(int, int, int);
int bx_key_split(const char *, char *, int, char *, int);
int bx_stage_present(const char *, const char *, const char *const *, const char *const *, int);
int bx_stage_out(const char *, const char *, const char *, const char *const *, const char *const *, const char *const *, int);
int bx_filter_none(const char *, unsigned, unsigned *);
int bp_prov_rc(int);
int bp_errno_keep(int, int);
int bp_base_empty(const char *);
int bp_parent_miss(int);
int bp_wrote_over(int, int);
const char *bx_compat_why(int, int, const char *);
int bx_opt_dash(const char *);
const char *bx_opt_why(int, const char *);
const char *bx_src_why(const char *);
const char *bx_mix_why(int);
int bx_tape_need(int, int);
const char *bx_tape_why(void);
int bx_tapebin_bad(int, int);
const char *bx_tapebin_why(void);
int bx_name_fit(int, int);
const char *bx_name_why(void);
const char *bx_cache_why(int, int);
int bx_frame_fit(int, int);
const char *bx_frame_why(void);
int bx_encode_bad(int, int);
const char *bx_encode_why(void);
const char *bx_route_why(void);
const char *bx_model_why(void);
int bx_tool(const char *);
int bx_cli_word(const char *);
int bx_more_src(const char *, int);
int bx_read_act(int);
int bx_read_stdin(const char *);
int bx_as_arch(const char *);
int bx_native_arm(const char *);
const char *bx_read_die(int);
int bx_read_open_line(const char *, const char *, char *, int);
int bx_read_fail_line(const char *, char *, int);
int bx_cli_line(const char *, char *, int);
const char *bx_as_targets(void);
const char *bx_as_usage(void);
const char *bx_view_usage(int);
const char *bx_ar_usage(void);
const char *bx_ar_big(void);
const char *bx_ar_kind(void);
const char *bx_ar_op(void);
int bx_as_opt_line(const char *, char *, int);
int bx_as_write_line(const char *, char *, int);
int bx_as_head_line(const char *, char *, int);
int bx_as_err_line(const char *, int, const char *, char *, int);
int bx_ar_write_line(const char *, char *, int);
int bx_link_none_line(const char *, char *, int);
int bx_link_start_line(const char *, char *, int);
int bx_link_tape_line(const char *, char *, int);
int bx_link_target_line(const char *, const char *, char *, int);
int bx_view_err_line(int, const char *, const char *, char *, int);
int bx_ledger_word(const char *);
const char *bx_temp_dir(int, const char *, const char *);
int bx_jobs(const char *);
int bx_jobs_pool(int);
int bx_unit_cache(int, const char *);
int bx_two_pass(const char *);
const char *bx_pkg_env(const char *, const char *);
const char *bx_pkg_path(const char *, const char *);
int bx_copy_fit(const char *, char *, int);
int bx_s_line(const char *, char *, int);
int bx_route_line(int, const char *, int, int, int, char *, int);
int bx_unit_file(const char *, int, int, char *, int);
int bx_pid_file(const char *, int, char *, int);
int bx_fwd_path(int, const char *, unsigned long long, char *, int);
int bx_dbg_probe(int, int, const char *, char *, int);
int bx_dbg_hexfail(int, char *, int);
int bx_dbg_lookup(int, int, const char *, char *, int);
int bx_dbg_host(int, int, int, char *, int);
int bx_dbg_stub(int, int, int, char *, int);
int bx_loader_why(const char *, char *, int);
int sg_fx_nr(int, int);
const char *sg_interp(int);
int rr_construct(long long, char *, long);
int rr_lookup_line(int, long long, char *, long);
int rr_artifact_line(int, int, char *, long);
int rr_memory_line(int, long long, char *, long);
int rr_output_line(int, int, long long, char *, long);
const char *rr_ledger_need(void);
const char *rr_ledger_strict(void);
const char *rr_ledger_long(void);
int rr_ledger_err(const char *, char *, long);
int rr_mem_image(long long, char *, long);
int rr_jobs_line(int, char *, long);
#ifdef _WIN32
#include "winprocess.c"
#endif

#ifdef __aarch64__
#define NATIVE_ARCH "arm64"
#else
#define NATIVE_ARCH "x86_64"
#endif
#ifdef _WIN32
#define NATIVE_OS "win"
#else
#ifdef __APPLE__
#define NATIVE_OS "osx"
#else
#define NATIVE_OS "lnx"
#endif
#endif

static int clierror(const char *s) {
    char line[4096];
    if (bx_cli_line(s,line,4096)==0) fputs(line,stderr);
    else fprintf(stderr, "unisacc: model driver: %s\n", s);
    return 1;
}
/* [TP-7/TP-8] Route dispatch only: validation is the finite model, not C grammar.
   A missing stage must fail closed, including raw -S/-c which bypass the backend. */
static int tape_domain_available(const char *route) {
    for (int i=0;i<PS;i++) {
        const char *const rowr=STAGES[i].route, *const rown=STAGES[i].name;
        int p=bx_stage_present(route,"tapecheck",&rowr,&rown,1);
        if (p==1) return 1;
        if (p==4) return 0;
    }
    return 0;
}
static int tape_domain(const char *route, Buf *in, const char *src) {
    if (bx_tape_need(0, tape_domain_available(route))) return clierror(bx_tape_why());
    return runroute_range(route,"tapecheck","tapecheck",in,src);
}
#include "bind.c"

int cxp_tapebin_path(const char *),cxp_tape_path(const char *),cxp_tapebin_target(const char *);
int cxp_object_path(const char *),cxp_archive_path(const char *);
#define tapebin_path cxp_tapebin_path
#define tape_path cxp_tape_path
#define tapebin_target cxp_tapebin_target
#define object_path cxp_object_path
#define archive_path cxp_archive_path
int cxi_read(const char *,unsigned char **,long *,char *,long);
/* Source IO has the public compiler diagnostic; package/resource IO keeps
   the runtime diagnostic. No source interpretation happens in this helper. */
static unsigned char *source_read(const char *path, int *len) {
    unsigned char *bytes=0; long length=0; char reason[128]; char line[4096];
    int rc=cxi_read(path,&bytes,&length,reason,sizeof reason);
    int act=bx_read_act(rc);
    if (act==1) {
        if (bx_read_open_line(path,reason,line,4096)==0) fputs(line,stderr);
        else fprintf(stderr,"unisacc: error: cannot open %s%s%s\n",path,reason[0]?": ":"",reason);
        exit(1);
    }
    if (act==2) {
        const char *shown=bx_read_stdin(path)==1?"stdin":path;
        if (bx_read_fail_line(shown,line,4096)==0) fputs(line,stderr);
        else fprintf(stderr,"run: cannot read %s\n",shown);
        die(bx_read_die(2));
    }
    if (act) die(bx_read_die(act));
    *len=(int)length;
    return bytes;
}
/* Transitional C caller of the independently compiled CX output module.
 * The implementation and resource ownership live only in cx/driver/output.cx. */
int cxo_write_file(const char *path,const void *data,long length,char *why,long whycap);
/* Transitional calls into the independent CX dependency module. */
int cxd_default_target(const char *,char *,long,char *,long);
int cxd_write(const char *,const char *,const char **,long,const char **,const int *,long,const char **,long,int,char *,long);
/* 0.0.17 R17-2: the unisacc linker joins -funit unit objects at the tape
   level; the joining is the reference's src/tapelink.c, which only builds a
   tape -- every compilation decision after it is the product route's. */
/* libc/posix stays in this driver. The linker and assembler modules only
   receive these two callbacks and never call the host themselves. */
static Buf linkbuf;
static int tl_emit(int c) { bput(&linkbuf,(unsigned char)c,0); return 0; }
static int tl_die(char *m) { fputs(m,stderr); exit(1); return 1; }
int tl_bind(int (*emit)(int), int (*die)(char *));
int tl_begin(void);
void tl_adopt_first(void);
int tl_target_differs(void);
const char *tl_first_text(void);
const char *tl_target_text(void);
int tl_entry_missing(void);
int tl_unitver(char *, int);
int tl_unit(char *, int, int);
int tl_note_links(char *, long);
char *tl_tape(char *, long, long *);
int tl_needs(char *, long);
int tl_check_undef(void);
int tl_os(char *, int);
int tl_num(int);
static Buf asmbuf;
static int asm_emit(char *p, long n) { for (long k=0;k<n;k++) bput(&asmbuf,(unsigned char)p[k],0); return 0; }
int at_bind(int (*emit)(char *, long));
int at_dis(char *, long long);
int at_nm(char *, long long);
int at_asm(char *, long long, int);
int at_header_arch(char *, long long);
const char *at_error(void);
int at_line(void);
/* 0.0.19 R19-10: -run on macOS forwards prototyped undefined functions to the
   host libc.  E3 (with \0cli/run-forward) appends a USLFW1 side-car naming
   them with their USLSIG3 signatures; the stub text is src/fwdstub.c's, the
   same bytes the reference front end writes. */
/* Signature bytes are cx/compiler/fwdsig.cx. This header stays the oracle and is not included. */
typedef struct {
    int kind;
    int width, uns, ptr, depth, isvoid, structbyval;
    unsigned long long base;
    const unsigned char *callback;
    unsigned long long callback_len;
} USForwardType;
typedef struct {
    int variadic, structbyval, n;
    USForwardType ret, args[33];
} USForwardSig;
typedef struct { int n; USForwardType ret, args[8]; } USForwardCallback;
int us_fw_u64(const unsigned char *, unsigned long long, unsigned long long *, unsigned long long *);
int us_forward_sig_decode(const unsigned char *, unsigned long long, const char *, unsigned long long, USForwardSig *);
int us_forward_callback_decode(const USForwardType *, USForwardCallback *);
const char *fw_simple_type(int, int, int, int, int, unsigned long long, int, int);
int fw_decl(const char *, int, const char *const *, int, char *, int);
int cg_cb_count(int);
int cg_cb_cap(unsigned long long);
int fw_side_head(const unsigned char *, unsigned long long);
int fw_side_bad(unsigned long long, unsigned long long, unsigned long long, unsigned long long);
const char *fw_side_why(int);
int fw_undef(int, int, int, int);
int fw_emit_miss(int);
int fw_undef_line(const char *, int, char *, int);
/* Stub text lives in cx/compiler/fwdstub.cx. src/fwdstub.c stays the reference oracle. */
int fwd_emit(char *, int, int, int *, int *, int *, int, int, int, int);
int fwd_emit_var(char *, int, int, int *, int *, int, int, int, int);
int fwd_emit_libs(void);
int fwd_pclear(void);
void fwd_reset(void);
int fwd_count(void);
char *fwd_text(void);
void fwd_set_ccw(int);
int fwd_ccw_on(void);
void fwd_set_maxargs(int);
void fwd_set_ptype(int, char *);
char **fwd_lib_slot(void);
int *fwd_lib_count(void);
/* Forward merge and the module loader are upper CX units (cx/compiler/fwdmerge.cx,
   cx/compiler/cxmod.cx). src/cxmod.c stays the reference. */
void fwm_reset(void);
int fwm_export(const unsigned char *, int);
int fwm_overflowed(void);
int fwm_merge(const Buf *, const Buf *, const Buf *, int, Buf *);
int fwm_bind(void *(*alloc)(void *, long long), void (*release)(void *), void (*fail)(char *));
int cxm_bind(int (*rd)(char *, char *, int), int (*wr)(char *, char *, int), int (*say)(int, char *, int), void (*sha)(char *, int, char *), char *tmpdir);
int cxm_find(char *);
int cxm_flag(char *);
int cxm_wants_interface(void);
int cxm_interface(char *);
void cxm_set_on(int);
int cxm_wants_plan(void);
int cxm_iscxpath(char *);
int cxm_plan(char **, int);
int cxm_count(void);
char *cxm_unit_path(int);
int cxm_planprint(char **, int);
int cxm_on_now(void);
int cxm_include_count(int);
char *cxm_incpath(int, int);
/* The callback graph carries a C function-pointer type.  Scalar pointees
   are enough for glob's errfunc and similar host APIs; other graphs retain
   the named refusal until their declaration can be printed exactly. */
static const char *fwd_type_text(const USForwardType *t) {
    if (!t) return 0;
    return fw_simple_type(t->isvoid, t->callback ? 1 : 0, t->structbyval,
                          t->kind, t->depth, t->base, t->width, t->uns);
}
static int fwd_callback_decl(const USForwardType *t,int index,char *out,size_t cap) {
    USForwardCallback cb={0};
    const char *args[8];
    const char *ret;
    int i;
    if (us_forward_callback_decode(t,&cb)) return 1;
    ret=fwd_type_text(&cb.ret);
    if (!ret) return 1;
    if (cg_cb_count(cb.n)) return 1;
    for (i=0;i<cb.n;i++) {
        args[i]=fwd_type_text(&cb.args[i]);
        if (!args[i]) return 1;
    }
    if (cg_cb_cap(cap)) return 1;
    return fw_decl(ret, index, args, cb.n, out, (int)cap) ? 1 : 0;
}
static int fwd_second;                           /* the recompile with the stubs: no side-car */
/* 0.0.30 F2': the per-unit front-end output (unit route: e2, e1) of the first pass, with that unit's
   USLATTR1 records, so the forwarding restart re-runs only the stub unit.  The unit route does not read
   the run-forward resource, so its bytes are the same in both passes; UNISA_FWD_UNITCACHE=0 turns it off. */
static struct { const char *path; unsigned char *b; int n; unsigned char *attr; unsigned nattr; } ucache[256];
static int nucache;
/* F2': units of one multi-source run are independent (the unit cache already replays them), so on
   POSIX hosts the not-yet-cached units compile in forked workers; each writes its bytes and attribute
   payloads to a file and the serial loop consumes them as cache entries.  A worker that fails leaves
   its unit to the serial path, so diagnostics come out exactly as before.  UNISA_JOBS=1 disables. */
#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
static int par_get(FILE *f, unsigned *v) { unsigned char q[4]; if (fread(q,1,4,f)!=4) return 0; *v=q[0]|q[1]<<8|q[2]<<16|(unsigned)q[3]<<24; return 1; }
static void par_put(FILE *f, unsigned v) { unsigned char q[4]={v&255,v>>8&255,v>>16&255,v>>24&255}; fwrite(q,1,4,f); }
#endif
#include <time.h>
static int ucache_find(const char *path) { for (int k=0;k<nucache;k++) if (!strcmp(ucache[k].path,path)) return k; return -1; }
static void ucache_replay(int k) {                /* append unit k's attribute records under ATTR_UNIT */
    long n=ATTRS.n, cap=ATTRS.cap;
    int rc=cxaf_merge(&ATTRS.b,&n,&cap,ucache[k].attr,ucache[k].nattr,(unsigned)ATTR_UNIT,realloc);
    if (rc != 0 && rc != 1) attr_error(rc);
    ATTRS.n=(int)n; ATTRS.cap=(int)cap;
    if (rc == 1) attr_publish();
}
static char fwd_path[600];
#include "fwdpaths.h"
typedef struct { unsigned char *b; long long *at; int n, cap; } FwiBuf;
typedef struct { const unsigned char *name; int n; const unsigned char *data; int len; } FwiRi;
int fwd_incremental(const char *, int, const FwiBuf *, int, FwiBuf *, FwiRi *, int *, FwiBuf *, int *, int *, char *,
                    int (*)(void), int (*)(void), int (*)(void), char *(*)(void),
                    void *(*)(void *, long long), void (*)(void *),
                    int (*)(char *, char *, void *, char *),
                    int (*)(char *, char *, char *, void *, char *),
                    int (*)(const void *, const void *, const void *, int, void *));
static int fwi_replay(void) { return rr_wanted(getenv("UNISA_FWD_REPLAY")); }
static int fwi_over(void) { return fwm_overflowed(); }
static int fwi_count(void) { return fwd_count(); }
static char *fwi_text(void) { return fwd_text(); }
static void *fwi_alloc(void *p, long long n) { return xrealloc(p, (size_t)(n ? n : 1)); }
static void fwi_release(void *p) { free(p); }
static int fwi_front(char *route, char *stage, void *tape, char *path) {
    return fwd_front(route, stage, (Buf *)tape, path);
}
static int fwi_range(char *route, char *a, char *b, void *tape, char *path) {
    return runroute_range(route, a, b, (Buf *)tape, path);
}
static int fwi_merge(const void *base, const void *u, const void *v, int unit, void *merged) {
    return fwm_merge((const Buf *)base, (const Buf *)u, (const Buf *)v, unit, (Buf *)merged);
}
#include "runreuse.h"
/* Multi-TU -run reuse: the unit front ends (E2/E1, where includes are read) run before
 * rr_after_units, partly in forked workers.  While armed, every unit's read and absent
 * probe paths are recorded (workers send theirs through the unit file) and become the
 * first receipts of the later E3 store, so a changed or newly present header misses. */
static int rr_units_rec, rr_u_n, rr_u_abn;
static char **rr_u_paths, **rr_u_abs;
/* Armed by the first pass of a multi-TU -run that restarts with forward stubs and uses no
 * host slot: its key and complete receipts, stored by the stub pass under multi/fwdexec. */
static int rr_rs_armed, rr_rs_n, rr_rs_an;
static char rr_rs_id[65], **rr_rs_p, **rr_rs_a;
static char **rr_paths_copy(char **v, int n) {
    char **c = xrealloc(0, (size_t)(n ? n : 1) * sizeof *c);
    for (int i = 0; i < n; i++) { size_t l = strlen(v[i]) + 1; c[i] = xrealloc(0, l); memcpy(c[i], v[i], l); }
    return c;
}
#ifndef _WIN32
static void par_paths(FILE *f, char **v, int n) {
    par_put(f, (unsigned)n);
    for (int i = 0; i < n; i++) { unsigned l = (unsigned)strlen(v[i]); par_put(f, l); fwrite(v[i], 1, l, f); }
}
static int par_take_paths(FILE *f, int absent) {
    unsigned n = 0, l = 0; char p[4100];
    if (!par_get(f, &n) || n > 65536) return 0;
    for (unsigned i = 0; i < n; i++) {
        if (!par_get(f, &l) || !l || l >= sizeof p || fread(p, 1, l, f) != l) return 0;
        p[l] = 0;
        if (absent) record_file_absent(p); else record_file_read(p);
    }
    return 1;
}
#endif
#include "../../include/unisa_sha256.h"
#define CXM_PATHW 512
int cxm_rd(char *path, char *buf, int cap) {
#ifndef _WIN32
    /* Open without waiting for a FIFO writer, then validate that same handle.
       A path stat followed by fopen would leave a replacement race. */
    if (!path || !buf || cap <= 0) return -3;
    int fd=open(path,O_RDONLY|O_NONBLOCK); if(fd<0)return bx_open_miss(0,0,errno==ENOENT);
    struct stat st; if(fstat(fd,&st)||!S_ISREG(st.st_mode)){close(fd);return -3;}
    size_t n=0; int bad=0;
    while(n<(size_t)cap){ssize_t z=read(fd,buf+n,(size_t)cap-n);
        if(z<0&&errno==EINTR)continue;
        if(z<0){bad=1;break;} if(!z)break; n+=(size_t)z;
    }
    if(close(fd))bad=1;
    return bad?-3:n>=(size_t)cap?-2:(int)n;
#else
    FILE *f=fopen(path,bx_file_mode(0)); if (!f) return bx_open_miss(0,0,errno==ENOENT);
    size_t n=fread(buf,1,(size_t)cap,f); int bad=ferror(f); if(fclose(f))bad=1;
    return bad ? -3 : n>=(size_t)cap ? -2 : (int)n;
#endif
}
int cxm_wr(char *path, char *buf, int n) {       /* beside it, then renamed: a reader never sees half a file */
#ifdef _WIN32
    FILE *f=fopen(path,bx_file_mode(1)); if (!f) return bx_open_miss(1,0,0);
    int bad=fwrite(buf,1,(size_t)n,f)!=(size_t)n; if (fclose(f)) bad=1; return bad;
#else
    char tmp[CXM_PATHW+32]; if (bx_pid_file(path,(int)getpid(),tmp,(int)sizeof tmp)!=0) snprintf(tmp,sizeof tmp,"%s.%d",path,(int)getpid());
    FILE *f=fopen(tmp,bx_file_mode(1)); if (!f) return bx_open_miss(1,0,0);
    int bad=fwrite(buf,1,(size_t)n,f)!=(size_t)n; if (fclose(f)) bad=1;
    if (bad || rename(tmp,path)) { remove(tmp); return 1; }
    return 0;
#endif
}
int cxm_say(int fd, char *s, int n) { fwrite(s,1,(size_t)n,fd==1 ? stdout : stderr); return 0; }
static void cxm_sha_host(char *p, int n, char *hex) { unisa_sha256_hex(p, (size_t)n, hex); }
static void *fwm_host_alloc(void *p, long long n) { return xrealloc(p, n ? (size_t)n : 1); }
static void fwm_host_release(void *p) { free(p); }
static void fwm_host_fail(char *m) { die(m); }
static int fwd_sidecar(Buf *in, Buf *tape) {     /* 0: no records (tape set); 1: stubs written; -1: error */
    unsigned long long at=0,n=(unsigned long long)in->n; const unsigned char *b=in->b; unsigned long long tl=0,cnt=0;
    if (fw_side_head(b,n)) { fputs(fw_side_why(0),stderr); return -1; }
    at=7;
    unsigned long long ccwc=0;
    if (us_fw_u64(b,n,&at,&tl) || us_fw_u64(b,n,&at,&cnt) || us_fw_u64(b,n,&at,&ccwc) || fw_side_bad(ccwc,tl,n,at)) { fputs(fw_side_why(1),stderr); return -1; }
    tape->b=xrealloc(0,(size_t)tl+1); memcpy(tape->b,b+at,(size_t)tl); tape->n=(int)tl; at+=tl;
    fwm_reset();
    fwd_set_ccw(ccwc>0);
    if (cnt==0) return 0;
    fwd_reset();
    for (unsigned long long r=0;r<cnt;r++) {
        unsigned long long nl=0,sl=0;
        if (us_fw_u64(b,n,&at,&nl) || nl>n-at || nl>1024) return -1;
        const char *nm=(const char *)b+at; at+=nl;
        if (us_fw_u64(b,n,&at,&sl) || sl>n-at) return -1;
        USForwardSig sig; memset(&sig,0,sizeof sig);
        int bad=us_forward_sig_decode(b+at,sl,nm,nl,&sig); at+=sl;
        if (fw_undef(bad, sig.structbyval, sig.ret.structbyval, sig.n)) {
            char line[1100];
            if (fw_undef_line(nm,(int)nl,line,1100)==0) fputs(line,stderr);
            return -1;
        }
        (void)fwm_export((const unsigned char *)nm,(int)nl);
        int kk[33],ww[33],uu[33]; char cbdecl[33][256];
        for (int k=0;k<sig.n;k++) { kk[k]=sig.args[k].kind; ww[k]=sig.args[k].width; uu[k]=0; }
        for (int k=0;k<sig.n;k++) if (sig.args[k].callback) {
            if (fwd_callback_decl(&sig.args[k],k,cbdecl[k],sizeof cbdecl[k])) {
                char line[1100];
                fwd_pclear();
                if (fw_undef_line(nm,(int)nl,line,1100)==0) fputs(line,stderr);
                return -1;
            }
            fwd_set_ptype(k,cbdecl[k]);
        }
        int emitted = sig.variadic
            ? fwd_emit_var((char *)nm,(int)nl,sig.n,kk,ww,sig.ret.isvoid,sig.ret.kind,sig.ret.width,sig.ret.uns)
            : fwd_emit((char *)nm,(int)nl,sig.n,kk,ww,uu,sig.ret.isvoid,sig.ret.kind,sig.ret.width,sig.ret.uns);
        fwd_pclear();
        if (fw_emit_miss(emitted)) { char line[1100]; if (fw_undef_line(nm,(int)nl,line,1100)==0) fputs(line,stderr); return -1; }
    }
    if (fwd_count() > 0) fwd_emit_libs();
#ifdef _WIN32
    { const char *td=getenv("TEMP"); unsigned long tag=(unsigned long)(uintptr_t)&at;
      if (bx_fwd_path(1, bx_env_dot(td), (unsigned long long)tag, fwd_path, (int)sizeof fwd_path)!=0)
          snprintf(fwd_path,sizeof fwd_path,"%s\\unisacc-forward-%lx.c",bx_env_dot(td),tag); }
#else
    { unsigned long tag=(unsigned long)(uintptr_t)&at;
      if (bx_fwd_path(0, 0, (unsigned long long)tag, fwd_path, (int)sizeof fwd_path)!=0)
          snprintf(fwd_path,sizeof fwd_path,"/tmp/unisacc-forward-%lx.c",tag); }
#endif
    FILE *f=fopen(fwd_path,bx_file_mode(1)); if (!f) return bx_open_miss(2,0,0);
    int werr=fwrite(fwd_text(),1,(size_t)fwd_count(),f)!=(size_t)fwd_count(); werr|=fclose(f)!=0;
    return werr ? -1 : 1;
}
/* Increment control is cx/compiler/fwdincrement.cx. exec/c/fwdincrement.c stays the oracle. */
static int product_view(int argc,char **argv) {
    int dis=argv[1][0]=='o';
    if (dis ? (argc!=4 || strcmp(argv[2],"-d")) : argc!=3) { fputs(bx_view_usage(dis),stderr); return 2; }
    const char *f=argv[dis ? 3 : 2]; int len=0; unsigned char *b=source_read(f,&len);
    asmbuf.n=0;
    if (at_bind(asm_emit)) return 1;
    if (dis ? at_dis((char *)b,len) : at_nm((char *)b,len)) { char line[4096]; if (bx_view_err_line(dis,f,at_error(),line,4096)==0) fputs(line,stderr); else fprintf(stderr,"%s: %s: %s (Linux objects; Mach-O and COFF: 0.0.19)\n",dis ? "objdump" : "nm",f,at_error()); free(b); return 1; }
    free(b); fwrite(asmbuf.b,1,asmbuf.n,stdout); return 0;
}
static int product_as(int argc,char **argv) {
    const char *in=0,*out="a.out"; int native_arm=bx_native_arm(NATIVE_ARCH); int arch=native_arm==1,explicit=0;
    for (int i=2;i<argc;i++) {
        if (!strcmp(argv[i],"-o") && i+1<argc) out=argv[++i];
        else if (!strcmp(argv[i],"-b") && i+1<argc) {
            const char *t=argv[++i];explicit=1;
            { int as_arch=bx_as_arch(t); if (as_arch==0 || as_arch==1) arch=as_arch;
            else { fputs(bx_as_targets(),stderr); return 2; } }
        }
        else if (argv[i][0]=='-' && argv[i][1]) { char line[4096]; if (bx_as_opt_line(argv[i],line,4096)==0) fputs(line,stderr); else fprintf(stderr,"as: unknown option %s\n",argv[i]); return 2; }
        else in=argv[i];
    }
    if (!in) { fputs(bx_as_usage(),stderr); return 2; }
    int len=0; unsigned char *b=source_read(in,&len);
    asmbuf.n=0;
    if (at_bind(asm_emit)) { free(b); return 1; }
    /* without -b the -S header names the target; an explicit -b must agree with it */
    if (explicit && at_header_arch((char *)b,len)>=0 && at_header_arch((char *)b,len)!=arch) { char line[4096]; if (bx_as_head_line(in,line,4096)==0) fputs(line,stderr); else fprintf(stderr,"as: %s: -S header target conflicts with -b\n",in); free(b); return 1; }
    if (at_asm((char *)b,len,arch)) { char line[4096]; if (bx_as_err_line(in,at_line(),at_error(),line,4096)==0) fputs(line,stderr); else fprintf(stderr,"as: %s:%d: %s\n",in,at_line(),at_error()[0] ? at_error() : "error"); free(b); return 1; }
    free(b);
    FILE *f=fopen(out,bx_file_mode(1)); if (!f) { char line[4096]; if (bx_as_write_line(out,line,4096)==0) fputs(line,stderr); else fprintf(stderr,"as: cannot write %s\n",out); return 1; }
    int bad=fwrite(asmbuf.b,1,asmbuf.n,f)!=(size_t)asmbuf.n; bad|=fclose(f)!=0;
    return bad;
}
typedef struct {
    int mode,level,warnings,nsources,runit,emitbin,deponly,src_tape,src_tapebin,linking,single,exec_restart;
    const char *target,*native,*linked_target,*only_target;
} CxtIn;
typedef struct { const char **target; int *mode,*runit,*clear_deps; } CxtOut;
int cxt_object(int,int,int,int,const char *,const char *,int,int *,int *,int *,int *,char *,long);
int cxt_decide(const CxtIn *,CxtOut *,char *,long);
int cxt_text(const char *,int,int,int,int,char *,long,char *,long);
static int cxt_error(int rc,const char *why) { if(rc==4){fputs(why,stderr);return 1;} return clierror(why); }
static int take_unit(char *t,long tl,int u) {
    if (u==0) tl_adopt_first();
    else if (tl_target_differs()) { char line[4096]; if (bx_link_target_line(tl_first_text(),tl_target_text(),line,4096)==0) fputs(line,stderr); else fprintf(stderr,"unisacc: error: the objects were compiled for different targets (%s, %s)\n",tl_first_text(),tl_target_text()); return 1; }
    tl_unitver(t,(int)tl); tl_unit(t,(int)tl,u); tl_note_links(t,tl);
    return 0;
}
int cxr_member(char *,long,int,char *,long,char **,long *,char *,long);
static int link_objects(const char **paths,int n,Buf *out) {
    int u=0;
    if (tl_bind(tl_emit,tl_die) || tl_begin()) return 1;
    linkbuf.n=0;
    for (int p=0;p<n;p++) {                      /* every object is taken */
        if (archive_path(paths[p])) continue;
        int len=0; unsigned char *b=source_read(paths[p],&len); long tl=0;
        char *t=tl_tape((char *)b,len,&tl);
        if (!t) { char line[4096]; if (bx_link_tape_line(paths[p],line,4096)==0) fputs(line,stderr); else fprintf(stderr,"unisacc: error: %s carries no unit tape (compile it with -c -b os/arch -funit; objects from other compilers are not linked yet)\n",paths[p]); free(b); return 1; }
        if (take_unit(t,tl,u)) { free(b); return 1; }
        u++; free(b);
    }
    for (int p=0;p<n;p++) {                      /* archives, in order: members that define a wanted name */
        if (!archive_path(paths[p])) continue;
        int len=0; unsigned char *b=source_read(paths[p],&len);
        char *used=calloc(65536,1); int changed=1; char nm[256],why[128];
        if (!used) { free(b); die(bx_read_die(6)); }
        while (changed) {
            changed=0;
            for (int i=0;i<65536;i++) {
                long ml=0; char *m=0; int rc=cxr_member((char *)b,len,i,nm,sizeof nm,&m,&ml,why,sizeof why);
                if (rc==1) break;
                if (rc) { fputs(why,stderr); free(b); free(used); return 1; }
                if (used[i]) continue;
                long tl=0; char *t=tl_tape(m,ml,&tl);
                if (t && tl_needs(t,tl)) { if (take_unit(t,tl,u)) { free(b); free(used); return 1; } u++; used[i]=1; changed=1; }
            }
        }
        free(used); free(b);
    }
    if (!u) { char line[4096]; if (bx_link_none_line(paths[0],line,4096)==0) fputs(line,stderr); else fprintf(stderr,"link: no input unit selected from %s\n",paths[0]); return 1; }
    if (tl_check_undef()) return 1;              /* R20-3: shared with the reference linker */
    if (tl_entry_missing()) { char line[4096]; if (bx_link_start_line(paths[0],line,4096)==0) fputs(line,stderr); else fprintf(stderr,"link: missing _start entry in %s\n",paths[0]); return 1; }
    tl_os("__init:\n",8);
    for (int k=0;k<u;k++) { tl_os("  call __init_u",15); tl_num(k); tl_o(10); }
    tl_os("  ret\n",6);
    *out=linkbuf; memset(&linkbuf,0,sizeof linkbuf);
    return 0;
}
/* `unisacc ar rcs|t|x LIB.a [MEMBER...]` (R17-3): the reference's archive
   code, with stdio for the files */
int cxr_operation(const char *,char *,long);
int cxr_append(char *,long,long,const char *,const char *,long,long *,char *,long);
static int product_ar(int argc,char **argv) {
    if (argc<4) { fputs(bx_ar_usage(),stderr); return 2; }
    const char *op=argv[2],*lib=argv[3];
    char why[128];
    if (cxr_operation(op,why,sizeof why)) { fputs(why,stderr); return 2; }
    if (op[0]=='r' || op[0]=='q') {
        long cap=8; for (int i=4;i<argc;i++) { int len=0; unsigned char *b=source_read(argv[i],&len); size_t namesz=strlen(argv[i]); free(b);
            if (namesz>(size_t)(LONG_MAX-400-len) || cap>LONG_MAX-400-len-(long)namesz) { fputs(bx_ar_big(),stderr); return 1; }
            cap+=len+400+(long)namesz; }
        char *ab=malloc((size_t)cap); if (!ab) die(bx_read_die(6)); memcpy(ab,"!<arch>\n",8); long at=8;
        for (int i=4;i<argc;i++) { int len=0; unsigned char *b=source_read(argv[i],&len); long next=0; int rc=cxr_append(ab,cap,at,argv[i],(char *)b,len,&next,why,sizeof why); free(b); if(rc){ fputs(why,stderr); free(ab); return 1; } at=next; }
        FILE *f=fopen(lib,bx_file_mode(1)); if (!f) { char line[4096]; if (bx_ar_write_line(lib,line,4096)==0) fputs(line,stderr); else fprintf(stderr,"ar: cannot write %s\n",lib); free(ab); return 1; }
        int bad=fwrite(ab,1,at,f)!=(size_t)at; bad|=fclose(f)!=0; free(ab);
        return bad;
    }
    int len=0; unsigned char *b=source_read(lib,&len); char nm[256]; int i=0;
    if (len<8 || memcmp(b,"!<arch>\n",8)) { fputs(bx_ar_kind(),stderr); return 1; }
    for (;;i++) {
        long ml=0; char *m=0; int rc=cxr_member((char *)b,len,i,nm,sizeof nm,&m,&ml,why,sizeof why);
        if (rc==1) break;
        if (rc) { fputs(why,stderr); free(b); return 1; }
        if (op[0]=='t') puts(nm);
        else if (op[0]=='x') { FILE *f=fopen(nm,bx_file_mode(1)); if (!f || fwrite(m,1,ml,f)!=(size_t)ml || fclose(f)) { char line[4096]; if (bx_ar_write_line(nm,line,4096)==0) fputs(line,stderr); else fprintf(stderr,"ar: cannot write %s\n",nm); return 1; } }
        else { fputs(bx_ar_op(),stderr); return 2; }
    }
    free(b); return 0;
}
typedef struct { unsigned char *b; long n,cap; } CliBlock;
typedef struct { char **libs; int *n; int cap; } CxpState;
int cxp_step(CxpState *,char **,int,int *,char *,long);
/* Transitional ABI shared with cx/driver/cli.cx; fields borrow argv. */
typedef struct {
    unsigned char **defs,**undefs,**forced,**incdir,**quotedir,**errorlimit;
    long *defs_n,*undefs_n,*forced_n,*incdir_n,*quotedir_n,*error_n;
    long *defs_cap,*undefs_cap,*forced_cap,*incdir_cap,*quotedir_cap,*error_cap;
    const char *last_include;
} CxcResources;
typedef struct {
    int (*record)(unsigned char **,long *,long *,const char *,char *,long);
    int (*line)(unsigned char **,long *,long *,const char *,char *,long);
    int (*cancel)(unsigned char **,long *,const char *,char *,long);
} CxcOps;
typedef struct {
    const char *out,*target,*pkg,*deps,*host_symbols_path,*host_provider_path;
    const char **deptargets; int *depquoted;
    int deptarget_cap,ndeptargets,mode,level,explicit_image,runit,force_origin,emitbin;
    int deponly,depphony,sawc,sawS,funit,warnings,defrun,werror,nostd,notrim;
    CxcResources *res;
    CxcOps *ops;
} CxcState;
int cxc_default_run(CxcState *,char **,int,char *,long);
int cxc_step(CxcState *,char **,int,int *,char *,long);
int cxa_record(unsigned char **,long *,long *,const char *,char *,long);
int cxa_line(unsigned char **,long *,long *,const char *,char *,long);
int cxa_raw(unsigned char **,long *,long *,const unsigned char *,long,char *,long);
int cxa_cancel(unsigned char **,long *,const char *,char *,long);
int cxa_includes(const char *,const unsigned char *,long,const unsigned char *,long,const unsigned char *,long,const unsigned char **,long *,char *,long);
static void args_check(int rc,const char *why) { if (rc) die(why); }
static void argbytes(CliBlock *b,const char *s) { char why[96]; int rc=cxa_record(&b->b,&b->n,&b->cap,s,why,sizeof why); args_check(rc,why); }
static void argraw(CliBlock *b,const unsigned char *p,long n) { char why[96]; int rc=cxa_raw(&b->b,&b->n,&b->cap,p,n,why,sizeof why); args_check(rc,why); }
/* the forced-include list one unit is compiled with: a .cx unit's starts with its built-in prelude cx.h, and one
   that uses modules ends with their interfaces (cxm_lists, CX2a) */
static CliBlock *cxm_lists;
static void unit_includes(ResourceInput *r, const char *path, const CliBlock *plain, const CliBlock *withcx) {
    int u=cxm_lists ? cxm_find((char *)path) : -1;
    const CliBlock *unit=(u>=0 && cxm_include_count(u)) ? &cxm_lists[u] : 0;
    const unsigned char *out; long n; char why[96];
    int rc=cxa_includes(path,plain->b,plain->n,withcx->b,withcx->n,unit?unit->b:0,unit?unit->n:-1,&out,&n,why,sizeof why);
    args_check(rc,why); r->data=out; r->len=(int)n;
}
char *cxe_environment(int,char **,int);
#define ARGRESOURCE(i,k,v) cli[i].name=(const unsigned char *)k; cli[i].n=sizeof(k)-1; cli[i].data=v.b; cli[i].len=v.n
/* A static compiler image (Linux, built by unisacc) has no host loader in its process, so a run
   whose program forwards to the host libc cannot be bound in memory.  main finishes it as the
   image -o writes (a dynamic ELF) and forward_exec replaces this process with it: argv,
   environment, stdio and exit status are then the program's own.  The image goes to an anonymous
   memory file and is exec'd by descriptor (execveat, AT_EMPTY_PATH), which needs no /proc.
   There is no on-disk fallback: a path in TMPDIR that is written, closed and reopened cannot be
   bound to the bytes that were written, so where memfd is unavailable or may not be executed the
   run is refused and -o is the way. */
#ifdef __linux__
static int fwd_exec;                             /* set once; the stub restart (fwd_second) re-enters main with it.  The CLI
                                                    main is entered once per process: this is not state for a reusable entry point */
#ifdef __aarch64__
#define FX_ARM 1
#define FX_LOADER "/lib/ld-linux-aarch64.so.1"
#else
#define FX_ARM 0
#define FX_LOADER "/lib64/ld-linux-x86-64.so.2"
#endif
static long fx_sys(long nr,long a,long b,long c,long d,long e) {   /* a result, or -errno */
#ifdef __UNISA__
    return __syscall6(nr,a,b,c,d,e);
#else
    long r=syscall(nr,a,b,c,d,e); return r<0 ? -(long)errno : r;
#endif
}
static int fx_fill(long fd,const Buf *image) {
    int done=0;
    while (done<image->n) {
        long wrote=fx_sys(sg_fx_nr(FX_ARM,3),fd,(long)(image->b+done),image->n-done,0,0);
        if (wrote==-4) continue;                 /* EINTR */
        if (wrote<=0) return 1;
        done+=(int)wrote;
    }
    return 0;
}
static long fx_exec(long fd,char **argv,char **envp) {
    return fx_sys(sg_fx_nr(FX_ARM,1),fd,(long)"",(long)argv,(long)envp,0x1000);   /* AT_EMPTY_PATH */
}
static int forward_exec(const Buf *image,char **argv) {
    long err=0; int noloader=0; char **envp=argv;
    while (*envp) envp++;                        /* the environment follows argv's NULL (runargs) */
    envp++;
    struct timespec fx_t0, fx_t1; int fx_time = rr_wanted(getenv("UNISACC_RR_TIME"));
    if (fx_time) clock_gettime(CLOCK_MONOTONIC, &fx_t0);
    fflush(stdout); fflush(stderr);
    /* MFD_CLOEXEC|MFD_EXEC; kernels before 6.3 reject MFD_EXEC with EINVAL */
    long fd=fx_sys(sg_fx_nr(FX_ARM,0),(long)"unisacc-run",1|16,0,0,0);
    if (fd==-22) fd=fx_sys(sg_fx_nr(FX_ARM,0),(long)"unisacc-run",1,0,0,0);
    if (fd>=0) {
        int filled = fx_fill(fd,image);
        if (fx_time) {   /* UNISACC_RR_TIME output stamp: memfd + image write up to the exec; the exec itself replaces this process */
            long long ns; char line[1024];
            clock_gettime(CLOCK_MONOTONIC, &fx_t1);
            ns = rr_ns((long long)fx_t0.tv_sec, (long long)fx_t0.tv_nsec, (long long)fx_t1.tv_sec, (long long)fx_t1.tv_nsec);
            if (rr_output_line(0, image->n, ns, line, (long)sizeof line) == 0) fputs(line, stderr);
            else fprintf(stderr, "RR_TIME\toutput\tkind=exec-prepare\tbytes=%d\tns=%lld\n", image->n, ns);
            fflush(stderr);
        }
        err=filled ? -5 : fx_exec(fd,argv,envp);   /* returns only on failure */
        noloader=err==-2;                        /* ENOENT by descriptor: the image's PT_INTERP file */
        fx_sys(sg_fx_nr(FX_ARM,2),fd,0,0,0,0);
    }
    else err=fd;
    if (noloader) {
        char why[1024];
        if (bx_loader_why(sg_interp(FX_ARM), why, 1024)==0) return clierror(why);
        return clierror("this program calls the host C library, and running it needs the host's dynamic loader "
            FX_LOADER ", which could not be opened here (as on a system without glibc); "
            "write the program with -o and run it where glibc is installed");
    }
    { char line[1024]; long long shown=(long long)(-err);
      if (rr_mem_image(shown, line, (long)sizeof line) == 0) fputs(line, stderr);
      else fprintf(stderr,"unisacc: model driver: this program calls the host C library, and its image could not be started from an anonymous memory file (errno %ld); write the program with -o and run that\n",-err); }
    return 1;
}
#endif

int main(int argc, char **argv) {
    RF_TRACE_STARTED = 0;
    { const char *td = getenv("TMPDIR");
      const char *te = bx_env_temp(td) ? getenv("TEMP") : 0;
      td = bx_temp_dir(0, td, te);
      if (cxm_bind(cxm_rd, cxm_wr, cxm_say, cxm_sha_host, td)) return 1;
      if (fwm_bind(fwm_host_alloc, fwm_host_release, fwm_host_fail)) return 1; }
    /* S step 1 (opt-in, non-stable CLI): --strict-resource-flow --resource-ledger DIR are taken out of argv first, so the
       option loops below never see the ledger directory as a source. */
    {   int w = 1;
        for (int i = 1; i < argc; i++) {
            { int word=bx_ledger_word(argv[i]);
            if (word==1) { RF_STRICT = 1; continue; }
            if (word==2) {
                char err[512];
                if (i + 1 >= argc) { fputs(rr_ledger_need(), stderr); return 2; }
                rl_clear(&RF_LEDGER); RF_LOADED = 0; RF_LEDGER_DIR[0] = 0;
                if (rl_load(argv[++i], &RF_LEDGER, err, sizeof err)) { char line[1024]; if (rr_ledger_err(err, line, (long)sizeof line)==0) fputs(line, stderr); else fprintf(stderr, "unisacc: %s\n", err); return 2; }
                { if (bx_copy_fit(argv[i], RF_LEDGER_DIR, (int)sizeof RF_LEDGER_DIR)) { fputs(rr_ledger_long(), stderr); return 2; } }
                RF_LOADED = 1; continue;
            } }
            argv[w++] = argv[i];
        }
        argc = w; argv[argc] = 0;
        if (RF_STRICT && !RF_LOADED) { fputs(rr_ledger_strict(), stderr); return 2; }
    }
    rr_cli_argc = argc;
    rr_cli_argv = argv;
    const char *src = 0, *out = 0, *target = 0, *pkg = 0, *deps = 0;
    int mode = 0, level = 0, explicit_image = 0, runit = 0, argstart = argc, force_origin=0, emitbin=0;
    int deponly=0, depphony=0, ndeptargets=0, sawc=0, funit=0, defrun=0;
    const char *host_symbols_path=0,*host_provider_path=0;
    const char **deptargets=xrealloc(0,argc*sizeof(char *)); int *depquoted=xrealloc(0,argc*sizeof(int));
    int warnings=0; Buf werror={0}; CliBlock errorlimit={0};
    const char **sources=xrealloc(0,argc*sizeof(char *)); int nsources=0;
    CliBlock defs={0}, undefs={0}, forced={0}, incdir={0}, quotedir={0}; Buf nostd={0}, libneed={0}, notrim={0};
    { int tool=bx_tool(argc>=2 ? argv[1] : 0);
    if (tool==1) { if(rf_trace_finish())return 2;return product_ar(argc,argv); }   /* R17-3 */
    if (tool==2) { if(rf_trace_finish())return 2;return product_as(argc,argv); }   /* R18-1 */
    if (tool==3) { if(rf_trace_finish())return 2;return product_view(argc,argv); } }   /* R18-4 */
    int sawS=0;
    CliBlock srcres={0};                   /* \0cli/source: the main source path */
    CxcState opts={0}; char cli_reason[128]; CxpState compat={fwd_lib_slot(),fwd_lib_count(),16};
    CxcResources resources={&defs.b,&undefs.b,&forced.b,&incdir.b,&quotedir.b,&errorlimit.b,
        &defs.n,&undefs.n,&forced.n,&incdir.n,&quotedir.n,&errorlimit.n,
        &defs.cap,&undefs.cap,&forced.cap,&incdir.cap,&quotedir.cap,&errorlimit.cap,0};
    opts.res=&resources;
    CxcOps cliops={cxa_record,cxa_line,cxa_cancel}; opts.ops=&cliops;
    opts.deptargets=deptargets; opts.depquoted=depquoted; opts.deptarget_cap=argc;
    if (cxc_default_run(&opts,argv,argc,cli_reason,sizeof cli_reason)) return clierror(cli_reason);
    for (int i=1;i<argc;) {
        const char *a=argv[i]; int next=i;
        int parsed=cxc_step(&opts,argv,argc,&next,cli_reason,sizeof cli_reason);
        if (!parsed) { i=next; continue; }
        if (parsed==7) die(cli_reason);
        if (parsed!=1) return clierror(cli_reason);
        { int word=bx_cli_word(a);
        if (word==1) {
            if(rf_trace_finish())return 2;printf("unisacc %s\n",UNISACC_VERSION);return 0;
        } else if (word==2) {
            if(rf_trace_finish())return 2;puts("unisacc FILE [-S|-c|-o FILE|-run]\nDiagnostic, non-stable: --emit-host-symbols FILE (-S/-c/-o, not -run)\nDiagnostic, non-stable: --host-provider FILE (12-byte capability filter; no addresses)");return 0;
        } else if (opts.runit && word==3) { argstart=i+1;break; }
        else if (opts.runit && src && a[0]!='-') {
            if(tapebin_path(src)||tape_path(src)){argstart=i;break;}
            if (bx_more_src(a, object_path(a))==1) sources[nsources++]=a;
            else {argstart=i;break;}
        } else if (a[0]=='-' && (a[1]=='l' || a[1]=='L' || a[1]=='x')) {
            int compat_next=i;int rc=cxp_step(&compat,argv,argc,&compat_next,cli_reason,sizeof cli_reason);
            { const char *compat_why=bx_compat_why(rc,opts.defrun,src); if(compat_why)return clierror(compat_why); }
            if(rc) return clierror(cli_reason);
            i=compat_next;continue;
        } else if (cxm_flag((char *)a)) { }
        else if (bx_opt_dash(a)==1) return clierror(bx_opt_why(opts.defrun, src));
        else { sources[nsources++]=a;if(!src)src=a; }
        }
        i++;
    }
    if(resources.last_include) INCDIR=resources.last_include;
    out=opts.out;target=opts.target;pkg=opts.pkg;deps=opts.deps;
    host_symbols_path=opts.host_symbols_path;host_provider_path=opts.host_provider_path;
    ndeptargets=opts.ndeptargets;mode=opts.mode;level=opts.level;explicit_image=opts.explicit_image;
    runit=opts.runit;defrun=opts.defrun;force_origin=opts.force_origin;emitbin=opts.emitbin;
    deponly=opts.deponly;depphony=opts.depphony;sawc=opts.sawc;sawS=opts.sawS;funit=opts.funit;warnings=opts.warnings;
    if(opts.werror)bput(&werror,1,0);if(opts.nostd)bput(&nostd,1,0);if(opts.notrim)bput(&notrim,1,0);
    { const char *src_why=bx_src_why(src); if(src_why)return clierror(src_why); }
    /* Resolve the whole graph for interfaces; only explicit sources belong to
       a separate -funit object. Other routes still compile dependencies. */
    int explicit_sources=nsources;
    if (cxm_wants_interface()) { if(rf_trace_finish())return 2;return cxm_interface((char *)src); }
    cxm_set_on(0);
    if (!object_path(src) && !tapebin_path(src) && !tape_path(src)) {
        int anycx=cxm_wants_plan();
        for (int j=0;j<nsources;j++) if (cxm_iscxpath((char *)sources[j])) anycx=1;
        if (anycx) {
            if (cxm_plan((char **)sources,nsources)) return 1;
            if (cxm_count()>nsources) sources=xrealloc(sources,cxm_count()*sizeof(char *));
            for (int j=0;j<cxm_count();j++) sources[j]=cxm_unit_path(j);
            nsources=cxm_count();
        }
    }
    if (cxm_wants_plan()) {
        if(rf_trace_finish())return 2;
        const char **opt=xrealloc(0,(forced.n+1)*sizeof(char *)); int nopt=0;
        for (int k=0;k<forced.n;k++) if (!k || !forced.b[k-1]) opt[nopt++]=(const char *)forced.b+k;
        cxm_planprint((char **)opt,nopt); free(opt); return 0;
    }
    if (funit && sawc && target && cxm_on_now()) nsources=explicit_sources;
    /* -c with -b: an object (R17-1).  The product writes objects through its
       own route (\0cli/object, \0cli/funit; exec/enc, in progress) -- until
       that route exists the driver says so instead of writing a tape. */
    /* -c with -b: object routes carry \0cli/object and \0cli/funit.
       Non-Linux objects are unit containers; the selected package must
       supply the requested route before the driver accepts them. */
    int objwant=0,asmwant=0,need_object_name=0; char route_reason[8192];
    char *object_reason=route_reason; size_t object_reason_cap=sizeof route_reason;
    if(target && strlen(target)>sizeof route_reason-512) { object_reason_cap=strlen(target)+512; object_reason=xrealloc(0,object_reason_cap); }
    int route_rc=cxt_object(mode,sawc,sawS,funit,target,out,object_path(src),&mode,&need_object_name,&objwant,&asmwant,object_reason,(long)object_reason_cap);
    if(route_rc) { int ret=cxt_error(route_rc,object_reason); if(object_reason!=route_reason)free(object_reason); return ret; }
    if(object_reason!=route_reason)free(object_reason);
    char default_object_target[520], deps_reason[128];
    if (objwant) { if (need_object_name) {
        if (cxd_default_target(src,default_object_target,sizeof default_object_target,deps_reason,sizeof deps_reason)) return clierror(deps_reason);
        out = default_object_target;
    } }
    /* unit objects (-funit): joined into one program tape here, before the
       route is chosen, because the program's target is the objects' */
    int linking=object_path(src); Buf linked={0};
    if (linking) {
        for (int j=0;j<nsources;j++) { const char *mix=bx_mix_why(object_path(sources[j])); if(mix)return clierror(mix); }
        if (link_objects(sources,nsources,&linked)) return 1;
        nsources=1;
    }
    int isbin=tapebin_path(src), istape=tape_path(src)||linking, tape_input=isbin||istape;
    int clear_deps=0, single=0, restart=0; const char *only_target=0;
#ifdef UNISA_SINGLE_TARGET
    single=1; only_target=UNISA_SINGLE_TARGET;
#endif
#ifdef __linux__
    /* forward_exec, on the stub restart: the same program as an image; no file is written, as in a run */
    int execrun=fwd_exec && runit;
    restart=fwd_exec;
#endif
    CxtIn route_in={mode,level,warnings,nsources,runit,emitbin,deponly,istape,isbin,linking,single,restart,target,NATIVE_OS "/" NATIVE_ARCH,linking?tl_first_text():0,only_target};
    CxtOut route_out={&target,&mode,&runit,&clear_deps};
    route_rc=cxt_decide(&route_in,&route_out,route_reason,sizeof route_reason);
    if(route_rc)return cxt_error(route_rc,route_reason);
    if(clear_deps)deps=0;
    unsigned host_target=host_target_id(target),host_filter=15;
    if(bx_diag_opt(host_symbols_path,host_provider_path,(int)host_target,mode,emitbin,asmwant))return clierror("bind: diagnostic option mode/target not supported");
    if(bx_diag_run(host_symbols_path,runit))return clierror("bind: --emit-host-symbols cannot be used with -run or stdout");
    if(host_symbols_path){
        const char *effective_out=out; const char *sym_out=0;
        if(bx_sym_out(out,mode,explicit_image,runit,&sym_out)==1)effective_out=sym_out;
        int same=host_same_output(host_symbols_path,effective_out);
        if(same)return clierror(bx_same_why(same,0));
        if(deps){
            const char *effective_deps=deps;char depname[4096];const char *chosen=0;
            int only=bx_dep_only(deps,deponly,out,&chosen);
            if(only==1)effective_deps=chosen;
            else if(!deps[0]){
                const char *base=out?out:src;int b=0,dot=0;
                if(bx_dep_mark(base,out?1:0,&b,&dot)||bx_dep_len(dot,b,(int)sizeof depname))return clierror(bx_dep_why());
                memcpy(depname,base+b,(size_t)(dot-b));depname[dot-b]='.';depname[dot-b+1]='d';depname[dot-b+2]=0;effective_deps=depname;
            }
            same=host_same_output(host_symbols_path,effective_deps);
            if(same)return clierror(bx_same_why(same,1));
        }
    }
    if(host_provider_path&&host_filter_file(host_provider_path,host_target,&host_filter))return 1;
    char route[96]; int n;
    route_rc=cxt_text(target,mode,nsources,warnings,level,route,sizeof route,route_reason,sizeof route_reason);
    if(route_rc)return cxt_error(route_rc,route_reason);
    pkg = bx_pkg_env(pkg, pkg ? 0 : getenv("UNISA_CONTAINER"));
    rr_pkg_path = bx_pkg_path(pkg, argv[0]);
    rr_special_input = !!(host_symbols_path || host_provider_path || *fwd_lib_count());
    ResourceInput cli[256]; memset(cli,0,sizeof cli);int nimports=0;
    unsigned char process_argc[8],process_argv[8],memory_text[8],memory_data[8],process_dl[4][8];
    char **runargs=0;
#ifdef __linux__
    if (bx_run_args(runit, execrun, 1)) {
#else
    if (bx_run_args(runit, 0, 0)) {
#endif
        int count=bx_run_count(argc, argstart);
        int envn=0; while (cxe_environment(argc,argv,envn)) envn++;
        runargs=xrealloc(0,(count+envn+2)*sizeof(char *));
        runargs[0]=(char *)src;
        for (int j=argstart;j<argc;j++) runargs[j-argstart+1]=argv[j];
        runargs[count]=0;
        for (int j=0;j<=envn;j++) runargs[count+1+j]=cxe_environment(argc,argv,j);
        lw_put(process_argc,8,(unsigned long long)count); lw_put(process_argv,8,(unsigned long long)(uintptr_t)runargs);
    }
    ARGRESOURCE(0,"\0cli/defines",defs);
    ARGRESOURCE(1,"\0cli/undefines",undefs);
    /* A .cx unit has its prelude built in: it is compiled with cx.h ahead of any -include, so a .cx module needs no
       include line of its own (docs/cx-spec-draft.md; cx.h is guarded, so a file that still includes it is unchanged).
       Per unit: other units -- a .c beside it, the host-forwarding stub unit -- keep exactly the -include list.
       (No comment line here may begin with the include directive: -ftrim-libc's quotedinc() reads raw lines and a
       non-<...> one turns library trimming off for the whole unit -- it cost 45 KB per slice once.) */
    CliBlock forced_cx={0}; argbytes(&forced_cx,"cx.h"); argraw(&forced_cx,forced.b,forced.n);
    Buf forced_view={0}; forced_view.b=forced.b; forced_view.n=(int)forced.n;
    fwd_plain_list=&forced_view;                  /* borrowed byte view for the forwarding unit */
    if (cxm_on_now() && cxm_count()) {                       /* a unit that uses modules: cx.h, -include, then the interfaces */
        cxm_lists=xrealloc(0,cxm_count()*sizeof(CliBlock)); memset(cxm_lists,0,cxm_count()*sizeof(CliBlock));
        for (int u=0;u<cxm_count();u++) if (cxm_include_count(u)) {
            argraw(&cxm_lists[u],forced_cx.b,forced_cx.n);
            for (int k=0;k<cxm_include_count(u);k++) argbytes(&cxm_lists[u],cxm_incpath(u,k));
        }
    }
#define UNIT_INCLUDES(path) unit_includes(&cli[2],(path),&forced,&forced_cx)
    ARGRESOURCE(2,"\0cli/includes",forced);
    if (src) UNIT_INCLUDES(src);                 /* the single-source routes; the per-unit loops set it per unit */
    ARGRESOURCE(3,"\0cli/include-dir",incdir);
    ARGRESOURCE(4,"\0cli/nostdinc",nostd);
    RI=cli; NRI=5;
    if (runit) {
        cli[5].name=(const unsigned char *)"\0process/argc";cli[5].n=13;cli[5].data=process_argc;cli[5].len=8;
        cli[6].name=(const unsigned char *)"\0process/argv";cli[6].n=13;cli[6].data=process_argv;cli[6].len=8;
        NRI=9;
#if defined(__APPLE__) || defined(__linux__)
        static const char *dlkeys[4] = {"\0process/dl/0", "\0process/dl/1", "\0process/dl/2", "\0process/dl/3"};
        for (int j=0;j<4;j++) {
            memset(process_dl[j],0,8); /* host addresses are bound only after lower */
            cli[NRI].name=(const unsigned char *)dlkeys[j]; cli[NRI].n=0;
            cli[NRI].data=process_dl[j]; cli[NRI].len=8; NRI++;
        }
        nimports=4;
#endif
#ifdef _WIN32
        nimports=process_own_imports(cli+9,243,(long)process_own_imports);NRI=9+nimports;
#endif
    }
    /* The MAIN source's path, for the E2 stage to name in a diagnostic (and
       for __FILE__, N17b): E2 is handed token bytes and never sees argv, so
       the name has to travel as a resource like the -I list does.  Same shape:
       the key starts with a NUL byte, `n` is the key length WITHOUT a NUL, and
       the value is the path with no terminator.  It is only meaningful when E2
       reads it, so nothing else has to change.  [N17b prerequisite] */
    srcres.n=0; if (src) argbytes(&srcres,src);
    ARGRESOURCE(NRI,"\0cli/source",srcres); NRI++;
    ARGRESOURCE(NRI,"\0cli/quote-dir",quotedir); NRI++;
    ARGRESOURCE(NRI,"\0cli/fno-trim-libc",notrim); NRI++;
    ARGRESOURCE(NRI,"\0cli/werror",werror); NRI++;
    static const unsigned char fwd_one=1;
    int fwdwant = bx_fwd_want(runit, mode, fwd_second, tape_input, linking, target) == 1;
    /* 0.0.33 W2: win forwards too (ucrtbase/kernel32/ws2_32 by GetProcAddress, like the reference);
       its hostcall delivers four register arguments, so a wider call is refused by name (src/main.c:26) */
    { int fwd_max=0; if (bx_fwd_maxargs(target, &fwd_max)==0) fwd_set_maxargs(fwd_max); }
    if (fwdwant) { cli[NRI].name=(const unsigned char *)"\0cli/run-forward"; cli[NRI].n=16; cli[NRI].data=&fwd_one; cli[NRI].len=1; NRI++; }
    if (fwd_second && fwd_ccw_on()) { cli[NRI].name=(const unsigned char *)"\0cli/ccw"; cli[NRI].n=8; cli[NRI].data=&fwd_one; cli[NRI].len=1; NRI++; }
    int fwd_restart = 0;
    Buf objres={0}, funitres={0};
    if (objwant) bput(&objres,1,0);
    if (funit) bput(&funitres,1,0);
    if (objwant) { ARGRESOURCE(NRI,"\0cli/object",objres); NRI++; }
    if (funit) { ARGRESOURCE(NRI,"\0cli/funit",funitres); NRI++; }
    ARGRESOURCE(NRI,"\0cli/error-limit",errorlimit); NRI++;
    /* 0.0.28 F1: ask E1 for its constructor/destructor side-car; run.c strips it and fills
       \0cli/attributes (empty until a stage writes records) for E3 */
    { static const unsigned char one=1; cli[NRI].name=(const unsigned char *)"\0cli/f1"; cli[NRI].n=7; cli[NRI].data=&one; cli[NRI].len=1; NRI++; }
    free(ATTRS.b); ATTRS.b=0; ATTRS.n=0; ATTRS.cap=0; ATTR_UNIT=0;
    ATTR_SLOT=NRI; cli[NRI].name=(const unsigned char *)"\0cli/attributes"; cli[NRI].n=15; cli[NRI].data=0; cli[NRI].len=0; NRI++;
    /* Target selection is input data; the E2 model chooses its declaration. */
    const char *predefine_target=target;
    cli[NRI].name=(const unsigned char *)"\0cli/target"; cli[NRI].n=11;
    cli[NRI].data=(const unsigned char *)predefine_target;
    cli[NRI].len=(int)strlen(predefine_target); NRI++;

    FILE_READ_RECORD=deps!=0;
    {   /* UNISACC_RR_TIME construction stamp: package parse (footer, members, route table) in this process */
        struct timespec pk_t0, pk_t1; int pk_time = rr_wanted(getenv("UNISACC_RR_TIME"));
        if (pk_time) clock_gettime(CLOCK_MONOTONIC, &pk_t0);
        package(pkg ? pkg : argv[0]);
        if (pk_time) {
            long long ns; char line[1024];
            clock_gettime(CLOCK_MONOTONIC, &pk_t1);
            ns = rr_ns((long long)pk_t0.tv_sec, (long long)pk_t0.tv_nsec, (long long)pk_t1.tv_sec, (long long)pk_t1.tv_nsec);
            if (rr_construct(ns, line, (long)sizeof line) == 0) fputs(line, stderr);
            else fprintf(stderr, "RR_TIME\tconstruction\tns=%lld\n", ns);
        }
    }
    if (bx_obj_foreign(objwant, target)==1) {
        int lower=0,encoder=0;
        for (int i=0;i<PS;i++) {
            const char *const rowr=STAGES[i].route, *const rown=STAGES[i].name, *const rowo=STAGES[i].out;
            if (bx_stage_present(route,"lower",&rowr,&rown,1)==1) lower=1;
            if (bx_stage_out(route,"elf","object-v1",&rowr,&rown,&rowo,1)==1) encoder=1;
        }
        if (bx_obj_route(lower, encoder)) { unpackage();return clierror(bx_route_why()); }
    }
    HostCompileContext host_context={host_target,host_filter,mode,runit,host_symbols_path,cli,process_dl[0]};
    if(bx_sym_need((int)host_target,mode,host_symbols_path)&&!host_stage_present("host-symbols","symbols")){unpackage();return clierror(bx_model_why());}

    if (bx_tape_need(mode, tape_domain_available(route))) {
        unpackage();
        return clierror(bx_tape_why());
    }
    Buf in = {0}; int rc=0;
    int rr_pre_hit = 0;
    int rf_source_opened = 0; /* image RI: source:image front gate opens at most once */
    char rr_pre_id[65];
    if (linking) in=linked;
    else if (nsources==1) {
        in.b = source_read(src,&in.n);
        if(isbin){
            Buf decoded={0};int origin=tapebin_target(target);int decoded_rc=0;
            if(origin)decoded_rc=tbc_decode(in.b,(size_t)in.n,origin,force_origin,&decoded);
            int bad=bx_tapebin_bad(origin,decoded_rc);
            free(in.b);in.b=0;in.n=0;
            if(bad)return clierror(bx_tapebin_why());
            in=decoded;
        }
    }
    else {
        /* Before unit NN, try reuse keyed by raw module sources and CLI inputs.
         * Image (-run/-o): USE34 host ELF. Object/tape (-c/-S): USE33 final artifact. */
        {
            int rr_can = rr_eligible(nsources, warnings, tape_input, !!deps, fwd_second, target);
#ifdef __linux__
            if (rr_can && runit && nsources > 1) {
                /* Multi-TU -run: a pure probe of the entry the stub pass (fwd_second) stored after
                 * its host_compile when it ran as forward_exec with no host slot (rr_rs_*). Nothing
                 * about runit, mode, deps or the process/provider context changes before a hit: a
                 * miss is the unchanged cold run, a hit replays exactly the stub pass tail. */
                char probe[96];
                if (bx_route_line(0, target, 0, 0, level, probe, (int)sizeof probe)!=0) {
                    n = snprintf(probe, sizeof probe, "%s/multi/fwdexec/O%d", target, level);
                    if (bx_name_fit(n, (int)sizeof probe)) return clierror(bx_name_why());
                }
                if (!rr_sources_hex((char **)sources, nsources, rr_pre_id)) {
                    Buf cached = {0};
                    int lk = rr_lookup(argv[0], probe, target, level, rr_pre_id, &cached);
                    { const char *cache_why=bx_cache_why(lk, 0); if(cache_why)return clierror(cache_why); }
                    if (rr_wanted(getenv("UNISACC_RR_DEBUG"))) {
                        char line[1024];
                        if (bx_dbg_probe(0, lk, probe, line, 1024)==0) fputs(line, stderr);
                        else fprintf(stderr, "unisacc: rr early multi-run probe lk=%d route=%s\n", lk, probe);
                    }
                    if (lk == 1) {   /* replay the stub pass tail: forward_exec of the same image tape */
                        free(in.b); in = cached; fwd_exec = 1; execrun = 1; runit = 0; mode = 0; if (!deponly) deps = 0;
                        goto rr_run_replay;
                    }
                    /* No stub restart: a cold run without a host slot finished on the image route
                     * after E3 and stored its USE34 entry there; a hit takes that same switch. */
                    if (bx_route_line(1, target, 0, 0, level, probe, (int)sizeof probe)!=0) {
                        n = snprintf(probe, sizeof probe, "%s/multi/image/O%d", target, level);
                        if (bx_name_fit(n, (int)sizeof probe)) return clierror(bx_name_why());
                    }
                    lk = rr_lookup_host(argv[0], probe, target, level, rr_pre_id, &cached);
                    { const char *cache_why=bx_cache_why(lk, 0); if(cache_why)return clierror(cache_why); }
                    if (rr_wanted(getenv("UNISACC_RR_DEBUG"))) {
                        char line[1024];
                        if (bx_dbg_probe(1, lk, probe, line, 1024)==0) fputs(line, stderr);
                        else fprintf(stderr, "unisacc: rr early multi-image probe lk=%d route=%s\n", lk, probe);
                    }
                    if (lk == 1) {
                        execrun = 1; runit = 0; mode = 0; if (!deponly) deps = 0;
                        host_context.native = 0; host_context.mode = 0;
                        for (int j = 5; j < 13; j++) cli[j].n = 0;
                        strcpy(route, probe);
                        in = cached; rr_pre_hit = 1;
                    }
                }
            } else if (rr_can && (runit || mode == 0 || mode == 2 || mode == 5)) {
                int hx;
                if (runit) {
                    execrun = 1; runit = 0; mode = 0; if (!deponly) deps = 0;
                    host_context.native = 0; host_context.mode = 0;
                    for (int j = 5; j < 13; j++) cli[j].n = 0;
                }
                if (mode == 0) {
                    if (bx_route_line(2, target, warnings, nsources>1, level, route, (int)sizeof route)!=0) {
                        n = snprintf(route, sizeof route, "%s/%s%simage/O%d", target,
                                     warnings ? "warn/" : "", nsources > 1 ? "multi/" : "", level);
                        if (bx_name_fit(n, (int)sizeof route)) return clierror(bx_name_why());
                    }
                }
                hx = rr_sources_hex((char **)sources, nsources, rr_pre_id);
                if (hx && rr_wanted(getenv("UNISACC_RR_DEBUG"))) {
                    char line[1024];
                    if (bx_dbg_hexfail(nsources, line, 1024)==0) fputs(line, stderr);
                    else fprintf(stderr, "unisacc: rr early sources_hex failed nsources=%d\n", nsources);
                }
                if (!hx) {
                    Buf cached = {0};
                    int lk;
                    if (mode == 0)
                        lk = rr_lookup_host(argv[0], route, target, level, rr_pre_id, &cached);
                    else
                        lk = rr_lookup(argv[0], route, target, level, rr_pre_id, &cached);
                    { const char *cache_why=bx_cache_why(lk, 0); if(cache_why)return clierror(cache_why); }
                    if (rr_wanted(getenv("UNISACC_RR_DEBUG"))) {
                        char line[1024];
                        if (bx_dbg_lookup(lk, mode, route, line, 1024)==0) fputs(line, stderr);
                        else fprintf(stderr, "unisacc: rr early lookup lk=%d mode=%d route=%s\n", lk, mode, route);
                    }
                    if (lk == 1) { in = cached; rr_pre_hit = 1; }
                }
            }
#endif
            if (rr_pre_hit) goto rr_after_units;
        }
#ifdef __linux__
        if (runit && nsources > 1 && !deps &&
            rr_eligible(nsources, warnings, tape_input, !!deps, fwd_second, target)) {
            FILE_READ_RECORD = 1; FILE_READ_COUNT = 0; FILE_READ_PATHS = 0;
            FILE_ABSENT_COUNT = 0; FILE_ABSENT_PATHS = 0; rr_units_rec = 1;
        }
#endif
        if (RF_STRICT || rf_capture_dir()) {   /* SC1 multi-source identity: before the first unit row is recorded */
            const char *uj0=getenv("UNISA_JOBS"); int rfjobs = bx_jobs(uj0);   /* the parallel unit path is the default (8) */
            if (rfjobs>1) { char line[1024]; if (rr_jobs_line(rfjobs, line, (long)sizeof line)==0) fputs(line, stderr); else fprintf(stderr,"unisacc: INTERFACE_ERROR resource-capture-parallel-unsupported UNISA_JOBS=%d (capture and strict multi-source runs need UNISA_JOBS=1)\n", rfjobs); return 2; }
            unsigned char **cb=xrealloc(0,nsources*sizeof *cb); int *cl=xrealloc(0,nsources*sizeof *cl);
            for (int j=0;j<nsources;j++) { cl[j]=0; cb[j]=source_read(sources[j],&cl[j]); }
            int fn=0; unsigned char *fb=rf_source_frame(nsources,sources,(const unsigned char **)cb,cl,&fn);
            /* Image RI: open source:image once here (not e3). Tape keeps historical e3 label. */
            int g = fb ? rf_identity_gate(fb,fn,mode,route, mode==0 ? "source" : "e3") : 2;
            if (!g && mode==0) rf_source_opened = 1;
            free(fb); for (int j=0;j<nsources;j++) free(cb[j]); free(cb); free(cl);
            if (g) return g;
        }
        char unitroute[96]; if (bx_route_line(3, target, warnings, 0, 0, unitroute, (int)sizeof unitroute)!=0) snprintf(unitroute,sizeof unitroute,"%s/%sunit",target,warnings ? "warn/" : "");
        const char *uc=getenv("UNISA_FWD_UNITCACHE");
        int cacheon = bx_unit_cache(deps, uc);
#ifndef _WIN32
        const char *uj=getenv("UNISA_JOBS"); int jobs = bx_jobs_pool(bx_jobs(uj));   /* F4: 8 with a rolling pool */
        if (cacheon && !fwd_second && !warnings && jobs > 1 && nsources > 1 && nucache + nsources <= 256) {
            const char *td=getenv("TMPDIR"); char pf[200]; int me=(int)getpid();
            long base = ATTRS.n ? ATTRS.n : 4;
            /* F4: a rolling pool -- a finished worker's slot takes the next unit at once */
            int pids[256], live=0, next=0;
            for (int j=0;j<nsources;j++) pids[j]=0;
            while (next<nsources || live>0) {
                while (next<nsources && live<jobs) {
                    int j=next++; pids[j]=(int)fork();
                    if (pids[j]==0) {
                        if (!freopen("/dev/null",bx_file_mode(2),stderr)) _exit(1);   /* a failing unit is reported by the serial path, once */
                        Buf unit={0}; ATTR_UNIT=j; unit.b=source_read(sources[j],&unit.n);
                        UNIT_INCLUDES(sources[j]);
                        int r=runroute(unitroute,&unit,sources[j]); if (r) _exit(1);
                        if (bx_unit_file(bx_temp_dir(1, td, 0), me, j, pf, (int)sizeof pf)!=0) snprintf(pf,sizeof pf,"%s/ua-unit-%d-%d",td&&*td?td:"/tmp",me,j);
                        FILE *f=fopen(pf,bx_file_mode(1)); if (!f) _exit(1);
                        long after = ATTRS.n ? ATTRS.n : 4; unsigned cnt=(unsigned)((after-base)/9);
                        par_put(f,(unsigned)unit.n); fwrite(unit.b,1,unit.n,f); par_put(f,cnt);
                        for (unsigned r2=0;r2<cnt;r2++) fwrite(ATTRS.b+base+9*r2+4,1,5,f);
                        par_paths(f,FILE_READ_PATHS,rr_units_rec?FILE_READ_COUNT:0);
                        par_paths(f,FILE_ABSENT_PATHS,rr_units_rec?FILE_ABSENT_COUNT:0);
                        if (fclose(f)) _exit(1);
                        _exit(0);
                    }
                    if (pids[j]>0) live++;
                }
                if (!live) break;
                int st=0, w=(int)waitpid(-1,&st,0), j=-1;
                if (w<=0) break;
                for (int q=0;q<nsources;q++) if (pids[q]==w) { j=q; break; }
                if (j<0) continue;
                live--; pids[j]=0;
                if (!WIFEXITED(st) || WEXITSTATUS(st)) continue;
                if (bx_unit_file(bx_temp_dir(1, td, 0), me, j, pf, (int)sizeof pf)!=0) snprintf(pf,sizeof pf,"%s/ua-unit-%d-%d",td&&*td?td:"/tmp",me,j);
                FILE *f=fopen(pf,bx_file_mode(0)); if (!f) continue;
                unsigned n=0,cnt=0; int ok=par_get(f,&n);
                unsigned char *b=ok?xrealloc(0,n?n:1):0; if (ok) ok=fread(b,1,n,f)==n && par_get(f,&cnt);
                unsigned char *at=ok?xrealloc(0,cnt?5*cnt:1):0; if (ok) ok=fread(at,1,5*(size_t)cnt,f)==5*(size_t)cnt;
                if (ok) ok=par_take_paths(f,0)&&par_take_paths(f,1);   /* unit reads join the parent's receipts */
                fclose(f); remove(pf);
                if (!ok || ucache_find(sources[j])>=0) { free(b); free(at); continue; }
                int k=nucache++; ucache[k].path=sources[j]; ucache[k].n=(int)n; ucache[k].b=b; ucache[k].nattr=cnt; ucache[k].attr=at;
            }
        }
        int prefilled = !fwd_second && nucache > 0;
#else
        int prefilled = 0;
#endif
        for (int j=0;j<nsources;j++) {
            Buf unit={0}; ATTR_UNIT=j;
            int hit = cacheon && (fwd_second || prefilled) ? ucache_find(sources[j]) : -1;
            if (hit >= 0) {                   /* the first pass's bytes and records for this unit */
                unit.n=ucache[hit].n; unit.b=xrealloc(0,unit.n?unit.n:1); memcpy(unit.b,ucache[hit].b,unit.n);
                ucache_replay(hit);
            } else {
                long before = ATTRS.n ? ATTRS.n : 4;
                unit.b=source_read(sources[j],&unit.n);
                UNIT_INCLUDES(sources[j]);
                rc=fwd_second&&!strcmp(sources[j],fwd_path) ? fwd_front(unitroute,0,&unit,sources[j]) : runroute(unitroute,&unit,sources[j]);
                if (rc) { free(unit.b); break; }
                if (cacheon && fwdwant && nucache < 256) {
                    int k=nucache++; ucache[k].path=sources[j];
                    ucache[k].n=unit.n; ucache[k].b=xrealloc(0,unit.n?unit.n:1); memcpy(ucache[k].b,unit.b,unit.n);
                    long after = ATTRS.n ? ATTRS.n : 4; unsigned cnt=(unsigned)((after-before)/9);
                    ucache[k].nattr=cnt; ucache[k].attr=xrealloc(0,cnt?5*cnt:1);
                    for (unsigned r=0;r<cnt;r++) memcpy(ucache[k].attr+5*r, ATTRS.b+before+9*r+4, 5);
                }
            }
            int namelen=strlen(sources[j]);
            long framed=(long)unit.n+4+(long)namelen;
            if (bx_frame_fit(unit.n, namelen)) return clierror(bx_frame_why());
            for (int k=0;k<4;k++) bput(&in,(framed>>(8*k))&255,0);
            {
                for (int k=0;k<4;k++) bput(&in,(namelen>>(8*k))&255,0);
                for (int k=0;k<namelen;k++) bput(&in,(unsigned char)sources[j][k],0);
            }
            for (int k=0;k<unit.n;k++) bput(&in,unit.b[k],0);
            free(unit.b);
        }
        if (rr_units_rec) {   /* hand the unit receipts to the E3 store; recording off until then */
            rr_u_paths = FILE_READ_PATHS; rr_u_n = FILE_READ_COUNT;
            rr_u_abs = FILE_ABSENT_PATHS; rr_u_abn = FILE_ABSENT_COUNT;
            FILE_READ_RECORD = 0; FILE_READ_COUNT = 0; FILE_READ_PATHS = 0;
            FILE_ABSENT_COUNT = 0; FILE_ABSENT_PATHS = 0;
        }
    }
rr_after_units:
    free(in.at); in.at=0; /* Framing positions are not input to model inference. */
    /* Tape keeps the historical single-source gate (raw bytes labeled e3:…/tape/O*).
     * Image RI B does not gate raw C here: e3 identity is applied after e1, before e3. */
    if (nsources<=1 && mode != 0 && (RF_STRICT || rf_capture_dir())) { int g = rf_identity_gate(in.b,in.n,mode,route,"e3"); if (g) return g; }
    if (!rc && tape_input) rc=tape_domain(route,&in,src); /* decoded Tapebin uses the same model */
    if (!rc && !(tape_input && mode==2)) {
        if (fwdwant) {
            /* E3 + post-host reuse. rr_hit: 1=e3, 2=host.
             * Eligible -run switches to image+forward_exec so USE34 caches a relocatable ELF and
             * skips the per-process memory NN (~18ms hot). Multi-TU may already be rr_pre_hit. */
            {
                int rr_hit = rr_pre_hit ? 2 : 0;
                char rr_id[65];
                /* Multi-TU -run takes the same pure-probe path as single-TU (rr_run_lookup). */
                int rr_can = rr_eligible(nsources, warnings, tape_input, !!deps, fwd_second, target) || rr_pre_hit;
#ifdef __linux__
                int rr_run_lookup = rr_can && runit;
#else
                int rr_run_lookup = 0;
#endif
                char rr_image_route[96];
                const char *rr_host_route = route;
                struct timespec rr_t0, rr_t1;
                int rr_time = rr_wanted(getenv("UNISACC_RR_TIME"));
                if (rr_pre_hit) memcpy(rr_id, rr_pre_id, 65);
#ifdef __linux__
                if (rr_run_lookup) {
                    /* Probe the image cache without changing the cold run route.
                     * E3 must still see process slots when a host provider is needed. */
                    if (bx_route_line(2, target, warnings, nsources>1, level, rr_image_route, (int)sizeof rr_image_route)!=0) {
                        n = snprintf(rr_image_route, sizeof rr_image_route, "%s/%s%simage/O%d", target,
                                     warnings ? "warn/" : "", nsources > 1 ? "multi/" : "", level);
                        if (bx_name_fit(n, (int)sizeof rr_image_route)) return clierror(bx_name_why());
                    }
                    rr_host_route = rr_image_route;
                }
#endif
                if (rr_time) clock_gettime(CLOCK_MONOTONIC, &rr_t0);
                if (!rr_pre_hit) {
                    if (rr_sources_hex((char **)sources, nsources, rr_id))
                        rr_can = 0;
                    if (rr_can) {
                        Buf cached = {0};
                        int lk = rr_lookup_host(argv[0], rr_host_route, target, level, rr_id, &cached);
                        { const char *cache_why=bx_cache_why(lk, 1); if(cache_why)rc=clierror(cache_why); else
                        if (lk == 1) {
                            free(in.b); in = cached; rr_hit = 2;
#ifdef __linux__
                            if (rr_run_lookup) {
                                execrun = 1; runit = 0; mode = 0; if (!deponly) deps = 0;
                                host_context.native = 0; host_context.mode = 0;
                                for (int j = 5; j < 13; j++) cli[j].n = 0;
                                strcpy(route, rr_image_route);
                            }
#endif
                        } else if (!rr_run_lookup) {
                            lk = rr_lookup(argv[0], route, target, level, rr_id, &cached);
                            { const char *cache_why=bx_cache_why(lk, 2); if(cache_why)rc=clierror(cache_why); else
                            if (lk == 1) { free(in.b); in = cached; rr_hit = 1; } }
                        }
                        }
                    }
                }
                if (rr_time) {
                    long long ns; char line[1024];
                    clock_gettime(CLOCK_MONOTONIC, &rr_t1);
                    ns = rr_ns((long long)rr_t0.tv_sec, (long long)rr_t0.tv_nsec, (long long)rr_t1.tv_sec, (long long)rr_t1.tv_nsec);
                    if (rr_lookup_line(rr_hit, ns, line, (long)sizeof line) == 0) fputs(line, stderr);
                    else fprintf(stderr, "RR_TIME\tlookup_hit=%d\tns=%lld\n", rr_hit, ns);
                }
                if (!rc && rr_hit < 2) {
                    int rr_rec = 0, rr_old_rec = FILE_READ_RECORD, rr_old_n = FILE_READ_COUNT, rr_old_ab = FILE_ABSENT_COUNT;
                    char **rr_old_p = FILE_READ_PATHS, **rr_old_a = FILE_ABSENT_PATHS;
                    if (!rr_hit) {
                        if (rr_can && !deps) {
                            FILE_READ_RECORD = 1; FILE_READ_COUNT = 0; FILE_READ_PATHS = 0;
                            FILE_ABSENT_COUNT = 0; FILE_ABSENT_PATHS = 0; rr_rec = 1;
                            if (rr_units_rec) {
                                FILE_READ_PATHS = rr_u_paths; FILE_READ_COUNT = rr_u_n;
                                FILE_ABSENT_PATHS = rr_u_abs; FILE_ABSENT_COUNT = rr_u_abn;
                            }
                        }
                        /* Image RI: source:image opens at most once (single-TU here, or multi before units).
                         * Front-end last stage: single uses e1 (e2+e1); multi/image has units then e3 (no e1).
                         * E3 second identity seals tokens+ATTRS; then e3 only. */
                        if (mode == 0 && (RF_STRICT || rf_capture_dir())) {
                            unsigned char *idb = 0; int idn = 0, g = 0;
                            const char *front_last = nsources > 1 ? "units" : "e1";
                            if (!rf_source_opened) {
                                g = rf_identity_gate(in.b,in.n,mode,route,"source");
                                if (g) rc = g; else rf_source_opened = 1;
                            }
                            if (!rc) rc = runroute_range(route,NULL,front_last,&in,src);
                            if (!rc) {
                                idb = rf_e3_identity_bytes(in.b,in.n,ATTRS.b,ATTRS.n,&idn);
                                g = idb ? rf_e3_second_identity(idb,idn,mode,route) : 2;
                                free(idb);
                                if (g) rc = g;
                            }
                            if (!rc) rc = runroute_range(route,"e3","e3",&in,src);
                        } else {
                            rc = runroute_range(route,NULL,"e3",&in,src);
                        }
                        if (!rc && rr_can && !rr_run_lookup)
                            (void)rr_store(argv[0], route, target, level, rr_id, &in, &ATTRS,
                                           FILE_READ_PATHS, FILE_READ_COUNT,
                                           FILE_ABSENT_PATHS, FILE_ABSENT_COUNT);
                        if (rr_rec) FILE_READ_RECORD = rr_old_rec;
                    }
                    if (!rc) {
                        Buf tape={0}; int fr=fwd_sidecar(&in,&tape);
                        if (fr<0) rc=1;
                        else if (fr==0) {
                            if (rr_can && rr_run_lookup && rr_rec && host_dl_slot(1)==0)
                                (void)rr_store(argv[0], rr_host_route, target, level, rr_id, &in, &ATTRS,
                                               FILE_READ_PATHS, FILE_READ_COUNT,
                                               FILE_ABSENT_PATHS, FILE_ABSENT_COUNT);
                            free(in.b); in=tape;
#ifdef __linux__
                            if (runit && host_dl_slot(1)==0) {
                                /* No host forwarding: E3 used the run route, but
                                 * the remaining tape may safely become an image. */
                                execrun=1; runit=0; mode=0; if (!deponly) deps=0;
                                host_context.native=0; host_context.mode=0;
                                for (int j=5;j<13;j++) cli[j].n=0;
                                if (bx_route_line(2, target, warnings, nsources>1, level, route, (int)sizeof route)!=0) {
                                    n=snprintf(route,sizeof route,"%s/%s%simage/O%d",target,warnings ? "warn/" : "",nsources>1 ? "multi/" : "",level);
                                    if (bx_name_fit(n, (int)sizeof route)) return clierror(bx_name_why());
                                }
                            } else if (runit) rr_can=0;
#endif
                            rc=tape_domain(route,&in,src); if (!rc) rc=host_compile(route,level ? "e4" : "prune",&in,src,&host_context);
                        }   /* side-car extraction precedes Tape validation */
                        else {
                            /* Forwarded symbols can depend on run-only process slots.
                             * A cold image key must not acquire their E3 receipts. Without a
                             * host slot the cold run itself finishes as this image (forward_exec
                             * below), so the E3 entry is stored exactly as in the fr==0 branch. */
                            if (rr_run_lookup) {
                                if (rr_rec && host_dl_slot(1)==0)
                                    (void)rr_store(argv[0], rr_host_route, target, level, rr_id, &in, &ATTRS,
                                                   FILE_READ_PATHS, FILE_READ_COUNT,
                                                   FILE_ABSENT_PATHS, FILE_ABSENT_COUNT);
                                else rr_can=0;
                            }
#ifdef __linux__
                            if (runit && host_dl_slot(1)==0) {
                                /* Static compiler image: finish as the image route does and exec the result
                                   (forward_exec).  Stages up to e3 are the same models on both routes; the
                                   run's process and loader resources are withdrawn, as an image has none. */
                                fwd_exec=1; execrun=1; runit=0; mode=0; if (!deponly) deps=0; host_context.native=0;host_context.mode=0;
                                for (int j=5;j<13;j++) cli[j].n=0;
                                if (bx_route_line(2, target, warnings, nsources>1, level, route, (int)sizeof route)!=0) {
                                    n=snprintf(route,sizeof route,"%s/%s%simage/O%d",target,warnings ? "warn/" : "",nsources>1 ? "multi/" : "",level);
                                    if (bx_name_fit(n, (int)sizeof route)) return clierror(bx_name_why());
                                }
                            } else if (runit) rr_can=0;
#endif
                            Buf merged={0};
                            int fast=fwd_ccw_on() ? 0 : fwd_incremental(target,level,(const FwiBuf *)&tape,nsources,(FwiBuf *)&merged,(FwiRi *)RI,&NRI,(FwiBuf *)&ATTRS,&ATTR_UNIT,&ATTR_SLOT,fwd_path,fwi_replay,fwi_over,fwi_count,fwi_text,fwi_alloc,fwi_release,fwi_front,fwi_range,fwi_merge);
                            if (fast>0) {
                                free(in.b); free(tape.b); in=merged;
                                rc=tape_domain(route,&in,src);
                                if (!rc) rc=host_compile(route,level ? "e4" : "prune",&in,src,&host_context);
                            } else if (fast<0) { free(tape.b); rc=1; }
                            else {
                                free(tape.b); fwd_restart=1;
                                if (rr_run_lookup && rr_rec && host_dl_slot(1)==0 && nsources > 1) {
                                    rr_rs_armed = 1; memcpy(rr_rs_id, rr_id, 65);
                                    rr_rs_p = rr_paths_copy(FILE_READ_PATHS, FILE_READ_COUNT); rr_rs_n = FILE_READ_COUNT;
                                    rr_rs_a = rr_paths_copy(FILE_ABSENT_PATHS, FILE_ABSENT_COUNT); rr_rs_an = FILE_ABSENT_COUNT;
                                }
                            }
                        }
                    }
                    if (!rc && rr_can && !fwd_restart) {
                        int hs = rr_store_host(argv[0], route, target, level, rr_id, &in, &ATTRS);
                        if (hs && rr_wanted(getenv("UNISACC_RR_DEBUG"))) {
                            char line[1024];
                            if (bx_dbg_host(hs, in.n, ATTRS.n, line, 1024)==0) fputs(line, stderr);
                            else fprintf(stderr, "unisacc: host reuse store failed rc=%d tape_n=%d attrs_n=%d\n",
                                    hs, in.n, ATTRS.n);
                        }
                    }
                    if (rr_rec) {
                        for (int ri = 0; ri < FILE_READ_COUNT; ri++) free(FILE_READ_PATHS[ri]);
                        free(FILE_READ_PATHS);
                        for (int ri = 0; ri < FILE_ABSENT_COUNT; ri++) free(FILE_ABSENT_PATHS[ri]);
                        free(FILE_ABSENT_PATHS);
                        FILE_READ_RECORD = rr_old_rec; FILE_READ_COUNT = rr_old_n; FILE_READ_PATHS = rr_old_p;
                        FILE_ABSENT_COUNT = rr_old_ab; FILE_ABSENT_PATHS = rr_old_a;
                    }
                }
            }
        } else {
            /* -c/-S (object/tape) final-artifact reuse (USE33 under object|tape route). */
#ifdef __linux__
            {
                int rr_hit = rr_pre_hit ? 1 : 0;
                char rr_id[65];
                int rr_can = rr_eligible(nsources, warnings, tape_input, !!deps, fwd_second, target) || rr_pre_hit;
                memset(rr_id, 0, sizeof rr_id);
                if (rr_pre_hit) memcpy(rr_id, rr_pre_id, 65);
                if (!rr_pre_hit && rr_can && (mode == 2 || mode == 5)) {
                    if (rr_sources_hex((char **)sources, nsources, rr_id))
                        rr_can = 0;
                    if (rr_can) {
                        Buf cached = {0};
                        int lk = rr_lookup(argv[0], route, target, level, rr_id, &cached);
                        { const char *cache_why=bx_cache_why(lk, 3); if(cache_why)rc=clierror(cache_why); else
                        if (lk == 1) { free(in.b); in = cached; rr_hit = 1; } }
                        if (rr_wanted(getenv("UNISACC_RR_TIME"))) {
                            char line[1024];
                            if (rr_artifact_line(rr_hit, mode, line, (long)sizeof line) == 0) fputs(line, stderr);
                            else fprintf(stderr, "RR_TIME\tartifact_hit=%d\tmode=%d\n", rr_hit, mode);
                        }
                    }
                }
                if (!rc && !rr_hit) {
                    int rr_rec = 0, rr_old_rec = FILE_READ_RECORD, rr_old_n = FILE_READ_COUNT, rr_old_ab = FILE_ABSENT_COUNT;
                    char **rr_old_p = FILE_READ_PATHS, **rr_old_a = FILE_ABSENT_PATHS;
                    if (rr_can && (mode == 2 || mode == 5) && !deps) {
                        FILE_READ_RECORD = 1; FILE_READ_COUNT = 0; FILE_READ_PATHS = 0;
                        FILE_ABSENT_COUNT = 0; FILE_ABSENT_PATHS = 0; rr_rec = 1;
                        if (!rr_id[0]) {
                            if (rr_sources_hex((char **)sources, nsources, rr_id))
                                rr_can = 0;
                        }
                    }
                    rc = host_compile(route, tape_input ? (level ? "e4" : "prune") : 0, &in, src, &host_context);
                    if (!rc && fwd_second && rr_rs_armed && execrun && host_dl_slot(1)==0) {
                        char rsr[96];
                        if (bx_route_line(0, target, 0, 0, level, rsr, (int)sizeof rsr)==0 || snprintf(rsr, sizeof rsr, "%s/multi/fwdexec/O%d", target, level) < (int)sizeof rsr)
                            { int rs_rc = rr_store(argv[0], rsr, target, level, rr_rs_id, &in, &ATTRS,
                                           rr_rs_p, rr_rs_n, rr_rs_a, rr_rs_an);
                              if (rr_wanted(getenv("UNISACC_RR_DEBUG"))) {
                                  char line[1024];
                                  if (bx_dbg_stub(rs_rc, rr_rs_n, rr_rs_an, line, 1024)==0) fputs(line, stderr);
                                  else fprintf(stderr, "unisacc: rr stub store rc=%d receipts=%d absent=%d\n", rs_rc, rr_rs_n, rr_rs_an);
                              } }
                        rr_rs_armed = 0;
                    }
                    if (!rc && rr_can && (mode == 2 || mode == 5) && rr_id[0]) {
                        (void)rr_store(argv[0], route, target, level, rr_id, &in, &ATTRS,
                                       FILE_READ_PATHS, FILE_READ_COUNT,
                                       FILE_ABSENT_PATHS, FILE_ABSENT_COUNT);
                    }
                    if (rr_rec) {
                        for (int ri = 0; ri < FILE_READ_COUNT; ri++) free(FILE_READ_PATHS[ri]);
                        free(FILE_READ_PATHS);
                        for (int ri = 0; ri < FILE_ABSENT_COUNT; ri++) free(FILE_ABSENT_PATHS[ri]);
                        free(FILE_ABSENT_PATHS);
                        FILE_READ_RECORD = rr_old_rec; FILE_READ_COUNT = rr_old_n; FILE_READ_PATHS = rr_old_p;
                        FILE_ABSENT_COUNT = rr_old_ab; FILE_ABSENT_PATHS = rr_old_a;
                    }
                }
            }
#else
            rc = host_compile(route,tape_input ? (level ? "e4" : "prune") : 0,&in,src,&host_context);
#endif
        }
    }
    if (!rc && fwd_restart) {                    /* compile again with the stubs as the last unit */
        unpackage(); RI=0; NRI=0; ATTR_SLOT=-1;
        char **av=xrealloc(0,(argc+2)*sizeof(char *)); int j=0,put=0;
        for (int i=0;i<argc;i++) { av[j++]=argv[i]; if (!put && argv[i]==src) { av[j++]=fwd_path; put=1; } }
        av[j]=0; fwd_second=1;
        return main(j,av);
    }
rr_run_replay:   /* multi-TU hit: the stub pass's post-host_compile tape, then its forward_exec */
    if(!rc&&mode==2&&host_symbols_path){Buf symbols={0};HostRequirements requirements;rc=host_symbol_stream(route,&in,src,&host_context,&symbols,&requirements);if(!rc)rc=rf_trace_finish();if(!rc)rc=host_side_write(host_symbols_path,&symbols);free(symbols.b);}
    MemoryImage plan; MemoryMap mapping;
    if (!rc && runit) {
        struct timespec mem_t0, mem_t1;
        int mem_time = rr_wanted(getenv("UNISACC_RR_TIME"));
        if (mem_time) clock_gettime(CLOCK_MONOTONIC, &mem_t0);
        if (bx_route_line(4, target, 0, 0, 0, route, (int)sizeof route)!=0) snprintf(route,sizeof route,"%s/memory",target);
        const char *twopass=getenv("UNISA_MEMORY_TWOPASS");
        if (!bx_two_pass(twopass)) {
            unsigned char memory_capacity[8];
            memory_reserve(&mapping);
            lw_put(memory_text,8,(unsigned long long)(uintptr_t)mapping.base);
            lw_put(memory_capacity,8,(unsigned long long)mapping.reserved);
            cli[7].name=(const unsigned char *)"\0memory/text";cli[7].n=12;cli[7].data=memory_text;cli[7].len=8;
            cli[8].name=(const unsigned char *)"\0memory/reserve";cli[8].n=15;cli[8].data=memory_capacity;cli[8].len=8;
            rc=runroute(route,&in,src);
            if (!rc) { memory_image(&in,&plan);memory_commit(&plan,&mapping); }
        } else {
        Buf first={0};first.n=in.n;first.b=xrealloc(0,in.n);memcpy(first.b,in.b,in.n);
        rc=runroute(route,&first,src);
        if (!rc) {
            memory_image(&first,&plan);memory_map(&plan,&mapping);free(first.b);
            lw_put(memory_text,8,(unsigned long long)(uintptr_t)mapping.base);lw_put(memory_data,8,(unsigned long long)(uintptr_t)(mapping.base+mapping.dataoff)+(unsigned long long)mr_apple(
#ifdef __APPLE__
                1
#else
                0
#endif
            ));
            cli[7].name=(const unsigned char *)"\0memory/text";cli[7].n=12;cli[7].data=memory_text;cli[7].len=8;
            cli[8].name=(const unsigned char *)"\0memory/data";cli[8].n=12;cli[8].data=memory_data;cli[8].len=8;
            rc=runroute(route,&in,src);
        }
        }
        if (mem_time) {
            long long ns; char line[1024];
            clock_gettime(CLOCK_MONOTONIC, &mem_t1);
            ns = rr_ns((long long)mem_t0.tv_sec, (long long)mem_t0.tv_nsec, (long long)mem_t1.tv_sec, (long long)mem_t1.tv_nsec);
            if (rr_memory_line(rc, ns, line, (long)sizeof line) == 0) fputs(line, stderr);
            else fprintf(stderr, "RR_TIME\tmemory_bind\trc=%d\tns=%lld\n", rc, ns);
        }
    }
    unpackage(); RI=0; NRI=0; ATTR_SLOT=-1;
    free(defs.b); free(undefs.b); free(forced.b); free(forced_cx.b); free(incdir.b);
    if (cxm_lists) { for (int u=0;u<cxm_count();u++) free(cxm_lists[u].b); free(cxm_lists); cxm_lists=0; }
    free(quotedir.b); free(srcres.b); free(errorlimit.b);
    free(werror.b); free(werror.at);
    if (rc) return rc;
    if(rf_trace_finish())return 2;
    if(emitbin){
        int enc=tbc_encode_product(&in,tape_input ? 0 : tapebin_target(target));
        if(bx_encode_bad(emitbin,enc))return clierror(bx_encode_why());
    }
    if (bx_dep_gate(deps, deponly, mode, runit)) {
        char *name=0; const char *chosen=0;
        int only=bx_dep_only(deps, deponly, out, &chosen);
        if (only==1) deps=chosen;       /* gcc: -M goes to -o or stdout */
        else if (!deps[0]) {
            /* -MD: -o with suffix .d, else the input's basename .d in the current directory */
            const char *base=out ? out : src; int b=0, dot=0;
            if (bx_dep_mark(base, out?1:0, &b, &dot)) return clierror(bx_dep_why());
            name=xrealloc(0,dot-b+3);memcpy(name,base+b,dot-b);name[dot-b]='.';name[dot-b+1]='d';name[dot-b+2]=0;deps=name;
        }
        char default_dep_target[520]; const char *dep_target=0;
        if (bx_dep_target(out, deponly, &dep_target)) {
            if (cxd_default_target(src,default_dep_target,sizeof default_dep_target,deps_reason,sizeof deps_reason)) { free(name); return clierror(deps_reason); }
            dep_target=default_dep_target;
        }
        int drc=cxd_write(deps,dep_target,sources,nsources,deptargets,depquoted,ndeptargets,(const char **)FILE_READ_PATHS,FILE_READ_COUNT,depphony,deps_reason,sizeof deps_reason);
        free(name);
        int dep_ret=bx_dep_ret(drc, deponly);
        if (dep_ret==2) return clierror(deps_reason);
        if (dep_ret==0) { free(sources); free(deptargets); free(depquoted); free(in.b); return 0; }
    }
    free(sources); free(deptargets); free(depquoted);
    if (!bx_run_enter(runit)) return memory_enter(&mapping,&plan,&in);
#ifdef __linux__
    if (!bx_exec_forward(execrun)) return forward_exec(&in,runargs);
#endif
    {
        const char *image=0;
#ifdef _WIN32
        int image_win=1;
#else
        int image_win=0;
#endif
        if (bx_image_name(out, mode, explicit_image, image_win, &image)==1) out=image;
    }
    if (asmwant) {
        asmbuf.n=0;
        if (at_bind(asm_emit)) { free(in.b); return 1; }
        if (at_dis((char *)in.b,in.n)) { char line[1024]; if (bx_s_line(at_error(),line,1024)==0) fputs(line,stderr); else fprintf(stderr,"unisacc: error: -S: %s\n",at_error()); free(in.b); return 1; }
        free(in.b); in.b=asmbuf.b; in.n=asmbuf.n; memset(&asmbuf,0,sizeof asmbuf);
    }
    /* No destination is opened before successful compilation. */
    struct timespec ow_t0, ow_t1; int ow_time = rr_wanted(getenv("UNISACC_RR_TIME"));
    if (ow_time) clock_gettime(CLOCK_MONOTONIC, &ow_t0);
    char output_reason[128];
    int done=in.n;
    int output_failed=cxo_write_file(out,in.b,in.n,output_reason,sizeof output_reason);
    free(in.b);
    if(output_failed)return clierror(output_reason);
    if (ow_time) {   /* UNISACC_RR_TIME output stamp: open + write + close of the image/tape */
        long long ns; char line[1024];
        clock_gettime(CLOCK_MONOTONIC, &ow_t1);
        ns = rr_ns((long long)ow_t0.tv_sec, (long long)ow_t0.tv_nsec, (long long)ow_t1.tv_sec, (long long)ow_t1.tv_nsec);
        if (rr_output_line(1, done, ns, line, (long)sizeof line) == 0) fputs(line, stderr);
        else fprintf(stderr, "RR_TIME\toutput\tkind=image\tbytes=%d\tns=%lld\n", done, ns);
    }
    return 0;
}

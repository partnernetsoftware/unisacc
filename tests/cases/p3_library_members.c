/* p3_library_members.c PACKAGE TARGET PHASE -- test-only native host for tests/p3-library-members.cx.
   Links a private copy of the runtime and uses only the public library API. Each phase runs on
   fresh contexts and prints exactly one line, "OK <phase> <detail>" or "ERR <phase> <us_error>":
     contract  address-free binding contract query        (library-contract route)
     compile   injected host functions, compile, relocate,
               call: two contexts keep their own hosts      (library-contract + library-selection)
     carrier   script export of a union by value, compile
               and relocate                                 (<target>/nativeabi, if certified)
   The exit status is 0 for OK and 1 for ERR; argument errors exit 2.
   PHASE api INCLUDE_DIR is an observer for library-domain cells: it prints one line per cell,
   "<cell>|<key>=<observation>|...", where an observation is "<rc>:<us_error>" for a refusal and
   the called value for a success. It asserts nothing itself; tests/p3-library-members.cx holds
   the expected lines. It exits 0 once every line is printed; a crash leaves lines missing. */
#include "exec/c/libunisacc.c"
#include <dirent.h>

static long add_a(long x, long y) { return x + y + 1000; }
static long add_b(long x, long y) { return x + y + 2000; }
static long twice_a(long x) { return 2 * x + 10; }
static long twice_b(long x) { return 2 * x + 20; }

static us_type_descriptor integer(void) { us_type_descriptor t; memset(&t, 0, sizeof t); t.kind = 1; t.width = 8; return t; }

static us_context *declare(const char *package, int b) {
    static us_type_descriptor two[2], one[1];
    us_signature add, twice;
    us_context *c = us_new(package);
    if (!c) return 0;
    two[0] = two[1] = one[0] = integer();
    memset(&add, 0, sizeof add); add.result = integer(); add.args = two; add.count = 2;
    memset(&twice, 0, sizeof twice); twice.result = integer(); twice.args = one; twice.count = 1;
    if (us_add_symbol(c, "hostadd", (void *)(b ? add_b : add_a), &add) ||
        us_add_symbol(c, "hosttwice", (void *)(b ? twice_b : twice_a), &twice)) {
        printf("ERR declare %s\n", us_error(c)); us_free(c); return 0;
    }
    return c;
}

static int phase_contract(const char *pkg, const char *target) {
    us_context *c = declare(pkg, 0); size_t n = 0; const void *p;
    if (!c) return 1;
    p = us_binding_contract(c, target, &n);
    if (!p || n <= 12 || memcmp(p, "USLCON1\n", 8)) { printf("ERR contract %s\n", us_error(c)); us_free(c); return 1; }
    printf("OK contract USLCON1 bytes=%zu\n", n); us_free(c); return 0;
}

static int phase_compile(const char *pkg, const char *target) {
    static const char *SRC = "long hostadd(long a, long b); long hosttwice(long a);\n"
                             "long answer(void) { return hostadd(hosttwice(5), 7); }\n";
    us_context *a = declare(pkg, 0), *b = declare(pkg, 1); long (*fa)(void), (*fb)(void); int rc = 1;
    if (!a || !b) { us_free(a); us_free(b); return 1; }
    if (us_add_source(a, "a.c", SRC) || us_compile(a, target, 1) || us_relocate(a)) printf("ERR compile %s\n", us_error(a));
    else if (us_add_source(b, "b.c", SRC) || us_compile(b, target, 1) || us_relocate(b)) printf("ERR compile %s\n", us_error(b));
    else {
        fa = (long (*)(void))us_sym(a, "answer"); fb = (long (*)(void))us_sym(b, "answer");
        if (fa && fb && fa() == 1027 && fb() == 2037) { printf("OK compile answers=1027,2037\n"); rc = 0; }
        else printf("ERR compile wrong-host-answers\n");
    }
    us_free(a); us_free(b); return rc;
}

static int phase_carrier(const char *pkg, const char *target) {
    static const char *SRC = "union mix { double d; long u; };\n"
                             "union mix bump(union mix x) { x.u += 17; return x; }\n";
    us_context *c = us_new(pkg); int rc = 1;
    if (!c) { printf("ERR carrier context\n"); return 1; }
    if (us_add_source(c, "u.c", SRC) || us_compile(c, target, 1) || us_relocate(c)) printf("ERR carrier %s\n", us_error(c));
    else { printf("OK carrier export=%s\n", us_sym(c, "bump") ? "present" : "absent"); rc = 0; }
    us_free(c); return rc;
}

/* ---- api observer: library-domain cells ------------------------------------------------------ */
static const char *ax_pkg, *ax_target, *ax_incdir;
static const char *AX_SEVEN = "long seven(void) { return 7; }\n";
static us_type_descriptor ax_int(unsigned width, unsigned uns) { us_type_descriptor t; memset(&t, 0, sizeof t); t.kind = 1; t.width = width; t.uns = uns; return t; }
static us_type_descriptor ax_ptr(void) { us_type_descriptor t; memset(&t, 0, sizeof t); t.kind = 2; t.depth = 1; t.width = 8; return t; }
static int ax_build(us_context *c, const char *name, const char *src) {
    return us_add_source(c, name, src) || us_compile(c, ax_target, 1) || us_relocate(c);
}
/* A success does not clear us_error, so only a refusal prints it: "<key>=0" or "<key>=<rc>:<error>". */
static void ax_refusal(const char *key, int rc, const us_context *c) { if (rc) printf("|%s=%d:%s", key, rc, us_error(c)); else printf("|%s=0", key); }
/* Build then call a long(void) export: "<key>=0:<value>", "<key>=<rc>:<error>" or "<key>=0:nosym:<error>". */
static void ax_value(const char *key, us_context *c, int rc, const char *fn) {
    long (*f)(void);
    if (rc) { printf("|%s=%d:%s", key, rc, us_error(c)); return; }
    f = (long (*)(void))us_sym(c, fn);
    if (!f) { printf("|%s=0:nosym:%s", key, us_error(c)); return; }
    printf("|%s=0:%ld", key, f());
}
static void ax_symstate(const char *key, us_context *c, const char *fn) {
    void *p = us_sym(c, fn); if (p) printf("|%s=kept", key); else printf("|%s=null:%s", key, us_error(c));
}

static void ax_add_source(void) {
    us_context *c; int rc;
    printf("lib.in.us_add_source");
    rc = us_add_source(NULL, "a.c", AX_SEVEN); printf("|null-ctx=%d:%s", rc, us_error(NULL));
    if (!(c = us_new(ax_pkg))) { puts("|context=null"); return; }
    rc = us_add_source(c, NULL, AX_SEVEN); ax_refusal("null-name", rc, c);
    rc = us_add_source(c, "a.c", NULL); ax_refusal("null-source", rc, c);
    ax_value("valid", c, ax_build(c, "a.c", AX_SEVEN), "seven");
    putchar('\n'); us_free(c);
}
static void ax_define(void) {
    us_context *c; int rc;
    printf("lib.in.us_define");
    if (!(c = us_new(ax_pkg))) { puts("|context=null"); return; }
    rc = us_define(c, NULL); ax_refusal("null", rc, c);
    rc = us_define(c, "A\nB"); ax_refusal("newline", rc, c);
    rc = us_define(c, "N=41") || us_define(c, "M");
    ax_value("value", c, rc ? rc : ax_build(c, "d.c", "long dv(void) { return N + M; }\n"), "dv");
    putchar('\n'); us_free(c);
}
static void ax_include_path(void) {
    us_context *c; int rc;
    printf("lib.in.us_include_path");
    if (!(c = us_new(ax_pkg))) { puts("|context=null"); return; }
    rc = us_include_path(c, NULL); ax_refusal("null", rc, c);
    rc = us_include_path(c, ax_incdir);
    ax_value("value", c, rc ? rc : ax_build(c, "i.c", "#include <hv.h>\nlong iv(void) { return HV; }\n"), "iv");
    putchar('\n'); us_free(c);
}
static void ax_signature_version(void) {
    us_context *c; int rc;
    printf("lib.in.us_set_signature_version");
    if (!(c = us_new(ax_pkg))) { puts("|context=null"); return; }
    rc = ax_build(c, "s.c", AX_SEVEN); ax_refusal("built", rc, c);
    rc = us_set_signature_version(c, 4); ax_refusal("v4", rc, c);
    rc = us_set_signature_version(c, 1); ax_refusal("v1", rc, c);
    rc = us_set_signature_version(c, 2); printf("|v2-same=%d", rc); ax_symstate("v2-image", c, "seven");
    rc = us_set_signature_version(c, 3); printf("|v3=%d", rc); ax_symstate("v3-image", c, "seven");
    ax_value("v3-rebuilt", c, us_compile(c, ax_target, 1) || us_relocate(c), "seven");
    putchar('\n'); us_free(c);
}
static void ax_relocate_order(void) {
    us_context *c; int rc;
    printf("lib.compile.us_relocate.order");
    if (!(c = us_new(ax_pkg))) { puts("|context=null"); return; }
    rc = us_compile(c, ax_target, 1); ax_refusal("compile-empty", rc, c);
    rc = us_add_source(c, "o.c", AX_SEVEN); printf("|added=%d", rc);
    rc = us_relocate(c); ax_refusal("relocate-first", rc, c);
    rc = us_compile(c, ax_target, 1); printf("|compiled=%d", rc);
    ax_symstate("sym-before-relocate", c, "seven");
    putchar('\n'); us_free(c);
}
static void ax_cross_target(void) {
    us_context *c; int rc; size_t n = 0; const void *p;
    printf("lib.compile.cross-target");
    if (!(c = us_new(ax_pkg))) { puts("|context=null"); return; }
    rc = us_add_source(c, "x.c", AX_SEVEN) || us_compile(c, "osx/arm64", 1); ax_refusal("compile", rc, c);
    p = us_tape(c, &n); printf("|tape=%s", p && n ? "nonempty" : "empty");
    p = us_binding_contract(c, "osx/arm64", &n);
    printf("|contract=%s", p && n > 12 && !memcmp(p, "USLCON1\n", 8) ? "USLCON1" : us_error(c));
    rc = us_relocate(c); ax_refusal("relocate", rc, c);
    putchar('\n'); us_free(c);
}
static void ax_run_main(void) {
    static const char *const argv2[] = {"prog", "3", 0};
    us_context *c; int rc, st = -1;
    printf("lib.run.us_run_main");
    if (!(c = us_new(ax_pkg))) { puts("|context=null"); return; }
    rc = ax_build(c, "m.c", "int main(int argc, char **argv) { return argc * 10 + (argv[1][0] - '0'); }\n");
    ax_refusal("built", rc, c);
    rc = us_run_main(c, -1, argv2, &st); ax_refusal("argc-negative", rc, c);
    rc = us_run_main(c, 1, NULL, &st); ax_refusal("argv-null", rc, c);
    rc = us_run_main(c, 2, argv2, NULL); ax_refusal("status-null", rc, c);
    rc = us_run_main(c, 2, argv2, &st); printf("|valid=%d:%d", rc, st);
    putchar('\n'); us_free(c);
}
static void ax_call_status(void) {
    us_context *c; int built, st, es; long v; long (*f)(void);
    printf("lib.run.us_call_status");
    if (!(c = us_new(ax_pkg))) { puts("|context=null"); return; }
    built = ax_build(c, "q.c", "#include <stdlib.h>\nlong ok(void) { return 5; }\nlong quit(void) { exit(9); return 1; }\n");
    ax_refusal("built", built, c);
    if (built) { putchar('\n'); us_free(c); return; }
    f = (long (*)(void))us_sym(c, "ok");
    if (f) { es = -1; v = f(); st = us_call_status(c, &es); printf("|ok=%ld:%d:%d", v, st, es); } else printf("|ok=nosym:%s", us_error(c));
    f = (long (*)(void))us_sym(c, "quit");
    if (f) { es = -1; v = f(); st = us_call_status(c, &es); printf("|exit=%ld:%d:%d", v, st, es); } else printf("|exit=nosym:%s", us_error(c));
    putchar('\n'); us_free(c);
}
static void ax_free(void) {
    us_context *c; int k, fd0 = -1, fd1 = -1, mp0 = -1, mp1 = -1;
    printf("lib.ctx.us_free");
    us_free(NULL); printf("|null=noop");
    for (k = 0; k < 5; k++) {
        if (!(c = us_new(ax_pkg))) { puts("|context=null"); return; }
        if (ax_build(c, "f.c", AX_SEVEN)) { printf("|cycle%d=%s\n", k, us_error(c)); us_free(c); return; }
        { long (*f)(void) = (long (*)(void))us_sym(c, "seven"); if (!f || f() != 7) { printf("|cycle%d=call\n", k); us_free(c); return; } }
        us_free(c);
        if (k == 1) {   /* two warm-up cycles, then three measured ones */
            DIR *d = opendir("/proc/self/fd"); struct dirent *e; FILE *m = fopen("/proc/self/maps", "r"); int ch;
            fd0 = 0; mp0 = 0;
            if (d) { while ((e = readdir(d))) if (e->d_name[0] != '.') fd0++; closedir(d); } else fd0 = -1;
            if (m) { while ((ch = fgetc(m)) != EOF) if (ch == '\n') mp0++; fclose(m); } else mp0 = -1;
        }
    }
    {
        DIR *d = opendir("/proc/self/fd"); struct dirent *e; FILE *m = fopen("/proc/self/maps", "r"); int ch;
        fd1 = 0; mp1 = 0;
        if (d) { while ((e = readdir(d))) if (e->d_name[0] != '.') fd1++; closedir(d); } else fd1 = -2;
        if (m) { while ((ch = fgetc(m)) != EOF) if (ch == '\n') mp1++; fclose(m); } else mp1 = -2;
    }
    if (fd0 == fd1 && fd0 > 0) printf("|fds=equal"); else printf("|fds=%d->%d", fd0, fd1);
    if (mp0 == mp1 && mp0 > 0) printf("|maps=equal"); else printf("|maps=%d->%d", mp0, mp1);
    putchar('\n');
}
static us_context *ax_reenter_ctx;
static char ax_reentered[4][192];
static long ax_reenter(void) {
    us_context *c = ax_reenter_ctx; int rc, st = 0; size_t n = 0; const void *p;
    rc = us_compile(c, ax_target, 1); snprintf(ax_reentered[0], sizeof ax_reentered[0], "%d:%s", rc, us_error(c));
    rc = us_run_main(c, 0, NULL, &st); snprintf(ax_reentered[1], sizeof ax_reentered[1], "%d:%s", rc, us_error(c));
    p = us_binding_contract(c, ax_target, &n); snprintf(ax_reentered[2], sizeof ax_reentered[2], "%s:%s", p ? "bytes" : "null", us_error(c));
    rc = us_relocate(c); snprintf(ax_reentered[3], sizeof ax_reentered[3], "%d:%s", rc, us_error(c));
    return 41;
}
static void ax_reentry(void) {
    us_signature s; us_context *c; int rc;
    printf("lib.ctx.reentry");
    if (!(c = us_new(ax_pkg))) { puts("|context=null"); return; }
    memset(&s, 0, sizeof s); s.result = ax_int(8, 0);
    rc = us_add_symbol(c, "hostreenter", (void *)ax_reenter, &s); ax_refusal("declared", rc, c);
    ax_reenter_ctx = c; strcpy(ax_reentered[0], "unset"); strcpy(ax_reentered[1], "unset"); strcpy(ax_reentered[2], "unset"); strcpy(ax_reentered[3], "unset");
    rc = rc ? rc : ax_build(c, "r.c", "long hostreenter(void);\nlong outer(void) { return hostreenter() + 1; }\n");
    if (rc) { printf("|built=%d:%s\n", rc, us_error(c)); us_free(c); return; }
    {
        long (*f)(void) = (long (*)(void))us_sym(c, "outer"); long v = f ? f() : -1;
        printf("|compile=%s|run-main=%s|contract=%s|relocate=%s|outer=%ld", ax_reentered[0], ax_reentered[1], ax_reentered[2], ax_reentered[3], v);
    }
    putchar('\n'); us_free(c);
}
static void ax_host_to_script(void) {
    int v[5] = {4, 1, 3, 0, 2}, i, es = -1; us_context *c; int (*cmp)(const void *, const void *);
    printf("lib.cb.host-to-script-api");
    if (!(c = us_new(ax_pkg))) { puts("|context=null"); return; }
    if (ax_build(c, "c.c", "int cmp(const void *a, const void *b) { int x = *(const int *)a, y = *(const int *)b; return (x > y) - (x < y); }\n")) {
        printf("|built=1:%s\n", us_error(c)); us_free(c); return;
    }
    cmp = (int (*)(const void *, const void *))us_sym(c, "cmp");
    if (!cmp) { printf("|cmp=nosym:%s\n", us_error(c)); us_free(c); return; }
    qsort(v, 5, sizeof v[0], cmp);
    printf("|sorted=%d,%d,%d,%d,%d", v[0], v[1], v[2], v[3], v[4]);
    i = us_call_status(c, &es); printf("|status=%d", i);
    putchar('\n'); us_free(c);
}
static long ax_fake_strlen(const char *s) { (void)s; return 99; }
static void ax_declare_import(void) {
    static us_type_descriptor one[1], many[2];
    us_signature s, bad; us_context *a, *b; int rc;
    printf("lib.bind.us_declare_import");
    one[0] = ax_ptr(); many[0] = many[1] = ax_int(8, 0);
    memset(&s, 0, sizeof s); s.result = ax_int(8, 1); s.args = one; s.count = 1;
    memset(&bad, 0, sizeof bad); bad.result = ax_int(8, 0); bad.args = many; bad.count = 2000;
    if (!(a = us_new(ax_pkg)) || !(b = us_new(ax_pkg))) { us_free(a); puts("|context=null"); return; }
    rc = us_declare_import(a, "strlen", &s);
    ax_value("process", a, rc ? rc : ax_build(a, "p.c", "unsigned long strlen(const char *);\nlong sl(void) { return (long)strlen(\"hello\"); }\n"), "sl");
    rc = us_add_symbol(b, "strlen", (void *)ax_fake_strlen, &s) || us_declare_import(b, "strlen", &s);
    ax_value("injection-first", b, rc ? rc : ax_build(b, "p.c", "unsigned long strlen(const char *);\nlong sl(void) { return (long)strlen(\"hello\"); }\n"), "sl");
    rc = us_declare_import(a, "bad", &bad); ax_refusal("bad", rc, a);
    ax_value("after-bad", a, 0, "sl");
    putchar('\n'); us_free(a); us_free(b);
}
static void ax_load_library(void) {
    static us_type_descriptor args[3];
    us_signature s; us_context *c; int rc;
    printf("lib.bind.us_load_library");
    if (!(c = us_new(ax_pkg))) { puts("|context=null"); return; }
    rc = us_load_library(c, ""); ax_refusal("empty", rc, c);
    rc = us_load_library(c, "/nonexistent/unisacc-p3-missing.so"); ax_refusal("missing", rc, c);
    rc = us_load_library(c, "libz.so.1"); ax_refusal("libz", rc, c);
    rc = us_load_library(c, "libz.so.1"); ax_refusal("duplicate", rc, c);
    args[0] = ax_int(8, 1); args[1] = ax_ptr(); args[2] = ax_int(4, 1);
    memset(&s, 0, sizeof s); s.result = ax_int(8, 1); s.args = args; s.count = 3;
    rc = us_declare_import(c, "adler32", &s);
    ax_value("adler32", c, rc ? rc : ax_build(c, "z.c", "unsigned long adler32(unsigned long, const unsigned char *, unsigned int);\n"
                                                    "long az(void) { return (long)adler32(1, (const unsigned char *)\"a\", 1); }\n"), "az");
    putchar('\n'); us_free(c);
}
static long ax_hv = 31;
static void ax_add_symbol(void) {
    static us_type_descriptor two[2];
    us_signature s, d; us_context *c; int rc;
    printf("lib.bind.us_add_symbol.invalid-and-data");
    if (!(c = us_new(ax_pkg))) { puts("|context=null"); return; }
    two[0] = two[1] = ax_int(8, 0);
    rc = us_add_symbol(c, "x", (void *)add_a, NULL); ax_refusal("null-signature", rc, c);
    memset(&s, 0, sizeof s); s.result = ax_int(8, 0); s.args = two; s.count = 2000;
    rc = us_add_symbol(c, "x", (void *)add_a, &s); ax_refusal("too-many", rc, c);
    memset(&d, 0, sizeof d); d.kind = 1; d.result = ax_int(8, 0); d.args = two; d.count = 1; d.extent = 8;
    rc = us_add_symbol(c, "x", &ax_hv, &d); ax_refusal("data-with-args", rc, c);
    d.args = 0; d.count = 0;
    rc = us_add_symbol(c, "hv", &ax_hv, &d);
    ax_value("data", c, rc ? rc : ax_build(c, "h.c", "extern long hv;\nlong gv(void) { return hv + 1; }\n"), "gv");
    s.count = 2; rc = us_add_symbol(c, "hostadd", (void *)add_a, &s); printf("|registered=%d", rc);
    ax_symstate("after-registration", c, "gv");
    putchar('\n'); us_free(c);
}
static int phase_api(const char *pkg, const char *target, const char *incdir) {
    ax_pkg = pkg; ax_target = target; ax_incdir = incdir;
    ax_add_source(); ax_define(); ax_include_path(); ax_signature_version(); ax_relocate_order();
    ax_cross_target(); ax_run_main(); ax_call_status(); ax_free(); ax_reentry();
    ax_host_to_script(); ax_declare_import(); ax_load_library(); ax_add_symbol();
    puts("API_DONE"); return 0;
}

int main(int argc, char **argv) {
    if (argc == 5 && !strcmp(argv[3], "api")) return phase_api(argv[1], argv[2], argv[4]);
    if (argc != 4) { fprintf(stderr, "usage: p3_library_members PACKAGE TARGET contract|compile|carrier | api INCLUDE_DIR\n"); return 2; }
    if (!strcmp(argv[3], "contract")) return phase_contract(argv[1], argv[2]);
    if (!strcmp(argv[3], "compile")) return phase_compile(argv[1], argv[2]);
    if (!strcmp(argv[3], "carrier")) return phase_carrier(argv[1], argv[2]);
    fprintf(stderr, "unknown phase %s\n", argv[3]); return 2;
}

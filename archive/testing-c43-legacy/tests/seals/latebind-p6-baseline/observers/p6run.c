/* p6run PACKAGE TARGET SOURCE CALLS OUTDIR: behaviour of one untyped library-mode program, public API only.
 * Providers are supplied (hostadd as a host function, hostvalue as a data object), then compile, relocate,
 * calls per CALLS (lines: "sym ret(l|i) [args...]", all args long), host data read back after the calls.
 * Output: OUTDIR/run.txt (no addresses in it). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include "exec/c/libunisacc.h"
extern int us_register_data_provider(us_context *, const char *, void *, const us_signature *);
extern int us_declare_injected_data(us_context *, const char *, const us_signature *);
static int hostvalue = 5;
static long host_add(long a, long b) { return a + b; }
static char *slurp(const char *path, size_t *n) {
    FILE *f = fopen(path, "rb"); char *b; if (!f) return 0;
    fseek(f, 0, SEEK_END); *n = (size_t)ftell(f); fseek(f, 0, SEEK_SET);
    b = malloc(*n + 1); if (!b || fread(b, 1, *n, f) != *n) { fclose(f); free(b); return 0; }
    b[*n] = 0; fclose(f); return b;
}
static FILE *out;
static void say(const char *fmt, ...);
int main(int argc, char **argv) {
    size_t sn = 0, cn = 0; char *src, *calls, *p, *e; us_context *c; int rc; char path[4096];
    us_signature fs, ds; char buf[256];
    if (argc != 6) { fprintf(stderr, "usage: p6run PACKAGE TARGET SOURCE CALLS OUTDIR\n"); return 2; }
    src = slurp(argv[3], &sn); calls = slurp(argv[4], &cn); if (!src || !calls) return 2;
    { char mk[4096]; snprintf(mk, sizeof mk, "mkdir -p '%s'", argv[5]); if (system(mk)) return 2; }
    snprintf(path, sizeof path, "%s/run.txt", argv[5]); out = fopen(path, "w"); if (!out) return 2;
    c = us_new(argv[1]); if (!c) { say("new failed"); fclose(out); return 3; }
    static us_type_descriptor two[2];
    memset(&fs, 0, sizeof fs); fs.kind = 0; fs.result.kind = 1; fs.result.width = 8;
    two[0].kind = 1; two[0].width = 8; two[1].kind = 1; two[1].width = 8;
    fs.args = two; fs.count = 2;
    memset(&ds, 0, sizeof ds); ds.kind = 1; ds.result.kind = 1; ds.result.width = 4; ds.extent = 4; ds.writable = 1;
    if (strstr(src, "hostadd")) {
        rc = us_add_symbol(c, "hostadd", (void *)host_add, &fs);
        say("provider hostadd rc=%d err=%s", rc, rc ? us_error(c) : "");
    }
    if (strstr(src, "hostvalue")) {
        rc = us_declare_injected_data(c, "hostvalue", &ds);
        say("declare hostvalue rc=%d err=%s", rc, rc ? us_error(c) : "");
        rc = us_register_data_provider(c, "hostvalue", &hostvalue, &ds);
        say("provider hostvalue rc=%d err=%s", rc, rc ? us_error(c) : "");
    }
    rc = us_add_source(c, "case.c", src);
    say("add_source rc=%d err=%s", rc, rc ? us_error(c) : "");
    rc = us_compile(c, argv[2], 1);
    say("compile rc=%d err=%s", rc, rc ? us_error(c) : "");
    if (!rc) {
        rc = us_relocate(c);
        say("relocate rc=%d err=%s", rc, rc ? us_error(c) : "");
        if (!rc) {
            for (p = calls; *p; p = e ? e + 1 : p + strlen(p)) {
                char *nl = strchr(p, '\n'); char *toks[8]; int nt = 0; char *q;
                if (nl) *nl = 0; e = nl;
                if (!*p || *p == '#') { if (nl) *nl = '\n'; continue; }
                strncpy(buf, p, sizeof buf - 1); buf[sizeof buf - 1] = 0; if (nl) *nl = '\n';
                for (q = strtok(buf, " "); q && nt < 8; q = strtok(0, " ")) toks[nt++] = q;
                if (nt < 2) continue;
                {   void *fn = us_sym(c, toks[0]);
                    if (!fn) { say("sym %s missing err=%s", toks[0], us_error(c)); continue; }
                    long a[2] = {0, 0}; int na = nt - 2; if (na > 2) na = 2;
                    for (int i = 0; i < na; i++) a[i] = atol(toks[2 + i]);
                    if (toks[1][0] == 'i') {
                        int r = na == 0 ? ((int (*)(void))fn)() : na == 1 ? ((int (*)(long))fn)(a[0]) : ((int (*)(long, long))fn)(a[0], a[1]);
                        say("call %s i -> %d", toks[0], r);
                    } else {
                        long r = na == 0 ? ((long (*)(void))fn)() : na == 1 ? ((long (*)(long))fn)(a[0]) : ((long (*)(long, long))fn)(a[0], a[1]);
                        say("call %s l -> %ld", toks[0], r);
                    }
                }
            }
            say("hostvalue after calls = %d", hostvalue);
            { int st = -1; int r2 = us_call_status(c, &st); say("call_status rc=%d status=%d err=%s", r2, st, r2 ? us_error(c) : ""); }
        }
    }
    fclose(out); us_free(c); free(src); free(calls);
    return 0;
}
static void say(const char *fmt, ...) { va_list ap; va_start(ap, fmt); vfprintf(out, fmt, ap); va_end(ap); fputc('\n', out); }

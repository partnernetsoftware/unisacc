/* p6probe PACKAGE TARGET SOURCE OUTDIR: one untyped library-mode program through the public API only.
 * Fragments are dumped (E3 wrapper, unbound lower after relocate, and for hostvalue the data fragment and
 * data unbound lower) with a status file recording every rc and error text.  Frozen as the P6 baseline reader
 * (2c2 typed-plan comparison) only; no typed API is used.  Build: see README.md in this directory. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "exec/c/libunisacc.h"
extern const void *us_library_data_fragment(us_context *, size_t *);
extern const void *us_library_data_lower_unbound(us_context *, size_t *);
extern int us_declare_injected_data(us_context *, const char *, const us_signature *);

static char *slurp(const char *path, size_t *n) {
    FILE *f = fopen(path, "rb"); char *b; if (!f) return 0;
    fseek(f, 0, SEEK_END); *n = (size_t)ftell(f); fseek(f, 0, SEEK_SET);
    b = malloc(*n + 1); if (!b || fread(b, 1, *n, f) != *n) { fclose(f); free(b); return 0; }
    b[*n] = 0; fclose(f); return b;
}
static void dump(const char *dir, const char *name, const void *p, size_t n) {
    char path[4096]; FILE *f; snprintf(path, sizeof path, "%s/%s.bin", dir, name);
    f = fopen(path, "wb"); if (!f) return;
    if (p && n) fwrite(p, 1, n, f);
    fclose(f);
}
static void status(const char *dir, const char *line) {
    char path[4096]; FILE *f; snprintf(path, sizeof path, "%s/status.txt", dir);
    f = fopen(path, "a"); if (f) { fputs(line, f); fputc('\n', f); fclose(f); }
}
static us_signature sig_fn(void) {
    static us_type_descriptor two[2];
    us_signature s; memset(&s, 0, sizeof s); (void)two; s.kind = 0;
    s.result.kind = 1; s.result.width = 8; return s;
}
static us_signature sig_data(void) {
    us_signature s; memset(&s, 0, sizeof s); s.kind = 1;
    s.result.kind = 1; s.result.width = 4; s.extent = 4; s.writable = 1; return s;
}
int main(int argc, char **argv) {
    char line[512]; size_t sn = 0, n = 0; char *src; const void *p;
    us_context *c; int rc; us_signature fs, ds;
    if (argc != 5) { fprintf(stderr, "usage: p6probe PACKAGE TARGET SOURCE OUTDIR\n"); return 2; }
    src = slurp(argv[3], &sn); if (!src) { fprintf(stderr, "no source\n"); return 2; }
    snprintf(line, sizeof line, "%s", argv[4]); { char mk[4096]; snprintf(mk, sizeof mk, "mkdir -p '%s'", argv[4]); if (system(mk)) return 2; }
    c = us_new(argv[1]); if (!c) return 3;
    fs = sig_fn(); ds = sig_data();
    if (strstr(src, "hostadd")) {
        us_type_descriptor *args = malloc(2 * sizeof *args); memset(args, 0, 2 * sizeof *args);
        args[0].kind = 1; args[0].width = 8; args[1].kind = 1; args[1].width = 8;
        fs.args = args; fs.count = 2; fs.result.kind = 1; fs.result.width = 8;
        rc = us_declare_injected_import(c, "hostadd", &fs);
        snprintf(line, sizeof line, "declare hostadd rc=%d err=%s", rc, rc ? us_error(c) : "");
        status(argv[4], line);
    }
    if (strstr(src, "hostvalue")) {
        rc = us_declare_injected_data(c, "hostvalue", &ds);
        snprintf(line, sizeof line, "declare hostvalue rc=%d err=%s", rc, rc ? us_error(c) : "");
        status(argv[4], line);
    }
    rc = us_add_source(c, "case.c", src);
    snprintf(line, sizeof line, "add_source rc=%d err=%s", rc, rc ? us_error(c) : "");
    status(argv[4], line);
    rc = us_compile(c, argv[2], 1);
    snprintf(line, sizeof line, "compile rc=%d err=%s", rc, rc ? us_error(c) : "");
    status(argv[4], line);
    if (!rc) {
        p = us_library_e3_fragment(c, &n); dump(argv[4], "e3", p, p ? n : 0);
        snprintf(line, sizeof line, "e3 fragment n=%zu", p ? n : 0); status(argv[4], line);
        rc = us_relocate(c);
        snprintf(line, sizeof line, "relocate rc=%d err=%s", rc, rc ? us_error(c) : "");
        status(argv[4], line);
        p = us_library_lower_unbound(c, &n); dump(argv[4], "lower", p, p ? n : 0);
        snprintf(line, sizeof line, "lower_unbound n=%zu", p ? n : 0); status(argv[4], line);
        p = us_library_data_fragment(c, &n); dump(argv[4], "datae3", p, p ? n : 0);
        snprintf(line, sizeof line, "data_fragment n=%zu", p ? n : 0); status(argv[4], line);
        p = us_library_data_lower_unbound(c, &n); dump(argv[4], "datalower", p, p ? n : 0);
        snprintf(line, sizeof line, "data_lower_unbound n=%zu", p ? n : 0); status(argv[4], line);
    }
    us_free(c); free(src);
    return 0;
}

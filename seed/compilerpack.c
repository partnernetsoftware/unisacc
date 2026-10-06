/* seed/compilerpack.c -- exec/c/compilerpack.py's package declaration without Python (0.0.32 B5).
 *
 *   compilerpack OUTDIR --o1 NET --e2 NET --nativeabi NET --models DIR [--kernels DIR] [--audit-dir DIR] ROUTE.tsv...
 *
 * Writes OUTDIR/routes.tsv (the same rows compiler_package() hands pack.build: unit, memory,
 * per-spec, object, tokens, quiet-located, warn, multi, warn/multi and nativeabi routes) and
 * OUTDIR/predefines/ (the pp-gen predefres facts), checks the kernel blobs, and retains the
 * table/network audit pairs (models.json byte-equal to retain_models).  DIR holds the model
 * jobs as NAME.tbl/NAME.net (object-lower-ARCH, object-enc-ARCH, tokenpp, tokenlex, warnlex,
 * warnparse, warnunits, errorparse, warnpp-shared).  seed/pack.c then writes the package:
 *   pack -o PKG --compressed --mount 006864722f include --mount 006b65726e656c2f KERNELS
 *        --mount 00707265646566696e65732f OUTDIR/predefines OUTDIR/routes.tsv
 * C99 plus POSIX (mkdir, realpath). */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include "json.h"
#include "sha256.h"

static void die(const char *m, const char *a) { fprintf(stderr, "compilerpack: %s%s%s\n", m, a ? " " : "", a ? a : ""); exit(1); }
static char *dup(const char *s) { size_t n = strlen(s) + 1; char *p = malloc(n); if (!p) die("out of memory", 0); return memcpy(p, s, n); }
static char *cat(const char *a, const char *b) { size_t x = strlen(a), y = strlen(b); char *p = malloc(x + y + 1); if (!p) die("out of memory", 0); memcpy(p, a, x); memcpy(p + x, b, y + 1); return p; }
static unsigned char *slurp(const char *path, size_t *n) {
    FILE *f = fopen(path, "rb"); if (!f) die("cannot read", path);
    size_t cap = 65536, k = 0, r; unsigned char *b = malloc(cap + 1);
    while ((r = fread(b + k, 1, cap - k, f)) > 0) { k += r; if (k == cap) { cap *= 2; b = realloc(b, cap + 1); } }
    fclose(f); b[k] = 0; *n = k; return b;
}
static void spit(const char *path, const void *p, size_t n) {
    FILE *f = fopen(path, "wb"); if (!f || fwrite(p, 1, n, f) != n || fclose(f)) die("cannot write", path);
}
static void mkdirs(const char *path) {
    char *p = dup(path);
    for (char *s = p + 1; *s; s++) if (*s == '/') { *s = 0; if (mkdir(p, 0777) && errno != EEXIST) die("cannot create", p); *s = '/'; }
    if (mkdir(p, 0777) && errno != EEXIST) die("cannot create", p);
    free(p);
}

typedef struct { char *c[5]; } Row;
typedef struct { Row *r; int n, cap; } Rows;
static void add(Rows *rs, const char *a, const char *b, const char *c, const char *d, const char *e) {
    if (rs->n == rs->cap) { rs->cap = rs->cap ? rs->cap * 2 : 256; rs->r = realloc(rs->r, rs->cap * sizeof *rs->r); }
    rs->r[rs->n++] = (Row){{dup(a), dup(b), dup(c), dup(d), dup(e)}};
}
static Rows copy(Rows *rs) { Rows o = {0}; for (int i = 0; i < rs->n; i++) add(&o, rs->r[i].c[0], rs->r[i].c[1], rs->r[i].c[2], rs->r[i].c[3], rs->r[i].c[4]); return o; }
static void set(char **slot, const char *v) { *slot = dup(v); }

typedef struct { char *suffix, *last, *opt; } Spec;
typedef struct { char *name; Spec spec[32]; int nspec; char *stage[7][5]; } Target;

static int split_tabs(char *line, char **c, int max) {
    int k = 0; c[k++] = line;
    for (char *p = line; *p; p++) if (*p == '\t') { *p = 0; if (k == max) return max + 1; c[k++] = p + 1; }
    return k;
}
static char *model(const char *dir, const char *name) { char *a = cat(dir, "/"), *b = cat(a, name), *c = cat(b, ".net"); free(a); free(b); return c; }
static char *resolved(const char *base, const char *rel) {
    char *p = rel[0] == '/' ? dup(rel) : cat(base, rel), buf[PATH_MAX];
    if (!realpath(p, buf)) die("cannot resolve", p);
    free(p); return dup(buf);
}
static int tcmp(const void *a, const void *b) { return strcmp(((const Target *)a)->name, ((const Target *)b)->name); }
static uint64_t le64(const unsigned char *p) { uint64_t v = 0; for (int i = 7; i >= 0; i--) v = v << 8 | p[i]; return v; }
static void sha_file(const char *path, char hex[65]) {
    size_t n; unsigned char *b = slurp(path, &n), dg[32]; Sha h; sha_init(&h); sha_put(&h, b, n); sha_end(&h, dg); free(b);
    for (int i = 0; i < 32; i++) sprintf(hex + 2 * i, "%02x", dg[i]);
}
typedef struct { char key[130]; char nh[65], th[65]; } Pair;
static int paircmp(const void *a, const void *b) { return strcmp(((const Pair *)a)->key, ((const Pair *)b)->key); }

int main(int argc, char **argv) {
    const char *out = 0, *o1 = 0, *e2 = 0, *abi = 0, *mdir = 0, *kern = 0, *audit = 0; char *man[16]; int nman = 0;
    for (int i = 1; i < argc; i++) {
        const char **slot = !strcmp(argv[i], "--o1") ? &o1 : !strcmp(argv[i], "--e2") ? &e2 : !strcmp(argv[i], "--nativeabi") ? &abi
                          : !strcmp(argv[i], "--models") ? &mdir : !strcmp(argv[i], "--kernels") ? &kern : !strcmp(argv[i], "--audit-dir") ? &audit : 0;
        if (slot && i + 1 < argc) *slot = argv[++i];
        else if (argv[i][0] == '-' || nman == 16) { out = 0; break; }
        else if (!out) out = argv[i];
        else man[nman++] = argv[i];
    }
    if (!out || !o1 || !e2 || !abi || !mdir || !nman) {
        fprintf(stderr, "usage: compilerpack OUTDIR --o1 NET --e2 NET --nativeabi NET --models DIR [--kernels DIR] [--audit-dir DIR] ROUTE.tsv...\n"); return 2;
    }
    char rbuf[PATH_MAX];
    if (!realpath(mdir, rbuf)) die("cannot resolve", mdir); mdir = dup(rbuf);
    char *o1r = resolved("", o1), *e2r = resolved("", e2), *abir = resolved("", abi);
    if (kern) {   /* the kernel directory holds exactly the two ISA blobs, UNIKERN1 headers intact */
        DIR *d = opendir(kern); struct dirent *e; int seen = 0, other = 0;
        if (!d) die("kernel directory must contain arm64 and x86_64 only", 0);
        while ((e = readdir(d))) {
            if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
            if (!strcmp(e->d_name, "arm64")) seen |= 1; else if (!strcmp(e->d_name, "x86_64")) seen |= 2; else other = 1;
        }
        closedir(d);
        if (seen != 3 || other) die("kernel directory must contain arm64 and x86_64 only", 0);
        for (int isa = 1; isa <= 2; isa++) {
            char *p = cat(kern, isa == 1 ? "/arm64" : "/x86_64"); size_t n; unsigned char *raw = slurp(p, &n);
            if (n < 40 || memcmp(raw, "UNIKERN1", 8)) die("invalid kernel header", 0);
            uint64_t kind = le64(raw + 8), entry = le64(raw + 16), slot = le64(raw + 24), length = le64(raw + 32);
            int bad = kind != (uint64_t)isa || length != n - 40 || entry >= length || slot + 8 > length || slot % 8;
            if (!bad) for (int j = 0; j < 8; j++) if (raw[40 + slot + j]) bad = 1;
            if (bad) die("invalid kernel ISA or extent", 0);
            free(raw); free(p);
        }
    }
    /* compiler-routes.tsv beside compilerpack.py: suffix, last stage, optimiser */
    Spec specs[32]; int nspecs = 0;
    {   size_t n; char *t = (char *)slurp("exec/c/compiler-routes.tsv", &n);
        for (char *s = strtok(t, "\n"); s; s = strtok(0, "\n")) {
            if (*s == '#') continue;
            char *c[4]; if (split_tabs(s, c, 3) != 3 || nspecs == 32) die("bad compiler-routes.tsv row", s);
            specs[nspecs++] = (Spec){dup(c[0]), dup(c[1]), dup(c[2])};
        }
    }
    Target tg[16]; int nt = 0; Rows rows = {0};
    static const char *const order[7] = {"e2", "e1", "e3", "e4", "prune", "lower", "elf"};
    for (int m = 0; m < nman; m++) {
        size_t n; char *t = (char *)slurp(man[m], &n), *dirend = strrchr(man[m], '/');
        char *base = dirend ? cat(dup(man[m]), "") : dup("./"); if (dirend) base[dirend - man[m] + 1] = 0;
        Target *T = &tg[nt]; int ns = 0;
        for (char *s = strtok(t, "\n"); s; s = strtok(0, "\n")) {
            if (*s == '#') continue;
            char *c[6]; if (split_tabs(s, c, 5) != 5 || ns == 7) die("invalid source route", man[m]);
            for (int j = 0; j < 5; j++) T->stage[ns][j] = dup(c[j]);
            ns++;
        }
        if (!ns) die("invalid source route", man[m]);
        T->name = T->stage[0][0];
        for (int j = 0; j < nt; j++) if (!strcmp(tg[j].name, T->name)) die("duplicate/mixed target", T->name);
        for (int j = 0; j < ns; j++) if (strcmp(T->stage[j][0], T->name)) die("duplicate/mixed target", T->name);
        if (ns != 7) die("unexpected image stages", T->name);
        for (int j = 0; j < 7; j++) if (strcmp(T->stage[j][1], order[j])) die("unexpected image stages", T->name);
        for (int j = 0; j < 7; j++) T->stage[j][4] = resolved(base, T->stage[j][4]);
        T->nspec = nspecs; memcpy(T->spec, specs, sizeof *specs * nspecs);
        int lnx = !strcmp(T->name, "lnx/x86_64") || !strcmp(T->name, "lnx/arm64");
        if (lnx) { T->spec[T->nspec++] = (Spec){"object/O0", "elf", "none"}; T->spec[T->nspec++] = (Spec){"object/O1", "elf", "O1"}; T->spec[T->nspec++] = (Spec){"object/O2", "elf", "O2"}; }
        char *unit = cat(T->name, "/unit"), *mem = cat(T->name, "/memory");
        for (int j = 0; j < 2; j++) add(&rows, unit, T->stage[j][1], T->stage[j][2], T->stage[j][3], T->stage[j][4]);
        add(&rows, mem, T->stage[6][1], T->stage[6][2], "memory-v1", T->stage[6][4]);
        for (int k = 0; k < T->nspec; k++) {
            char *a = cat(T->name, "/"), *route = cat(a, T->spec[k].suffix);
            for (int j = 0; j < 7; j++) {
                const char *name = T->stage[j][1], *outf = T->stage[j][3], *mod = T->stage[j][4];
                if (!strcmp(name, "e4") && !strcmp(T->spec[k].opt, "none")) continue;
                if (!strcmp(name, "e4") && !strcmp(T->spec[k].opt, "O1")) mod = o1r;
                if (!strncmp(T->spec[k].suffix, "object/", 7) && !strcmp(name, "elf")) outf = "object-v1";
                add(&rows, route, name, T->stage[j][2], outf, mod);
                if (!strcmp(name, T->spec[k].last)) break;
            }
        }
        nt++;
    }
    qsort(tg, nt, sizeof *tg, tcmp);
    /* Linux object routes take the object lower/enc models */
    for (int t = 0; t < nt; t++) {
        const char *arch = !strcmp(tg[t].name, "lnx/x86_64") ? "x86_64" : !strcmp(tg[t].name, "lnx/arm64") ? "arm64" : 0;
        if (!arch) continue;
        char *pre = cat(tg[t].name, "/object/"), *lw = cat("object-lower-", arch), *en = cat("object-enc-", arch);
        char *lm = model(mdir, lw), *em = model(mdir, en);
        for (int i = 0; i < rows.n; i++) if (!strncmp(rows.r[i].c[0], pre, strlen(pre))) {
            if (!strcmp(rows.r[i].c[1], "lower")) set(&rows.r[i].c[4], lm);
            else if (!strcmp(rows.r[i].c[1], "elf")) set(&rows.r[i].c[4], em);
        }
    }
    for (int i = 0; i < rows.n; i++) if (!strcmp(rows.r[i].c[1], "e2")) set(&rows.r[i].c[4], e2r);
    add(&rows, "tokens", "tokenpp", "src.c", "pp.text", model(mdir, "tokenpp"));
    add(&rows, "tokens", "tokenlex", "pp.text", "tokens.plain", model(mdir, "tokenlex"));
    char *wlex = model(mdir, "warnlex"), *wparse = model(mdir, "warnparse"), *units = model(mdir, "warnunits"),
         *qparse = model(mdir, "errorparse"), *wpp = model(mdir, "warnpp-shared");
    Rows ordinary = copy(&rows);
    for (int t = 0; t < nt; t++) {
        Target *T = &tg[t]; char *slash = cat(T->name, "/"), *unit = cat(T->name, "/unit");
        for (int i = 0; i < rows.n; i++) {
            Row *r = &rows.r[i]; int quiet = !strcmp(r->c[0], unit);
            for (int k = 0; k < T->nspec && !quiet; k++) {
                if (!strcmp(T->spec[k].suffix, "pp")) continue;
                char *route = cat(slash, T->spec[k].suffix); quiet = !strcmp(r->c[0], route); free(route);
            }
            if (!quiet) continue;
            if (!strcmp(r->c[1], "e2")) { set(&r->c[3], "pp.locations"); set(&r->c[4], wpp); }
            else if (!strcmp(r->c[1], "e1")) { set(&r->c[2], "pp.locations"); set(&r->c[3], "tokens.locations"); set(&r->c[4], wlex); }
            else if (!strcmp(r->c[1], "e3")) { set(&r->c[2], "tokens.locations"); set(&r->c[4], qparse); }
        }
        char *wu = cat(T->name, "/warn/unit");
        add(&rows, wu, "e2", "src.c", "pp.locations", wpp);
        add(&rows, wu, "e1", "pp.locations", "tokens.locations", wlex);
        for (int k = 0; k < T->nspec; k++) {
            if (!strcmp(T->spec[k].suffix, "pp")) continue;
            char *route = cat(slash, T->spec[k].suffix), *wa = cat(T->name, "/warn/"), *wroute = cat(wa, T->spec[k].suffix);
            for (int i = 0; i < ordinary.n; i++) {
                Row r = ordinary.r[i];
                if (strcmp(r.c[0], route)) continue;
                const char *mod = !strcmp(r.c[1], "e1") ? wlex : !strcmp(r.c[1], "e3") ? wparse : !strcmp(r.c[1], "e2") ? wpp : r.c[4];
                const char *in = r.c[2], *o = r.c[3];
                if (!strcmp(r.c[1], "e2")) o = "pp.locations";
                else if (!strcmp(r.c[1], "e1")) { in = "pp.locations"; o = "tokens.locations"; }
                else if (!strcmp(r.c[1], "e3")) in = "tokens.locations";
                add(&rows, wroute, r.c[1], in, o, mod);
            }
        }
    }
    Rows base = copy(&rows);
    for (int warn = 0; warn < 2; warn++)
        for (int t = 0; t < nt; t++) {
            Target *T = &tg[t];
            for (int k = 0; k < T->nspec; k++) {
                if (!strcmp(T->spec[k].suffix, "pp")) continue;
                char *a = cat(T->name, warn ? "/warn/multi/" : "/multi/"), *route = cat(a, T->spec[k].suffix);
                char *b = cat(T->name, warn ? "/warn/" : "/"), *src = cat(b, T->spec[k].suffix);
                add(&rows, route, "units", "units.locations", "tokens.locations", units);
                for (int i = 0; i < base.n; i++) {
                    Row r = base.r[i];
                    if (!strcmp(r.c[0], src) && strcmp(r.c[1], "e2") && strcmp(r.c[1], "e1")) add(&rows, route, r.c[1], r.c[2], r.c[3], r.c[4]);
                }
            }
        }
    for (int t = 0; t < nt; t++) add(&rows, cat(tg[t].name, "/nativeabi"), "nativeabi", "USLSIG2", "USLNCAR1", abir);
    mkdirs(out);
    {   char *p = cat(out, "/routes.tsv"); FILE *f = fopen(p, "wb"); if (!f) die("cannot write", p);
        for (int i = 0; i < rows.n; i++) fprintf(f, "%s\t%s\t%s\t%s\t%s\n", rows.r[i].c[0], rows.r[i].c[1], rows.r[i].c[2], rows.r[i].c[3], rows.r[i].c[4]);
        if (fclose(f)) die("cannot write", p);
    }
    /* predefines: exec/facts/pp-gen.tsv predefres, one file per target */
    {   size_t n; char *t = (char *)slurp("exec/facts/pp-gen.tsv", &n), *s = strstr(t, "\n=predefres\tjson\t");
        if (!s) die("no predefres fact in exec/facts/pp-gen.tsv", 0);
        s += strlen("\n=predefres\tjson\t"); char *e = strchr(s, '\n'); if (e) *e = 0;
        JDoc d; memset(&d, 0, sizeof d); jparse_text(&d, s, (long)strlen(s), "predefres");
        if (d.n[0].type != JOBJ) die("predefres is not an object", 0);
        char *pdir = cat(out, "/predefines"); mkdirs(pdir);
        for (long k = jkid(&d, 0); k >= 0; k = jnext(&d, k)) {
            if (d.n[k].type != JSTR) die("predefres value is not a string", 0);
            char *key = malloc(d.n[k].klen + 1); memcpy(key, jkey(&d, k), d.n[k].klen); key[d.n[k].klen] = 0;
            if (strstr(key, "..") || key[0] == '/') die("bad predefres key", key);
            char *a = cat(pdir, "/"), *p = cat(a, key), *sl = strrchr(p, '/');
            *sl = 0; mkdirs(p); *sl = '/';
            spit(p, d.buf + d.n[k].str, (size_t)d.n[k].slen);
        }
    }
    if (audit) {   /* retain_models: each distinct network/table pair, keyed by both digests */
        mkdirs(audit);
        char *rec = cat(audit, "/models.json"); remove(rec);
        Pair *pairs = calloc(rows.n, sizeof *pairs); int np = 0;
        for (int i = 0; i < rows.n; i++) {
            char *net = rows.r[i].c[4], *tbl = dup(net), *dot = strrchr(tbl, '.'), *sl = strrchr(tbl, '/');
            if (!dot || (sl && dot < sl)) die("network without suffix", net);
            strcpy(dot, ".tbl");
            Pair p; sha_file(net, p.nh); sha_file(tbl, p.th); snprintf(p.key, sizeof p.key, "%s-%s", p.nh, p.th);
            int seen = 0; for (int j = 0; j < np; j++) if (!strcmp(pairs[j].key, p.key)) { seen = 1; break; }
            if (seen) { free(tbl); continue; }
            size_t n; unsigned char *b;
            char *a = cat(audit, "/"), *k = cat(a, p.key), *dn = cat(k, ".net"), *dt = cat(k, ".tbl");
            b = slurp(net, &n); spit(dn, b, n); free(b);
            b = slurp(tbl, &n); spit(dt, b, n); free(b);
            pairs[np++] = p; free(tbl);
        }
        if (!np) die("empty model audit", 0);
        qsort(pairs, np, sizeof *pairs, paircmp);
        FILE *f = fopen(rec, "wb"); if (!f) die("cannot write", rec);
        fputs("{\n  \"pairs\": {\n", f);
        for (int j = 0; j < np; j++)
            fprintf(f, "    \"%s\": {\n      \"network_sha256\": \"%s\",\n      \"table_sha256\": \"%s\"\n    }%s\n", pairs[j].key, pairs[j].nh, pairs[j].th, j + 1 < np ? "," : "");
        fputs("  }\n}\n", f);
        if (fclose(f)) die("cannot write", rec);
    }
    printf("compiler routes: %d rows -> %s/routes.tsv\n", rows.n, out);
    return 0;
}

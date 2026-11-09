/* seed/ident.c -- canonical source-packages-v2 identity, C99 + POSIX.
 * Default: one combined SHA256 line. ROOT packages emits its canonical table;
 * ROOT changes OLD_TABLE compares it; list-current lists physical inputs;
 * list-aliases and package-inputs expose topology and ownership respectively.
 * legacy-projection/list-legacy retain only the historical projected algorithm.
 * write/check/manifest keep their record protocol, with the new source SID.
 */
#define _XOPEN_SOURCE 700
#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "sha256.h"

static void die(const char *why, const char *what) { fprintf(stderr, "ident: %s: %s\n", why, what); exit(1); }
static char **paths; static size_t npaths, cap;
static void add(const char *p) {
    if (npaths == cap) { cap = cap ? cap * 2 : 1024; paths = realloc(paths, cap * sizeof *paths); if (!paths) die("out of memory", p); }
    paths[npaths] = malloc(strlen(p) + 1); if (!paths[npaths]) die("out of memory", p);
    strcpy(paths[npaths++], p);
}
static int part_cmp(const void *x, const void *y) {
    const unsigned char *a = *(unsigned char *const *)x, *b = *(unsigned char *const *)y;
    for (;; a++, b++) {
        int ca = *a == '/' ? 1 : *a ? *a + 1 : 0, cb = *b == '/' ? 1 : *b ? *b + 1 : 0;
        if (ca != cb) return ca - cb;
        if (!ca) return 0;
    }
}
static const char *suffix(const char *p) {
    const char *base = strrchr(p, '/'), *dot; base = base ? base + 1 : p;
    dot = strrchr(base, '.'); return dot && dot != base ? dot : "";
}
static int legacy_projection, list_paths;
static int keep(const char *rel) {
    static const char *const exts[] = {".py", ".c", ".cx", ".h", ".inc", ".tsv", ".json", ".sh"};
    const char *sfx = suffix(rel); size_t i; int ok = 0;
    if (legacy_projection && !strcmp(sfx, ".cx")) return 0;
    for (i = 0; i < sizeof exts / sizeof *exts; i++) if (!strcmp(sfx, exts[i])) ok = 1;
    if (!ok) return 0;
    if (!strcmp(rel, "cx/check_memory.cx") || !strcmp(rel, "cx/check_owned.cx")) return 0;
    if (!strncmp(rel, "seed/", 5)) {
        const char *base = strrchr(rel, '/') + 1;
        return !strcmp(base, "tbl.c") || !strcmp(base, "net.c") || !strcmp(base, "json.h")
            || !strcmp(base, "gen.c") || !strcmp(base, "facts.h") || !strcmp(base, "ident.c")
            || !strcmp(base, "blob.c") || !strcmp(base, "sha256.h") || !strcmp(base, "pack.c")
            || !strcmp(base, "compilerpack.c") || !strcmp(base, "ape.c");
    }
    if (!strncmp(rel, "exec/", 5)) {   /* 0.0.32 X31: *check.sh and *check.py under exec are verifiers, not inputs */
        size_t n = strlen(rel);
        if ((n >= 8 && !strcmp(rel + n - 8, "check.sh")) || (n >= 8 && !strcmp(rel + n - 8, "check.py")) || (n >= 8 && !strcmp(rel + n - 8, "check.cx"))) return 0;
    }
    if ((!strcmp(sfx, ".c") || !strcmp(sfx, ".h")) && (!strncmp(rel, "probes/", 7) || strstr(rel, "/probes/")))
        return 0;   /* 0.0.35 P8: probe sources are test inputs (same rule as exec/pipeline/models.py closure) */
    if (!strncmp(rel, "exec/build/", 11))
        return !strchr(rel + 11, '/') && !strcmp(sfx, ".py");
    return 1;
}
/* rglob('*') without following directory symlinks; is_file() follows file symlinks */
static void walk(const char *rel) {
    DIR *d = opendir(rel); struct dirent *e;
    if (!d) die("cannot open directory", rel);
    while ((e = readdir(d))) {
        char *p; struct stat ls, st;
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        p = malloc(strlen(rel) + strlen(e->d_name) + 2); if (!p) die("out of memory", rel);
        sprintf(p, "%s/%s", rel, e->d_name);
        if (lstat(p, &ls)) die("cannot stat", p);
        if (S_ISDIR(ls.st_mode)) walk(p);
        else if (!stat(p, &st) && S_ISREG(st.st_mode) && keep(p)) add(p);
        free(p);
    }
    closedir(d);
}
static void file_entry(Sha *h, const char *rel) {
    /* read(2), not stdio: the product's <stdio.h> refuses ferror() in a unit
       that never calls fgetc (archive/plans/v0.0.32.md D3) */
    Sha f; unsigned char buf[1 << 16], dg[32]; long n; int fd = open(rel, O_RDONLY);
    if (fd < 0) die("cannot open", rel);
    sha_init(&f);
    while ((n = read(fd, buf, sizeof buf)) > 0) sha_put(&f, buf, (size_t)n);
    if (n < 0) die("read failed", rel);
    close(fd);
    sha_end(&f, dg);
    if (list_paths) { printf("%s\t", rel); for (int i=0;i<32;i++) printf("%02x", dg[i]); putchar('\n'); }
    sha_put(h, rel, strlen(rel) + 1);
    sha_put(h, dg, 32);
}
static void historical_identity(char hex[65]) {
    /* Release metadata (release/candidate.tsv) is deliberately outside this
       closure. Product sources, including the fixed version injection slot and
       every byte of limits/drive/version declarations, remain ordinary inputs:
       no version-value projection may hide a semantic edit in those files. */
    /* "unisa" left the list on 2026-10-30: its last Python files were archived and their data moved to exec/ */
    static const char *const dirs[] = {"exec", "src", "kernel", "include", "weights", "seed", "cx"};
    Sha h; unsigned char out[32]; size_t i, first; int j;
    npaths = 0;
    sha_init(&h);
    for (j = 0; j < (int)(sizeof dirs / sizeof *dirs) - (legacy_projection ? 1 : 0); j++) {
        first = npaths; walk(dirs[j]);
        qsort(paths + first, npaths - first, sizeof *paths, part_cmp);
    }
    for (i = 0; i < npaths; i++) file_entry(&h, paths[i]);
    file_entry(&h, "iterate/kernel/typekw.tsv");
    {   /* Keep parity with models.closure: construction adapters affect inputs.
           The bootstrap product itself is recorded separately, not a source. */
        static const char *const support[] = {"bound", "bound.c", "cxbin", "safedel.cx", "lib.sh",
            "build_ref.sh", "export_ref.sh", "refshim.h", "reffoot.h", "sourceflat.cx",
            "ape-pack.cx", "release-ape.cx", "cxlib/releaseversion.cx"};
        for (j = 0; j < (int)(sizeof support / sizeof *support); j++) {
            char path[128]; snprintf(path, sizeof path, "tools/%s", support[j]); file_entry(&h, path);
        }
    }
    first = npaths;
    {   DIR *d = opendir("exec/c/asm"); struct dirent *e;
        if (!d) die("cannot open directory", "exec/c/asm");
        while ((e = readdir(d))) {
            size_t n = strlen(e->d_name); char p[1024];
            if (n > 2 && !strcmp(e->d_name + n - 2, ".S") && n < sizeof p - 12) {
                sprintf(p, "exec/c/asm/%s", e->d_name); add(p);
            }
        }
        closedir(d);
    }
    qsort(paths + first, npaths - first, sizeof *paths, part_cmp);
    for (i = first; i < npaths; i++) file_entry(&h, paths[i]);
    sha_end(&h, out);
    for (i = 0; i < 32; i++) sprintf(hex + 2 * i, "%02x", out[i]);
}

/* Canonical source-packages-v2: physical bytes and alias topology are separate. */
static const char *const packages[] = {"cx/sys", "cx/mem", "cx/text", "cx/core", "cx/driver", "cx/runtime", "cx/compiler", "src", "exec", "seed", "weights", "include"};
typedef struct { char *path; int owner, support; char digest[65]; } Input;
typedef struct { char *logical, *target; int owner; } Alias;
static Input *inputs; static size_t ni;
static Alias *aliases; static size_t na;
static char **visited; static size_t nv;
static char root_real[4096], package_hex[12][65];
static int output_mode;
static char *copy_string(const char *s) {
    char *p = malloc(strlen(s) + 1); if (!p) die("out of memory", s); strcpy(p, s); return p;
}
static void *grow(void *p, size_t n) { p = realloc(p, n); if (!p) die("out of memory", "source packages"); return p; }
static int under(const char *p, const char *base) { size_t n = strlen(base); return !strncmp(p, base, n) && (!p[n] || p[n] == '/'); }
static int owner_of(const char *p) {
    int i;
    if (!strcmp(p, "iterate/kernel/typekw.tsv")) return 8;
    for (i = 0; i < 12; i++) if (under(p, packages[i])) return i;
    if (under(p, "cx")) return 3;
    if (under(p, "ext/memx")) return 1;
    if (under(p, "ext/textx")) return 2;
    if (under(p, "ext/sysx")) return 0;
    if (under(p, "ext/crypto") || under(p, "ext/flownet") || under(p, "flownet")) return 3;
    return 9;
}
static void canonical(const char *p, char out[4096]) {
    char abs[4096]; size_t n = strlen(root_real);
    if (!realpath(p, abs)) die("cannot resolve source (dangling, loop, or inaccessible)", p);
    if (strncmp(abs, root_real, n) || (abs[n] && abs[n] != '/')) die("source escapes ROOT", p);
    if (!abs[n]) strcpy(out, "."); else strcpy(out, abs + n + 1);
    if (strchr(out, '\t') || strchr(out, '\n') || strchr(out, '\r')) die("noncanonical source path", p);
}
static void remember_alias(const char *logical, const char *target) {
    char parent[4096], physical_parent[4096], declaration[4096];
    const char *slash, *base; struct stat st; size_t i, n; int z, owner;
    if (!strcmp(logical, target)) return;
    if (lstat(logical, &st)) die("cannot inspect source alias", logical);
    /* A different resolved path can be inherited from a directory alias.
       Only a link declared at this leaf contributes a topology record. */
    if (!S_ISLNK(st.st_mode)) return;
    slash = strrchr(logical, '/'); base = slash ? slash + 1 : logical;
    if (slash) {
        n = (size_t)(slash - logical);
        if (n >= sizeof parent) die("source alias parent too long", logical);
        memcpy(parent, logical, n); parent[n] = 0;
    } else strcpy(parent, ".");
    canonical(parent, physical_parent);
    if (!strcmp(physical_parent, ".")) z = snprintf(declaration, sizeof declaration, "%s", base);
    else z = snprintf(declaration, sizeof declaration, "%s/%s", physical_parent, base);
    if (z < 0 || (size_t)z >= sizeof declaration) die("source alias declaration too long", logical);
    owner = owner_of(declaration);
    for (i = 0; i < na; i++) if (!strcmp(aliases[i].logical, declaration) && !strcmp(aliases[i].target, target)) return;
    aliases = grow(aliases, (na + 1) * sizeof *aliases);
    aliases[na].logical = copy_string(declaration); aliases[na].target = copy_string(target); aliases[na++].owner = owner;
}
static int string_cmp(const void *a, const void *b) { return strcmp(*(char *const *)a, *(char *const *)b); }
static int assembly(const char *p) { return under(p, "exec/c/asm") && !strcmp(suffix(p), ".S"); }
/* Only named documentation and Finder metadata are known non-source here.
   Cache/artifact-looking names and unknown suffixes may be real inputs. */
static int conservative_excluded(const char *p) {
    const char *base = strrchr(p, '/');
    base = base ? base + 1 : p;
    return !strcmp(base, "README.md") || !strcmp(base, ".DS_Store");
}
static void collect(const char *logical, int force) {
    char physical[4096]; struct stat st; size_t i;
    int conservative = under(logical, "seed") || under(logical, "include");
    if (!force && conservative && conservative_excluded(logical)) return;
    canonical(logical, physical);
    conservative = conservative || under(physical, "seed") || under(physical, "include");
    if (stat(physical, &st)) die("cannot stat source", logical);
    if (force && !S_ISREG(st.st_mode)) die("mandatory support is not a regular file", logical);
    if ((S_ISREG(st.st_mode) || S_ISDIR(st.st_mode)) && !(st.st_mode & 0444)) die("source has no read permission", logical);
    if (S_ISDIR(st.st_mode) && !(st.st_mode & 0111)) die("source directory has no search permission", logical);
    if (S_ISDIR(st.st_mode)) {
        DIR *d; struct dirent *e; char **children = NULL; size_t nc = 0;
        remember_alias(logical, physical);
        for (i = 0; i < nv; i++) if (!strcmp(visited[i], physical)) return;
        visited = grow(visited, (nv + 1) * sizeof *visited); visited[nv++] = copy_string(physical);
        d = opendir(physical); if (!d) die("cannot open source directory", logical);
        for (;;) {
            errno = 0; e = readdir(d);
            if (!e) { if (errno) die("cannot read source directory", logical); break; }
            if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
            children = grow(children, (nc + 1) * sizeof *children); children[nc++] = copy_string(e->d_name);
        }
        if (closedir(d)) die("cannot close source directory", logical);
        qsort(children, nc, sizeof *children, string_cmp);
        for (i = 0; i < nc; i++) {
            char *p = malloc(strlen(logical) + strlen(children[i]) + 2); if (!p) die("out of memory", logical);
            sprintf(p, "%s/%s", logical, children[i]); collect(p, force); free(p); free(children[i]);
        }
        free(children);
        return;
    }
    if (!S_ISREG(st.st_mode)) return;
    if (!force && !conservative && !keep(logical) && !assembly(logical)) return;
    remember_alias(logical, physical);
    for (i = 0; i < ni; i++) if (!strcmp(inputs[i].path, physical)) return;
    inputs = grow(inputs, (ni + 1) * sizeof *inputs);
    inputs[ni].path = copy_string(physical); inputs[ni].owner = owner_of(physical);
    inputs[ni].support = !under(physical, packages[inputs[ni].owner]); ni++;
}
static int input_cmp(const void *a, const void *b) {
    const Input *x = a, *y = b; return x->owner != y->owner ? x->owner - y->owner : strcmp(x->path, y->path);
}
static int alias_cmp(const void *a, const void *b) {
    const Alias *x = a, *y = b; int c;
    if (x->owner != y->owner) return x->owner - y->owner;
    c = strcmp(x->logical, y->logical); return c ? c : strcmp(x->target, y->target);
}
static void finish_hex(Sha *h, char hex[65]) {
    unsigned char dg[32]; int j; sha_end(h, dg); for (j = 0; j < 32; j++) sprintf(hex + 2*j, "%02x", dg[j]);
}
static void combined(char rows[12][65], char hex[65]) {
    Sha h; int j; sha_init(&h); sha_put(&h, "source-packages-v2\n", 19);
    for (j = 0; j < 12; j++) { sha_put(&h, packages[j], strlen(packages[j])); sha_put(&h, "\t", 1); sha_put(&h, rows[j], 64); sha_put(&h, "\n", 1); }
    finish_hex(&h, hex);
}
static long file_digest(const char *path, char hex[65]);
static void identity(char hex[65]) {
    static const char *const support[] = {"bound", "bound.c", "cxbin", "safedel.cx", "lib.sh", "build_ref.sh", "export_ref.sh", "refshim.h", "reffoot.h", "sourceflat.cx", "ape-pack.cx", "release-ape.cx", "cxlib/releaseversion.cx"};
    size_t i; int j; char p[256];
    if (legacy_projection) { historical_identity(hex); return; }
    if (!realpath(".", root_real)) die("cannot resolve ROOT", ".");
    visited = grow(visited, sizeof *visited); visited[0] = copy_string("."); nv = 1;
    /* Conservative roots first: a prior narrower directory alias must not
       mark their physical trees visited before unknown inputs are collected. */
    collect("seed", 0); collect("include", 0);
    for (j = 0; j < 12; j++) if (j != 9 && j != 11) collect(packages[j], 0);
    collect("cx", 0); collect("kernel", 0); collect("iterate/kernel/typekw.tsv", 1);
    for (i = 0; i < sizeof support / sizeof *support; i++) { snprintf(p, sizeof p, "tools/%s", support[i]); collect(p, 1); }
    qsort(inputs, ni, sizeof *inputs, input_cmp); qsort(aliases, na, sizeof *aliases, alias_cmp);
    for (i = 0; i < ni; i++) file_digest(inputs[i].path, inputs[i].digest);
    for (j = 0; j < 12; j++) {
        Sha h; sha_init(&h); sha_put(&h, "package-files-v1\n", 17);
        for (i = 0; i < ni; i++) if (inputs[i].owner == j) {
            sha_put(&h, inputs[i].path, strlen(inputs[i].path) + 1); sha_put(&h, inputs[i].digest, 64); sha_put(&h, "\n", 1);
        }
        sha_put(&h, "package-aliases-v1\n", 19);
        for (i = 0; i < na; i++) if (aliases[i].owner == j) {
            sha_put(&h, aliases[i].logical, strlen(aliases[i].logical) + 1); sha_put(&h, aliases[i].target, strlen(aliases[i].target) + 1);
        }
        finish_hex(&h, package_hex[j]);
    }
    combined(package_hex, hex);
    if (output_mode == 1) {
        puts("source-packages-v2"); for (j = 0; j < 12; j++) printf("%s\t%s\n", packages[j], package_hex[j]);
    } else if (output_mode == 2) {
        for (i = 0; i < na; i++) printf("%s\t%s\t%s\n", packages[aliases[i].owner], aliases[i].logical, aliases[i].target);
    } else if (output_mode == 3) {
        for (i = 0; i < ni; i++) printf("%s\t%s\t%s\t%s\n", packages[inputs[i].owner], inputs[i].support ? "support" : "file", inputs[i].path, inputs[i].digest);
    } else if (list_paths) for (i = 0; i < ni; i++) printf("%s\t%s\n", inputs[i].path, inputs[i].digest);
}
static void changes(const char *path) {
    char old[12][65], line[256], before[65], after[65]; int j, k; FILE *f = fopen(path, "rb");
    if (!f) die("cannot open old package table", path);
    if (!fgets(line, sizeof line, f) || strcmp(line, "source-packages-v2\n")) die("invalid package table header", path);
    for (j = 0; j < 12; j++) {
        size_t n = strlen(packages[j]);
        if (!fgets(line, sizeof line, f) || strlen(line) != n + 66 || strncmp(line, packages[j], n) || line[n] != '\t' || line[n+65] != '\n') die("invalid package table row", path);
        for (k = 0; k < 64; k++) if (!((line[n+1+k] >= '0' && line[n+1+k] <= '9') || (line[n+1+k] >= 'a' && line[n+1+k] <= 'f'))) die("invalid package digest", path);
        memcpy(old[j], line+n+1, 64); old[j][64] = 0;
    }
    if (fgetc(f) != EOF || ferror(f)) die("trailing or unreadable package table", path);
    fclose(f); combined(old, before); identity(after);
    for (j = 0; j < 12; j++) if (strcmp(old[j], package_hex[j])) printf("%s\t%s\t%s\n", packages[j], old[j], package_hex[j]);
    printf("before\t%s\nafter\t%s\n", before, after);
}
/* sha256 of a file (absolute or relative to ROOT) and its size */
/* B5c: HEX may be NULL -- the build record no longer stores the artifact digest,
   but the size cross-check still needs the byte count. */
static long file_digest(const char *path, char hex[65]) {
    Sha f; unsigned char buf[1 << 16], dg[32]; long n, total = 0; int i, fd = open(path, O_RDONLY);
    if (fd < 0) die("cannot open", path);
    sha_init(&f);
    while ((n = read(fd, buf, sizeof buf)) > 0) { sha_put(&f, buf, (size_t)n); total += n; }
    if (n < 0) die("read failed", path);
    close(fd); sha_end(&f, dg);
    if (hex) for (i = 0; i < 32; i++) sprintf(hex + 2 * i, "%02x", dg[i]);
    return total;
}
static void json_str(FILE *o, const char *v) {
    const unsigned char *p = (const unsigned char *)v;
    fputc('"', o);
    for (; *p; p++) {
        if (*p == '"' || *p == '\\') fprintf(o, "\\%c", *p);
        else if (*p == '\n') fputs("\\n", o);
        else if (*p == '\r') fputs("\\r", o);
        else if (*p == '\t') fputs("\\t", o);
        else if (*p == '\b') fputs("\\b", o);
        else if (*p == '\f') fputs("\\f", o);
        else if (*p < 0x20) fprintf(o, "\\u%04x", *p);
        else if (*p >= 0x80) die("non-ASCII build setting", v);
        else fputc(*p, o);
    }
    fputc('"', o);
}
extern char **environ;
static int env_cmp(const void *x, const void *y) { return strcmp(*(char *const *)x, *(char *const *)y); }

/* ---- B5c follow-up: the record must carry the manifest identity the verifier
 * rebuilds (tools/source-package.cx root_epoch_execution_record compares it byte
 * for byte).  This mirrors tools/cxlib/productpkg.cx pp_identity_kind /
 * pp_product_digest for the split2 layout:
 *   product_kind -> "split2" (unisacc.com + unisacc.com.dev are both present)
 *   product_digest -> sha256 over the member lines
 *     "<name>	<kind>		<sha256>	<bytes>	<index>
"
 *   members sorted by name; the launcher is named "unisacc.com", the other
 *   members come from ape-receipt.txt when it names them, else the two defaults. */
#define MF_MAX 16
#define MF_NAME 256
static char mf_name[MF_MAX][MF_NAME], mf_sha[MF_MAX][65];
static long mf_bytes[MF_MAX];
static int mf_n;

static int mf_present(const char *p) {
    struct stat st;
    return lstat(p, &st) == 0;
}
static int mf_is_launcher(const char *n) { return strcmp(n, "unisacc.com") == 0; }
static const char *mf_kind(const char *n) {
    if (!strcmp(n, "unisacc.com")) return "base";
    if (!strcmp(n, "entry.elf")) return "stage";
    return "resource";
}
static int mf_cmp(const void *x, const void *y) { return strcmp(*(char *const *)x, *(char *const *)y); }
static int mf_add(const char *n) {
    int i;
    if (mf_n >= MF_MAX || strlen(n) >= MF_NAME) die("too many product members", n);
    for (i = 0; i < mf_n; i++) if (!strcmp(mf_name[i], n)) return 0;
    strcpy(mf_name[mf_n], n); mf_n++;
    return 1;
}
/* Fill mf_name[]/mf_sha[]/mf_bytes[] from ROOT.  Returns 0, or -1 when the layout
   is neither split2 nor a lone unisacc.com. */
static int manifest_members(const char *root) {
    char dev[4400], p[4400], rec[1 << 16], name[MF_NAME], *order[MF_MAX];
    int rec_ok, i, j, n;
    struct stat st;
    mf_n = 0;
    sprintf(dev, "%s/unisacc.com.dev", root);
    if (stat(dev, &st) != 0 || !S_ISDIR(st.st_mode)) return -1;
    mf_add("unisacc.com");
    sprintf(p, "%s/ape-receipt.txt", root);
    rec_ok = mf_present(p);
    if (rec_ok) {
        int fd = open(p, O_RDONLY); long rn;
        if (fd < 0) return -1;
        rn = read(fd, rec, sizeof rec - 1); close(fd);
        if (rn <= 0) return -1;
        rec[rn] = 0;
        {   long a = 0;
            while (a < rn) {
                long e = a, st_;
                while (e < rn && rec[e] != '\n') e++;
                /* Same rule as tools/cxlib/productpkg.cx pp_receipt_line: skip blank and
                   NOT_-prefixed marker lines, take the *basename* of the second field. */
                st_ = a;
                while (st_ < e && (rec[st_] == ' ' || rec[st_] == '\t')) st_++;
                {
                    long end = st_;
                    while (end < e && rec[end] != '\t' && rec[end] != ' ') end++;
                    if (end > st_ && !(end - st_ >= 4 && rec[st_] == 'N' && rec[st_ + 1] == 'O' &&
                                       rec[st_ + 2] == 'T' && rec[st_ + 3] == '_')) {
                        long p2 = end, q2, base;
                        while (p2 < e && (rec[p2] == '\t' || rec[p2] == ' ')) p2++;
                        q2 = p2;
                        while (q2 < e && rec[q2] != '\t' && rec[q2] != ' ') q2++;
                        if (q2 == p2) { p2 = st_; q2 = end; }
                        base = p2;
                        {   long r2;
                            for (r2 = p2; r2 < q2; r2++) if (rec[r2] == '/') base = r2 + 1;
                        }
                        if (q2 > base && q2 - base < MF_NAME) {
                            memcpy(name, rec + base, (size_t)(q2 - base)); name[q2 - base] = 0;
                            if (!mf_is_launcher(name)) mf_add(name);
                        }
                    }
                }
                a = e + 1;
            }
        }
    } else {
        mf_add("p3-package.bin");
        mf_add("entry.elf");
    }
    /* sort members 1..n-1 by name (the launcher stays first, as productpkg does) */
    for (i = 1; i < mf_n; i++) order[i - 1] = mf_name[i];
    qsort(order, (size_t)(mf_n - 1), sizeof *order, mf_cmp);
    for (i = 1; i < mf_n; i++) strcpy(mf_name[i], order[i - 1]);
    for (i = 0; i < mf_n; i++) {
        const char *src = mf_is_launcher(mf_name[i]) ? root : dev;
        char mp[4400];
        sprintf(mp, "%s/%s", src, mf_name[i]);
        if (lstat(mp, &st) != 0) return -1;
        if (S_ISLNK(st.st_mode)) return -1;
        mf_bytes[i] = file_digest(mp, mf_sha[i]);
    }
    (void)n; (void)j;
    return 0;
}
/* product_digest: sha256 over the canonical manifest text (see above). */
static int manifest_digest(const char *root, char hex[65]) {
    Sha h; unsigned char dg[32];
    int i, k;
    char line[MF_NAME + 200];
    if (manifest_members(root)) return -1;
    sha_init(&h);
    for (i = 0; i < mf_n; i++) {
        int l = snprintf(line, sizeof line, "%s\t%s\t\t%s\t%ld\t%d\n",
                         mf_name[i], mf_kind(mf_name[i]), mf_sha[i], mf_bytes[i], i);
        if (l < 0 || (size_t)l >= sizeof line) return -1;
        sha_put(&h, line, (size_t)l);
    }
    sha_end(&h, dg);
    for (k = 0; k < 32; k++) sprintf(hex + 2 * k, "%02x", dg[k]);
    return 0;
}

static void write_record(const char *root, const char *artifact, const char *expected) {
    char cur[65], commit[64] = {0}, cmd[2048], dest[2048], tmp[2100];
    char **set = NULL; size_t ns = 0, i; long bytes; FILE *pp, *o;
    identity(cur);
    if (strcmp(cur, expected)) die("product inputs changed during packaging", artifact);
    bytes = file_digest(artifact, 0);
    if (strlen(root) > 1000) die("root too long", root);
    sprintf(cmd, "git -C '%s' rev-parse HEAD", root);
    pp = popen(cmd, "r");
    if (!pp || !fgets(commit, sizeof commit, pp) || pclose(pp) != 0) die("git rev-parse failed", root);
    commit[strcspn(commit, "\n")] = 0;
    for (i = 0; environ[i]; i++) {
        const char *e = environ[i];
        if (!strncmp(e, "E1", 2) || !strncmp(e, "E2", 2) || !strncmp(e, "E3", 2) || !strncmp(e, "E4", 2)
            || !strncmp(e, "PYTHONHASHSEED", 14)) {
            set = realloc(set, (ns + 1) * sizeof *set); if (!set) die("out of memory", e);
            set[ns++] = (char *)e;
        }
    }
    qsort(set, ns, sizeof *set, env_cmp);
    if (strlen(artifact) > 2000) die("path too long", artifact);
    sprintf(dest, "%s.build.json", artifact); sprintf(tmp, "%s.tmp", dest);
    o = fopen(tmp, "w"); if (!o) die("cannot write", tmp);
    /* B5c: the artifact section is retired. The product identity is the manifest
       (six fields, tools/manifest-gen.cx), not sha256(file), so a build record that
       carried artifact_sha256 could disagree with it -- which is exactly what made
       source-package.cx refuse with root-epoch-record-shape. `bytes` stays as a
       cheap size cross-check against the artifact. */
    fprintf(o, "{\n  \"bytes\": %ld,\n  \"commit\": ", bytes);
    json_str(o, commit);
    fprintf(o, ",\n  \"schema\": 1,\n  \"settings\": ");
    if (!ns) fputs("{}", o);
    else {
        fputs("{\n", o);
        for (i = 0; i < ns; i++) {
            char *eq = strchr(set[i], '='), key[256];
            size_t kl = (size_t)(eq - set[i]);
            if (kl >= sizeof key) die("setting name too long", set[i]);
            memcpy(key, set[i], kl); key[kl] = 0;
            fputs("    ", o); json_str(o, key); fputs(": ", o); json_str(o, eq + 1);
            fputs(i + 1 < ns ? ",\n" : "\n", o);
        }
        fputs("  }", o);
    }
    {   char pdig[65]; int pdok = manifest_digest(root, pdig) == 0;
        fprintf(o, ",\n  \"sources_sha256\": \"%s\"", cur);
        if (pdok) {
            fprintf(o, ",\n  \"product_kind\": \"split2\"");
            fprintf(o, ",\n  \"product_digest\": \"%s\"", pdig);
        }
        fprintf(o, "\n}\n");
    }
    if (fclose(o)) die("write failed", tmp);
    if (rename(tmp, dest)) die("cannot rename", tmp);
    free(set);
}
/* the value after `"KEY": ` in a record this tool or provenance.py wrote */
static int field(const char *text, const char *key, char *out, size_t cap) {
    char pat[64]; const char *p; size_t n = 0;
    sprintf(pat, "\"%s\": ", key);
    p = strstr(text, pat); if (!p) return 0;
    p += strlen(pat); if (*p == '"') p++;
    while (*p && *p != '"' && *p != ',' && *p != '\n' && n + 1 < cap) out[n++] = *p++;
    out[n] = 0; return 1;
}
static void check_record(const char *artifact) {
    char path[2100], text[1 << 14], v[128], cur[65], art[65]; long bytes; int fd; long n;
    if (strlen(artifact) > 2000) die("path too long", artifact);
    sprintf(path, "%s.build.json", artifact);
    fd = open(path, O_RDONLY);
    if (fd < 0) { fprintf(stderr, "product freshness: %s: no build record\n", artifact); exit(1); }
    n = read(fd, text, sizeof text - 1); close(fd);
    if (n <= 0) { fprintf(stderr, "product freshness: %s: unreadable build record\n", artifact); exit(1); }
    text[n] = 0;
    identity(cur);
    if (!field(text, "schema", v, sizeof v) || strcmp(v, "1") || !field(text, "sources_sha256", v, sizeof v) || strcmp(v, cur)) {
        fprintf(stderr, "product freshness: %s: product inputs changed; rebuild with make com\n", artifact); exit(1);
    }
    bytes = file_digest(artifact, art);
    /* B5c: the artifact section is retired, so only the size cross-check remains.
       A record that still carries artifact_sha256 is refused by name: silently
       accepting it would let a stale identity survive the cutover. */
    if (strstr(text, "\"artifact_sha256\":")) {
        fprintf(stderr, "product freshness: %s: build record still carries artifact_sha256; rebuild with make com\n", artifact); exit(1);
    }
    {   char b[32]; sprintf(b, "%ld", bytes);
        if (!field(text, "bytes", cur, sizeof cur) || strcmp(cur, b)) {
            fprintf(stderr, "product freshness: %s: artifact size differs from its build record\n", artifact); exit(1);
        }
    }
    printf("product freshness: verified %s\n", artifact);
}
/* Stage completion records for exec/c/buildcompiler.sh (was its inline Python).
   The key covers the source identity (which includes exec/c/asm) and the
   E1-E4/PYTHONHASHSEED settings; it only has to agree with itself inside one build tree. */
static void stage_key(char hex[65]) {
    Sha h; unsigned char out[32]; char id[65]; char **env; size_t n = 0, i;
    char **set;
    identity(id);
    sha_init(&h);
    sha_put(&h, "compiler-container", 19);
    sha_put(&h, id, 65);
    for (env = environ; *env; env++) n++;
    set = malloc((n + 1) * sizeof *set); if (!set) die("out of memory", "env");
    n = 0;
    for (env = environ; *env; env++)
        if (!strncmp(*env, "E1", 2) || !strncmp(*env, "E2", 2) || !strncmp(*env, "E3", 2)
            || !strncmp(*env, "E4", 2) || !strncmp(*env, "PYTHONHASHSEED", 14)) set[n++] = *env;
    qsort(set, n, sizeof *set, env_cmp);
    for (i = 0; i < n; i++) sha_put(&h, set[i], strlen(set[i]) + 1);
    free(set);
    sha_end(&h, out);
    for (i = 0; i < 32; i++) sprintf(hex + 2 * i, "%02x", out[i]);
}
static size_t stage_names(const char *stage, char names[32][256]) {
    static const char *const sh[] = {"e2", "e1", "e3", "e4", "o1", "prune", "nativeabi"};
    static const char *const ex[] = {"json", "tbl", "net"};
    size_t n = 0, i, j;
    if (strlen(stage) > 200) die("stage name too long", stage);
    if (!strcmp(stage, "shared")) {
        for (i = 0; i < 7; i++) for (j = 0; j < 3; j++) sprintf(names[n++], "shared/%s.%s", sh[i], ex[j]);
        strcpy(names[n++], "kernels/arm64"); strcpy(names[n++], "kernels/x86_64");
    } else {
        for (i = 0; i < 2; i++) for (j = 0; j < 3; j++) sprintf(names[n++], "%s/%s.%s", stage, i ? "elf" : "lower", ex[j]);
        sprintf(names[n++], "%s/route.tsv", stage);
    }
    qsort(names, n, sizeof *names, (int (*)(const void *, const void *))strcmp);
    return n;
}
static char *slurp(const char *path, int must) {
    char *b; long n, got = 0; struct stat st; int fd = open(path, O_RDONLY);
    if (fd < 0) { if (must) die("cannot open", path); return 0; }
    if (fstat(fd, &st)) die("cannot stat", path);
    b = malloc((size_t)st.st_size + 1); if (!b) die("out of memory", path);
    while (got < st.st_size && (n = read(fd, b + got, (size_t)(st.st_size - got))) > 0) got += n;
    close(fd); b[got] = 0; return b;
}
/* json.dumps(data, sort_keys=True), as the Python version wrote it */
static char *stage_record(const char *t, const char *stage, const char *key, int forwrite) {
    char names[32][256], path[4096], dg[65]; size_t n = stage_names(stage, names), i, len;
    char *r = malloc(256 + n * 400); if (!r) die("out of memory", stage);
    len = (size_t)sprintf(r, "{\"inputs\": \"%s\", \"outputs\": {", key);
    for (i = 0; i < n; i++) {
        if (strlen(t) + strlen(names[i]) + 2 >= sizeof path) die("path too long", names[i]);
        sprintf(path, "%s/%s", t, names[i]);
        if (file_digest(path, dg) == 0 && forwrite) die("empty stage output", path);
        len += (size_t)sprintf(r + len, "%s\"%s\": \"%s\"", i ? ", " : "", names[i], dg);
    }
    strcpy(r + len, "}}");
    return r;
}
static void stage_manifest(const char *mode, const char *t, const char *stage) {
    char key[65], path[4096], *rec, *old; FILE *o;
    if (strlen(t) + strlen(stage) + 40 >= sizeof path) die("path too long", t);
    stage_key(key);
    if (!strcmp(mode, "identity")) { printf("%s\n", key); return; }
    sprintf(path, "%s/%s/manifest.json", t, stage);
    if (!strcmp(mode, "write")) {
        char tmp[4096], in[4096], *exp;
        sprintf(in, "%s/%s/input.sha256", t, stage);
        exp = slurp(in, 1); exp[strcspn(exp, " \t\r\n")] = 0;
        if (strcmp(exp, key)) die("model source changed during", stage);
        rec = stage_record(t, stage, key, 1);
        sprintf(tmp, "%s/%s/manifest.tmp", t, stage);
        if (!(o = fopen(tmp, "w"))) die("cannot write", tmp);
        fputs(rec, o);
        if (fclose(o) || rename(tmp, path)) die("cannot write", path);
        return;
    }
    if (strcmp(mode, "check")) die("usage", "ident ROOT manifest identity|write|check TREE STAGE");
    if (!(old = slurp(path, 0))) die("missing completed dependency", stage);
    rec = stage_record(t, stage, key, 0);
    if (strcmp(old, rec)) die("stale or changed stage dependency", stage);
}
int main(int argc, char **argv) {
    char hex[65];
    const char *root = argc >= 2 ? argv[1] : ".";
    char abs1[4096];
    if (argc == 6 && !strcmp(argv[2], "manifest")) {
        char t[4096];
        if (argv[4][0] == '/') { if (strlen(argv[4]) >= sizeof t) die("path too long", argv[4]); strcpy(t, argv[4]); }
        else { if (!getcwd(t, sizeof t - strlen(argv[4]) - 2)) die("getcwd failed", argv[4]); strcat(t, "/"); strcat(t, argv[4]); }
        if (chdir(root)) die("cannot enter", root);
        stage_manifest(argv[3], t, argv[5]);
        return 0;
    }
    if (argc == 3 && (!strcmp(argv[2], "packages") || !strcmp(argv[2], "list-aliases") || !strcmp(argv[2], "package-inputs"))) {
        output_mode = !strcmp(argv[2], "packages") ? 1 : !strcmp(argv[2], "list-aliases") ? 2 : 3;
        if (chdir(root)) die("cannot enter", root); identity(hex); return 0;
    }
    if (argc == 4 && !strcmp(argv[2], "changes")) {
        if (argv[3][0] == '/') { if (strlen(argv[3]) >= sizeof abs1) die("path too long", argv[3]); strcpy(abs1, argv[3]); }
        else { if (!getcwd(abs1, sizeof abs1 - strlen(argv[3]) - 2)) die("getcwd failed", argv[3]); strcat(abs1, "/"); strcat(abs1, argv[3]); }
        if (chdir(root)) die("cannot enter", root); changes(abs1); return 0;
    }
    if (argc == 3 && (!strcmp(argv[2], "list-current") || !strcmp(argv[2], "list-legacy"))) { if (chdir(root)) die("cannot enter", root); legacy_projection = !strcmp(argv[2], "list-legacy"); list_paths = 1; identity(hex); return 0; }
    if (argc == 3 && !strcmp(argv[2], "legacy-projection")) { if (chdir(root)) die("cannot enter", root); legacy_projection = 1; identity(hex); puts(hex); return 0; }
    if (!(argc == 1 || argc == 2 || (argc == 5 && !strcmp(argv[2], "write")) || (argc == 4 && !strcmp(argv[2], "check"))))
        die("usage", "ident [ROOT] | ident ROOT write ARTIFACT START | ident ROOT check ARTIFACT");
    /* the artifact path is taken relative to the caller's directory, before entering ROOT */
    if (argc >= 4) {
        if (argv[3][0] == '/') { if (strlen(argv[3]) >= sizeof abs1) die("path too long", argv[3]); strcpy(abs1, argv[3]); }
        else { if (!getcwd(abs1, sizeof abs1 - strlen(argv[3]) - 2)) die("getcwd failed", argv[3]); strcat(abs1, "/"); strcat(abs1, argv[3]); }
    }
    if (chdir(root)) die("cannot enter", root);
    if (argc == 5) { write_record(".", abs1, argv[4]); return 0; }
    if (argc == 4) { check_record(abs1); return 0; }
    identity(hex);
    printf("%s\n", hex);
    return 0;
}

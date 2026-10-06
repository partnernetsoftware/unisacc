/* seed/ident.c -- the product source identity, in C99 + POSIX (0.0.32 B5).
 *
 * Prints what `exec/c/provenance.py identity` prints: SHA-256 over the
 * models.closure() file set (exec unisa src kernel include weights seed,
 * filtered the same way, plus iterate/kernel/typekw.tsv), followed by
 * the .S files in exec/c/asm.  Each file contributes its POSIX path, a NUL, and the raw
 * 32-byte SHA-256 of its contents.  Paths sort by their parts, byte-wise
 * (Python's sorted(key=parts)), which is plain byte order with '/' lowest.
 *
 * Usage: ident [ROOT]                          the identity (default ROOT: .)
 *        ident ROOT write ARTIFACT START_DIGEST  ARTIFACT.build.json, as provenance.py write
 *        ident ROOT check ARTIFACT               verify it, as provenance.py check
 *        ident ROOT manifest identity|write|check TREE STAGE   buildcompiler stage records
 * The record is byte-equal to Python's json.dumps(indent=2, sort_keys=True).
 */
#include <dirent.h>
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
static int keep(const char *rel) {
    static const char *const exts[] = {".py", ".c", ".h", ".inc", ".tsv", ".json", ".sh"};
    const char *sfx = suffix(rel); size_t i; int ok = 0;
    for (i = 0; i < sizeof exts / sizeof *exts; i++) if (!strcmp(sfx, exts[i])) ok = 1;
    if (!ok) return 0;
    if (!strncmp(rel, "seed/", 5)) {
        const char *base = strrchr(rel, '/') + 1;
        return !strcmp(base, "tbl.c") || !strcmp(base, "net.c") || !strcmp(base, "json.h")
            || !strcmp(base, "gen.c") || !strcmp(base, "facts.h") || !strcmp(base, "ident.c")
            || !strcmp(base, "blob.c") || !strcmp(base, "sha256.h") || !strcmp(base, "pack.c")
            || !strcmp(base, "compilerpack.c") || !strcmp(base, "ape.c");
    }
    if (!strncmp(rel, "exec/", 5)) {   /* 0.0.32 X31: *check.sh and *check.py under exec are verifiers, not inputs */
        size_t n = strlen(rel);
        if ((n >= 8 && !strcmp(rel + n - 8, "check.sh")) || (n >= 8 && !strcmp(rel + n - 8, "check.py"))) return 0;
    }
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
       that never calls fgetc (plans/v0.0.32.md D3) */
    Sha f; unsigned char buf[1 << 16], dg[32]; long n; int fd = open(rel, O_RDONLY);
    if (fd < 0) die("cannot open", rel);
    sha_init(&f);
    while ((n = read(fd, buf, sizeof buf)) > 0) sha_put(&f, buf, (size_t)n);
    if (n < 0) die("read failed", rel);
    close(fd);
    sha_end(&f, dg);
    sha_put(h, rel, strlen(rel) + 1);
    sha_put(h, dg, 32);
}
static void identity(char hex[65]) {
    static const char *const dirs[] = {"exec", "unisa", "src", "kernel", "include", "weights", "seed"};
    Sha h; unsigned char out[32]; size_t i, first; int j;
    npaths = 0;
    sha_init(&h);
    for (j = 0; j < 7; j++) {
        first = npaths; walk(dirs[j]);
        qsort(paths + first, npaths - first, sizeof *paths, part_cmp);
    }
    for (i = 0; i < npaths; i++) file_entry(&h, paths[i]);
    file_entry(&h, "iterate/kernel/typekw.tsv");
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
/* sha256 of a file (absolute or relative to ROOT) and its size */
static long file_digest(const char *path, char hex[65]) {
    Sha f; unsigned char buf[1 << 16], dg[32]; long n, total = 0; int i, fd = open(path, O_RDONLY);
    if (fd < 0) die("cannot open", path);
    sha_init(&f);
    while ((n = read(fd, buf, sizeof buf)) > 0) { sha_put(&f, buf, (size_t)n); total += n; }
    if (n < 0) die("read failed", path);
    close(fd); sha_end(&f, dg);
    for (i = 0; i < 32; i++) sprintf(hex + 2 * i, "%02x", dg[i]);
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
static void write_record(const char *root, const char *artifact, const char *expected) {
    char cur[65], art[65], commit[64] = {0}, cmd[2048], dest[2048], tmp[2100];
    char **set = NULL; size_t ns = 0, i; long bytes; FILE *pp, *o;
    identity(cur);
    if (strcmp(cur, expected)) die("product inputs changed during packaging", artifact);
    bytes = file_digest(artifact, art);
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
    fprintf(o, "{\n  \"artifact_sha256\": \"%s\",\n  \"bytes\": %ld,\n  \"commit\": ", art, bytes);
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
    fprintf(o, ",\n  \"sources_sha256\": \"%s\"\n}\n", cur);
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
    {   char b[32]; sprintf(b, "%ld", bytes);
        if (!field(text, "artifact_sha256", v, sizeof v) || strcmp(v, art) || !field(text, "bytes", cur, sizeof cur) || strcmp(cur, b)) {
            fprintf(stderr, "product freshness: %s: artifact differs from its build record\n", artifact); exit(1);
        }
    }
    printf("product freshness: verified %s\n", art);
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

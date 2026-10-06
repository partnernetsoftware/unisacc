/* seed/pack.c -- exec/c/pack.py without Python (0.0.32 B5).
 *
 *   pack -o OUT [--compressed] [--no-codec-cache] [--mount PREFIX_HEX DIR]... MANIFEST...
 *
 * Same bytes as pack.py: manifest rows route/stage/in/out/model, equal networks
 * stored once, Q action prefixes compacted (C lines), and with --compressed the
 * P3 form: UNINETB1 varint encoding, raw DEFLATE level 9 and CRC32 through zlib
 * (the same zlib Python links, so the blobs agree byte for byte).  Resources are
 * mounted files sorted by path components.  The codec cache is a Python speed-up
 * only and is not used here; --no-codec-cache is accepted for argument parity.
 * C99 plus POSIX directory reading and zlib.   build: cc -std=c99 seed/pack.c -lz */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <zlib.h>

static const int ARITY[] = {0,1,1,2,2,4,4,2,2,1,3,3,1,1,0,0,1,1,2,0,0,1,2,2,3,0,1,1,1,1,0,3,3,1,1,2,0,0,1,2,1,1,1,1,2,1,1,1,0,0,1,4,4,2,2,1};
#define NOPS ((int)(sizeof ARITY / sizeof ARITY[0]))

static void die(const char *m, const char *a) { fprintf(stderr, "pack: %s%s%s\n", m, a ? " " : "", a ? a : ""); exit(1); }
typedef struct { unsigned char *p; size_t n, cap; } Buf;
static void put(Buf *b, const void *s, size_t n) {
    if (b->n + n > b->cap) { b->cap = (b->n + n) * 2 + 64; b->p = realloc(b->p, b->cap); if (!b->p) die("out of memory", 0); }
    memcpy(b->p + b->n, s, n); b->n += n;
}
static void puts_(Buf *b, const char *s) { put(b, s, strlen(s)); }
static unsigned char *slurp(const char *path, size_t *n) {
    FILE *f = fopen(path, "rb"); if (!f) die("cannot read", path);
    Buf b = {0}; unsigned char tmp[65536]; size_t k;
    while ((k = fread(tmp, 1, sizeof tmp, f)) > 0) put(&b, tmp, k);
    fclose(f); put(&b, "", 1); b.n--; *n = b.n; return b.p;
}

/* ---- Q prefix compaction (compact_q) ---- */
typedef struct { int64_t *v; int n; } Act;          /* opcode + operands */
typedef struct { Act *a; int n; } Seq;
static int act_eq(Act x, Act y) { return x.n == y.n && !memcmp(x.v, y.v, x.n * sizeof *x.v); }
static uint64_t act_hash(Act x, int parent) {
    uint64_t h = 1469598103934665603ull ^ (uint64_t)parent;
    for (int i = 0; i < x.n; i++) { h ^= (uint64_t)x.v[i]; h *= 1099511628211ull; }
    return h;
}
typedef struct { int parent, first; Act act; } Node;  /* trie node: first sequence through it */
static Node *nodes; static int nnodes, ncap; static int *slot; static size_t nslot;
static int trie_find(int parent, Act a) {
    if (!nslot) return -1;
    size_t i = act_hash(a, parent) & (nslot - 1);
    for (; slot[i] >= 0; i = (i + 1) & (nslot - 1))
        if (nodes[slot[i]].parent == parent && act_eq(nodes[slot[i]].act, a)) return slot[i];
    return -1;
}
static int trie_add(int parent, Act a, int qi) {
    int f = trie_find(parent, a); if (f >= 0) return f;
    if (2 * (size_t)(nnodes + 1) > nslot) {          /* grow and rehash */
        free(slot); nslot = nslot ? nslot * 2 : 1024; slot = malloc(nslot * sizeof *slot);
        for (size_t i = 0; i < nslot; i++) slot[i] = -1;
        for (int k = 0; k < nnodes; k++) {
            size_t i = act_hash(nodes[k].act, nodes[k].parent) & (nslot - 1);
            while (slot[i] >= 0) i = (i + 1) & (nslot - 1);
            slot[i] = k;
        }
    }
    if (nnodes == ncap) { ncap = ncap ? ncap * 2 : 1024; nodes = realloc(nodes, ncap * sizeof *nodes); }
    nodes[nnodes] = (Node){parent, qi, a};
    size_t i = act_hash(a, parent) & (nslot - 1);
    while (slot[i] >= 0) i = (i + 1) & (nslot - 1);
    slot[i] = nnodes; return nnodes++;
}
static int split(const unsigned char *s, size_t n, const unsigned char **w, size_t *wl, int max) {
    int k = 0; size_t i = 0;
    while (i < n) {
        while (i < n && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r' || s[i] == '\v' || s[i] == '\f')) i++;
        if (i >= n) break;
        size_t j = i; while (j < n && !(s[j] == ' ' || s[j] == '\t' || s[j] == '\n' || s[j] == '\r' || s[j] == '\v' || s[j] == '\f')) j++;
        if (k == max) die("too many fields", 0);
        w[k] = s + i; wl[k] = j - i; k++; i = j;
    }
    return k;
}
static int64_t num(const unsigned char *s, size_t n) {
    char t[32]; if (n == 0 || n >= sizeof t) die("bad integer", 0);
    memcpy(t, s, n); t[n] = 0; char *e; long long v = strtoll(t, &e, 10);
    if (*e) die("bad integer", t); return v;
}
static void acts(int64_t *flat, int nflat, int count, Seq *out) {
    if (count < 0 || count > nflat) die("bad Q action count", 0);
    int at = 0;
    for (int c = 0; c < count; c++) {
        if (at >= nflat || flat[at] < 0 || flat[at] >= NOPS) die("bad Q opcode or truncated action", 0);
        int end = at + 1 + ARITY[flat[at]]; if (end > nflat) die("truncated Q operands", 0);
        out->a[out->n++] = (Act){flat + at, end - at}; at = end;
    }
    if (at != nflat) die("extra Q operands", 0);
}
typedef struct { const unsigned char *p; size_t n; } Line;   /* includes its '\n' */
static Line *lines_of(const unsigned char *d, size_t n, int *count) {
    int k = 0, cap = 1024; Line *L = malloc(cap * sizeof *L);
    for (size_t i = 0; i < n;) {
        size_t j = i; while (j < n && d[j] != '\n') j++;
        if (j < n) j++;
        if (k == cap) { cap *= 2; L = realloc(L, cap * sizeof *L); }
        L[k++] = (Line){d + i, j - i}; i = j;
    }
    *count = k; return L;
}
static void compact_q(const unsigned char *d, size_t n, Buf *out) {
    int nl; Line *L = lines_of(d, n, &nl);
    if (!nl || d[n - 1] != '\n') die("truncated network", 0);
    const unsigned char *w[16]; size_t wl[16];
    if (split(L[0].p, L[0].n - 1, w, wl, 16) != 7 || wl[0] != 1 || w[0][0] != 'N') die("bad network header", 0);
    int64_t nq = num(w[2], wl[2]), nstr = num(w[4], wl[4]); int64_t start = 1 + nstr;
    if (nq <= 0 || nstr < 0 || start + nq > nl) die("bad network Q extent", 0);
    nnodes = 0; free(slot); slot = 0; nslot = 0;
    Seq *seqs = calloc(nq, sizeof *seqs);
    const unsigned char **ww = 0; size_t *wwl = 0; int wcap = 0;
    for (int64_t qi = 0; qi < nq; qi++) {
        Line ln = L[start + qi];
        int maxw = (int)ln.n / 2 + 2;
        if (maxw > wcap) { wcap = maxw; ww = realloc(ww, wcap * sizeof *ww); wwl = realloc(wwl, wcap * sizeof *wwl); }
        int k = split(ln.p, ln.n, ww, wwl, wcap);
        if (k < 2) die("truncated Q record", 0);
        int64_t *flat = malloc((k + 1) * sizeof *flat); Seq s = {0};
        if (wwl[0] == 1 && ww[0][0] == 'Q') {
            for (int i = 2; i < k; i++) flat[i - 2] = num(ww[i], wwl[i]);
            s.a = malloc((k + 1) * sizeof *s.a); acts(flat, k - 2, (int)num(ww[1], wwl[1]), &s);
        } else if (wwl[0] == 1 && ww[0][0] == 'C') {
            if (k < 4) die("truncated Q record", 0);
            int64_t ref = num(ww[1], wwl[1]), pre = num(ww[2], wwl[2]);
            if (ref < 0 || ref >= qi || pre <= 0 || pre > seqs[ref].n) die("bad Q prefix reference", 0);
            for (int i = 4; i < k; i++) flat[i - 4] = num(ww[i], wwl[i]);
            s.a = malloc((pre + k + 1) * sizeof *s.a);
            for (int i = 0; i < pre; i++) s.a[s.n++] = seqs[ref].a[i];
            acts(flat, k - 4, (int)num(ww[3], wwl[3]), &s);
        } else die("unknown Q tag", 0);
        seqs[qi] = s;
    }
    for (int64_t i = 0; i < start; i++) put(out, L[i].p, L[i].n);
    Buf cand = {0};
    for (int64_t qi = 0; qi < nq; qi++) {
        Seq s = seqs[qi]; int node = -1, ref = -1, pre = 0;
        for (int ai = 0; ai < s.n; ai++) {
            int f = trie_find(node, s.a[ai]); if (f < 0) break;
            node = f; ref = nodes[f].first; pre = ai + 1;
        }
        Line wire = L[start + qi];
        if (pre) {
            char t[64]; cand.n = 0;
            snprintf(t, sizeof t, "C %d %d %d", ref, pre, s.n - pre); puts_(&cand, t);
            for (int ai = pre; ai < s.n; ai++)
                for (int j = 0; j < s.a[ai].n; j++) { snprintf(t, sizeof t, " %lld", (long long)s.a[ai].v[j]); puts_(&cand, t); }
            put(&cand, "\n", 1);
            if (cand.n < wire.n) wire = (Line){cand.p, cand.n};
        }
        put(out, wire.p, wire.n);
        node = -1; for (int ai = 0; ai < s.n; ai++) node = trie_add(node, s.a[ai], (int)qi);
    }
    for (int64_t i = start + nq; i < nl; i++) put(out, L[i].p, L[i].n);
    free(cand.p); free(L); free(ww); free(wwl);
}

/* ---- UNINETB1 (networkformat.encode) ---- */
static void varint(Buf *b, int64_t x) {
    uint64_t v = ((uint64_t)x << 1) ^ (uint64_t)(x >> 63); unsigned char c;
    while (v >= 128) { c = (unsigned char)((v & 127) | 128); put(b, &c, 1); v >>= 7; }
    c = (unsigned char)v; put(b, &c, 1);
}
static int hexv(int c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1; }
static void binary_encode(const unsigned char *d, size_t n, Buf *out) {
    put(out, "UNINETB1", 8);
    int nl; Line *L = lines_of(d, n, &nl);
    const unsigned char **w = 0; size_t *wl = 0; int wcap = 0;
    for (int i = 0; i < nl; i++) {
        int maxw = (int)L[i].n / 2 + 2;
        if (maxw > wcap) { wcap = maxw; w = realloc(w, wcap * sizeof *w); wl = realloc(wl, wcap * sizeof *wl); }
        int k = split(L[i].p, L[i].n - (L[i].p[L[i].n - 1] == '\n'), w, wl, wcap);
        if (!k) die("invalid binary network", 0);
        put(out, w[0], wl[0]);
        if (wl[0] == 1 && w[0][0] == 'S') {
            if (k != 2) die("invalid binary network", 0);
            if (wl[1] == 1 && w[1][0] == '-') { varint(out, 0); continue; }
            if (wl[1] % 2) die("invalid binary network", 0);
            varint(out, (int64_t)(wl[1] / 2));
            for (size_t j = 0; j < wl[1]; j += 2) {
                int a = hexv(w[1][j]), b = hexv(w[1][j + 1]); if (a < 0 || b < 0) die("invalid binary network", 0);
                unsigned char c = (unsigned char)(a * 16 + b); put(out, &c, 1);
            }
        } else for (int j = 1; j < k; j++) varint(out, num(w[j], wl[j]));
    }
    free(L); free(w); free(wl);
}

/* ---- resources ---- */
typedef struct { char *rel; char *full; } File;
static File *files; static int nfiles, fcap;
static void walk(const char *dir, const char *rel) {
    DIR *d = opendir(dir); if (!d) die("cannot read directory", dir);
    struct dirent *e;
    while ((e = readdir(d))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        size_t a = strlen(dir) + strlen(e->d_name) + 2, b = strlen(rel) + strlen(e->d_name) + 2;
        char *full = malloc(a), *r = malloc(b);
        snprintf(full, a, "%s/%s", dir, e->d_name); snprintf(r, b, "%s%s%s", rel, *rel ? "/" : "", e->d_name);
        struct stat ls, st;
        if (lstat(full, &ls)) die("cannot stat", full);
        if (S_ISDIR(ls.st_mode)) { walk(full, r); free(full); free(r); continue; }  /* rglob: no symlinked dirs */
        if (stat(full, &st) == 0 && S_ISREG(st.st_mode)) {
            if (nfiles == fcap) { fcap = fcap ? fcap * 2 : 256; files = realloc(files, fcap * sizeof *files); }
            files[nfiles++] = (File){r, full};
        } else { free(full); free(r); }
    }
    closedir(d);
}
static int partcmp(const void *x, const void *y) {   /* PurePath order: component lists */
    const unsigned char *a = (const unsigned char *)((const File *)x)->rel, *b = (const unsigned char *)((const File *)y)->rel;
    for (;;) {
        int ea = !*a || *a == '/', eb = !*b || *b == '/';
        if (ea && eb) { if (!*a && !*b) return 0; if (!*a) return -1; if (!*b) return 1; a++; b++; continue; }
        if (ea) return -1; if (eb) return 1;
        if (*a != *b) return *a < *b ? -1 : 1;
        a++; b++;
    }
}
typedef struct { unsigned char *k; size_t kn; unsigned char *v; size_t vn; } Res;
static Res *res; static int nres, rcap;

static int valid_name(const char *s) {
    if (!*s) return 0;
    for (; *s; s++) if (!((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z') || (*s >= '0' && *s <= '9') || strchr("_./-", *s))) return 0;
    return 1;
}

int main(int argc, char **argv) {
    const char *outp = 0; int compressed = 0; const char *mp[64], *md[64]; int nm = 0; const char *man[256]; int nman = 0;
    for (int i = 1; i < argc; i++) {
        if ((!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output")) && i + 1 < argc) outp = argv[++i];
        else if (!strcmp(argv[i], "--compressed")) compressed = 1;
        else if (!strcmp(argv[i], "--no-codec-cache")) ;
        else if (!strcmp(argv[i], "--mount") && i + 2 < argc && nm < 64) { mp[nm] = argv[i + 1]; md[nm++] = argv[i + 2]; i += 2; }
        else if (argv[i][0] == '-' || nman == 256) { fprintf(stderr, "usage: pack -o OUT [--compressed] [--mount PREFIX_HEX DIR]... MANIFEST...\n"); return 2; }
        else man[nman++] = argv[i];
    }
    if (!outp || !nman) { fprintf(stderr, "usage: pack -o OUT [--compressed] [--mount PREFIX_HEX DIR]... MANIFEST...\n"); return 2; }
    Buf head = {0}; char t[256];
    typedef struct { char *route, *stage, *in, *out; int model; } Stage;
    Stage *st = 0; int nst = 0, stcap = 0;
    unsigned char **mod = 0; size_t *modn = 0; int nmod = 0;
    for (int m = 0; m < nman; m++) {
        size_t n; char *text = (char *)slurp(man[m], &n);
        char *dirend = strrchr(man[m], '/'); size_t dl = dirend ? (size_t)(dirend - man[m]) : 0;
        int lineno = 0;
        for (char *s = text; s < text + n;) {
            char *e = memchr(s, '\n', text + n - s); if (!e) e = text + n;
            *e = 0; lineno++; char *ln = s; s = e + 1;
            size_t ll = strlen(ln); if (ll && ln[ll - 1] == '\r') ln[--ll] = 0;
            if (!*ln || *ln == '#') continue;
            char *c[6]; int k = 0; c[k++] = ln;
            for (char *p = ln; *p; p++) if (*p == '\t') { *p = 0; if (k == 6) break; c[k++] = p + 1; }
            snprintf(t, sizeof t, "%s:%d:", man[m], lineno);
            if (k != 5 || !valid_name(c[0]) || !valid_name(c[1]) || !valid_name(c[2]) || !valid_name(c[3])) die(t, "expected five columns and valid names");
            const char *last = 0;
            for (int i = 0; i < nst; i++) {
                if (!strcmp(st[i].route, c[0]) && !strcmp(st[i].stage, c[1])) die(t, "duplicate stage");
                if (!strcmp(st[i].route, c[0])) last = st[i].out;
            }
            if (last && strcmp(last, c[2])) die(t, "format mismatch");
            size_t pl = dl + strlen(c[4]) + 2; char *path = malloc(pl);
            if (dirend && c[4][0] != '/') snprintf(path, pl, "%.*s/%s", (int)dl, man[m], c[4]); else snprintf(path, pl, "%s", c[4]);
            size_t dn; unsigned char *data = slurp(path, &dn);
            if (dn < 2 || memcmp(data, "N ", 2) || data[dn - 1] != '\n') die(t, "expected canonical network text");
            int idx = -1;
            for (int i = 0; i < nmod; i++) if (modn[i] == dn && !memcmp(mod[i], data, dn)) { idx = i; break; }
            if (idx < 0) { mod = realloc(mod, (nmod + 1) * sizeof *mod); modn = realloc(modn, (nmod + 1) * sizeof *modn);
                           mod[nmod] = data; modn[nmod] = dn; idx = nmod++; } else free(data);
            if (nst == stcap) { stcap = stcap ? stcap * 2 : 64; st = realloc(st, stcap * sizeof *st); }
            st[nst++] = (Stage){c[0], c[1], c[2], c[3], idx};
        }
    }
    if (!nst) die("empty package", 0);
    for (int m = 0; m < nm; m++) {
        size_t hl = strlen(mp[m]); if (hl % 2) die("bad mount prefix", mp[m]);
        unsigned char kp[256]; if (hl / 2 > sizeof kp) die("bad mount prefix", mp[m]);
        for (size_t j = 0; j < hl; j += 2) {
            int a = hexv(mp[m][j]), b = hexv(mp[m][j + 1]); if (a < 0 || b < 0) die("bad mount prefix", mp[m]);
            kp[j / 2] = (unsigned char)(a * 16 + b);
        }
        struct stat sb; if (stat(md[m], &sb) || !S_ISDIR(sb.st_mode)) die("resource mount is not a directory:", md[m]);
        nfiles = 0; walk(md[m], "");
        if (!nfiles) die("empty resource mount:", md[m]);
        qsort(files, nfiles, sizeof *files, partcmp);
        for (int f = 0; f < nfiles; f++) {
            size_t rl = strlen(files[f].rel), kn = hl / 2 + rl; unsigned char *key = malloc(kn + 1);
            memcpy(key, kp, hl / 2); memcpy(key + hl / 2, files[f].rel, rl);
            size_t vn; unsigned char *v = slurp(files[f].full, &vn); int dup = 0;
            for (int r = 0; r < nres; r++) if (res[r].kn == kn && !memcmp(res[r].k, key, kn)) {
                if (res[r].vn != vn || memcmp(res[r].v, v, vn)) die("conflicting resource", files[f].rel);
                dup = 1; break;
            }
            if (dup) { free(key); free(v); continue; }
            if (nres == rcap) { rcap = rcap ? rcap * 2 : 256; res = realloc(res, rcap * sizeof *res); }
            res[nres++] = (Res){key, kn, v, vn};
        }
    }
    if (compressed) snprintf(t, sizeof t, "P 3 %d %d %d\n", nmod, nst, nres);
    else if (nres) snprintf(t, sizeof t, "P 2 %d %d %d\n", nmod, nst, nres);
    else snprintf(t, sizeof t, "P 1 %d %d\n", nmod, nst);
    puts_(&head, t);
    for (int i = 0; i < nst; i++) {
        puts_(&head, "D "); puts_(&head, st[i].route); puts_(&head, " "); puts_(&head, st[i].stage); puts_(&head, " ");
        puts_(&head, st[i].in); puts_(&head, " "); puts_(&head, st[i].out); snprintf(t, sizeof t, " %d\n", st[i].model); puts_(&head, t);
    }
    for (int i = 0; i < nmod; i++) {
        Buf q = {0}; compact_q(mod[i], modn[i], &q);
        if (!compressed) { snprintf(t, sizeof t, "M %zu\n", q.n); puts_(&head, t); put(&head, q.p, q.n); free(q.p); continue; }
        Buf raw = {0}; binary_encode(q.p, q.n, &raw); free(q.p);
        unsigned long crc = crc32(0L, raw.p, (uInt)raw.n);
        z_stream z; memset(&z, 0, sizeof z);
        if (deflateInit2(&z, 9, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) != Z_OK) die("deflate init failed", 0);
        uLong cap = deflateBound(&z, raw.n) + 64; unsigned char *blob = malloc(cap);
        z.next_in = raw.p; z.avail_in = (uInt)raw.n; z.next_out = blob; z.avail_out = (uInt)cap;
        if (deflate(&z, Z_FINISH) != Z_STREAM_END) die("deflate failed", 0);
        size_t bn = cap - z.avail_out; deflateEnd(&z);
        snprintf(t, sizeof t, "M %zu %zu 1 %lu\n", bn, raw.n, crc); puts_(&head, t); put(&head, blob, bn);
        free(blob); free(raw.p);
    }
    for (int r = 0; r < nres; r++) {
        snprintf(t, sizeof t, "F %zu %zu\n", res[r].kn, res[r].vn); puts_(&head, t);
        put(&head, res[r].k, res[r].kn); put(&head, res[r].v, res[r].vn);
    }
    if (head.n >= (size_t)1 << 31) die("package exceeds runtime byte extent", 0);
    FILE *f = fopen(outp, "wb"); if (!f || fwrite(head.p, 1, head.n, f) != head.n || fclose(f)) die("cannot write", outp);
    printf("package: %zu bytes -> %s\n", head.n, outp);
    return 0;
}

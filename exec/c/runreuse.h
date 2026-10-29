/* Cross-process tape reuse for the 0.0.43 development driver.
 * USE33: E3 tape + ATTRS + present/absent receipts (key = prod+src+route+target+level).
 * USE34: post-host_compile tape + ATTRS; receipts cloned from matching USE33 entry; file `<key>.host`.
 * Product and source bytes are bound to each lookup. UNISACC_RUN_CACHE=0 disables.
 */
#ifndef UNISACC_RUNREUSE_H
#define UNISACC_RUNREUSE_H

#include "../../include/unisa_sha256.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#if defined(__linux__) && defined(__x86_64__)
#include <sys/socket.h>
#endif

enum { RR_VER = 11 }; /* early host for -run/-o; final artifact for -c/-S */

/* User sources and include receipts may be symlinks (e.g. tests/cxlib/*.cx -> tools/).
 * Follow for content identity; keep lstat for cache/product files themselves. */
static int rr_stat_reg(const char *path, struct stat *st) {
    if (!path || !st || stat(path, st) || !S_ISREG(st->st_mode) || st->st_size < 0 ||
        st->st_size > (1 << 26))
        return 1;
    return 0;
}

/* A cached failed -I probe is still absent only for ENOENT.  ENOTDIR and
 * EACCES make a fresh compile fail instead of continuing to the next path. */
static int rr_still_absent(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) return 0;
    return errno == ENOENT;
}

static int rr_off(void) {
    const char *e = getenv("UNISACC_RUN_CACHE");
    return e && !strcmp(e, "0");
}

static int rr_special_input;
static const char *rr_pkg_path;
static int rr_eligible(int nsources, int warnings, int tape_input, int deps,
                       int fwd_second, const char *target) {
    if (rr_off() || fwd_second || rr_special_input) return 0;
    /* Module `use` expands to multi-TU; framed input hex still binds identity. */
    if (nsources < 1 || nsources > 16 || warnings || tape_input || deps) return 0;
    if (RF_STRICT || rf_capture_dir()) return 0;
#if !(defined(__linux__) && defined(__x86_64__))
    (void)target;
    return 0;
#else
    return target && !strcmp(target, "lnx/x86_64");
#endif
}

static int rr_mkdir(const char *dir) {
    struct stat st;
    if (mkdir(dir, 0700) == 0) return 0;
    if (errno != EEXIST) return 1;
    if (lstat(dir, &st) || !S_ISDIR(st.st_mode) || (st.st_mode & 0777) != 0700) return 1;
    return 0;
}

static int rr_dir(char *out, size_t n) {
#ifdef _WIN32
    (void)out; (void)n;
    return 1; /* reuse is disabled on this target */
#else
    const char *e = getenv("UNISACC_E3CACHE");
    int k;
    if (e && e[0]) {
        if (strlen(e) + 1 > n) return 1;
        memcpy(out, e, strlen(e) + 1);
        return rr_mkdir(out);
    }
    k = snprintf(out, n, "%s/unisacc-e3cache-%d",
                 getenv("TMPDIR") && getenv("TMPDIR")[0] ? getenv("TMPDIR") : "/tmp",
                 (int)getuid());
    if (k < 0 || (size_t)k >= n) return 1;
    return rr_mkdir(out);
#endif
}

#if defined(__linux__) && defined(__x86_64__)
static int rr_afalg_hex(const unsigned char *data, size_t n, char hex[65]) {
    struct { unsigned short family; unsigned char type[14]; unsigned feat, mask; unsigned char name[64]; } addr;
    int sfd, bfd, i;
    unsigned char md[32];
    ssize_t sent, got;
    memset(&addr, 0, sizeof addr);
    addr.family = 38; /* Linux AF_ALG */
    memcpy(addr.type, "hash", 5);
    memcpy(addr.name, "sha256", 7);
    sfd = socket(38, 5, 0); /* SOCK_SEQPACKET */
    if (sfd < 0) return 1;
    if (bind(sfd, (const struct sockaddr *)&addr, sizeof addr)) { close(sfd); return 1; }
    bfd = accept(sfd, 0, 0);
    if (bfd < 0) { close(sfd); return 1; }
    sent = write(bfd, data, n);
    if (sent != (ssize_t)n) { close(bfd); close(sfd); return 1; }
    got = read(bfd, md, 32);
    close(bfd); close(sfd);
    if (got != 32) return 1;
    for (i = 0; i < 32; i++) {
        hex[2*i] = "0123456789abcdef"[md[i] >> 4];
        hex[2*i+1] = "0123456789abcdef"[md[i] & 15];
    }
    hex[64] = 0;
    return 0;
}
#endif

static int rr_hash_file(const char *path, char hex[65]) {
    FILE *f;
    struct stat st;
    unsigned char *all;
    size_t off = 0, n;
    if (rr_stat_reg(path, &st)) return 1;
    f = fopen(path, bx_file_mode(0));
    if (!f) return 1;
    all = (unsigned char *)malloc((size_t)st.st_size ? (size_t)st.st_size : 1);
    if (!all) { fclose(f); return 1; }
    while (off < (size_t)st.st_size) {
        n = fread(all + off, 1, (size_t)st.st_size - off, f);
        if (!n) { free(all); fclose(f); return 1; }
        off += n;
    }
    fclose(f);
#if defined(__linux__) && defined(__x86_64__)
    if (off < 65536 || rr_afalg_hex(all, off, hex))
#endif
        unisa_sha256_hex(all, off, hex);
    free(all);
    return 0;
}

static int rr_write_atomic(const char *final_path, const unsigned char *data, size_t n);

/* Prefer APE container path (UNISA_CONTAINER) over extracted argv0. */
static const char *rr_product_path(const char *argv0) {
    const char *c = getenv("UNISA_CONTAINER");
    if (c && c[0]) return c;
    return argv0;
}

/* A cache file is not a product identity. Hash the actual APE or ELF bytes on
 * each process; memoization below only avoids repeated hashes in one process. */
static int rr_product_hex(const char *product, char hex[65]) {
    const char *path = rr_product_path(product);
    static char held_path[4096], held_hex[65];
    if (!path) return 1;
    if (held_hex[0] && !strcmp(path, held_path)) { memcpy(hex, held_hex, 65); return 0; }
    if (strlen(path) >= sizeof held_path || rr_hash_file(path, hex)) return 1;
    strcpy(held_path, path); memcpy(held_hex, hex, 65);
    return 0;
}

static int rr_key(char key[65], const char *product, const char *route, const char *target,
                  int level, const char src_hex[65]) {
    char prod[65], pkg[65], blob[640];
    int n;
    if (!product || !route || !target || !src_hex) return 1;
    if (rr_product_hex(product, prod)) return 1;
    if (rr_pkg_path && strcmp(rr_pkg_path, rr_product_path(product))) {
        static char held_pkg_path[4096], held_pkg_hex[65];
        if (held_pkg_hex[0] && !strcmp(rr_pkg_path, held_pkg_path)) memcpy(pkg, held_pkg_hex, 65);
        else {
            if (strlen(rr_pkg_path) >= sizeof held_pkg_path || rr_hash_file(rr_pkg_path, pkg)) return 1;
            strcpy(held_pkg_path, rr_pkg_path); memcpy(held_pkg_hex, pkg, 65);
        }
    } else memcpy(pkg, prod, 65);
    n = snprintf(blob, sizeof blob, "USE33\nver=%d\nprod=%s\npkg=%s\nroute=%s\ntarget=%s\nlevel=%d\nsrc=%s\n",
                 RR_VER, prod, pkg, route, target, level, src_hex);
    if (n < 0 || (size_t)n >= sizeof blob) return 1;
    unisa_sha256_hex(blob, (size_t)n, key);
    return 0;
}

static int rr_cli_argc;
static char **rr_cli_argv;
/* Bind options and the working directory before raw source bytes. This is
 * needed for -D, -include, -iquote, -I, and relative resource paths. */
/* Identity for one or many TUs: CLI and repeated (path \\0 || file bytes). */
static int rr_sources_hex(char **sources, int nsources, char hex[65]) {
    unsigned char *acc = 0;
    size_t at = 0, cap = 0;
    int j;
    char cwd[4096];
    if (!sources || nsources < 1 || nsources > 16 || !hex) return 1;
    if (!getcwd(cwd, sizeof cwd)) return 1;
    cap = 65536;
    acc = (unsigned char *)malloc(cap);
    if (!acc) return 1;
    {
        size_t n = strlen(cwd) + 1;
        memcpy(acc + at, cwd, n); at += n;
        for (j = 1; j < rr_cli_argc; j++) {
            if (!strcmp(rr_cli_argv[j], "--")) break;
            n = strlen(rr_cli_argv[j]) + 1;
            if (at + n > cap) { free(acc); return 1; }
            memcpy(acc + at, rr_cli_argv[j], n); at += n;
        }
    }
    for (j = 0; j < nsources; j++) {
        struct stat st;
        FILE *f;
        size_t need, n, pl;
        unsigned char *chunk;
        if (!sources[j] || rr_stat_reg(sources[j], &st)) {
            free(acc);
            return 1;
        }
        pl = strlen(sources[j]);
        need = at + pl + 1 + (size_t)st.st_size;
        if (need > cap) {
            size_t nc = need + 4096;
            unsigned char *nacc = (unsigned char *)realloc(acc, nc);
            if (!nacc) { free(acc); return 1; }
            acc = nacc;
            cap = nc;
        }
        memcpy(acc + at, sources[j], pl);
        at += pl;
        acc[at++] = 0;
        f = fopen(sources[j], bx_file_mode(0));
        if (!f) { free(acc); return 1; }
        chunk = acc + at;
        n = 0;
        while (n < (size_t)st.st_size) {
            size_t r = fread(chunk + n, 1, (size_t)st.st_size - n, f);
            if (!r) { fclose(f); free(acc); return 1; }
            n += r;
        }
        fclose(f);
        at += n;
    }
    unisa_sha256_hex(acc, at, hex);
    free(acc);
    return 0;
}

static int rr_write_atomic(const char *final_path, const unsigned char *data, size_t n) {
    char tmp[600];
    int fd, k;
    size_t off = 0;
    k = snprintf(tmp, sizeof tmp, "%s.%d.tmp", final_path, (int)getpid());
    if (k < 0 || (size_t)k >= sizeof tmp) return 1;
#ifdef _WIN32
    fd = open(tmp, O_WRONLY | O_CREAT | O_EXCL, 0600);
#else
    fd = open(tmp, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
#endif
    if (fd < 0) return 1;
    while (off < n) {
        ssize_t w = write(fd, data + off, n - off);
        if (w < 0 && errno == EINTR) continue;
        if (w <= 0) { close(fd); unlink(tmp); return 1; }
        off += (size_t)w;
    }
    if (fsync(fd) || close(fd)) { unlink(tmp); return 1; }
    if (rename(tmp, final_path)) { unlink(tmp); return 1; }
    return 0;
}

static int rr_store(const char *product, const char *route, const char *target, int level,
                    const char src_hex[65], const Buf *tape, const Buf *attrs,
                    char **paths, int pathn, char **absent, int absentn) {
    char dir[400], key[65], path[500], hex[65], *phash;
    unsigned char *payload;
    size_t need, at = 0, i;
    unsigned tn, an, hn, abn;
    if (!tape || !tape->b || tape->n < 0 || !attrs || !src_hex) return 1;
    if (pathn < 0 || (pathn > 0 && !paths)) return 1;
    if (absentn < 0 || (absentn > 0 && !absent)) return 1;
    if (rr_dir(dir, sizeof dir)) return 1;
    if (rr_key(key, product, route, target, level, src_hex)) return 1;
    tn = (unsigned)tape->n;
    an = (unsigned)(attrs->n < 0 ? 0 : attrs->n);
    hn = (unsigned)pathn;
    abn = (unsigned)absentn;
    need = 8 + 4 + 4 + 4 + 4 + 64 + (size_t)tn + (size_t)an;
    phash = (char *)malloc(hn ? (size_t)hn * 65 : 1);
    if (!phash) return 1;
    {
        long long *psz = (long long *)malloc(hn ? (size_t)hn * 16 : 1);
        if (!psz) { free(phash); return 1; }
        for (i = 0; i < hn; i++) {
            size_t pl = strlen(paths[i]);
            struct stat st;
            if (pl > 65535 || rr_stat_reg(paths[i], &st) ||
                rr_hash_file(paths[i], phash + i * 65)) {
                free(psz); free(phash); return 1;
            }
            psz[i * 2] = (long long)st.st_size;
            psz[i * 2 + 1] = (long long)st.st_mtime;
            need += 2 + pl + 8 + 8 + 64;
        }
        for (i = 0; i < abn; i++) {
            size_t pl = strlen(absent[i]);
            if (pl > 65535) { free(psz); free(phash); return 1; }
            need += 2 + pl;
        }
        payload = (unsigned char *)malloc(need ? need : 1);
        if (!payload) { free(psz); free(phash); return 1; }
        memcpy(payload, "USE33\n\0\0", 8); at = 8;
        payload[at++] = tn & 255; payload[at++] = (tn >> 8) & 255;
        payload[at++] = (tn >> 16) & 255; payload[at++] = (tn >> 24) & 255;
        payload[at++] = an & 255; payload[at++] = (an >> 8) & 255;
        payload[at++] = (an >> 16) & 255; payload[at++] = (an >> 24) & 255;
        payload[at++] = hn & 255; payload[at++] = (hn >> 8) & 255;
        payload[at++] = (hn >> 16) & 255; payload[at++] = (hn >> 24) & 255;
        payload[at++] = abn & 255; payload[at++] = (abn >> 8) & 255;
        payload[at++] = (abn >> 16) & 255; payload[at++] = (abn >> 24) & 255;
        unisa_sha256_hex(tape->b, (size_t)tn, hex);
        memcpy(payload + at, hex, 64); at += 64;
        for (i = 0; i < hn; i++) {
            size_t pl = strlen(paths[i]);
            unsigned long long sz = (unsigned long long)psz[i * 2];
            unsigned long long mt = (unsigned long long)psz[i * 2 + 1];
            int b;
            payload[at++] = pl & 255; payload[at++] = (pl >> 8) & 255;
            memcpy(payload + at, paths[i], pl); at += pl;
            for (b = 0; b < 8; b++) payload[at++] = (unsigned char)((sz >> (8 * b)) & 255);
            for (b = 0; b < 8; b++) payload[at++] = (unsigned char)((mt >> (8 * b)) & 255);
            memcpy(payload + at, phash + i * 65, 64); at += 64;
        }
        free(psz);
        free(phash);
    }
    for (i = 0; i < abn; i++) {
        size_t pl = strlen(absent[i]);
        payload[at++] = pl & 255; payload[at++] = (pl >> 8) & 255;
        memcpy(payload + at, absent[i], pl); at += pl;
    }
    if (tn) memcpy(payload + at, tape->b, tn); at += tn;
    if (an && attrs->b) memcpy(payload + at, attrs->b, an); at += an;
    snprintf(path, sizeof path, "%s/%s", dir, key);
    if (rr_write_atomic(path, payload, at)) { free(payload); return 1; }
    free(payload);
    return 0;
}

static int rr_lookup(const char *product, const char *route, const char *target, int level,
                     const char src_hex[65], Buf *tape) {
    char dir[400], key[65], cpath[500], hex[65], tape_got[65], hdr_got[65], cur[65];
    FILE *f;
    unsigned char fixed[8 + 4 + 4 + 4 + 4 + 64];
    unsigned tn, an, hn, abn, i;
    size_t n, body_need;
    unsigned char *body;
    struct stat st;
    if (!tape || !src_hex) return -1;
    tape->b = 0; tape->n = 0; tape->cap = 0;
    if (rr_dir(dir, sizeof dir)) return 0;
    if (rr_key(key, product, route, target, level, src_hex)) return 0;
    snprintf(cpath, sizeof cpath, "%s/%s", dir, key);
    if (lstat(cpath, &st) || !S_ISREG(st.st_mode) || st.st_size < (off_t)sizeof fixed) return 0;
    f = fopen(cpath, bx_file_mode(0));
    if (!f) return 0;
    if (fread(fixed, 1, sizeof fixed, f) != sizeof fixed) { fclose(f); return 0; }
    if (memcmp(fixed, "USE33\n\0\0", 8)) { fclose(f); return 0; }
    tn = (unsigned)fixed[8] | (unsigned)fixed[9] << 8 | (unsigned)fixed[10] << 16 | (unsigned)fixed[11] << 24;
    an = (unsigned)fixed[12] | (unsigned)fixed[13] << 8 | (unsigned)fixed[14] << 16 | (unsigned)fixed[15] << 24;
    hn = (unsigned)fixed[16] | (unsigned)fixed[17] << 8 | (unsigned)fixed[18] << 16 | (unsigned)fixed[19] << 24;
    abn = (unsigned)fixed[20] | (unsigned)fixed[21] << 8 | (unsigned)fixed[22] << 16 | (unsigned)fixed[23] << 24;
    if (tn > (1u << 26) || an > (1u << 26) || hn > 65536 || abn > 65536) { fclose(f); return 0; }
    memcpy(tape_got, fixed + 24, 64); tape_got[64] = 0;
    for (i = 0; i < hn; i++) {
        unsigned char plb[2], meta[16];
        unsigned pl;
        char *pp;
        struct stat hst;
        unsigned long long sz = 0, mt = 0;
        int b;
        if (fread(plb, 1, 2, f) != 2) { fclose(f); return 0; }
        pl = (unsigned)plb[0] | (unsigned)plb[1] << 8;
        if (!pl || pl > 4096) { fclose(f); return 0; }
        pp = (char *)malloc(pl + 1);
        if (!pp) { fclose(f); return -1; }
        if (fread(pp, 1, pl, f) != pl || fread(meta, 1, 16, f) != 16 ||
            fread(hdr_got, 1, 64, f) != 64) { free(pp); fclose(f); return 0; }
        pp[pl] = 0; hdr_got[64] = 0;
        for (b = 0; b < 8; b++) sz |= (unsigned long long)meta[b] << (8 * b);
        for (b = 0; b < 8; b++) mt |= (unsigned long long)meta[8 + b] << (8 * b);
        if (rr_stat_reg(pp, &hst) || rr_hash_file(pp, cur) ||
            memcmp(cur, hdr_got, 64)) { free(pp); fclose(f); return 0; }
        free(pp);
    }
    for (i = 0; i < abn; i++) {
        unsigned char plb[2];
        unsigned pl;
        char *pp;
        if (fread(plb, 1, 2, f) != 2) { fclose(f); return 0; }
        pl = (unsigned)plb[0] | (unsigned)plb[1] << 8;
        if (!pl || pl > 4096) { fclose(f); return 0; }
        pp = (char *)malloc(pl + 1);
        if (!pp) { fclose(f); return -1; }
        if (fread(pp, 1, pl, f) != pl) { free(pp); fclose(f); return 0; }
        pp[pl] = 0;
        /* Still absent? If the file now exists (follow links), invalidate. */
        if (!rr_still_absent(pp)) { free(pp); fclose(f); return 0; }
        free(pp);
    }
    body_need = (size_t)tn + (size_t)an;
    {
        long pos = ftell(f);
        struct stat cst;
        if (pos < 0 || fstat(fileno(f), &cst) || cst.st_size < (off_t)pos ||
            cst.st_size - (off_t)pos != (off_t)body_need ||
            cst.st_size > (off_t)(1 << 26)) { fclose(f); return 0; }
    }
    body = (unsigned char *)malloc(body_need ? body_need : 1);
    if (!body) { fclose(f); return -1; }
    n = fread(body, 1, body_need, f);
    fclose(f);
    if (n != body_need) { free(body); return 0; }
    unisa_sha256_hex(body, (size_t)tn, hex);
    if (memcmp(hex, tape_got, 64)) { free(body); return 0; }
    tape->b = (unsigned char *)malloc(tn ? tn : 1);
    if (!tape->b) { free(body); return -1; }
    if (tn) memcpy(tape->b, body, tn);
    tape->n = (int)tn; tape->cap = (int)tn;
    free(ATTRS.b); ATTRS.b = 0; ATTRS.n = 0; ATTRS.cap = 0;
    if (an) {
        ATTRS.b = (unsigned char *)malloc(an);
        if (!ATTRS.b) { free(tape->b); tape->b = 0; tape->n = 0; free(body); return -1; }
        memcpy(ATTRS.b, body + tn, an);
        ATTRS.n = (int)an; ATTRS.cap = (int)an;
        if (ATTR_SLOT >= 0) { RI[ATTR_SLOT].data = ATTRS.b; RI[ATTR_SLOT].len = ATTRS.n; }
    }
    free(body);
    return 1;
}

/* Post-host tape: same receipts as USE33 sibling; skip host_compile on hit. */
static int rr_store_host(const char *product, const char *route, const char *target, int level,
                         const char src_hex[65], const Buf *tape, const Buf *attrs) {
    char dir[400], key[65], e3path[500], hostpath[520], hex[65];
    FILE *f;
    unsigned char fixed[8 + 4 + 4 + 4 + 4 + 64];
    unsigned tn, an, hn, abn, e3_tn, e3_an;
    size_t receipt_n = 0, i, at = 0, need;
    unsigned char *receipts = 0, *payload;
    struct stat st;
    if (!tape || !tape->b || tape->n < 0 || !attrs || !src_hex) return 1;
    if (rr_dir(dir, sizeof dir)) return 1;
    if (rr_key(key, product, route, target, level, src_hex)) return 1;
    snprintf(e3path, sizeof e3path, "%s/%s", dir, key);
    snprintf(hostpath, sizeof hostpath, "%s/%s.host", dir, key);
    if (lstat(e3path, &st) || !S_ISREG(st.st_mode)) return 1;
    f = fopen(e3path, bx_file_mode(0));
    if (!f) return 1;
    if (fread(fixed, 1, sizeof fixed, f) != sizeof fixed || memcmp(fixed, "USE33\n\0\0", 8)) {
        fclose(f); return 1;
    }
    e3_tn = (unsigned)fixed[8] | (unsigned)fixed[9] << 8 | (unsigned)fixed[10] << 16 | (unsigned)fixed[11] << 24;
    e3_an = (unsigned)fixed[12] | (unsigned)fixed[13] << 8 | (unsigned)fixed[14] << 16 | (unsigned)fixed[15] << 24;
    hn = (unsigned)fixed[16] | (unsigned)fixed[17] << 8 | (unsigned)fixed[18] << 16 | (unsigned)fixed[19] << 24;
    abn = (unsigned)fixed[20] | (unsigned)fixed[21] << 8 | (unsigned)fixed[22] << 16 | (unsigned)fixed[23] << 24;
    /* fixed already consumed tape sha; copy receipt records only. */
    if (e3_tn > (1u << 26) || e3_an > (1u << 26) || hn > 65536 || abn > 65536) { fclose(f); return 1; }
    {
        long start, end, body;
        start = ftell(f);
        for (i = 0; i < hn; i++) {
            unsigned char plb[2];
            unsigned pl;
            if (fread(plb, 1, 2, f) != 2) { fclose(f); return 1; }
            pl = (unsigned)plb[0] | (unsigned)plb[1] << 8;
            if (!pl || pl > 4096 || fseek(f, (long)pl + 16 + 64, SEEK_CUR)) { fclose(f); return 1; }
        }
        for (i = 0; i < abn; i++) {
            unsigned char plb[2];
            unsigned pl;
            if (fread(plb, 1, 2, f) != 2) { fclose(f); return 1; }
            pl = (unsigned)plb[0] | (unsigned)plb[1] << 8;
            if (!pl || pl > 4096 || fseek(f, (long)pl, SEEK_CUR)) { fclose(f); return 1; }
        }
        end = ftell(f);
        body = end - start;
        if (body < 0 || (size_t)body > (1u << 24) ||
            fstat(fileno(f), &st) || st.st_size < (off_t)end ||
            st.st_size - (off_t)end != (off_t)((size_t)e3_tn + (size_t)e3_an) ||
            st.st_size > (off_t)(1 << 26)) { fclose(f); return 1; }
        receipt_n = (size_t)body;
        receipts = (unsigned char *)malloc(receipt_n ? receipt_n : 1);
        if (!receipts) { fclose(f); return 1; }
        if (fseek(f, start, SEEK_SET) || fread(receipts, 1, receipt_n, f) != receipt_n) {
            free(receipts); fclose(f); return 1;
        }
        (void)e3_tn; (void)e3_an;
        fclose(f);
    }
    tn = (unsigned)tape->n;
    an = (unsigned)(attrs->n < 0 ? 0 : attrs->n);
    need = 8 + 4 + 4 + 4 + 4 + 64 + receipt_n + (size_t)tn + (size_t)an;
    payload = (unsigned char *)malloc(need ? need : 1);
    if (!payload) { free(receipts); return 1; }
    memcpy(payload, "USE34\n\0\0", 8); at = 8;
    payload[at++] = tn & 255; payload[at++] = (tn >> 8) & 255;
    payload[at++] = (tn >> 16) & 255; payload[at++] = (tn >> 24) & 255;
    payload[at++] = an & 255; payload[at++] = (an >> 8) & 255;
    payload[at++] = (an >> 16) & 255; payload[at++] = (an >> 24) & 255;
    payload[at++] = hn & 255; payload[at++] = (hn >> 8) & 255;
    payload[at++] = (hn >> 16) & 255; payload[at++] = (hn >> 24) & 255;
    payload[at++] = abn & 255; payload[at++] = (abn >> 8) & 255;
    payload[at++] = (abn >> 16) & 255; payload[at++] = (abn >> 24) & 255;
    unisa_sha256_hex(tape->b, (size_t)tn, hex);
    memcpy(payload + at, hex, 64); at += 64;
    if (receipt_n) memcpy(payload + at, receipts, receipt_n);
    at += receipt_n;
    free(receipts);
    if (tn) memcpy(payload + at, tape->b, tn); at += tn;
    if (an && attrs->b) memcpy(payload + at, attrs->b, an); at += an;
    if (rr_write_atomic(hostpath, payload, at)) { free(payload); return 1; }
    free(payload);
    return 0;
}

static int rr_lookup_host(const char *product, const char *route, const char *target, int level,
                          const char src_hex[65], Buf *tape) {
    char dir[400], key[65], cpath[520], hex[65], tape_got[65], hdr_got[65], cur[65];
    FILE *f;
    unsigned char fixed[8 + 4 + 4 + 4 + 4 + 64];
    unsigned tn, an, hn, abn, i;
    size_t n, body_need;
    unsigned char *body;
    struct stat st;
    if (!tape || !src_hex) return -1;
    tape->b = 0; tape->n = 0; tape->cap = 0;
    if (rr_dir(dir, sizeof dir)) return 0;
    if (rr_key(key, product, route, target, level, src_hex)) return 0;
    snprintf(cpath, sizeof cpath, "%s/%s.host", dir, key);
    if (lstat(cpath, &st) || !S_ISREG(st.st_mode) || st.st_size < (off_t)sizeof fixed) return 0;
    f = fopen(cpath, bx_file_mode(0));
    if (!f) return 0;
    if (fread(fixed, 1, sizeof fixed, f) != sizeof fixed) { fclose(f); return 0; }
    if (memcmp(fixed, "USE34\n\0\0", 8)) { fclose(f); return 0; }
    tn = (unsigned)fixed[8] | (unsigned)fixed[9] << 8 | (unsigned)fixed[10] << 16 | (unsigned)fixed[11] << 24;
    an = (unsigned)fixed[12] | (unsigned)fixed[13] << 8 | (unsigned)fixed[14] << 16 | (unsigned)fixed[15] << 24;
    hn = (unsigned)fixed[16] | (unsigned)fixed[17] << 8 | (unsigned)fixed[18] << 16 | (unsigned)fixed[19] << 24;
    abn = (unsigned)fixed[20] | (unsigned)fixed[21] << 8 | (unsigned)fixed[22] << 16 | (unsigned)fixed[23] << 24;
    if (tn > (1u << 26) || an > (1u << 26) || hn > 65536 || abn > 65536) { fclose(f); return 0; }
    memcpy(tape_got, fixed + 24, 64); tape_got[64] = 0;
    for (i = 0; i < hn; i++) {
        unsigned char plb[2], meta[16];
        unsigned pl;
        char *pp;
        struct stat hst;
        unsigned long long sz = 0, mt = 0;
        int b;
        if (fread(plb, 1, 2, f) != 2) { fclose(f); return 0; }
        pl = (unsigned)plb[0] | (unsigned)plb[1] << 8;
        if (!pl || pl > 4096) { fclose(f); return 0; }
        pp = (char *)malloc(pl + 1);
        if (!pp) { fclose(f); return -1; }
        if (fread(pp, 1, pl, f) != pl || fread(meta, 1, 16, f) != 16 ||
            fread(hdr_got, 1, 64, f) != 64) { free(pp); fclose(f); return 0; }
        pp[pl] = 0; hdr_got[64] = 0;
        for (b = 0; b < 8; b++) sz |= (unsigned long long)meta[b] << (8 * b);
        for (b = 0; b < 8; b++) mt |= (unsigned long long)meta[8 + b] << (8 * b);
        if (rr_stat_reg(pp, &hst) || rr_hash_file(pp, cur) ||
            memcmp(cur, hdr_got, 64)) { free(pp); fclose(f); return 0; }
        free(pp);
    }
    for (i = 0; i < abn; i++) {
        unsigned char plb[2];
        unsigned pl;
        char *pp;
        if (fread(plb, 1, 2, f) != 2) { fclose(f); return 0; }
        pl = (unsigned)plb[0] | (unsigned)plb[1] << 8;
        if (!pl || pl > 4096) { fclose(f); return 0; }
        pp = (char *)malloc(pl + 1);
        if (!pp) { fclose(f); return -1; }
        if (fread(pp, 1, pl, f) != pl) { free(pp); fclose(f); return 0; }
        pp[pl] = 0;
        if (!rr_still_absent(pp)) { free(pp); fclose(f); return 0; }
        free(pp);
    }
    body_need = (size_t)tn + (size_t)an;
    {
        long pos = ftell(f);
        struct stat cst;
        if (pos < 0 || fstat(fileno(f), &cst) || cst.st_size < (off_t)pos ||
            cst.st_size - (off_t)pos != (off_t)body_need ||
            cst.st_size > (off_t)(1 << 26)) { fclose(f); return 0; }
    }
    body = (unsigned char *)malloc(body_need ? body_need : 1);
    if (!body) { fclose(f); return -1; }
    n = fread(body, 1, body_need, f);
    fclose(f);
    if (n != body_need) { free(body); return 0; }
    unisa_sha256_hex(body, (size_t)tn, hex);
    if (memcmp(hex, tape_got, 64)) { free(body); return 0; }
    tape->b = (unsigned char *)malloc(tn ? tn : 1);
    if (!tape->b) { free(body); return -1; }
    if (tn) memcpy(tape->b, body, tn);
    tape->n = (int)tn; tape->cap = (int)tn;
    free(ATTRS.b); ATTRS.b = 0; ATTRS.n = 0; ATTRS.cap = 0;
    if (an) {
        ATTRS.b = (unsigned char *)malloc(an);
        if (!ATTRS.b) { free(tape->b); tape->b = 0; tape->n = 0; free(body); return -1; }
        memcpy(ATTRS.b, body + tn, an);
        ATTRS.n = (int)an; ATTRS.cap = (int)an;
        if (ATTR_SLOT >= 0) { RI[ATTR_SLOT].data = ATTRS.b; RI[ATTR_SLOT].len = ATTRS.n; }
    }
    free(body);
    return 1;
}

#endif

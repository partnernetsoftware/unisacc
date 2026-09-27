/* memmap: address-space analysis of a Linux /proc/PID/maps listing.
 *
 *   cat /proc/self/maps | unisacc -run memmap.c -
 *
 * With no argument it reads a built-in listing, so the default output is
 * deterministic.  It classifies every region, sums them by kind, groups the
 * file mappings into images, and audits the layout: regions that are both
 * writable and executable, overlaps, and the biggest holes between regions.
 * Addresses are parsed as unsigned 64-bit, so kernel-half lines such as
 * [vsyscall] are handled.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXR 2048
#define MAXI 128

enum { K_CODE_FILE, K_CODE_ANON, K_RODATA, K_DATA_FILE, K_ANON, K_HEAP, K_STACK,
       K_KERNEL, K_GUARD, K_OTHER, NKIND };

static const char *kname[NKIND] = {
    "code (file)", "code (anon)", "read-only (file)", "data (file)", "anonymous rw",
    "heap", "stack", "kernel-provided", "guard (---)", "other"
};

struct reg {
    unsigned long lo, hi, off, ino;
    char perm[5];
    char path[96];
    int kind;
};

struct image {
    char path[96];
    unsigned long lo, hi, mapped;
    int segs;
};

static struct reg R[MAXR];
static struct image I[MAXI];
static int ord[MAXR];
static int NR, NI;

static const char *sample[] = {
    "55d0a1a00000-55d0a1a02000 r--p 00000000 08:01 1310725                    /usr/bin/demo",
    "55d0a1a02000-55d0a1a08000 r-xp 00002000 08:01 1310725                    /usr/bin/demo",
    "55d0a1a08000-55d0a1a0b000 r--p 00008000 08:01 1310725                    /usr/bin/demo",
    "55d0a1a0b000-55d0a1a0d000 r--p 0000a000 08:01 1310725                    /usr/bin/demo",
    "55d0a1a0d000-55d0a1a0e000 rw-p 0000c000 08:01 1310725                    /usr/bin/demo",
    "55d0a2c7d000-55d0a2c9e000 rw-p 00000000 00:00 0                          [heap]",
    "7f3a4c000000-7f3a4c021000 rw-p 00000000 00:00 0",
    "7f3a4c021000-7f3a50000000 ---p 00000000 00:00 0",
    "7f3a54800000-7f3a54828000 r--p 00000000 08:01 917505                     /usr/lib/x86_64-linux-gnu/libc.so.6",
    "7f3a54828000-7f3a549bd000 r-xp 00028000 08:01 917505                     /usr/lib/x86_64-linux-gnu/libc.so.6",
    "7f3a549bd000-7f3a54a15000 r--p 001bd000 08:01 917505                     /usr/lib/x86_64-linux-gnu/libc.so.6",
    "7f3a54a15000-7f3a54a16000 ---p 00215000 08:01 917505                     /usr/lib/x86_64-linux-gnu/libc.so.6",
    "7f3a54a16000-7f3a54a1a000 r--p 00215000 08:01 917505                     /usr/lib/x86_64-linux-gnu/libc.so.6",
    "7f3a54a1a000-7f3a54a1c000 rw-p 00219000 08:01 917505                     /usr/lib/x86_64-linux-gnu/libc.so.6",
    "7f3a54a1c000-7f3a54a29000 rw-p 00000000 00:00 0",
    "7f3a54b10000-7f3a54b12000 rwxp 00000000 00:00 0",
    "7f3a54b30000-7f3a54b31000 r--p 00000000 08:01 917990                     /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2",
    "7f3a54b31000-7f3a54b58000 r-xp 00001000 08:01 917990                     /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2",
    "7f3a54b58000-7f3a54b62000 r--p 00028000 08:01 917990                     /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2",
    "7f3a54b62000-7f3a54b64000 r--p 00031000 08:01 917990                     /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2",
    "7f3a54b64000-7f3a54b66000 rw-p 00033000 08:01 917990                     /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2",
    "7ffc8a2d5000-7ffc8a2f6000 rw-p 00000000 00:00 0                          [stack]",
    "7ffc8a3e3000-7ffc8a3e7000 r--p 00000000 00:00 0                          [vvar]",
    "7ffc8a3e7000-7ffc8a3e9000 r-xp 00000000 00:00 0                          [vdso]",
    "ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]",
    0
};

static unsigned long hex(const char **pp)
{
    const char *s = *pp;
    unsigned long v = 0;
    for (;; s++) {
        int d;
        if (*s >= '0' && *s <= '9') d = *s - '0';
        else if (*s >= 'a' && *s <= 'f') d = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F') d = *s - 'A' + 10;
        else break;
        v = v * 16 + (unsigned long)d;
    }
    *pp = s;
    return v;
}

static int classify(const struct reg *r)
{
    int hasx = r->perm[2] == 'x', hasw = r->perm[1] == 'w', hasr = r->perm[0] == 'r';
    int file = r->path[0] != 0 && r->path[0] != '[';
    if (strcmp(r->path, "[heap]") == 0) return K_HEAP;
    if (strncmp(r->path, "[stack", 6) == 0) return K_STACK;
    if (r->path[0] == '[') return K_KERNEL;
    if (!hasr && !hasw && !hasx) return K_GUARD;
    if (hasx) return file ? K_CODE_FILE : K_CODE_ANON;
    if (file) return hasw ? K_DATA_FILE : K_RODATA;
    return hasw ? K_ANON : K_OTHER;
}

static void add_line(const char *s)
{
    struct reg *r;
    int k;
    if (NR >= MAXR) return;
    r = &R[NR];
    r->lo = hex(&s);
    if (*s != '-') return;
    s++;
    r->hi = hex(&s);
    if (*s != ' ' || r->hi <= r->lo) return;
    s++;
    for (k = 0; k < 4 && *s && *s != ' '; k++, s++) r->perm[k] = *s;
    r->perm[k] = 0;
    if (k != 4 || *s != ' ') return;
    s++;
    r->off = hex(&s);
    if (*s != ' ') return;
    while (*s == ' ') s++;
    while (*s && *s != ' ') s++;              /* dev */
    while (*s == ' ') s++;
    r->ino = 0;
    while (*s >= '0' && *s <= '9') { r->ino = r->ino * 10 + (unsigned long)(*s - '0'); s++; }
    while (*s == ' ' || *s == '\t') s++;
    for (k = 0; s[k] && s[k] != '\n' && s[k] != '\r' && k < 95; k++) r->path[k] = s[k];
    r->path[k] = 0;
    r->kind = classify(r);
    NR++;
}

static const char *sz(unsigned long b)
{
    static char buf[4][24];
    static int rot;
    char *o = buf[rot++ & 3];
    if (b >= (1UL << 40)) sprintf(o, "%lu.%luT", b >> 40, ((b * 10) >> 40) % 10);
    else if (b >= (1UL << 30)) sprintf(o, "%lu.%luG", b >> 30, ((b * 10) >> 30) % 10);
    else if (b >= (1UL << 20)) sprintf(o, "%lu.%luM", b >> 20, ((b * 10) >> 20) % 10);
    else sprintf(o, "%luK", b >> 10);
    return o;
}

static int by_lo(const void *a, const void *b)
{
    unsigned long x = R[*(const int *)a].lo, y = R[*(const int *)b].lo;
    return x < y ? -1 : (x > y);
}

static int by_span(const void *a, const void *b)
{
    const struct image *x = (const struct image *)a, *y = (const struct image *)b;
    unsigned long sx = x->hi - x->lo, sy = y->hi - y->lo;
    if (sx != sy) return sx > sy ? -1 : 1;
    return strcmp(x->path, y->path);
}

int main(int argc, char **argv)
{
    char buf[512];
    FILE *f;
    int i, k, ngap = 0, nover = 0, nwx = 0;
    unsigned long kcount[NKIND], ksum[NKIND];
    struct { unsigned long size, lo, hi; } gap[3];

    if (argc > 1) {
        f = strcmp(argv[1], "-") == 0 ? stdin : fopen(argv[1], "r");
        if (f == 0) { fprintf(stderr, "memmap: cannot open %s\n", argv[1]); return 1; }
        while (fgets(buf, sizeof buf, f)) add_line(buf);
    } else {
        /* Deliberately NOT reading this process's own /proc/self/maps here:
         * it is real data, but the map is a property of the SPECIFIC
         * binary asking (a cc build and a unisacc build of the same
         * source have different segments), so it would break the
         * cc-vs-unisacc byte-for-byte check every other example in this
         * directory relies on, and ASLR makes it change between runs of
         * the same binary too. That is a real reason, not a shortcut. */
        fprintf(stderr, "memmap: no input given -- showing a built-in "
                        "SAMPLE, not real data. Try:\n"
                        "  cat /proc/PID/maps | unisacc -run memmap.c -\n");
        for (i = 0; sample[i]; i++) add_line(sample[i]);
    }
    if (NR == 0) { fprintf(stderr, "memmap: no regions\n"); return 1; }

    for (i = 0; i < NKIND; i++) { kcount[i] = 0; ksum[i] = 0; }
    for (i = 0; i < NR; i++) {
        kcount[R[i].kind]++;
        ksum[R[i].kind] += R[i].hi - R[i].lo;
        ord[i] = i;
    }
    qsort(ord, NR, sizeof ord[0], by_lo);

    printf("== regions (%d)\n", NR);
    for (k = 0; k < NR; k++) {
        const struct reg *r = &R[ord[k]];
        printf("%012lx %-4s %7s  %-16s %s\n", r->lo, r->perm, sz(r->hi - r->lo), kname[r->kind], r->path);
    }

    printf("\n== by kind\n");
    for (i = 0; i < NKIND; i++)
        if (kcount[i]) printf("%-16s %3lu regions %9s\n", kname[i], kcount[i], sz(ksum[i]));

    for (i = 0; i < NR; i++) {
        struct reg *r = &R[i];
        int j;
        if (r->path[0] == 0 || r->path[0] == '[') continue;
        for (j = 0; j < NI; j++) if (strcmp(I[j].path, r->path) == 0) break;
        if (j == NI) {
            if (NI >= MAXI) continue;
            strcpy(I[j].path, r->path);
            I[j].lo = r->lo; I[j].hi = r->hi; I[j].mapped = 0; I[j].segs = 0;
            NI++;
        }
        if (r->lo < I[j].lo) I[j].lo = r->lo;
        if (r->hi > I[j].hi) I[j].hi = r->hi;
        I[j].mapped += r->hi - r->lo;
        I[j].segs++;
    }
    qsort(I, NI, sizeof I[0], by_span);
    printf("\n== images (file mappings grouped by path)\n");
    for (i = 0; i < NI && i < 6; i++)
        printf("%012lx span %9s mapped %9s %2d segments  %s\n", I[i].lo, sz(I[i].hi - I[i].lo),
               sz(I[i].mapped), I[i].segs, I[i].path);

    printf("\n== audit\n");
    for (i = 0; i < NR; i++)
        if (R[i].perm[1] == 'w' && R[i].perm[2] == 'x') {
            nwx++;
            printf("W+X: %012lx-%012lx %s  %s\n", R[i].lo, R[i].hi, sz(R[i].hi - R[i].lo),
                   R[i].path[0] ? R[i].path : "(anonymous)");
        }
    for (k = 0; k + 1 < NR; k++) {
        const struct reg *a = &R[ord[k]], *b = &R[ord[k + 1]];
        if (b->lo < a->hi) {
            nover++;
            printf("overlap: %012lx-%012lx and %012lx-%012lx\n", a->lo, a->hi, b->lo, b->hi);
        } else if (b->lo > a->hi && !(a->hi <= (1UL << 47) && b->lo >= 0xffff800000000000UL)) {
            /* the gap across the non-canonical hole is not a real hole */
            unsigned long g = b->lo - a->hi;
            int p = ngap < 3 ? ngap : 2;
            if (ngap < 3 || g > gap[2].size) {
                while (p > 0 && gap[p - 1].size < g) { gap[p] = gap[p - 1]; p--; }
                gap[p].size = g; gap[p].lo = a->hi; gap[p].hi = b->lo;
                if (ngap < 3) ngap++;
            }
        }
    }
    printf("W+X regions %d, overlaps %d\n", nwx, nover);
    printf("lowest start %012lx, last region ends %lx\n", R[ord[0]].lo, R[ord[NR - 1]].hi);
    for (i = 0; i < ngap; i++)
        printf("hole %d: %012lx..%012lx = %s\n", i + 1, gap[i].lo, gap[i].hi, sz(gap[i].size));
    return 0;
}

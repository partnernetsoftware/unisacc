/* memmap: address-space analysis of a Linux /proc/PID/maps listing.
 *
 *   cat /proc/self/maps | unisacc -run memmap.c -
 *
 * With no argument on Linux it reads its own /proc/self/maps.
 * On macOS it queries this process through libproc and libffi.
 * Other platforms accept an explicit captured map. It sums regions by kind and groups the
 * file mappings into images, and audits the layout: regions that are both
 * writable and executable, overlaps, and the biggest holes between regions.
 * Addresses are parsed as unsigned 64-bit, so kernel-half lines such as
 * [vsyscall] are handled.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __APPLE__
#include <unisacc_ffi.h>
#endif

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


#ifdef __APPLE__
static int scan_mac_maps(void)
{
    long raw[12], result, ownpid;
    unsigned long address = 0, end;
    int pid, flavor = 7, size = sizeof raw, *host_errno;
    char filename[1024];
    int pk[5] = { UFFI_INT, UFFI_INT, UFFI_ULONG, UFFI_POINTER, UFFI_INT };
    int fk[4] = { UFFI_INT, UFFI_ULONG, UFFI_POINTER, UFFI_UINT };
    void *lib, *info, *getpidfn, *errorfn, *namefn, *buffer, *values[5];
    struct reg *r;
    unsigned int namesize;
    lib = uffi_dlopen("/usr/lib/libSystem.B.dylib", 2);
    if (!lib || !(info = uffi_dlsym(lib, "proc_pidinfo")) ||
        !(getpidfn = uffi_dlsym(lib, "getpid")) || !(errorfn = uffi_dlsym(lib, "__error")) ||
        !(namefn = uffi_dlsym(lib, "proc_regionfilename"))) {
        fprintf(stderr, "memmap: cannot resolve libproc APIs\n"); return 0;
    }
    if (uffi_call(getpidfn, UFFI_INT, 0, 0, 0, -1, &ownpid) ||
        uffi_call(errorfn, UFFI_POINTER, 0, 0, 0, -1, &host_errno)) return 0;
    pid = ownpid;
    fprintf(stderr, "memmap: querying this process, pid %d\n", pid);
    for (;;) {
        buffer = raw; values[0] = &pid; values[1] = &flavor; values[2] = &address;
        values[3] = &buffer; values[4] = &size; *host_errno = 0;
        /* SDK proc_regioninfo: protection@0, offset@16, address@80, size@88; 96 bytes. */
        if (uffi_call(info, UFFI_INT, pk, values, 5, -1, &result)) return 0;
        if (!result && *host_errno == 22 && NR > 0) break; /* no next region */
        if (result != sizeof raw) {
            fprintf(stderr, "memmap: region query failed (bytes %ld, errno %d)\n", result, *host_errno); return 0;
        }
        end = (unsigned long)raw[10] + (unsigned long)raw[11];
        if (!raw[11] || (unsigned long)raw[10] < address || end <= (unsigned long)raw[10]) return 0;
        if (NR >= MAXR) { fprintf(stderr, "memmap: region capacity reached\n"); return 0; }
        r = &R[NR]; r->lo = raw[10]; r->hi = end; r->off = raw[2];
        r->perm[0] = raw[0] & 1 ? 'r' : '-'; r->perm[1] = raw[0] & 2 ? 'w' : '-';
        r->perm[2] = raw[0] & 4 ? 'x' : '-';
        r->perm[3] = *(unsigned int *)((char *)raw + 12) & 2 ? 's' : 'p'; r->perm[4] = 0;
        filename[0] = 0; buffer = filename; namesize = sizeof filename;
        values[0] = &pid; values[1] = &r->lo; values[2] = &buffer; values[3] = &namesize;
        if (uffi_call(namefn, UFFI_INT, fk, values, 4, -1, &result)) return 0;
        if (result <= 0) filename[0] = 0;
        filename[1023] = 0; strncpy(r->path, filename, 95);
        r->path[95] = 0; r->kind = classify(r); NR++; address = end;
    }
    uffi_dlclose(lib); return NR > 0;
}
#endif

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
    int capture = argc == 2 && strcmp(argv[1], "--capture") == 0;
    FILE *f = 0;
    int i, k, ngap = 0, nover = 0, nwx = 0;
    unsigned long kcount[NKIND], ksum[NKIND];
    struct { unsigned long size, lo, hi; } gap[3];

    if (argc > 1 && !capture) {
        f = strcmp(argv[1], "-") == 0 ? stdin : fopen(argv[1], "r");
    } else {
#ifdef __linux__
        f = fopen("/proc/self/maps", "r");
#else
#ifdef __APPLE__
        if (!scan_mac_maps()) { fprintf(stderr, "memmap: live mapping query failed\n"); return 1; }
#else
        fprintf(stderr, "memmap: provide real maps on this platform\n"); return 1;
#endif
#endif
    }
    if (argc > 1 && !capture && f == 0) { fprintf(stderr, "memmap: cannot open maps input\n"); return 1; }
    if (f) {
        while (fgets(buf, sizeof buf, f)) add_line(buf);
        if (f != stdin) fclose(f);
    }
    if (NR == 0) { fprintf(stderr, "memmap: no regions\n"); return 1; }

    if (capture) {
        for (i = 0; i < NR; i++) printf("%lx-%lx %s %lx 00:00 %lu %s\n", R[i].lo, R[i].hi, R[i].perm, R[i].off, R[i].ino, R[i].path);
        return 0;
    }

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
